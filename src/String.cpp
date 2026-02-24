#include "String.h"
#include "WinInterop.h"

const char* ReadEntireFileAsString(const char* fileName)
{
    FILE* file;
    if (file = fopen(fileName, "rb"))
    {
        fseek(file, 0, SEEK_END);
        const long fileLength = ftell(file);
        fseek(file, 0, SEEK_SET);
        char* buffer = new char[fileLength + 1];
        fread(buffer, 1, fileLength, file);
        fclose(file);
        buffer[fileLength] = 0;
        return buffer;
    }
    return nullptr;
}

std::vector<i32> TextToIntArray(const char* text, const char lineEnd)
{
    std::vector<i32> workingNums;
    std::vector<i32> nums;
    workingNums.reserve(10);
    nums.reserve(16);

    for (i32 i = 0; text[i] != 0; i++)
    {

        //ASSERT((data.text[i] - '0' >= 0  && data.text[i] - '0' <= '9') || data.text[i] == lineEnd);
        if (text[i] - '0' >= 0  && text[i] - '0' <= '9')
        {
            i32 value = text[i] - '0';
            workingNums.push_back(value);
        }
        else if (text[i] == lineEnd)
        {
            i32 value = 0;
            for (i32 j = 0; j < workingNums.size(); j++)
            {
                value += (workingNums[j] * (int)pow(10, workingNums.size() - 1 - j));
            }
            ASSERT(value); //should not occurr
            if (value)
            {
                nums.push_back(value);
                workingNums.clear();
            }
        }
    }
    delete text;
    return nums;
}

std::vector<i32> TextToIntArray(const char* text)
{
    std::vector<int> nums;
    //std::string workingText = text;
    std::string_view test = text;
    i32 index = 0;

    for (i32 i = 0; i == 0 || text[i - 1] != 0; i++)
    {
        if (text[i] >= '0' && text[i] <= '9')
            index++;
        else if (index)
        {
            //nums.push_back(atoi(workingText.substr(i - index, index).c_str()));
            nums.push_back(atoi(test.substr(i - index, index).data()));
            index = 0;
        }
    }
    return nums;
}

bool TextDetection(const std::string& string, const std::string& check)
{
    for (i32 i = 0; i + check.size() <= string.size(); i++)
    {
        if (string.substr(i, check.size()) == check)
            return true;
    }
    return false;
}

void TextRemoval(std::string& string, const std::string& removalRef)
{
    std::string_view sv = string;
    auto begin = string.begin();
    auto end = string.begin();
    i32 i = 0;
    while (end != string.end())
    {
        if (sv.substr(i, removalRef.size()) == removalRef)
        {
            begin = end;
            for (i32 j = 0; j < removalRef.size(); j++)
                end++;
            string.erase(begin, end);
            end = string.begin();
            i = 0;
            //return;

        }
        else
        {
            end++;
            i++;
        }
    }
}

void TextRemoveAfter(std::string& string, const std::string& removalRef)
{
    std::string_view sv = string;
    auto begin = string.begin();
    auto end = string.begin();
    i32 i = 0;
    while (end != string.end())
    {
        if (sv.substr(i, removalRef.size()) == removalRef)
        {
            begin = end;
            string.erase(begin, string.end());
            return;
        }
        else
        {
            end++;
            i++;
        }
    }
}

void TextAddition(std::string& string, const std::string& additionalText, const std::string& after)
{
    std::string_view sv = string;
    for (i32 i = 0; i < string.size() - after.size(); i++)
    {
        if (sv.substr(i, (after.size())) == after && !(sv.substr(i, (after.size() + additionalText.size())) == (after + additionalText)))
        {
            string.insert(i + after.size(), additionalText);
        }
    }
}

std::vector<std::string> TextToStringArray(const char* text)
{
    std::string_view sv = text;
    std::vector<std::string> result;

    i32 tokenLength = 0;
    for (i32 i = 0; text[i - 1] != 0; i++)
    {
        if (text[i] >= ' ' && text[i] <= '~')
        {
            tokenLength++;
        }
        else
        {
            if (tokenLength)
            {

                std::string token;
                token = sv.substr(i - tokenLength, tokenLength);
                result.push_back(token);
                tokenLength = 0;
            }
        }
    }
    ASSERT(tokenLength == 0);
    return result;
}

