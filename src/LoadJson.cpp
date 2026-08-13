#include "LoadJson.h"
#include "CashUtil/CashUtil.h"
#include "Settings.h"
#include "Networking.h"

#include "json.hpp"

#include <fstream>
#include <iostream>

using Json = nlohmann::json;

#define TO_JSON_COMMON_MEMBER(_name, _struct) j[#_name] = _struct ## . ## _name
#define FROM_JSON_COMMON_MEMBER(_name, _struct) j.at(#_name).get_to(_struct ## . ## _name)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec2,   x, y      );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec3,   x, y, z   );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec4,   x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat2,   x, y      );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat3,   x, y, z   );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat4,   x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat4x3, x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Quat,   x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec2d,  x, y      );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec3d,  x, y, z   );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec4d,  x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat2d,  x, y      );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat3d,  x, y, z   );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat4d,  x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat4x3d,x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Quatd,  x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec2I,  x, y      );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec3I,  x, y, z   );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec4I,  x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec2U,  x, y      );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec3U,  x, y, z   );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec4U,  x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat2I,  x, y      );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat3I,  x, y, z   );
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mat4I,  x, y, z, w);


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThemeSettings, color, style)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EnvironmentVariables, github_api_key)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NetworkSettings, configs)
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NetAdapterConfig, name, ip, gateway, dns1, dns2, dhcp_enabled, ddns_enabled)

void to_json(Json& j, const SysIP4 & ip)
{
    const std::string s = ip.ToString();
    j = s.c_str();
}
void from_json(const Json& j, SysIP4& ip) 
{
    const std::string& s = j;
    ip.FromString(s);
}

void to_json(Json& j, const SysIP4Subnet& sub)
{
    const std::string s = sub.ToString();
    j = s;
}
void from_json(const Json& j, SysIP4Subnet& sub) 
{
    const std::string& s = j;
    sub.FromString(s);
}
    
void to_json(Json& j, const SysNetAdapterConfig& adapter) 
{
    TO_JSON_COMMON_MEMBER(name,         adapter);
    TO_JSON_COMMON_MEMBER(ip,           adapter.ip);
    TO_JSON_COMMON_MEMBER(subnet,       adapter.ip);
    TO_JSON_COMMON_MEMBER(gateway,      adapter);
    TO_JSON_COMMON_MEMBER(dns,          adapter);
    TO_JSON_COMMON_MEMBER(dhcp_enabled, adapter);
    TO_JSON_COMMON_MEMBER(ddns_enabled, adapter);
}
void from_json(const Json& j, SysNetAdapterConfig& adapter) 
{
    FROM_JSON_COMMON_MEMBER(name,           adapter);
    FROM_JSON_COMMON_MEMBER(ip,             adapter.ip);
    FROM_JSON_COMMON_MEMBER(subnet,         adapter.ip);
    FROM_JSON_COMMON_MEMBER(gateway,        adapter);
    FROM_JSON_COMMON_MEMBER(dns,            adapter);
    FROM_JSON_COMMON_MEMBER(dhcp_enabled,   adapter);
    FROM_JSON_COMMON_MEMBER(ddns_enabled,   adapter);
}


template <typename T>
void WriteJson(const T* s, const std::wstring& filename)
{
    VALIDATE(s);
	Json data = *s;
    const Path path = filename;
	std::ofstream file(path); //TODO(CSH): Update to file system
    VALIDATE(file.good());
    VALIDATE(file.is_open());
    file << data.dump(4);
}

template <typename T>
bool LoadJson(T* s, const std::wstring& filename)
{
    VALIDATE_V(s, false);
	std::ifstream file(filename); // TODO : Update to file system
    VALIDATE_V(file.good(), false);
	try
	{
		Json data;
		data = Json::parse(file, nullptr, true);
		*s = data.template get<T>();
        return true;
	}
	catch (...) // TODO : Add different catch methods for both parse errors and type errors
	{
        FAIL;
		*s = T();
		file.close();
        T t = T();
		WriteJson(&t, filename);
	}
    return true;
}

template <typename T>
bool ReadJson(T* s, const Json& j)
{
    VALIDATE_V(s, false);
	try
	{
		*s = j.template get<T>();
        return true;
	}
	catch (...) // TODO : Add different catch methods for both parse errors and type errors
	{
		*s = T();
	}
    return true;
}

#define READWRITE_JSON_CPP(name)\
struct name;\
void Write ## name(const name* s, const std::wstring& filename) { WriteJson(s, filename); }\
bool Read  ## name(      name* s, const std::wstring& filename)  { return LoadJson(s, filename); }

READWRITE_JSON_CPP(EnvironmentVariables);

const wchar_t* s_settings_filename = L"settings.json";
const char* s_network_name = "Network";
const char* s_theme_name = "Themes";
#define JSON_WRITE_COMMON(__name) j[s_ ## __name ## _name] = g_ ## __name ## _settings
#define JSON_READ_COMMON(__name)                            \
if (j.contains(s_ ## __name ## _name))                      \
{                                                           \
    g_ ## __name ## _settings = j[s_ ## __name ## _name];   \
}REQUIRE_SEMICOLON


void WriteSettings()
{
    Json j;
    JSON_WRITE_COMMON(network);
    JSON_WRITE_COMMON(theme);


    const Path path = s_settings_filename;
	std::ofstream file(path); //TODO(CSH): Update to file system
    VALIDATE(file.good());
    VALIDATE(file.is_open());
    file << j.dump(4);
}
bool ReadSettings()
{
	std::ifstream file(s_settings_filename); // TODO : Update to file system
    if (!file.good())
    {
        return false;
    }
    Json j;
    try {
        j = Json::parse(file, nullptr, true);
    }
    catch (...)
    {
        return false;
    }
    JSON_READ_COMMON(network);
    JSON_READ_COMMON(theme);

    return true;
}


bool JsonSafeGet(std::string& out, const void* json_obj, const char* property_name)
{
    JSON_REF_FROM_VOID();
    if (j.contains(property_name))
    {
        out = j[property_name];
        return true;
    }
    return false;
}

