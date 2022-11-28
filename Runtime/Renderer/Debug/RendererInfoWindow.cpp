#include "RendererInfoWindow.h"

#include "Core/Misc/ConsoleManager.h"

#include "RHI/RHIInterface.h"

#include "Renderer/Renderer.h"

#include "Application/ApplicationInterface.h"

#include <imgui.h>

TAutoConsoleVariable<bool> GDrawRendererInfo("Renderer.DrawRendererInfo", true);

TSharedRef<FRendererInfoWindow> FRendererInfoWindow::Create()
{
    return new FRendererInfoWindow();
}

void FRendererInfoWindow::Tick()
{
    FGenericWindowRef MainViewport = FApplicationInterface::Get().GetMainViewport();

    const uint32 WindowWidth  = MainViewport->GetWidth();
    const uint32 WindowHeight = MainViewport->GetHeight();
    const float Width  = 300.0f;
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

    const FString AdapterName = RHIGetAdapterName();
    ImGui::Text("Adapter: ");
    ImGui::NextColumn();

    ImGui::Text("%s", AdapterName.GetCString());
    ImGui::NextColumn();

    FRHICommandStatistics Statistics = FRenderer::Get().GetStatistics();

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

    ImGui::Text("%d", Statistics.NumCommands);

    ImGui::Columns(1);

    ImGui::End();
}

bool FRendererInfoWindow::IsTickable()
{
    return GDrawRendererInfo.GetValue();
}
