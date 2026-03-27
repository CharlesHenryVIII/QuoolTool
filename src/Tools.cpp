#include "Tools.h"
#include "String.h"
#include "ImguiHelper.h"
#include "Tracy.hpp"
#include "LoadJson.h"
#include "WinInterop.h"
#include "Scripts.h"
#include "Wininterop_file.h"
#include "pugixml.hpp"

#include "xlsxwriter.h"

#include <vector>
#include <array>
#include <fstream>
#include <charconv>

AsyncData<lxw_workbook*> s_workbook;

struct ScriptData {
    std::string name;
    std::string output;
    AsyncData<lxw_workbook*>* workbook;
    bool using_quotes = true;
};

typedef void (*ScriptFunction)(ScriptData&);
struct ScriptJob : Job
{
    std::wstring path;
    std::wstring args;
    Atomic<bool>* completed;
    ScriptFunction func;
    ScriptData data;
    
    void RunJob() override
    {
        ZoneScopedN("ScriptJob");

        i32 r = RunProcess(path, args, &data.output);
        bool success = !r;

        if (func)
        {
            ZoneScopedN("ScriptJob func");
            func(data);
        }

        if (completed)
        {
            ASSERT(*completed == false);
            (*completed) = true;
        }
    }
};

struct WorkbookJob : Job
{
    AsyncData<lxw_workbook*>* workbook;
    Atomic<ScriptState>* state;

    void RunJob() override
    {
        ZoneScopedN("Workbook Job");
        if (workbook)
        {
            TRACY_LOCK(workbook->lock);
            workbook_close(workbook->data);
        }
        else
            FAIL;
        if (state)
        {
            *state = ScriptState_Finished;
        }
        else
            FAIL;
    }
};


struct ScriptInfo {
    std::string name;
    Atomic<ScriptInfoFlags> flags = ScriptInfoFlags(ScriptInfoFlags_Enabled);
    ScriptFunction func = nullptr;
    std::wstring cmdline; //function
    //break
    Atomic<bool> completed = false;
};

lxw_format* CreateTitleFormat(lxw_workbook* book)
{
    lxw_format* format = workbook_add_format(book);
    format_set_bold(format);
    format_set_align(format, LXW_ALIGN_CENTER);
    format_set_align(format, LXW_ALIGN_VERTICAL_CENTER);
    format_set_border(format, LXW_BORDER_THICK);
    //format_set_font_name(format, "Aptos Narrow");
    return format;
}

lxw_format* CreateDataFormat(lxw_workbook* book)
{
    lxw_format* format = workbook_add_format(book);
    format_set_align(format, LXW_ALIGN_LEFT);
    return format;
}

void ExcelWriteTitles(lxw_workbook* book, lxw_worksheet* sheet, size_t column_widths[PWSH_MAX_COLUMNS], const PowershellResponse& array)
{
    const std::array<std::string, PWSH_MAX_COLUMNS>& row = array[0];
    ASSERT(PWSH_MAX_COLUMNS == row.size());
    lxw_format* title_format = CreateTitleFormat(book);
    worksheet_set_row(sheet, 0, 30, NULL);
    for (i32 i = 0; i < row.size(); i++)
    {
        const std::string& title = row[i];
        if (title.empty())
            continue;
        worksheet_write_string(sheet, 0, i, title.c_str(), title_format);
        column_widths[i] = Max(column_widths[i], title.size() + 4);
    }
}

void ExcelWriteData(lxw_workbook* book, lxw_worksheet* sheet, size_t column_widths[PWSH_MAX_COLUMNS], const PowershellResponse& array)
{
    lxw_format* format = CreateDataFormat(book);
    for (i32 row = 1; row < array.size(); row++)
    {
        for (i32 col = 0; col < array[row].size(); col++)
        {
            const auto& title = array[row][col];
            if (title.empty())
                continue;
            worksheet_write_string(sheet, row, col, title.c_str(), format);
            column_widths[col] = Max(column_widths[col], title.size());
        }
    }
}

void ExcelAutoSizeColumnWidth(lxw_worksheet* sheet, size_t column_widths[16])
{
    for (i32 i = 0; i < PWSH_MAX_COLUMNS; i++)
    {
        if (column_widths[i] <= 0)
            continue;
        double width = (double)column_widths[i] + 1;// / 10.0;
        worksheet_set_column(sheet, i, i, width, NULL);
    }
}

void ExcelWritePowershellData(lxw_workbook* book, lxw_worksheet* sheet, const PowershellResponse& array)
{
    size_t column_widths[16] = {};
    ASSERT(arrsize(column_widths) == array[0].size());
    ExcelWriteTitles(book, sheet, column_widths, array);
    ExcelWriteData(book, sheet, column_widths, array);
    ExcelAutoSizeColumnWidth(sheet, column_widths);
}

