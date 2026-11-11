#include "ModiconTagging.h"
#include "ImguiHelper.h"
#include "Archive.h"

static_assert(TagType_Count == 15);
const char* g_TagTypeNames[TagType_Count] = {
    "BOOL",
    "BYTE",
    "DATE",
    "DINT",
    "DT",
    "DWORD",
    "EBOOL",
    "INT",
    "REAL",
    "STRING",
    "TIME",
    "TOD",
    "UDINT",
    "UINT",
    "WORD",
};
const char* g_modbus_prefixes[ModbusType_Count] = {
    "M",
    "MW",
};

const char* xmltitle_exchange   = "VariablesExchangeFile";
const char* xmltitle_datablock  = "dataBlock";
const char* xmltitle_var        = "variables";
const char* xmltitle_type       = "typeName";
const char* xmltitle_name       = "name";
const char* xmltitle_address    = "topologicalAddress";

void ModiconInit(ModiconData** data)
{
    VALIDATE(data && !(*data));
    *data = new ModiconData();
}

void ModiconDestroy(ModiconData** data)
{
    VALIDATE(data && *data);
    delete (*data);
}

ModbusType GetModbusType(TagType type)
{
    static_assert(TagType_Count == 15);
    switch (type)
    {
    case TagType_EBOOL:
        return ModbusType_M;

    case TagType_BYTE:  [[fallthrough]];
    case TagType_DINT:  [[fallthrough]];
    case TagType_DWORD: [[fallthrough]];
    case TagType_INT:   [[fallthrough]];
    case TagType_REAL:  [[fallthrough]];
    case TagType_UDINT: [[fallthrough]];
    case TagType_UINT:  [[fallthrough]];
    case TagType_WORD: 
        return ModbusType_MW;

    case TagType_BOOL:  [[fallthrough]];
    case TagType_DATE:  [[fallthrough]];
    case TagType_DT:    [[fallthrough]];
    case TagType_Count: [[fallthrough]];
    default: FAIL; return {};
    }
}

TagType GetModbusType(const char* s)
{
    TagType r = TagType_BOOL;
    i32 i = 0;
    for (; i < TagType_Count; i++)
    {
        if (StringCompare(StringCase_Sensitive, s, g_TagTypeNames[i]))
        {
            r = TagType(i);
            break;
        }
    }
    VALIDATE_VM(i != TagType_Count, r, LogLevel_Error, "Failed to get Modbus Type from: '%s'", s);
    return r;
}

i32 GetModbusSize(TagType type)
{
    //TODO(CSH): Does this need to be converted to use ModbusType?
    static_assert(TagType_Count == 15);
    switch (type)
    {
    case TagType_EBOOL:
        return 1;

    case TagType_INT:   [[fallthrough]];
    case TagType_UINT:  [[fallthrough]];
    case TagType_WORD: 
        return 1;

    case TagType_DINT:  [[fallthrough]];
    case TagType_DWORD: [[fallthrough]];
    case TagType_REAL:  [[fallthrough]];
    case TagType_UDINT: 
        return 2;

    case TagType_BOOL:  [[fallthrough]];
    case TagType_BYTE:  [[fallthrough]];
    case TagType_DATE:  [[fallthrough]];
    case TagType_DT:    [[fallthrough]];
    case TagType_STRING:[[fallthrough]];
    case TagType_TIME:  [[fallthrough]];
    case TagType_TOD:   [[fallthrough]];
    case TagType_Count: [[fallthrough]];
    default: FAIL; return 0;
    }
}

struct TagData {
    InlineString<64> name = {};
    TagType type = {};
    u32 address = {};
};

static struct ParseData {
    AsyncData<pugi::xml_document> xml;
    i32 type_counts[TagType_Count] = {};
    StaticArray<TagData, Megabytes(1)> tags = {};
} s_parse;


struct ParseXMLJob : Job
{
    Path filepath;
    void RunJob() override
    {
        ZoneScopedN("ParseXMLJob");
        VALIDATE_M(!filepath.empty(), LogLevel_Warning, "Invalid filepath to parse XML");

        s_parse.xml.Start();
        s_parse.xml.data = {};
        s_parse.xml.data = GetXmlDocFromFile(filepath, StringEncoding_UTF8);
        s_parse.xml.End(!s_parse.xml.data.empty());
        VALIDATE_M(!s_parse.xml.data.empty(), LogLevel_Error, "Failed to parse XML file: %s", filepath);
        const pugi::xml_node base = s_parse.xml.data.child(xmltitle_exchange);
        VALIDATE_M(!base.empty(), LogLevel_Error, "Failed to find '%s' inside of XML data", xmltitle_exchange);
        const pugi::xml_node block = base.child(xmltitle_datablock);
        VALIDATE_M(!block.empty(), LogLevel_Error, "Failed to find '%s' inside of XML data", xmltitle_datablock);

        for (pugi::xml_node var = block.child(xmltitle_var); var; var = var.next_sibling(xmltitle_var))
        {
            const pugi::xml_attribute type_attr = var.attribute(xmltitle_type);
            const pugi::xml_attribute name_attr = var.attribute(xmltitle_name);
            if (type_attr.empty() || name_attr.empty())
            {
                LOG(LogLevel_Warning, "Failed to get tag info. Type(%s), Name(%s), Node(%s)", type_attr.as_string(), name_attr.as_string(), var.value());
                FAIL;
                continue;
            }
            const TagType type = GetModbusType(type_attr.as_string());
            s_parse.type_counts[type]++;
            s_parse.tags.Add({
                .name = name_attr.as_string(),
                .type = type,
                .address = 0,
                });
        }

        {
            ZoneScopedN("Sort tags");
            
            auto func = [](const void* _a, const void* _b) {
                const TagData* a = (TagData*)_a;
                const TagData* b = (TagData*)_b;
                const ModbusType a_mod = GetModbusType(a->type);
                const ModbusType b_mod = GetModbusType(b->type);

                //sort EBOOL vs non-EBOOL
                if (a_mod != b_mod)
                    return b_mod - a_mod;

                //sort by tagname
                return -StringNaturalCompare(a->name.data, b->name.data);
                };
            QuickSort((u8*)&s_parse.tags.data, (i32)s_parse.tags.used, sizeof(s_parse.tags[0]), func);
        }
    }
};

