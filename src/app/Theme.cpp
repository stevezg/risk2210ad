#include "Theme.h"

namespace app::theme {

void apply() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 10.0f;
    s.ChildRounding = 8.0f;
    s.FrameRounding = 6.0f;
    s.PopupRounding = 8.0f;
    s.GrabRounding = 6.0f;
    s.TabRounding = 6.0f;
    s.ScrollbarRounding = 8.0f;
    s.WindowBorderSize = 1.0f;
    s.FrameBorderSize = 1.0f;
    s.PopupBorderSize = 1.0f;
    s.WindowPadding = {14, 12};
    s.FramePadding = {10, 6};
    s.ItemSpacing = {10, 8};
    s.WindowTitleAlign = {0.0f, 0.5f};

    ImVec4* c = s.Colors;
    const ImVec4 bg{0.030f, 0.045f, 0.080f, 0.86f};
    const ImVec4 bgChild{0.045f, 0.065f, 0.110f, 0.60f};
    const ImVec4 frame{0.070f, 0.120f, 0.190f, 0.80f};
    const ImVec4 frameHover{0.110f, 0.220f, 0.330f, 0.90f};
    const ImVec4 border{0.20f, 0.60f, 0.80f, 0.45f};

    c[ImGuiCol_Text] = kText;
    c[ImGuiCol_TextDisabled] = kTextDim;
    c[ImGuiCol_WindowBg] = bg;
    c[ImGuiCol_ChildBg] = bgChild;
    c[ImGuiCol_PopupBg] = {0.030f, 0.045f, 0.080f, 0.96f};
    c[ImGuiCol_Border] = border;
    c[ImGuiCol_BorderShadow] = {0, 0, 0, 0};
    c[ImGuiCol_FrameBg] = frame;
    c[ImGuiCol_FrameBgHovered] = frameHover;
    c[ImGuiCol_FrameBgActive] = {0.15f, 0.35f, 0.50f, 1.0f};
    c[ImGuiCol_TitleBg] = {0.020f, 0.035f, 0.065f, 0.95f};
    c[ImGuiCol_TitleBgActive] = {0.040f, 0.090f, 0.150f, 0.98f};
    c[ImGuiCol_TitleBgCollapsed] = {0.020f, 0.035f, 0.065f, 0.70f};
    c[ImGuiCol_MenuBarBg] = bg;
    c[ImGuiCol_ScrollbarBg] = {0, 0, 0, 0.2f};
    c[ImGuiCol_ScrollbarGrab] = kCyanDim;
    c[ImGuiCol_ScrollbarGrabHovered] = kCyan;
    c[ImGuiCol_ScrollbarGrabActive] = kCyan;
    c[ImGuiCol_CheckMark] = kCyan;
    c[ImGuiCol_SliderGrab] = kCyanDim;
    c[ImGuiCol_SliderGrabActive] = kCyan;
    c[ImGuiCol_Button] = {0.09f, 0.22f, 0.34f, 0.85f};
    c[ImGuiCol_ButtonHovered] = {0.15f, 0.40f, 0.58f, 1.0f};
    c[ImGuiCol_ButtonActive] = {0.25f, 0.65f, 0.85f, 1.0f};
    c[ImGuiCol_Header] = {0.09f, 0.22f, 0.34f, 0.70f};
    c[ImGuiCol_HeaderHovered] = {0.15f, 0.40f, 0.58f, 0.90f};
    c[ImGuiCol_HeaderActive] = {0.25f, 0.65f, 0.85f, 1.0f};
    c[ImGuiCol_Separator] = border;
    c[ImGuiCol_SeparatorHovered] = kCyan;
    c[ImGuiCol_SeparatorActive] = kCyan;
    c[ImGuiCol_ResizeGrip] = {0.2f, 0.6f, 0.8f, 0.25f};
    c[ImGuiCol_ResizeGripHovered] = kCyanDim;
    c[ImGuiCol_ResizeGripActive] = kCyan;
    c[ImGuiCol_Tab] = frame;
    c[ImGuiCol_TabHovered] = frameHover;
    c[ImGuiCol_TabSelected] = {0.15f, 0.35f, 0.50f, 1.0f};
    c[ImGuiCol_PlotLines] = kCyan;
    c[ImGuiCol_PlotHistogram] = kAmber;
    c[ImGuiCol_TextSelectedBg] = {0.25f, 0.65f, 0.85f, 0.35f};
    c[ImGuiCol_NavCursor] = kCyan;
    c[ImGuiCol_ModalWindowDimBg] = {0, 0, 0, 0.55f};
}

void header(const char* text) {
    ImGui::TextColored(kCyan, "%s", text);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    dl->AddLine({p.x, p.y}, {p.x + w, p.y}, ImGui::ColorConvertFloat4ToU32({0.35f, 0.9f, 1.0f, 0.55f}), 1.5f);
    dl->AddLine({p.x, p.y + 2}, {p.x + w * 0.6f, p.y + 2}, ImGui::ColorConvertFloat4ToU32({0.35f, 0.9f, 1.0f, 0.18f}), 3.0f);
    ImGui::Dummy({0, 6});
}

void stat(const char* label, const char* value, ImVec4 valueColor) {
    ImGui::TextColored(kTextDim, "%s", label);
    ImGui::SameLine(120);
    ImGui::TextColored(valueColor, "%s", value);
}

}  // namespace app::theme
