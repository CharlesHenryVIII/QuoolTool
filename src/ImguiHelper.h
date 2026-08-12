#pragma once
#include "CashUtil/CashUtil.h"
#include "Imgui.h"
#include "Settings.h"

struct CitectData;
bool ImguiInit();
void ImguiDestroy();
union SDL_Event;
void ImguiNewFrame();

void ImguiMain(AppData& data);
void ImguiTextCentered(const std::string& text, const Color* color = nullptr);
bool ImguiPath(const std::string& name, const std::string& hint, std::wstring& out_path, const bool add_final_slash);
bool ImguiPath(const std::string& name, const std::string& hint, Path& out_path);
void ImguiText(const std::wstring& ws);
void ImguiCenterWrappedText(const char* text, float start_position, float wrap_width);
void ImguiAlignForWidth(float width, float alignment = 0.5f);
void ImguiDrawDashedLine(ImDrawList* drawList, ImVec2 p1, ImVec2 p2, ImU32 col, float thickness, float dash_len, float dash_gap);
void ImguiDrawDashedRect(ImDrawList* drawList, ImVec2 p_min, ImVec2 p_max, ImU32 col, float thickness, float dash_len, float dash_gap);

union SysIP4;
//union SysIP6;
struct SysIP4AndSubnet;
//union SysIP6AndSubnet;
struct SysIP4Subnet;
//union SysIP6Subnet;
//union SysMacAddress;
struct SysNetAdapterConfig;

bool ImguiEdit(SysIP4& a, bool align_right);
//bool ImguiEdit(SysIP6* a);
bool ImguiEdit(SysIP4Subnet& a);
//bool ImguiEdit(SysIP6Subnet* a);
bool ImguiEdit(SysIP4AndSubnet& a);
//bool ImguiEdit(SysIP6AndSubnet* a);
//bool ImguiView(SysMacAddress* a);
bool ImguiEdit(std::string& s, const char* hint, const std::string& title, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
bool ImguiEdit(      SysNetAdapterConfig& c);
void ImguiView(const SysNetAdapterConfig& c);

MATH_PREFIX ImVec2 HadamardProduct(const ImVec2& a, const ImVec2& b)
{
    return { a.x * b.x, a.y * b.y };
}

MATH_PREFIX ImVec4 ToImguiColor(Color c)
{
    ImVec4 r = *((ImVec4*)&c);
    return r;
}