std::vector<std::string> TextToStringArray(const char* text, const char* lineEnd)
{
    ASSERT(lineEnd);
    ASSERT(text);

    std::string_view sv = text;
    std::vector<std::string> result;

    if (*text == '\n')
        int test = 1;

    i32 tokenLength = 0;
    for (i32 i = 0; text[i] != 0; i++)
    {
        bool isLineEnd = false;
        if (text[i] == lineEnd[0] || text[i + 1] == 0)
        {
            i32 incrimenter = 0;
            while (text[i + incrimenter] == lineEnd[incrimenter])
            {

                if (lineEnd[incrimenter + 1] == 0)
                {
                    isLineEnd = true;
                    break;
                }
                incrimenter++;
            }
            if (tokenLength && (isLineEnd || text[i + 1] == 0))
            {

                std::string token;
                token = sv.substr(i - tokenLength, tokenLength + 1);
                result.push_back(token);
                tokenLength = 0;
                i += incrimenter;
                isLineEnd = true;
            }
        }
        if (!isLineEnd && *text != *lineEnd)
            tokenLength++;

    }
    //ASSERT(tokenLength == 0);
    return result;
}

std::vector<std::string> FileToStringArray(const char* fileName)
{
    const char* str = ReadEntireFileAsString(fileName);
    if (str == nullptr)
        return {};
    else
        return TextToStringArray(str);
}

std::vector<std::string> FileToStringArray(const char* fileName, const char* lineEnd)
{
    const char* str = ReadEntireFileAsString(fileName);
    if (str == nullptr)
        return {};
    else
        return TextToStringArray(str, lineEnd);
}

std::vector<i32> FileToIntArray(const char* fileName, const char lineEnd)
{
    const char* str = ReadEntireFileAsString(fileName);
    if (str == nullptr)
        return {};
    else
        return TextToIntArray(str, lineEnd);
}

std::vector<i32> FileToIntArray(const char* fileName)
{
    const char* str = ReadEntireFileAsString(fileName);
    if (str == nullptr)
        return {};
    else
        return TextToIntArray(str);
}

i32 NumberLengthInString(const std::string& string, i32& i)
{

    i32 j = 0;
    while (string[i + j] >= '0' && string[i + j] <= '9')
        j++;
    return j;
}

i32 StringToInt(const std::string& string, i32 i, i32 length)
{
    return atoi(string.substr(i, length).c_str());
}

i32 StringToInt(const std::string& string, i32 i)
{
    return StringToInt(string, i, NumberLengthInString(string, i));
}

bool StringCompare(StringCase case_sensitivity, const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;

    switch (case_sensitivity)
    {
    case StringCase_Sensitive:
        if (strcmp(a.c_str(), b.c_str()) == 0)
            return true;
        break;
    case StringCase_Insensitive:
        if (_stricmp(a.c_str(), b.c_str()) == 0)
            return true;
        break;
    }
    return false;
}

bool ContainsString(const std::wstring& source, const std::wstring& find, StringCase case_insensitive)
{
    std::string s;
    ConvertWideCharToMultiByte(s, source);
    std::string f;
    ConvertWideCharToMultiByte(f, find);
    if (case_insensitive == StringCase_Insensitive)
    {
        ToLower(s);
        ToLower(f);
    }
    if (s.contains(f.c_str()))
        return true;
    return false;
}

bool CopyFile(const Path& source, const Path& dest)
{
    if (source.empty() || dest.empty())
    {
        DebugPrint("Failed to copy file EMPTY: src: \"%s\" dest: \"%s\"", source.string().c_str(), dest.string().c_str());
        FAIL;
        return false;
    }
    std::error_code ec;
    fs::create_directories(dest, ec);
    if (ec)
    {
        DebugPrint("Failed to create directories for dest: \"%s\"", dest.string().c_str());
        FAIL;
        return false;
    }

    bool result = std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec)
    {
        DebugPrint("Failed to copy file src: \"%s\" dest: \"%s\"", source.string().c_str(), dest.string().c_str());
        FAIL;
        return false;
    }
    return result;
}

