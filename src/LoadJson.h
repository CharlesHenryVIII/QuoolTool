#pragma once

#define JSON_REF_FROM_VOID()\
VALIDATE_V(json_obj, false); \
Json& j = *(Json*)json_obj

#include <string>

#define READWRITE_JSON_H(name)\
struct name;\
void Write ## name(const name* s, const std::wstring& filename);\
bool Read  ## name(      name* s, const std::wstring& filename)

//READWRITE_JSON_H(Settings);
READWRITE_JSON_H(EnvironmentVariables);

#undef READWRITE_JSON_H

void WriteSettings();
bool ReadSettings();

bool JsonSafeGet(std::string& out, const void* json_obj, const char* property_name);
