#pragma once

namespace a320 {

// ImGui-based debug overlay rendered inside X-Plane.
// Toggle with the keyboard shortcut defined in Plugin.cpp (Ctrl+Shift+D).
class DebugMenu {
public:
    void draw();           // called from XPLMDrawCallback or flight loop
    bool isVisible() const { return m_visible; }
    void toggle()          { m_visible = !m_visible; }

private:
    void drawSystemsPage();
    void drawNetworkPage();
    void drawDataRefPage();
    void drawFailurePage();

    bool    m_visible      = false;
    int     m_activePage   = 0;
};

} // namespace a320