bool CopyFileRelative(const Path& source, const Path& dest, const Path& relative)
{
    if (source.empty() || dest.empty() || relative.empty())
    {
        DebugPrint("Failed to copy file EMPTY: src: \"%s\" dest: \"%s\" relative: \"%s\"", source.string().c_str(), dest.string().c_str(), relative.string().c_str());
        FAIL;
        return false;
    }

    std::error_code ec;
    fs::create_directories(dest / relative, ec);
    if (ec)
    {
        DebugPrint("Failed to create directories for dest: \"%s\"", (dest / relative).string().c_str());
        FAIL;
        return false;
    }

    bool result = fs::copy_file(source / relative, dest / relative, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        DebugPrint("Failed to copy file src: \"%s\" dest: \"%s\" relative: \"%s\"", source.string().c_str(), dest.string().c_str(), relative.string().c_str());
        FAIL;
        return false;
    }
    return result;
}

bool CopyFolderRelative(const Path& source, const Path& dest, const Path& relative)
{
    if (source.empty() || dest.empty() || relative.empty())
    {
        DebugPrint("Failed to copy folder EMPTY: src: \"%s\" dest: \"%s\" relative: \"%s\"", source.string().c_str(), dest.string().c_str(), relative.string().c_str());
        FAIL;
        return false;
    }

    std::error_code ec;
    fs::create_directories(dest / relative, ec);
    if (ec)
    {
        DebugPrint("Failed to create directories for dest: \"%s\"", (dest / relative).string().c_str());
        FAIL;
        return false;
    }

    fs::copy(source / relative, dest / relative, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        DebugPrint("Failed to copy folder src: \"%s\" dest: \"%s\" relative: \"%s\"", source.string().c_str(), dest.string().c_str(), relative.string().c_str());
        FAIL;
        return false;
    }
    return true;
}

void CopyString(char** dest, const char* source, const u64 max_length)
{
    memmove(*dest, source, Min<size_t>(max_length, strnlen_s(source, max_length)));
}

void PathRemoveExtension(std::wstring& path)
{
    size_t end = path.find_last_of(L".");
    if (end == std::wstring::npos)
    {
        DebugPrint("GetFilename() failed to find \".\" for %s", path.c_str());
        return;
    }

    path.erase(end, path.size() - end);
}

std::wstring PathGetFilename(const std::wstring& path)
{
    size_t start = path.find_last_of(L"\\");
    if (start == std::wstring::npos)
        start = path.find_last_of(L"/");
    if (start == std::wstring::npos)
    {
        DebugPrint("GetFilename() failed to find \"/\" or \"\\\" for %s", path.c_str());
        return path;
    }

    size_t end = path.find_last_of(L".");
    if (end == std::wstring::npos)
    {
        DebugPrint("GetFilename() failed to find \".\" for %s", path.c_str());
        return L"";
    }

    if (start++ >= path.size() || start >= end)
    {
        DebugPrint("GetFilename() failed to get valid range for start(%i) and end(%i) for %s", start, end, path.c_str());
        return L"";
    }

    std::wstring_view view = path;
    std::wstring result;
    result = view.substr(start, end - start);
    return result;
}

std::wstring PathGetFilenameWithExtension(const std::wstring& path)
{
    std::wstring result = L"";
    size_t start = path.find_last_of(L"\\");
    if (start == std::wstring::npos)
        start = path.find_last_of(L"/");
    if (start == std::wstring::npos)
    {
        DebugPrint("Failed to GetFilename() for %s", path.c_str());
        return result;
    }

    if (start++ >= path.size())
    {
        DebugPrint("Failed to GetFilename() for %s", path.c_str());
        return result;
    }

    std::wstring_view view = path;
    result = view.substr(start);
    return result;
}

std::wstring PathGetExtension(const std::wstring& path)
{
    std::wstring result = L"";
    const size_t start = path.find_last_of(L'.');
    if (start == std::wstring::npos)
    {
        DebugPrint("Failed to GetFilenameExtension() for %s", path.c_str());
        return result;
    }

    std::wstring_view view = path;
    result = view.substr(start);
    return result;
}

void PathCleanSlashs(std::wstring& s)
{
    size_t pos = s.find(L'\\');
    while (pos != std::wstring::npos)
    {
        s.replace(pos, 1, L"/", 1);
        pos = s.find(L'\\');
    }
}

std::wstring PathAddEndSlash(const std::wstring& s)
{
    if (s.back() != L'/')
        return s + L"/";
    return s;
}

std::wstring PathConcat(const std::wstring& a, const std::wstring& b)
{
    if (a.back() == L'/' || a.back() == L'\\')
    {
        return a + b;
    }
    return a + L"/" + b;
}
