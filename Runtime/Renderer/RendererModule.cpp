#include "RendererModule.h"
#include "Core/Modules/ModuleManager.h"
#include "Application/Application.h"

#include <imgui.h>

IMPLEMENT_ENGINE_MODULE(FRendererModule, Renderer);

bool FRendererModule::Load()
{
    if (FApplication::IsInitialized())
    {
        ImGuiContext* NewImGuiContext     = reinterpret_cast<ImGuiContext*>(FApplication::Get().GetContext());
        ImGuiContext* CurrentImGuiContext = ImGui::GetCurrentContext();
        if (NewImGuiContext != CurrentImGuiContext)
        {
            ImGui::SetCurrentContext(NewImGuiContext);
        }
    }

    return true;
}