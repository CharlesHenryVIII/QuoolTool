#include "LoadJson.h"
#include "WinInterop_File.h"
#include "Settings.h"
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


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SettingsCitect, project_path, program_files_path, program_files_86)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, citect, backup_path, color, style)

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