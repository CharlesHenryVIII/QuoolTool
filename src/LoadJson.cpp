#include "LoadJson.h"
#include "WinInterop_File.h"
#include "VideoData.h"
#include "WinInterop.h"

#include "json.hpp"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

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


//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PathSettings, source_path, dest_path, mkv_path);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SettingsCitect, program_data_path, program_files_path, deployment_path)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, citect, color, style)

//void to_json(json& j, const std::wstring& ws)
//{
//    std::string s;
//    ConvertWideCharToMultiByte(s, ws);
//    j = s.c_str();
//}
//void from_json(const json& j, std::wstring& ws)
//{
//    std::string s;
//    j.get_to(s);
//    ConvertMultibyteToWideChar(ws, s);
//}
//
//const char* program_data_name   = "program_data";
//const char* program_files_name  = "program_files";
//const char* deployment_name     = "deployment";
//#if _DEBUG
////static_assert(sizeof(Settings) == 128);
//#endif
//
//template <typename T>
//void from(const json& j, T& a, const char* name)
//{
//    if (j.find(name) != j.end())
//        from_json(j[name], a);
//}
//template <typename T>
//void to(json& j, const T& a, const char* name)
//{
//    if (j.find(name) != j.end())
//        from_json(j[name], a);
//}
//
//void to_json(json& j, const SettingsCitect& ps)
//{
//    to_json(j[program_data_name],   ps.program_data_path);
//    to_json(j[program_files_name],  ps.program_files_path);
//    to_json(j[deployment_name],     ps.deployment_path);
//}
//void from_json(const json& j, SettingsCitect& ps)
//{
//    from_json(j[program_data_name],     ps.program_data_path);
//    from_json(j[program_files_name],    ps.program_files_path);
//    from_json(j[deployment_name],       ps.deployment_path);
//}
//
//const char* citect_name = "citect";
//const char* color_name = "color";
//const char* style_name = "style";
//void to_json(json& j, const Settings& ps)
//{
//    to_json(j[citect_name],     ps.citect);
//    to_json(j[color_name],      ps.color);
//    to_json(j[style_name],      ps.style);
//}
//void from_json(const json& j, Settings& ps)
//{
//    from(j, ps.citect, citect_name);
//    from_json(j[color_name],      ps.color);
//    from_json(j[style_name],      ps.style);
//}

template <typename T>
void WriteJson(const T* s, const std::wstring& filename)
{
    VALIDATE(s);
	json data = *s;
	File file = File(filename, FileMode_Write, true);
	file.Write(data.dump(4));
}

template <typename T>
bool LoadJson(T* s, const std::wstring& filename)
{
    VALIDATE_V(s, false);
	std::ifstream file(filename); // TODO : Update to file system
    VALIDATE_V(file.good(), false);
	try
	{
		json data;
		data = json::parse(file, nullptr, true);
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

#define READWRITE_JSON_CPP(name)\
struct name;\
void Write ## name(const name* s, const std::wstring& filename) { WriteJson(s, filename); }\
bool Read  ## name(      name* s, const std::wstring& filename)  { return LoadJson(s, filename); }

READWRITE_JSON_CPP(Settings);