void ModiconImGui(ModiconData& data)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    Threading& threading = Threading::GetInstance();
    ImGuiWindowFlags section_flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove;

    const bool file_exists = !data.file.empty() && FileExists(ToString(data.file).c_str());

#define FILE_TITLE "Modicon XSY File"
    if (ImGui::BeginChild(FILE_TITLE, { 0, 90 }, true, section_flags))
    {
        ZoneScopedN(FILE_TITLE);
        ImguiTextCentered(FILE_TITLE);
        ImGui::NewLine();

        std::function<void(void)> func = [&data]() {
            ParseXMLJob* job = new ParseXMLJob();
            job->filepath = data.file;
            Threading::GetInstance().SubmitJob(job);
            };
        ImguiFilePath("Modicon XSY File", "Please select the XSY file to modify", &data.file, SysFileNavigationFlags_OnlyFiles, func);
    }
    ImGui::EndChild();

    {
        //HAVE FILE

#define TAGADD_TITLE "Modicon Tag Starting Addresses"
        if (ImGui::BeginChild(TAGADD_TITLE, { 0, 0 }, true, section_flags))
        {
            ZoneScopedN(TAGADD_TITLE);
            ImguiTextCentered(TAGADD_TITLE);
            ImGui::NewLine();

            for (i32 i = 0; i < ModbusType_Count; i++)
            {
                const char* s = ArenaPush(&g_arena, "%%%s:", g_modbus_prefixes[i]);
                const char* _s = ArenaPush(&g_arena, "##%s", s);
                ImGui::Text("%s", s);
                ImGui::SameLine();
                ImGui::SetCursorPosX(50);
                ImGui::PushItemWidth(150);
                ImGui::InputInt(_s, &data.starting_addresses[i]);
            }
        }

        if (!file_exists)
            ImGui::BeginDisabled();
        if (ImGui::Button("Update Modbus Addresses", { -1, 30 }))
        {
            if (s_parse.xml.state == AsyncStatus_FetchedSuccess && !s_parse.xml.data.empty())
            {
                //s_parse.xml.Start();
                //Defer{ s_parse.xml.End(!s_parse.xml.data.empty()); };
                //pugi::xml_document doc(s_parse.xml.data);
                pugi::xml_node base = s_parse.xml.data.child(xmltitle_exchange);
                VALIDATE_M(!base.empty(), LogLevel_Error, "Failed to find '%s' inside of XML data", xmltitle_exchange);
                pugi::xml_node block = base.child(xmltitle_datablock);
                VALIDATE_M(!block.empty(), LogLevel_Error, "Failed to find '%s' inside of XML data", xmltitle_datablock);


                {
                    i32 address_indices[ModbusType_Count];
                    static_assert(sizeof(address_indices) == sizeof(data.starting_addresses));
                    memmove(address_indices, data.starting_addresses, sizeof(address_indices));

                    for (i32 i = 0; i < s_parse.tags.used; i++)
                    {
                        TagData& t = s_parse.tags[i];
                        const i32 size = GetModbusSize(t.type);
                        const ModbusType modbus_type = GetModbusType(t.type);

                        i32& index = address_indices[modbus_type];
                        t.address = index;
                        index += size;
                    }
                }

                InlineString<16> buffer = {};
                for (pugi::xml_node var = block.child(xmltitle_var); var; var = var.next_sibling(xmltitle_var))
                {
                    const pugi::xml_attribute type_attr = var.attribute(xmltitle_type);
                    const pugi::xml_attribute name_attr = var.attribute(xmltitle_name);
                    const char* name = name_attr.as_string();
                    if (type_attr.empty() || name_attr.empty())
                    {
                        LOG(LogLevel_Warning, "Failed to get tag info. Type(%s), Name(%s), Node(%s)", type_attr.as_string(), name_attr.as_string(), var.value());
                        FAIL;
                        continue;
                    }
                    const TagType tag_type = GetModbusType(type_attr.as_string());
                    ModbusType modbus_type = GetModbusType(tag_type);
                    const char* modbus_prefix = g_modbus_prefixes[modbus_type];

                    u32 address = 0;
                    i32 i = 0;
                    for (; i < s_parse.tags.used; i++)
                    {
                        const TagData& t = s_parse.tags[i];
                        if (StringCompare(StringCase_Sensitive, t.name.data, name))
                        {
                            address = t.address;
                            break;
                        }
                    }
                    ASSERT(i < s_parse.tags.used);
                    
                    Sprintf(buffer.data, buffer.GetCount(), "%%%s%i", modbus_prefix, address);

                    pugi::xml_attribute add_attr = var.attribute(xmltitle_address);
                    if (add_attr.empty())
                    {
                        add_attr = var.append_attribute(xmltitle_address);
                    }
                    VERIFY(add_attr.set_value(buffer.data));
                    buffer.Clear();
                }

                VERIFY(s_parse.xml.data.save_file(ToString(data.file).c_str(), "\t"));
            }
            else
            {
                LOG(LogLevel_Error, "Can't update modbus addresses.  The file state is %s and/or empty", GetAsyncStatusString(s_parse.xml.state));
            }
        }
        if (!file_exists)
            ImGui::EndDisabled();
        ImGui::EndChild();
    }
}
