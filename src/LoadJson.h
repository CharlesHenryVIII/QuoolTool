#pragma once

#include <string>

#define READWRITE_JSON_H(name)\
struct name;\
void Write ## name(const name* s, const std::wstring& filename);\
bool Read  ## name(      name* s, const std::wstring& filename)

READWRITE_JSON_H(Settings);
READWRITE_JSON_H(EnvironmentVariables);

#undef READWRITE_JSON_H
