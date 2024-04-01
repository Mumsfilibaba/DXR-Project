#include "RendererInfoWindow.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Application/Application.h"
#include "Renderer/SceneRenderer.h"

#include <imgui.h>

static TAutoConsoleVariable<bool> CVarDrawRendererInfo(
    "Renderer.DrawRendererInfo",
    "Enables the drawing of the Renderer Info Window", 
    true);

void FRendererInfoWindow::Paint()
{
    if (CVarDrawRendererInfo.GetValue())
    {
        const FString AdapterName = RHIGetAdapterName();

        const ImVec2 MainViewportPos  = FImGui::GetMainViewportPos();
        const ImVec2 DisplaySize      = FImGui::GetDisplaySize();
        const ImVec2 TextSize         = ImGui::CalcTextSize(AdapterName.GetCString());
        const ImVec2 FrameBufferScale = FImGui::GetDisplayFramebufferScale();

        const float WindowWidth  = DisplaySize.x;
        const float WindowHeight = DisplaySize.y;
        const float Scale        = FrameBufferScale.x;
        const float ColumnWidth  = 105.0f * Scale;
	    const float Width        = FMath::Max(TextSize.x + ColumnWidth + 15.0f * Scale, 300.0f * Scale);
	    const float Height       = WindowHeight * 0.8f;

        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
        ImGui::SetNextWindowPos(ImVec2(MainViewportPos.x + WindowWidth, MainViewportPos.y + 10.0f * Scale), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Always);

        const ImGuiWindowFlags Flags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoDocking;

        ImGui::Begin("Renderer Window", nullptr, Flags);

        ImGui::Text("Renderer Status:");
        ImGui::Separator();

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, ColumnWidth);

        ImGui::Text("Adapter: ");
        ImGui::NextColumn();

        ImGui::Text("%s", AdapterName.GetCString());
        ImGui::NextColumn();

        const FRHICommandStatistics& Statistics = Renderer->GetStatistics();
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
}
