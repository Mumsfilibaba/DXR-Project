#include "RendererInfoWindow.h"

#include "Core/Debug/Console/ConsoleManager.h"

#include "RHI/RHIInterface.h"

#include "Renderer/Renderer.h"

#include "Application/ApplicationInstance.h"

#include <imgui.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-variable 

TAutoConsoleVariable<bool> GDrawRendererInfo("renderer.DrawRendererInfo", false);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RendererInfoWindow

TSharedRef<CRendererInfoWindow> CRendererInfoWindow::Make()
{
    return dbg_new CRendererInfoWindow();
}

void CRendererInfoWindow::Tick()
{
    TSharedRef<CPlatformWindow> MainViewport = CApplicationInstance::Get().GetMainViewport();

    const uint32 WindowWidth = MainViewport->GetWidth();
    const uint32 WindowHeight = MainViewport->GetHeight();
    const float Width = 300.0f;
    const float Height = WindowHeight * 0.8f;

    ImGui::SetNextWindowPos(ImVec2(float(WindowWidth), 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Always);

    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Renderer Window", nullptr, Flags);

    ImGui::Text("Renderer Status:");
    ImGui::Separator();

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, 100.0f);

    const CString AdapterName = RHIGetAdapterName();
    ImGui::Text("Adapter: ");
    ImGui::NextColumn();

    ImGui::Text("%s", AdapterName.CStr());
    ImGui::NextColumn();

    SRendererStatistics Statistics = GRenderer.GetStatistics();

    ImGui::Text("DrawCalls: ");
    ImGui::NextColumn();

    ImGui::Text("%d", Statistics.NumDrawCalls);
    ImGui::NextColumn();

    ImGui::Text("DispatchCalls: ");
    ImGui::NextColumn();

    ImGui::Text("%d", Statistics.NumDispatchCalls);
    ImGui::NextColumn();

    ImGui::Text("Command Count: ");
    ImGui::NextColumn();

    ImGui::Text("%d", Statistics.NumRenderCommands);

    ImGui::Columns(1);

    ImGui::End();
}

bool CRendererInfoWindow::IsTickable()
{
    return GDrawRendererInfo.GetBool();
}