void ScriptPrograms(ScriptData& data)
{
    PowershellResponse array;
    ParsePowershell(array, data.output);
    switch (array.size())
    {
    case 0: return;
    case 1: FAIL; return; //only title?  no title?
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_worksheet* sheet = workbook_add_worksheet(data.workbook->data, data.name.c_str());
    ExcelWritePowershellData(data.workbook->data, sheet, array);
}

void ScriptProcessor(ScriptData& data)
{
    PowershellResponse array;
    ParsePowershell(array, data.output);
    switch (array.size())
    {
    case 0: return;
    case 1: FAIL; return; //only title?  no title?
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_worksheet* sheet = workbook_add_worksheet(data.workbook->data, data.name.c_str());
    ExcelWritePowershellData(data.workbook->data, sheet, array);
}

void ScriptSysinfo(ScriptData& data)
{
    PowershellResponse array;
    ParseSysinfo(array, data.output);
    if (!array.size())
    {
        FAIL;
        return;
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_workbook* book = data.workbook->data;
    lxw_worksheet* sheet = workbook_add_worksheet(book, data.name.c_str());

    size_t column_widths[16] = {};
    ExcelWriteTitles(book, sheet, column_widths, array);
    ExcelWriteData(book, sheet, column_widths, array);
    ExcelAutoSizeColumnWidth(sheet, column_widths);
}

void ScriptCsv(ScriptData& data)
{
    ZoneScoped;
    PowershellResponse array;
    ParseCSV(array, data.output, data.using_quotes);
    if (!array.size())
    {
        FAIL;
        return;
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_workbook* book = data.workbook->data;
    lxw_worksheet* sheet = workbook_add_worksheet(book, data.name.c_str());
    size_t column_widths[16] = {};
    ExcelWriteTitles(book, sheet, column_widths, array);
    ExcelWriteData(book, sheet, column_widths, array);
    ExcelAutoSizeColumnWidth(sheet, column_widths);
}

//TODO(CSH): Redo this for powershell:
//"powershell -command "Get-CimInstance Win32_NetworkAdapterConfiguration | ConvertTo-Csv -NoTypeInformation"
void ScriptIpconfig(ScriptData& data)
{
    ZoneScoped;

    const std::vector<std::string> rows = TextToStringArray(data.output.c_str(), "\n");
    if (!rows.size())
    {
        FAIL;
        return;
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_workbook* book = data.workbook->data;
    lxw_worksheet* sheet = workbook_add_worksheet(data.workbook->data, data.name.c_str());
    VALIDATE(sheet);
    lxw_format* title_format = CreateTitleFormat(book);
    lxw_format* data_format = CreateDataFormat(book);
    const size_t end_len = 2; //character count of "/r/n";
    const char* pad_end_s = ". :";
    const size_t pad_end_s_len = strlen(pad_end_s) + 1;

    for (i32 i = 0; i < rows.size(); i++)
    {
        const std::string& begin_row = rows[i];
        if (begin_row == "\r\n" || begin_row == "\r\r")
            continue;

        if (begin_row.find(pad_end_s) == std::string::npos)
        {
            //title
            const std::string title = begin_row.substr(0, begin_row.size() - end_len);
            worksheet_merge_range(sheet, i, 0, i, 1, title.c_str(), title_format);
            worksheet_set_row(sheet, i, 30, NULL);
        }
        else
        {
            //data
            for (;i < rows.size() && rows[i] != "\r\n" && rows[i] != "\r\r"; i++)
            {
                const std::string& row = rows[i];
                if (row == "\r" || row == "\n")
                    continue;
                const size_t key_len = row.find(" .");
                std::string key = row.substr(0, key_len);
                StringRemoveLeading(key, ' ');
                worksheet_write_string(sheet, i, 0, key.c_str(), data_format);

                const size_t pad_end = row.find(pad_end_s);
                size_t end_loc = row.find('\r');
                if (end_loc == std::string::npos)
                    end_loc = row.size() - 1;

                const size_t start = pad_end + pad_end_s_len;
                const std::string name = row.substr(start, end_loc - start);
                if (name.size() > 0)
                    worksheet_write_string(sheet, i, 1, name.c_str(), data_format);
            }
        }
    }
}

void ScriptXPDisks(ScriptData& data)
{
    ZoneScoped;
    const std::vector<std::string> rows = TextToStringArray(data.output.c_str(), "\n");
    if (!rows.size())
    {
        FAIL;
        return;
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_workbook* book = data.workbook->data;
    lxw_worksheet* sheet = workbook_add_worksheet(book, data.name.c_str());
    lxw_format* title_format = CreateTitleFormat(book);
    lxw_format* data_format = CreateDataFormat(book);

    const size_t end_len = 2; //character count of "/r/n";

    i32 row_i = 0;
    for (; row_i < rows.size(); row_i++)
    {
        const std::string& row = rows[row_i];
        if (row.find_first_of("---") == 0)
        {
            std::string title = row.substr(0, row.size() - end_len);
            StringRemoveLeading(title, '-');
            StringRemoveTrailing(title, '-');
            worksheet_merge_range(sheet, row_i, 0, row_i, 3, title.c_str(), title_format);
            worksheet_set_row(sheet, row_i, 35, NULL);
            ++row_i;

            const std::string& title_row = rows[row_i];
            const std::vector<std::string> titles = TextToStringArray(title_row.substr(0, title_row.size() - 1).c_str(), ",");
            for (i32 j = 0; j < titles.size(); j++)
            {
                worksheet_write_string(sheet, row_i, j, titles[j].c_str(), title_format);
            }
            worksheet_set_row(sheet, row_i, 30, NULL);
            ++row_i;

            for (i32 j = 0; j < titles.size(); j++)
            {
                const std::string& data = rows[row_i];
                const std::vector<std::string> d = TextToStringArray(data.c_str(), ",");
                for (i32 k = 0; k < d.size(); k++)
                {
                    worksheet_write_string(sheet, row_i, k, d[k].c_str(), data_format);
                }
            }
        }
    }
}

void ScriptNetstat(ScriptData& data)
{
    ZoneScoped;
    const std::vector<std::string> rows = TextToStringArray(data.output.c_str(), "\n");
    if (!rows.size())
    {
        FAIL;
        return;
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_workbook* book = data.workbook->data;
    lxw_worksheet* sheet = workbook_add_worksheet(book, data.name.c_str());
    lxw_format* title_format = CreateTitleFormat(book);
    lxw_format* data_format = CreateDataFormat(book);

    const i32 title_index = 3;
    const i32 data_index = 4;
    const std::string& titles = rows[title_index];
    i32 len[PWSH_MAX_COLUMNS] = {};

    static const std::string titles_text[] = { "Proto", "Local Address", "Foreign Address", "State", "PID" };
    {
        i32 prev = 0;
        i32 len_i = 0;
        const i32 start = (i32)titles.find_first_not_of(' ');
        for (i32 i = start; i < titles.size(); i++)
        {
            const size_t r = titles.find(titles_text[len_i]);
            if (r == std::string::npos)
            {
                FAIL;
                continue;
            }
            i = (i32)r + (i32)titles_text[len_i].size();
            while (titles[i] == ' ')
                i++;

            len[len_i++] = i - prev;
            prev = i;
        }
    }

    for (i32 i = 0; i < arrsize(titles_text); i++)
    {
        worksheet_write_string(sheet, 0, i, titles_text[i].c_str(), title_format);
    }
    worksheet_set_row(sheet, 0, 30, NULL);

    for (i32 row_i = data_index; row_i < rows.size(); row_i++)
    {
        std::string_view row = rows[row_i];
        row = row.substr(0, row.find_last_not_of('\r') + 1);
        size_t prev = 0;
        for (i32 col = 0; col < arrsize(len); col++)
        {
            if (!len[col])
                break;
            std::string_view sv = row.substr(prev, len[col]);
            size_t final_char = sv.find_last_not_of(' ');
            size_t first_char = sv.find_first_not_of(' ');
            if (first_char == std::string::npos)
                first_char = 0;
            if (final_char == std::string::npos)
                final_char = 0;
            if (first_char > 0 && final_char > 0)
                sv = sv.substr(first_char, final_char - first_char + 1);
            std::string s(sv);
            worksheet_write_string(sheet, row_i - title_index, col, s.c_str(), data_format);
            prev = prev + len[col];
        }
    }
}

enum VarType : u32 {
    VarType_Invalid,
    VarType_String,
    VarType_i8,
    VarType_i16,
    VarType_i32,
    VarType_i64,
    VarType_u8,
    VarType_u16,
    VarType_u32,
    VarType_u64,
    VarType_Bool,
    VarType_Datetime,
    VarType_Count,
};

struct KeyValPair {
    std::string name;
    std::string value;
    VarType val_type = VarType_String;
};

void ScriptSystemInfoXP(const Path& full_filename, std::vector<KeyValPair>& pairs)
{
    std::wstring ws;
    File file(full_filename.string(), FileMode_Read, false);
    file.GetData();
    if (file.m_binaryDataIsValid)
    {
        wchar_t* wc = (wchar_t*)file.m_dataBinary.data();
        std::wstring ws(wc, file.m_dataBinary.size() / 2);
        size_t start_offset = 3;
        ws = ws.substr(start_offset, ws.size() - start_offset);
        std::string s;
        ConvertWideCharToMultiByte(s, ws);
        const std::vector<std::string> rows = TextToStringArray(s.c_str(), "\n");
        if (rows.size() != 2)
        {
            FAIL;
            return;
        }
        std::vector<std::string> names = TextToStringArray(rows[0].c_str(), ",");
        std::vector<std::string> values= TextToStringArray(rows[1].c_str(), ",");
        if (!names.size() || !values.size() || names.size() != values.size())
        {
            FAIL;
            return;
        }

        auto CleanString = [](std::string _string)
        {
            TextRemoval(_string, ",");
            TextRemoval(_string, "\r");
            TextRemoval(_string, "\n");
            StringRemoveTrailing(_string, ' ');
            StringRemoveLeading(_string, ' ');
        };

        //offset becuase the first name/value is "node"
        for (i32 i = 1; i < names.size(); i++)
        {
            CleanString(names[i]);
            CleanString(values[i]);
            KeyValPair p = {
                .name = names[i],
                .value = values[i],
            };
            pairs.push_back(p);
        }
    }
}

bool GetBoolFromString(const char* s)
{
    return StringCompare(StringCase_Insensitive, s, "TRUE");
}

VarType GetVarTypeFromString(const char* s)
{
    if (StringCompare(StringCase_Insensitive, s, "uint32"))
        return VarType_u32;
    else if (StringCompare(StringCase_Insensitive, s, "uint64"))
        return VarType_u64;
    else if (StringCompare(StringCase_Insensitive, s, "uint8"))
        return VarType_u8;
    else if (StringCompare(StringCase_Insensitive, s, "uint16"))
        return VarType_u16;
    else if (StringCompare(StringCase_Insensitive, s, "string"))
        return VarType_String;
    else if (StringCompare(StringCase_Insensitive, s, "datetime"))
        return VarType_Datetime;
    else if (StringCompare(StringCase_Insensitive, s, "boolean"))
        return VarType_Bool;
    else if (StringCompare(StringCase_Insensitive, s, "sint8"))
        return VarType_i8;
    else if (StringCompare(StringCase_Insensitive, s, "sint16"))
        return VarType_i16;
    else if (StringCompare(StringCase_Insensitive, s, "sint32"))
        return VarType_i32;
    else if (StringCompare(StringCase_Insensitive, s, "sint64"))
        return VarType_i64;
    else
    {

        FAIL;
        return VarType_Invalid;
    }
}

pugi::xml_document GetXmlDocFromFile(const char* filename)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename);
    if (!result)
    {
        DebugPrint("Error XML [%s] parsed with errors", filename);
        DebugPrint("Error description: %s", result.description());
        DebugPrint("Error offset: %i (error at [...%i]\n", result.offset, (filename + result.offset));
        FAIL;
        return {};
    }
    return doc;
}

lxw_datetime StringToDatetime(const char* s)
{
    std::string_view sv(s);
    VALIDATE_V(sv.size() >= 21, {});

    std::string_view year = sv.substr(0, 4);
    std::string_view month = sv.substr(4, 2);
    std::string_view day = sv.substr(6, 2);
    std::string_view hour = sv.substr(8, 2);
    std::string_view min = sv.substr(10, 2);
    std::string_view sec = sv.substr(12, 9);
    lxw_datetime dt;
    std::from_chars_result r;
    r = std::from_chars(year.data(), year.data() + year.size(), dt.year);
    r = std::from_chars(month.data(), month.data() + month.size(), dt.month);
    r = std::from_chars(day.data(), day.data() + day.size(), dt.day);
    r = std::from_chars(hour.data(), hour.data() + hour.size(), dt.hour);
    r = std::from_chars(min.data(), min.data() + min.size(), dt.min);
    r = std::from_chars(sec.data(), sec.data() + sec.size(), dt.sec);

    return dt;
}


void ScriptProgramsXML(ScriptData& data)
{
    ZoneScoped;
    pugi::xml_document doc = GetXmlDocFromFile(data.output.c_str());
    if (doc.empty())
        return;
    TRACY_LOCK(data.workbook->lock);
    lxw_workbook* book = data.workbook->data;
    lxw_worksheet* sheet = workbook_add_worksheet(book, data.name.c_str());
    lxw_format* title_format = CreateTitleFormat(book);
    worksheet_write_string(sheet, 0, 0, "Program", title_format);
    worksheet_write_string(sheet, 0, 1, "Version", title_format);
    worksheet_set_row(sheet, 0, 30, NULL);
    lxw_format* data_format = CreateDataFormat(book);

    const pugi::xml_node programs = doc.child("Programs");
    i32 i = 1;
    for (pugi::xml_node program = programs.child("Program"); program; program = program.next_sibling("Program"))
    {
        const pugi::xml_node display_name = program.child("DisplayName");
        const pugi::xml_node display_vers = program.child("DisplayVersion");
        const char* name = display_name.child_value();
        const char* vers = display_vers.child_value();
        worksheet_write_string(sheet, i, 0, name, data_format);
        worksheet_write_string(sheet, i, 1, vers, data_format);
        ++i;
    }
}

void WriteTypeToXLSX(lxw_worksheet* sheet, u32 row, u32 col, const char* s, const VarType type, lxw_format* format)
{

#define FROM_UCHARS_AND_ERROR_OR_WRITE(_type)                   \
{                                                               \
    _type v = (_type)strtoull(s, nullptr, 10);                         \
    worksheet_write_number(sheet, row, col, (double)v, format); \
    break;                                                      \
}REQUIRE_SEMICOLON

#define FROM_ICHARS_AND_ERROR_OR_WRITE(_type)                   \
{                                                               \
    _type v = (_type)strtoll(s, nullptr, 10);                         \
    worksheet_write_number(sheet, row, col, (double)v, format); \
    break;                                                      \
}REQUIRE_SEMICOLON

    if (!s || !s[0])
        return;

    const char* s_end = s + std::strlen(s);

    switch (type)
    {
    case VarType_i8:    FROM_ICHARS_AND_ERROR_OR_WRITE(i8);
    case VarType_i16:   FROM_ICHARS_AND_ERROR_OR_WRITE(i16);
    case VarType_i32:   FROM_ICHARS_AND_ERROR_OR_WRITE(i32);
    case VarType_i64:   FROM_ICHARS_AND_ERROR_OR_WRITE(i64);
    case VarType_u8:    FROM_UCHARS_AND_ERROR_OR_WRITE(u8);
    case VarType_u16:   FROM_UCHARS_AND_ERROR_OR_WRITE(u16);
    case VarType_u32:   FROM_UCHARS_AND_ERROR_OR_WRITE(u32);
    case VarType_u64:   FROM_UCHARS_AND_ERROR_OR_WRITE(u64);
    case VarType_Bool: worksheet_write_boolean(sheet, row, col, GetBoolFromString(s), format); break;
    case VarType_Datetime:
    {
        lxw_datetime dt = StringToDatetime(s);
        worksheet_write_datetime(sheet, row, col, &dt, format);
        break;
    }
    default: FAIL; [[fallthrough]];
    case VarType_String: worksheet_write_string(sheet, row, col, s, format); break;
    }
#undef FROM_UCHARS_AND_ERROR_OR_WRITE
#undef FROM_ICHARS_AND_ERROR_OR_WRITE
}

void ScriptProcessorXML(ScriptData& data)
{
    ZoneScoped;
    pugi::xml_document doc = GetXmlDocFromFile(data.output.c_str());
    if (doc.empty())
        return;
    TRACY_LOCK(data.workbook->lock);
    lxw_workbook* book = data.workbook->data;
    lxw_worksheet* sheet = workbook_add_worksheet(book, data.name.c_str());
    lxw_format* title_format = CreateTitleFormat(book);
    lxw_format* data_format = CreateDataFormat(book);

    const pugi::xml_node results = doc.child("COMMAND").child("RESULTS");
    const pugi::xml_node inst = results.child("CIM").child("INSTANCE");

    i32 i = 0;
    for (pugi::xml_node prop = inst.child("PROPERTY"); prop; prop = prop.next_sibling("PROPERTY"))
    {
        const char* key = prop.attribute("NAME").value();
        const char* type = prop.attribute("TYPE").value();

        worksheet_write_string(sheet, 0, i, key, title_format);
        const VarType t = GetVarTypeFromString(type);
        const char* v = prop.child_value("VALUE");
        WriteTypeToXLSX(sheet, 1, i, v, t, data_format);
        ++i;
    }
}

void ScriptNetworkXML(const ScriptData& data)
{
    ZoneScoped;
    //1. load all the networks
    //2. fill out the data from network_settings that match
    //3. output data to excel in a reasonable format

    struct NetworkInterface {
        std::string Description;
        bool DHCPEnabled;
        bool IPEnabled;
        std::string MACAddress;
        std::string SettingID; //GUID
    };

    struct NetworkSettings {
        std::string guid; //GUID
        bool EnabledDHCP;
        std::string IPAddress;
        std::string SubnetMask;
        std::string DefaultGateway;
        i32 DefaultGatewayMetric;
        std::string DhcpIPAddress;
        std::string DhcpSubnetMask;
        std::string DhcpServer;
        std::vector<std::string> DhcpNameServer;
        std::string DhcpDefaultGateway;
        std::string DhcpDomain;
    };

    const char* interfaces_filename = "networks.xml";
    const char* settings_filename = "network_settings.xml";
    pugi::xml_document interface_doc = GetXmlDocFromFile(PathConcat(data.output, interfaces_filename).c_str());
    pugi::xml_document settings_doc = GetXmlDocFromFile(PathConcat(data.output, settings_filename).c_str());
    std::vector<NetworkInterface> interfaces;
    std::vector<NetworkSettings> settings;

    const pugi::xml_node cim = interface_doc.child("COMMAND").child("RESULTS").child("CIM");
    for (pugi::xml_node inst = cim.child("INSTANCE"); inst; inst = inst.next_sibling("INSTANCE"))
    {
        NetworkInterface net = {};
        for (pugi::xml_node prop = inst.child("PROPERTY"); prop; prop = prop.next_sibling("PROPERTY"))
        {
            const char* key = prop.attribute("NAME").value();
            const char* type = prop.attribute("TYPE").value();
            const char* value = prop.child_value("VALUE");

            if (StringCompare(StringCase_Insensitive, key, "Description"))
                net.Description = value;
            else if (StringCompare(StringCase_Insensitive, key, "DHCPEnabled"))
                net.DHCPEnabled = GetBoolFromString(value);
            else if (StringCompare(StringCase_Insensitive, key, "IPEnabled"))
                net.IPEnabled = GetBoolFromString(value);
            else if (StringCompare(StringCase_Insensitive, key, "MACAddress"))
                net.MACAddress = value;
            else if (StringCompare(StringCase_Insensitive, key, "SettingID"))
            {
                if (value)
                {
                    net.SettingID = value;
                    if (value[0] == '{')
                    {
                        net.SettingID = net.SettingID.substr(1, net.SettingID.size() - 2);
                    }
                }
            }
            else
                FAIL;
        }
        interfaces.push_back(net);
    }

    const pugi::xml_node inters = settings_doc.child("NetworkInterfaces");
    for (pugi::xml_node interface = inters.child("Interface"); interface; interface = interface.next_sibling("interfaceANCE"))
    {
        NetworkSettings set = {};
        set.guid = interface.attribute("GUID").value();
        if (set.guid.size() > 36)
        {
            //format the guid
            //"HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces\{4B0712E7-2915-47FE-943A-294969D41C69}"
            set.guid = set.guid.substr(set.guid.size() - 36 - 1, 36);
        }
        set.EnabledDHCP = GetBoolFromString(interface.child_value("EnableDHCP"));
        set.DefaultGatewayMetric = atoi(interface.child_value("DefaultGatewayMetric"));
        set.IPAddress = interface.child_value("IPAddress");
        set.SubnetMask = interface.child_value("SubnetMask");
        set.DefaultGateway = interface.child_value("DefaultGateway");
        set.DhcpIPAddress = interface.child_value("DhcpIPAddress");
        set.DhcpSubnetMask = interface.child_value("DhcpSubnetMask");
        set.DhcpServer = interface.child_value("DhcpServer");
        set.DhcpDefaultGateway = interface.child_value("DhcpDefaultGateway");
        set.DhcpDomain = interface.child_value("DhcpDomain");
        set.DhcpNameServer = TextToStringArray(interface.child_value("DhcpNameServer"), " ");
        settings.push_back(set);
    }

    TRACY_LOCK(data.workbook->lock);
    lxw_workbook* book = data.workbook->data;
    lxw_worksheet* sheet = workbook_add_worksheet(book, data.name.c_str());
    lxw_format* title_format = CreateTitleFormat(book);
    lxw_format* data_format = CreateDataFormat(book);
#define WORKSHEET_WRITE_KEY_VAL_STRING(_name) \
                worksheet_write_string(sheet, row_i, 0, #_name, data_format);\
                worksheet_write_string(sheet, row_i, 1, set. ## _name ##.c_str(), data_format);\
                ++row_i
    i32 row_i = 0;
    for (i32 i = 0; i < interfaces.size(); i++)
    {
        const NetworkInterface& inter = interfaces[i];

        worksheet_merge_range(sheet, row_i, 0, row_i, 1, inter.Description.c_str(), title_format);
        worksheet_set_row(sheet, row_i, 30, NULL);
        ++row_i;
        worksheet_write_string(sheet, row_i, 0, "MACAddress", data_format);
        worksheet_write_string(sheet, row_i, 1, inter.MACAddress.c_str(), data_format);
        ++row_i;
        worksheet_write_string(sheet, row_i, 0, "DHCPEnabled", data_format);
        worksheet_write_boolean(sheet, row_i, 1, inter.DHCPEnabled, data_format);
        ++row_i;
        worksheet_write_string(sheet, row_i, 0, "IPEnabled", data_format);
        worksheet_write_boolean(sheet, row_i, 1, inter.IPEnabled, data_format);
        ++row_i;

        for (const auto& set : settings)
        {
            if (StringCompare(StringCase_Insensitive, set.guid, inter.SettingID))
            {
                WORKSHEET_WRITE_KEY_VAL_STRING(IPAddress);
                WORKSHEET_WRITE_KEY_VAL_STRING(SubnetMask);
                WORKSHEET_WRITE_KEY_VAL_STRING(DefaultGateway);
                WORKSHEET_WRITE_KEY_VAL_STRING(DhcpIPAddress);
                WORKSHEET_WRITE_KEY_VAL_STRING(DhcpSubnetMask);
                WORKSHEET_WRITE_KEY_VAL_STRING(DhcpServer);
                WORKSHEET_WRITE_KEY_VAL_STRING(DhcpDefaultGateway);
                WORKSHEET_WRITE_KEY_VAL_STRING(DhcpDomain);

                worksheet_write_string(sheet, row_i, 0, "DefaultGatewayMetric", data_format);
                worksheet_write_number(sheet, row_i, 1, set.DefaultGatewayMetric, data_format);
                ++row_i;

                for (i32 j = 0; j < set.DhcpNameServer.size(); ++j)
                {
                    const std::string& s = set.DhcpNameServer[j];
                    const std::string name = ToString("DhcpNameServer %i", j);
                    worksheet_write_string(sheet, row_i, 0, name.c_str(), data_format);
                    worksheet_write_string(sheet, row_i, 1, s.c_str(), data_format);
                    ++row_i;
                }
                break;
            }
        }
        ++row_i;
    }

#undef WORKSHEET_WRITE_KEY_VAL
}

void ScriptSystemInfoXML(const ScriptData& data)
{
    std::vector<KeyValPair> sysinfo_pairs;
    size_t column_widths[PWSH_MAX_COLUMNS] = {};

    {
        ZoneScopedN("os.xml");
        pugi::xml_document doc = GetXmlDocFromFile(PathConcat(data.output, "os.xml").c_str());
        if (doc.empty())
            return;
        const pugi::xml_node inst = doc.child("COMMAND").child("RESULTS").child("CIM").child("INSTANCE");
        i32 i = 1;
        for (pugi::xml_node prop = inst.child("PROPERTY"); prop; prop = prop.next_sibling("PROPERTY"))
        {
            KeyValPair p;
            p.name = prop.attribute("NAME").value();
            p.value = prop.child_value("VALUE");
            p.val_type = GetVarTypeFromString(prop.attribute("TYPE").value());
            sysinfo_pairs.push_back(p);
            column_widths[0] = Max(column_widths[0], p.name.size() + 1);
            column_widths[1] = Max(column_widths[0], p.value.size() + 1);
            ++i;
        }
    }

    {
        ZoneScopedN("computersystem.xml");
        pugi::xml_document doc = GetXmlDocFromFile(PathConcat(data.output, "computersystem.xml").c_str());
        if (doc.empty())
            return;
        const pugi::xml_node inst = doc.child("COMMAND").child("RESULTS").child("CIM").child("INSTANCE");
        i32 i = 1;
        for (pugi::xml_node prop = inst.child("PROPERTY"); prop; prop = prop.next_sibling("PROPERTY"))
        {
            KeyValPair p;
            p.name = prop.attribute("NAME").value();
            p.value = prop.child_value("VALUE");

            if (StringCompare(StringCase_Insensitive, p.name.c_str(), "TotalPhysicalMemory"))
            {
                u64 size = (u64)strtoull(p.value.c_str(), nullptr, 10);
                StringGetReadableByteSize(p.value, size);
                p.val_type = VarType_String;
            }
            else
            {
                const char* type = prop.attribute("TYPE").value();
                p.val_type = GetVarTypeFromString(type);
            }

            column_widths[0] = Max(column_widths[0], p.name.size() + 1);
            column_widths[1] = Max(column_widths[0], p.value.size() + 1);
            sysinfo_pairs.push_back(p);
            ++i;
        }
    }

    {
        ZoneScopedN("timezone.xml");
        pugi::xml_document doc = GetXmlDocFromFile(PathConcat(data.output, "timezone.xml").c_str());
        if (doc.empty())
            return;
        const pugi::xml_node inst = doc.child("COMMAND").child("RESULTS").child("CIM").child("INSTANCE");
        i32 i = 1;
        for (pugi::xml_node prop = inst.child("PROPERTY"); prop; prop = prop.next_sibling("PROPERTY"))
        {
            KeyValPair p;
            p.name = prop.attribute("NAME").value();
            p.value = prop.child_value("VALUE");
            p.val_type = GetVarTypeFromString(prop.attribute("TYPE").value());
            sysinfo_pairs.push_back(p);
            column_widths[0] = Max(column_widths[0], p.name.size() + 1);
            column_widths[1] = Max(column_widths[0], p.value.size() + 1);
            ++i;
        }
    }

    {
        ZoneScopedN("bios.xml");
        pugi::xml_document doc = GetXmlDocFromFile(PathConcat(data.output, "bios.xml").c_str());
        if (doc.empty())
            return;
        const pugi::xml_node inst = doc.child("COMMAND").child("RESULTS").child("CIM").child("INSTANCE");
        i32 i = 1;
        for (pugi::xml_node prop = inst.child("PROPERTY"); prop; prop = prop.next_sibling("PROPERTY"))
        {
            KeyValPair p;
            p.name = ToString("Bios %s", prop.attribute("NAME").value());
            p.value = prop.child_value("VALUE");
            const char* type = prop.attribute("TYPE").value();
            p.val_type = GetVarTypeFromString(type);
            sysinfo_pairs.push_back(p);
            column_widths[0] = Max(column_widths[0], p.name.size() + 1);
            column_widths[1] = Max(column_widths[0], p.value.size() + 1);
            ++i;
        }
    }

    {
        ZoneScopedN("gpu.xml");
        pugi::xml_document doc = GetXmlDocFromFile(PathConcat(data.output, "gpu.xml").c_str());
        if (doc.empty())
            return;
        const pugi::xml_node inst = doc.child("COMMAND").child("RESULTS").child("CIM").child("INSTANCE");
        i32 i = 1;
        for (pugi::xml_node prop = inst.child("PROPERTY"); prop; prop = prop.next_sibling("PROPERTY"))
        {
            KeyValPair p;
            p.name = ToString("Gpu %s", prop.attribute("NAME").value());
            p.value = prop.child_value("VALUE");
            const char* type = prop.attribute("TYPE").value();
            p.val_type = GetVarTypeFromString(type);
            column_widths[0] = Max(column_widths[0], p.name.size() + 1);
            column_widths[1] = Max(column_widths[0], p.value.size() + 1);
            sysinfo_pairs.push_back(p);
            ++i;
        }
    }

    TRACY_LOCK(s_workbook.lock);
    lxw_format* title_format = CreateTitleFormat(s_workbook.data);
    lxw_format* data_format = CreateDataFormat(s_workbook.data);
    lxw_worksheet* sheet = workbook_add_worksheet(s_workbook.data, "System Info");
    const i32 title_row_index = 0;
    const i32 name_col_index = 0;
    const i32 value_col_index = 1;
    worksheet_set_row(sheet, title_row_index, 30, NULL);
    worksheet_write_string(sheet, title_row_index, name_col_index, "Name", title_format);
    worksheet_write_string(sheet, title_row_index, value_col_index, "Value", title_format);
    for (i32 i = 0; i < sysinfo_pairs.size(); i++)
    {
        const KeyValPair& p = sysinfo_pairs[i];
        worksheet_write_string(sheet, i + 1, name_col_index, p.name.c_str(), data_format);
        WriteTypeToXLSX(sheet, i + 1, value_col_index, p.value.c_str(), p.val_type, data_format);
    }
    ExcelAutoSizeColumnWidth(sheet, column_widths);
}

void ScriptDisksXML(const ScriptData& data)
{
    ZoneScoped;

    struct PhysicalDisk {
        std::string Description;
        std::string DeviceID;
        std::string FirmwareRevision;
        std::string InterfaceType;
        std::string Manufacturer;
        std::string MediaType;
        std::string Model;
        std::string Name;
        u32 Partitions;
        std::string Size;//reformat
        std::string Status;
    };
    const i32 physical_disk_member_count = 11;

    struct LogicalDisk {
        std::string Caption;
        std::string Description;
        std::string DeviceID;
        std::string FileSystem;
        std::string FreeSpace; //reformat
        std::string Size; //reformat
    };
    const i32 logical_disk_member_count = 6;

    std::vector<PhysicalDisk> physical_disks;
    std::vector<LogicalDisk> logical_disks;
    const pugi::xml_document physical_doc = GetXmlDocFromFile(PathConcat(data.output, "physical_disks.xml").c_str());
    const pugi::xml_document logical_doc = GetXmlDocFromFile(PathConcat(data.output, "logical_disks.xml").c_str());
    if (physical_doc.empty() || logical_doc.empty())
        return;

    {
        const pugi::xml_node cim = physical_doc.child("COMMAND").child("RESULTS").child("CIM");
        for (pugi::xml_node inst = cim.child("INSTANCE"); inst; inst = inst.next_sibling("INSTANCE"))
        {
            PhysicalDisk disk = {};
            for (pugi::xml_node prop = inst.child("PROPERTY"); prop; prop = prop.next_sibling("PROPERTY"))
            {
                const char* key = prop.attribute("NAME").value();
                const char* type = prop.attribute("TYPE").value();
                const char* value = prop.child_value("VALUE");

                if (StringCompare(StringCase_Insensitive, key, "Description"))
                    disk.Description = value;
                else if (StringCompare(StringCase_Insensitive, key, "DeviceID"))
                    disk.DeviceID = value;
                else if (StringCompare(StringCase_Insensitive, key, "FirmwareRevision"))
                    disk.FirmwareRevision = value;
                else if (StringCompare(StringCase_Insensitive, key, "InterfaceType"))
                    disk.InterfaceType = value;
                else if (StringCompare(StringCase_Insensitive, key, "Manufacturer"))
                    disk.Manufacturer = value;
                else if (StringCompare(StringCase_Insensitive, key, "MediaType"))
                    disk.MediaType = value;
                else if (StringCompare(StringCase_Insensitive, key, "Model"))
                    disk.Model = value;
                else if (StringCompare(StringCase_Insensitive, key, "Name"))
                    disk.Name = value;
                else if (StringCompare(StringCase_Insensitive, key, "Status"))
                    disk.Status = value;
                else if (StringCompare(StringCase_Insensitive, key, "Size"))
                {
                    u64 size = (u64)strtoull(value, nullptr, 10);
                    StringGetReadableByteSize(disk.Size, size);
                }
                else if (StringCompare(StringCase_Insensitive, key, "Partitions"))
                {
                    u64 size = (u64)strtoull(value, nullptr, 10);
                    disk.Partitions = (u32)size;
                }
                else
                    FAIL;
            }
            physical_disks.push_back(disk);
        }
    }

    {
        const pugi::xml_node cim = logical_doc.child("COMMAND").child("RESULTS").child("CIM");
        for (pugi::xml_node inst = cim.child("INSTANCE"); inst; inst = inst.next_sibling("INSTANCE"))
        {
            LogicalDisk disk = {};
            for (pugi::xml_node prop = inst.child("PROPERTY"); prop; prop = prop.next_sibling("PROPERTY"))
            {
                const char* key = prop.attribute("NAME").value();
                const char* type = prop.attribute("TYPE").value();
                const char* value = prop.child_value("VALUE");


                if (StringCompare(StringCase_Insensitive, key, "Caption"))
                    disk.Caption = value;
                else if (StringCompare(StringCase_Insensitive, key, "Description"))
                    disk.Description = value;
                else if (StringCompare(StringCase_Insensitive, key, "DeviceID"))
                    disk.DeviceID = value;
                else if (StringCompare(StringCase_Insensitive, key, "FileSystem"))
                    disk.FileSystem = value;
                else if (StringCompare(StringCase_Insensitive, key, "FreeSpace"))
                {
                    u64 size = (u64)strtoull(value, nullptr, 10);
                    StringGetReadableByteSize(disk.FreeSpace, size);
                }
                else if (StringCompare(StringCase_Insensitive, key, "Size"))
                {
                    u64 size = (u64)strtoull(value, nullptr, 10);
                    StringGetReadableByteSize(disk.Size, size);
                }
            }
            logical_disks.push_back(disk);
        }
    }

    TRACY_LOCK(s_workbook.lock);
    lxw_worksheet* sheet = workbook_add_worksheet(s_workbook.data, data.name.c_str());
    lxw_format* title_format = CreateTitleFormat(s_workbook.data);
    lxw_format* data_format = CreateDataFormat(s_workbook.data);

    i32 row_i = 0;
    worksheet_set_row(sheet, row_i, 35, NULL);
    worksheet_merge_range(sheet, row_i, 0, row_i, physical_disk_member_count - 1, "Physical Disks", title_format);
    ++row_i;

    //I would kill for some meta programming
    //                    struct PhysicalDisk vvvvvv
    worksheet_write_string(sheet, row_i,  0, "Description",     title_format);
    worksheet_write_string(sheet, row_i,  1, "DeviceID",        title_format);
    worksheet_write_string(sheet, row_i,  2, "FirmwareRevision",title_format);
    worksheet_write_string(sheet, row_i,  3, "InterfaceType",   title_format);
    worksheet_write_string(sheet, row_i,  4, "Manufacturer",    title_format);
    worksheet_write_string(sheet, row_i,  5, "MediaType",       title_format);
    worksheet_write_string(sheet, row_i,  6, "Model",           title_format);
    worksheet_write_string(sheet, row_i,  7, "Name",            title_format);
    worksheet_write_string(sheet, row_i,  8, "Partitions",      title_format);
    worksheet_write_string(sheet, row_i,  9, "Size",            title_format);
    worksheet_write_string(sheet, row_i, 10, "Status",          title_format);

    ++row_i;
    for (i32 i = 0; i < physical_disks.size(); i++)
    {
        const PhysicalDisk& d = physical_disks[i];
        worksheet_write_string(sheet, row_i,  0, d.Description.c_str(),     data_format);
        worksheet_write_string(sheet, row_i,  1, d.DeviceID.c_str(),        data_format);
        worksheet_write_string(sheet, row_i,  2, d.FirmwareRevision.c_str(),data_format);
        worksheet_write_string(sheet, row_i,  3, d.InterfaceType.c_str(),   data_format);
        worksheet_write_string(sheet, row_i,  4, d.Manufacturer.c_str(),    data_format);
        worksheet_write_string(sheet, row_i,  5, d.MediaType.c_str(),       data_format);
        worksheet_write_string(sheet, row_i,  6, d.Model.c_str(),           data_format);
        worksheet_write_string(sheet, row_i,  7, d.Name.c_str(),            data_format);
        worksheet_write_number(sheet, row_i,  8, (double)d.Partitions,      data_format);
        worksheet_write_string(sheet, row_i,  9, d.Size.c_str(),            data_format);
        worksheet_write_string(sheet, row_i, 10, d.Status.c_str(),          data_format);
        ++row_i;
    }
    //adding space between the two sections
    ++row_i;
    ++row_i;


    worksheet_set_row(sheet, row_i, 35, NULL);
    worksheet_merge_range(sheet, row_i, 0, row_i, logical_disk_member_count - 1, "Logical Disks", title_format);
    ++row_i;

    //                   struct LogicalDisk  vvvvvv
    worksheet_write_string(sheet, row_i,  0, "Caption",     title_format);
    worksheet_write_string(sheet, row_i,  1, "Description", title_format);
    worksheet_write_string(sheet, row_i,  2, "DeviceID",    title_format);
    worksheet_write_string(sheet, row_i,  3, "FileSystem",  title_format);
    worksheet_write_string(sheet, row_i,  4, "FreeSpace",   title_format);
    worksheet_write_string(sheet, row_i,  5, "Size",        title_format);

    ++row_i;
    for (i32 i = 0; i < logical_disks.size(); i++)
    {
        const LogicalDisk& d = logical_disks[i];
        worksheet_write_string(sheet, row_i,  0, d.Caption.c_str(),     data_format);
        worksheet_write_string(sheet, row_i,  1, d.Description.c_str(), data_format);
        worksheet_write_string(sheet, row_i,  2, d.DeviceID.c_str(),    data_format);
        worksheet_write_string(sheet, row_i,  3, d.FileSystem.c_str(),  data_format);
        worksheet_write_string(sheet, row_i,  4, d.FreeSpace.c_str(),   data_format);
        worksheet_write_string(sheet, row_i,  5, d.Size.c_str(),        data_format);
        ++row_i;
    }
}

void ConvertFolderToXLSX(const Path& path)
{
    bool valid = false;
    Path quool_tool_info_path = path / L"quooltoolinfo.xml";
    std::string computer_name;
    if (fs::exists(quool_tool_info_path))
    {

        pugi::xml_document doc = GetXmlDocFromFile(quool_tool_info_path.string().c_str());
        if (!doc.empty())
        {
            const pugi::xml_node sys = doc.child("SystemIdentity");
            computer_name = sys.child_value("ComputerName");
            const char* family = sys.child_value("Family");
            const char* type = sys.child_value("Type");
            const char* version = sys.child_value("Version");

            VALIDATE(StringCompare(StringCase_Insensitive, family, "Windows"));
            VALIDATE(StringCompare(StringCase_Insensitive, type, "XP"));
            VALIDATE(StringCompare(StringCase_Insensitive, version, "1"));
            valid = true;
        }
    }
    if (!valid)
    {
        DebugPrint("Error: Failed to convert folder to XLSX, couldn't get proper type.txt");
        return;
    }
    if (!computer_name.size())
    {
        DebugPrint("Error: Failed to get computer name from quooltoolinfo.xml");
        computer_name = "SystemInfo";
    }

    //Path output_folder;
    //GetOutputFolder(output_folder, td);
    const Path excel_file = path / (computer_name + ".xlsx");
    {
        TRACY_LOCK(s_workbook.lock);
        s_workbook.data = workbook_new(excel_file.string().c_str());
    }

    {
        ScriptData sd = {
            .name = "System Info",
            .output = path.string(),
            .workbook = &s_workbook,
        };
        ScriptSystemInfoXML(sd);
    }

    {
        ScriptData sd = {
            .name = "Network",
            .output = path.string(),
            .workbook = &s_workbook,
        };
        ScriptNetworkXML(sd);
    }

    {
        Path file = path / L"processor.xml";
        if (fs::exists(file))
        {
            ScriptData sd = {
                .name = "Processor",
                .output = file.string(),
                .workbook = &s_workbook,
            };
            ScriptProcessorXML(sd);
        }
    }

    {
        ScriptData sd = {
            .name = "Disks",
            .output = path.string(),
            .workbook = &s_workbook,
        };
        ScriptDisksXML(sd);
    }

    {
        Path file = path / L"programs.xml";
        if (fs::exists(file))
        {
            ScriptData sd = {
                .name = "Programs",
                .output = file.string(),
                .workbook = &s_workbook,
            };
            ScriptProgramsXML(sd);
        }
    }

    {
        Path filepath = path / L"netstat.txt";
        std::string data;
        FileReadAll(data, filepath);
        if (data.size())
        {
            ScriptData sd = {
                .name = "Netstat",
                .output = data,
                .workbook = &s_workbook,
            };
            ScriptNetstat(sd);
        }
        else
        {
            DebugPrint("Warning: Couldn't locate/load netstat.txt: %s", filepath.string().c_str());
        }
    }

    workbook_close(s_workbook.data);
}

void ScriptNetworkXML(ScriptData& data)
{
    //ZoneScoped;
    //PowershellResponse array;
    //ParseCSV(array, data.output, data.using_quotes);
    //if (!array.size())
    //{
    //    FAIL;
    //    return;
    //}
    //TRACY_LOCK(data.workbook->lock);
    //lxw_workbook* book = data.workbook->data;
    //lxw_worksheet* sheet = workbook_add_worksheet(book, data.name.c_str());
    //size_t column_widths[16] = {};
    //ExcelWriteTitles(book, sheet, column_widths, array);
    //ExcelWriteData(book, sheet, column_widths, array);
    //ExcelAutoSizeColumnWidth(sheet, column_widths);
    ZoneScoped;
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(data.output.c_str());
    if (!result)
    {
        DebugPrint("Error description: %s", result.description());
        DebugPrint("Error offset: %i (error at [...%i]\n", result.offset);
        FAIL;
        return;
    }
    if (doc.empty())
    {
        DebugPrint("Error %s, failed to get data from doc.load_string(): %s", data.name.c_str(), data.output.c_str());
        FAIL;
        return;
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_workbook* book = data.workbook->data;
    lxw_worksheet* sheet = workbook_add_worksheet(book, data.name.c_str());
    lxw_format* title_format = CreateTitleFormat(book);
    lxw_format* data_format = CreateDataFormat(book);

    struct NetworkInfo {
        const char* Description;
        const char* IPEnabled;
        const char* IPAddress;
        const char* IPSubnet;
        const char* SettingID;
        const char* DefaultIPGateway;
        const char* DHCPEnabled;
        const char* DHCPServer;
        const char* DNSDomain;
        const char* DNSDomainSuffixSearchOrder;
        const char* DNSHostName;
        const char* DomainDNSRegistrationEnabled;
        const char* MACAddres;
    };

    const pugi::xml_node objects = doc.child("Objects");
    i32 i = 1;
    for (pugi::xml_node object = objects.child("Object"); object; object = object.next_sibling("Object"))
    {
        NetworkInfo net = {};
        for (pugi::xml_node prop = object.child("Property"); prop; prop = prop.next_sibling("Property"))
        {
            const char* name = prop.attribute("Name").value();
            const char* value = prop.child_value();
            //if (StringCompare(StringCase_Insensitive, key, "Description"))
               // net.Description = value;
            //else if (StringCompare(StringCase_Insensitive, key, "DeviceID"))
            //    net.DeviceID = value;
            //else if (StringCompare(StringCase_Insensitive, key, "FirmwareRevision"))
            //    net.FirmwareRevision = value;
            //else if (StringCompare(StringCase_Insensitive, key, "InterfaceType"))
            //    net.InterfaceType = value;
            //else if (StringCompare(StringCase_Insensitive, key, "Manufacturer"))
            //    net.Manufacturer = value;
            //else if (StringCompare(StringCase_Insensitive, key, "MediaType"))
            //    net.MediaType = value;
            //else if (StringCompare(StringCase_Insensitive, key, "Model"))
            //    net.Model = value;
            //else if (StringCompare(StringCase_Insensitive, key, "Name"))
            //    net.Name = value;
            //else if (StringCompare(StringCase_Insensitive, key, "Status"))
            //    net.Status = value;

            //const pugi::xml_node display_vers = prop.child("DisplayVersion");
            //const char* name = display_name.child_value();
            //const char* vers = display_vers.child_value();
            //worksheet_write_string(sheet, i, 0, name, data_format);
            //worksheet_write_string(sheet, i, 1, vers, data_format);
            ++i;
        }
    }
}

ScriptInfo s_scripts[] = {
    { .name = "System Info",.func = ScriptCsv,          .cmdline = g_script_systeminfo_text,    },
    { .name = "ipconfig",   .func = ScriptIpconfig,     .cmdline = g_script_ipconfig_text,      },
    { .name = "Network",    .func = ScriptNetworkXML,   .cmdline = g_script_network_text,       },
    { .name = "Netstat TCP",.func = ScriptCsv,          .cmdline = g_script_netstat_tcp_text,   },
    { .name = "Netstat UPD",.func = ScriptCsv,          .cmdline = g_script_netstat_udp_text,   },
    { .name = "Programs",   .func = ScriptCsv,          .cmdline = g_script_programs_text,      },
    { .name = "Processor",  .func = ScriptCsv,          .cmdline = g_script_processor_text,     },
    { .name = "Disk",       .func = ScriptCsv,          .cmdline = g_script_disk_text,          },
};

std::string s_log;

void GetOutputFolder(Path& out, const ToolsData& td)
{
    out.clear();
    if (fs::exists(td.output_path))
        out = Path(td.output_path);
}

void ImguiLog(const std::string& s)
{
    DebugPrint(s.c_str());
    s_log += s + "\n";
}
void ImguiLog(const std::wstring& ws)
{
    std::string s;
    ConvertWideCharToMultiByte(s, ws);
    ImguiLog(s);
}

void ToolsImGui(ToolsData& td)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    Threading& threading = Threading::GetInstance();
    ImGuiWindowFlags section_flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove;

    //State
    i32 enabled_scripts = 0;
    i32 completed_scripts = 0;
    for (i32 i = 0; i < arrsize(s_scripts); i++)
    {
        ScriptInfo& s = s_scripts[i];
        if (!FlagExists(s.flags, ScriptInfoFlags_Enabled))
            continue;
        enabled_scripts++;
        if (s_scripts[i].completed)
            completed_scripts++;
    }
    if (td.state == ScriptState_Scripts && enabled_scripts == completed_scripts)
    {
        td.state = ScriptState_Workbook;
        WorkbookJob* job = new WorkbookJob();
        job->workbook = &s_workbook;
        job->state = &td.state;
        threading.SubmitJob(job);
    }
    const bool scripts_running = td.state == ScriptState_Scripts;

    
    #define FILES_TITLE "File Paths"
    if (ImGui::BeginChild(FILES_TITLE, { 0, 90 }, true, section_flags))
    {
        ZoneScopedN(FILES_TITLE);
        ImguiTextCentered(FILES_TITLE);
        ImGui::NewLine();

        {
            bool locked = td.lock.try_lock();
            Defer{ td.lock.unlock(); };
            ImGui::BeginDisabled(!locked);
            ImGui::BeginGroup();
            if (ImguiPath("Output Folder", "Please select the folder to output data to", td.output_path))
                WriteSettings(&g_data.settings, g_settings_filename);
            ImGui::EndGroup();
            ImGui::EndDisabled();
        }


    }
    ImGui::EndChild();

    #define SCRIPTS_TITLE "Scripts"
    if (ImGui::BeginChild(SCRIPTS_TITLE, { 0, 125 }, true, section_flags))
    {
        ZoneScopedN(SCRIPTS_TITLE);
        ImguiTextCentered(SCRIPTS_TITLE);
        ImGui::NewLine();

        std::error_code ec;
        ImGui::BeginDisabled(scripts_running);
        float height = 40;
        const ImVec2 button_size(125.0f, 60.0f);
        ImGui::SetCursorPosX(Max((ImGui::GetContentRegionAvail().x - (arrsize(s_scripts) * button_size.x)) * 0.5f, ImGui::GetStyle().ItemSpacing.x));
        const float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        for (i32 i = 0; i < arrsize(s_scripts); i++)
        {
            ScriptInfo& s = s_scripts[i];
            ImGui::PushID(i);
            ImGui::BeginDisabled(s.completed);
            ImGui::BeginGroup();

            bool pressed = ImGui::InvisibleButton("##btn", button_size);
            bool hovered = ImGui::IsItemHovered();
            bool active = ImGui::IsItemActive();
            if (pressed)
                FlagToggle(s.flags, ScriptInfoFlags_Enabled);

            ImVec2 p_min = ImGui::GetItemRectMin();
            ImVec2 p_max = ImGui::GetItemRectMax();

            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImU32 background_color = ImGui::GetColorU32(ImGuiCol_Button);
            if (active)
                background_color = ImGui::GetColorU32(ImGuiCol_ButtonActive);
            if (hovered)
                background_color = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
            draw->AddRectFilled(p_min, p_max, background_color, 6.0f);
            draw->AddRect(p_min, p_max, ImGui::GetColorU32(ImGuiCol_Border), 6.0f);

            ImVec2 title_size = ImGui::CalcTextSize(s.name.c_str());
            ImVec2 title_pos = {
                p_min.x + (button_size.x - title_size.x) * 0.5f,
                p_min.y + 6.0f
            };
            draw->AddText(title_pos, ImGui::GetColorU32(ImGuiCol_Text), s.name.c_str());

            std::string selected_text = "Enabled";
            ImU32 selected_color = IM_COL32(0, 255, 0, 255);
            if (s.completed)
            {
                selected_text = "Completed";
                selected_color = IM_COL32(0, 255, 0, 255);
            }
            else if (!FlagExists(s.flags, ScriptInfoFlags_Enabled))
            {
                selected_text = "Disabled";
                selected_color = IM_COL32(255, 0, 0, 255);
            }
            ImVec2 value_size = ImGui::CalcTextSize(selected_text.c_str());
            ImVec2 value_pos = {
                p_min.x + (button_size.x - value_size.x) * 0.5f,
                p_min.y + (button_size.y - value_size.y) * 0.5f + 6.0f
            };
            draw->AddText(value_pos, selected_color, selected_text.c_str());

            ImGui::EndGroup();
            ImGui::EndDisabled();
            ImGui::PopID();

            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + ImGui::GetStyle().ItemSpacing.x + button_size.x; // Expected position if next button was on same line

            float text_start = ImGui::GetCursorPosX() + ImGui::GetStyle().ItemSpacing.x / 2;
            if (i + 1 < arrsize(s_scripts) && next_button_x2 < window_visible_x2)
                ImGui::SameLine();
        }
        ImGui::EndDisabled();
    }
    ImGui::EndChild();



    #define ACTION_TITLE "Action"
    const ImVec2 action_scale = { 0.15f, 0 };
    const ImVec2 action_size = HadamardProduct(viewport->WorkSize, action_scale);
    if (ImGui::BeginChild(ACTION_TITLE, action_size, true, section_flags))
    {
        ZoneScopedN(ACTION_TITLE);

        ImGui::BeginDisabled(scripts_running || (!td.output_path.empty() && !fs::exists(td.output_path)));
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        if (ImGui::Button("Run Scripts", avail))
        {
            ZoneScopedN("Run Scripts");
            ImguiLog("Running Scripts:");
            td.state = ScriptState_Scripts;

            Path output_folder;
            GetOutputFolder(output_folder, td);
            const Path excel_file = output_folder / (g_sysinfo.name + L".xlsx");
            {
                TRACY_LOCK(s_workbook.lock);
                s_workbook.data = workbook_new(excel_file.string().c_str());
            }
            for (i32 i = 0; i < arrsize(s_scripts); i++)
            {
                ScriptInfo& s = s_scripts[i];
                if (!FlagExists(s.flags, ScriptInfoFlags_Enabled) || s.completed)
                    continue;

                ZoneScopedN("Run Script");
                ScriptJob* job = new ScriptJob();
                job->path;
                job->args = s.cmdline;
                const std::string name = s.name + ".txt";
                job->func = s.func;
                job->data.workbook = &s_workbook;
                job->data.name = s.name;
                job->completed = &s.completed;
                threading.SubmitJob(job);

                ImguiLog(ToString("Running: %s", s.name.c_str()));
            }
        }
        ImGui::EndDisabled();
        
    }
    ImGui::EndChild();


#if 1
    ImGui::SameLine();
    #define LOG_TITLE "Progress"
    const ImVec2 progress_scale = { 0, 0 };
    ImVec2 progress_size = HadamardProduct(viewport->WorkSize, progress_scale);
    if (ImGui::BeginChild(LOG_TITLE, progress_size, ImGuiChildFlags_Borders, section_flags))
    {
        ZoneScopedN(LOG_TITLE);
        ImguiTextCentered(LOG_TITLE);
        //ImGui::Separator();

        float progress_bar_height = 50;
        switch (td.state)
        {
        case ScriptState_Scripts:
        {
            const ImVec2 ip_scale = { 0, 0.75 };
            ImVec2 ip_size = HadamardProduct(ImGui::GetContentRegionAvail(), ip_scale);
            if (ImGui::BeginChild("IndividualProgress", ip_size, ImGuiChildFlags_Borders, section_flags))
            {
                const float individual_height = 30.0f;
                for (i32 i = 0; i < arrsize(s_scripts); i++)
                {
                    ScriptInfo& s = s_scripts[i];
                    if (!FlagExists(s.flags, ScriptInfoFlags_Enabled))
                        continue;

                    if (s.completed)
                    {
                        ImGui::ProgressBar(1.0f, ImVec2(-FLT_MIN, individual_height), s.name.c_str());
                    }
                    else
                    {
                        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-FLT_MIN, individual_height), s.name.c_str());
                    }

                }
            }
            ImGui::EndChild();

            ImVec2 max = ImGui::GetWindowContentRegionMax();
            ImGui::SetCursorPosY(max.y - progress_bar_height);

            if (td.state != ScriptState_Scripts)
                ImGui::ProgressBar(1.0f, ImVec2(-FLT_MIN, progress_bar_height), "Completed");
            else
            {
                std::string title = ToString("%i/%i", completed_scripts, enabled_scripts);
                ImGui::ProgressBar(float(completed_scripts) / float(enabled_scripts), ImVec2(-FLT_MIN, progress_bar_height), title.c_str());
            }
            break;
        }
        case ScriptState_Workbook:
        {
            ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-FLT_MIN, progress_bar_height), "Saving Excel File");
            break;
        }
        case ScriptState_Finished:
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Finished");
            break;
        }
        }

    }
    ImGui::EndChild();



    const char* popup_name = "Drop Files";
    if (g_sysinfo.drop_active && !ImGui::IsPopupOpen(popup_name))
        ImGui::OpenPopup(popup_name);
    if (!g_sysinfo.drop_file.empty())
    {
        ConvertFolderToXLSX(g_sysinfo.drop_file);
        g_sysinfo.drop_file.clear();
    }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.8f, 0.8f, 0.8f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 15.0f);
    ImVec2 dnd_size = { 768.0f, 500.0f };//{ 1024, 600 }
    ImGui::SetNextWindowSize(dnd_size);
    ImGui::SetNextWindowPos((viewport->Size - dnd_size) / 2.0f);
    if (ImGui::BeginPopupModal(popup_name, NULL, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        if (!g_sysinfo.drop_active)
            ImGui::CloseCurrentPopup();

        ImVec2 pop_size = ImGui::GetWindowSize();
        ImguiTextCentered("Drag and Drop Folders Here");
        ImGui::Separator();

        const char* cancel_button_text = "Cancel Drag and Drop";
        float cancel_text_width = ImGui::CalcTextSize(cancel_button_text).x;
        float cancel_button_height = 50.0f;
        ImVec2 pad = ImGui::GetStyle().WindowPadding;

        ImVec2 size(dnd_size.x - pad.x * 2, dnd_size.y - pad.y * 2 - ImGui::GetCursorPosY() - cancel_button_height);
        ImVec2 min = ImGui::GetCursorScreenPos();
        ImVec2 max = ImVec2(min.x + size.x, min.y + size.y);
        ImGui::InvisibleButton("Drop Zone", size);
        ImU32 bg_col = IM_COL32(50, 50, 50, 50);
        if (g_frame_index == 1000)
            i32 test = 1;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
            bg_col = IM_COL32(100, 100, 100, 150);
        ImU32 borderColor = IM_COL32(200, 200, 200, 255);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(min, max, bg_col);
        ImguiDrawDashedRect(drawList, min, max, borderColor, 2.0f, 6.0f, 4.0f);

        ImGui::SetCursorPosX((pop_size.x - cancel_text_width) / 2);
        if (ImGui::Button(cancel_button_text, ImVec2(0, cancel_button_height))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

#else

    ImGui::SameLine();
    #define LOG_TITLE "Log"
    const ImVec2 log_scale = { 0, 0 };
    ImVec2 log_size = HadamardProduct(viewport->WorkSize, log_scale);
    if (ImGui::BeginChild(LOG_TITLE, log_size, ImGuiChildFlags_Borders, section_flags))
    {
        ZoneScopedN(LOG_TITLE);
        TextCentered(LOG_TITLE);
        ImGui::Separator();

        ImDrawList* draw = ImGui::GetWindowDrawList();
        //Screen-space position of the content area
        ImVec2 area_min = ImGui::GetCursorScreenPos();
        ImVec2 area_size = ImGui::GetContentRegionAvail();
        ImVec2 area_max = ImVec2(area_min.x + area_size.x, area_min.y + area_size.y);

        //Clip to child window and add black console background
        draw->PushClipRect(area_min, area_max, true);
        draw->AddRectFilled(area_min, area_max, IM_COL32(0, 0, 0, 255), 6.0f, ImDrawFlags_RoundCornersNone);
        draw->PopClipRect();

        //Padding inside the log area
        ImGui::SetCursorScreenPos(area_min + ImVec2(8, 6));
        ImGui::PushFont(g_data.fonts[FontIndex_Monospace]);
#if 1
        ImGui::TextUnformatted(s_log.c_str());
#else
        ImGui::TextUnformatted("This is a test of the monospace font");
#endif
        ImGui::PopFont();

    }
    ImGui::EndChild();
#endif
}
