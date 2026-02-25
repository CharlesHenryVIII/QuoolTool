#pragma once
#include "String.h"
#include "Imgui.h"

void ImguiMain();
void TextCentered(std::string text);
bool ImguiPath(const std::string& name, const std::string& hint, std::wstring& out_path, const bool add_final_slash);
bool ImguiPath(const std::string& name, const std::string& hint, Path& out_path);
void ImguiText(const std::wstring& ws);

[[nodiscard]] inline ImVec2 HadamardProduct(const ImVec2& a, const ImVec2& b)
{
    return { a.x * b.x, a.y * b.y };
}

extern const wchar_t* g_settings_filename;
