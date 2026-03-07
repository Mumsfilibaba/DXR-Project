#include "Core/Misc/CoreDelegates.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "Engine/EngineModule.h"

#include <imgui.h>

IMPLEMENT_ENGINE_MODULE(FEngineModule, Engine);

FEngineModule::~FEngineModule()
{
    CoreDelegates::PreEngineInitDelegate.Unbind(PreEngineInitHandle);
}

bool FEngineModule::Load()
{
    PreEngineInitHandle = CoreDelegates::PreEngineInitDelegate.AddLambda([]()
    {
        if (IImguiPlugin::IsEnabled())
        {
            ImGuiContext* Context = IImguiPlugin::Get().GetImGuiContext();
            ImGui::SetCurrentContext(Context);
        }
        else
        {
            CHECK(false);
        }
    });

    return true;
}
