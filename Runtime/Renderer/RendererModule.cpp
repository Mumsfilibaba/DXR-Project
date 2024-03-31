#include "RendererModule.h"
#include "Core/Misc/CoreDelegates.h"
#include "Application/Application.h"

#include <imgui.h>

IMPLEMENT_ENGINE_MODULE(FRendererModule, Renderer);

static void InitContext()
{
    if (FImGui::IsInitialized())
    {
        ImGuiContext* NewImGuiContext     = FImGui::GetContext();
        ImGuiContext* CurrentImGuiContext = ImGui::GetCurrentContext();
        if (NewImGuiContext != CurrentImGuiContext)
        {
            ImGui::SetCurrentContext(NewImGuiContext);
        }
    }
    else
    {
        CHECK(false);
    }
}

bool FRendererModule::Load()
{
    PostApplicationCreateHandle = CoreDelegates::PostApplicationCreateDelegate.AddLambda([this]()
    {
        InitContext();

        FDelegateHandle Handle = PostApplicationCreateHandle;
        PostApplicationCreateHandle = FDelegateHandle();
        CoreDelegates::PostApplicationCreateDelegate.Unbind(Handle);
    });

    return true;
}
