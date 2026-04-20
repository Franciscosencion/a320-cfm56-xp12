#include "DebugMenu.h"
#include "core/DataRefManager.h"
#include <imgui.h>

namespace a320 {

static const char* kPages[] = { "Systems", "Network", "DataRefs", "Failures" };

void DebugMenu::draw()
{
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 20),    ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("A320 Debug##a320dbg", &m_visible)) {
        ImGui::End();
        return;
    }

    ImGui::BeginTabBar("##tabs");
    for (int i = 0; i < 4; ++i) {
        if (ImGui::BeginTabItem(kPages[i])) {
            m_activePage = i;
            ImGui::EndTabItem();
        }
    }
    ImGui::EndTabBar();

    switch (m_activePage) {
        case 0: drawSystemsPage(); break;
        case 1: drawNetworkPage(); break;
        case 2: drawDataRefPage(); break;
        case 3: drawFailurePage(); break;
    }

    ImGui::End();
}

void DebugMenu::drawSystemsPage()
{
    ImGui::TextDisabled("Systems inspector — populated as systems are implemented");
}

void DebugMenu::drawNetworkPage()
{
    ImGui::TextDisabled("Network stats — populated once NetworkServer is active");
}

void DebugMenu::drawDataRefPage()
{
    ImGui::TextDisabled("Custom dataref browser — will list all a320/* datarefs");
}

void DebugMenu::drawFailurePage()
{
    ImGui::TextDisabled("Failure injection — populated as systems are implemented");
}

} // namespace a320
