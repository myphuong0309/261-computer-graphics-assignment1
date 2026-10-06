// =============================================================================
// Gui.h - Dear ImGui interface: menu bar, object inspector, lighting / camera panels
// =============================================================================
#pragma once
#include "App.h"

namespace Gui {

// Call between ImGui::NewFrame() and ImGui::Render().
void draw(AppState& app);

} // namespace Gui
