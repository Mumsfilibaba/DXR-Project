#include "EngineModule.h"
#include "Core/Misc/CoreDelegates.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include <imgui.h>

IMPLEMENT_ENGINE_MODULE(FEngineModule, Engine);

bool FEngineModule::Load()
{
    PreEngineInitHandle = CoreDelegates::PreEngineInitDelegate.AddLambda([this]()
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