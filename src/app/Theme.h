#pragma once
#include "imgui.h"

namespace app::theme {

// Palette shared by ImGui and the raylib scene so both read as one design.
inline const ImVec4 kCyan{0.35f, 0.90f, 1.00f, 1.0f};
inline const ImVec4 kCyanDim{0.20f, 0.55f, 0.70f, 1.0f};
inline const ImVec4 kAmber{1.00f, 0.72f, 0.25f, 1.0f};
inline const ImVec4 kRed{1.00f, 0.30f, 0.30f, 1.0f};
inline const ImVec4 kText{0.82f, 0.92f, 1.00f, 1.0f};
inline const ImVec4 kTextDim{0.50f, 0.62f, 0.72f, 1.0f};

void apply();
/// Section header in the accent colour with a thin glowing rule under it.
void header(const char* text);
/// A one-line stat readout: dim label, bright value.
void stat(const char* label, const char* value, ImVec4 valueColor = kText);

}  // namespace app::theme
