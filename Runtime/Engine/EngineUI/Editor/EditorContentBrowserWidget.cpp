#include "Engine/EngineUI/Editor/EditorContentBrowserWidget.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include <imgui.h>
#include <imgui_internal.h>

FEditorContentBrowserWidget::FEditorContentBrowserWidget()
    : ImGuiDelegateHandle()
    , bVisible(true)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorContentBrowserWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorContentBrowserWidget::~FEditorContentBrowserWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FEditorContentBrowserWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    if (ImGui::Begin("Content Browser", &bVisible))
    {
        ImGui::TextDisabled("Content");

        ImGui::Separator();

        ImGui::TextUnformatted("Path: /Game");

        ImGui::Separator();

        ImGui::Selectable("Crate_01.asset", false);
        ImGui::Selectable("Door.asset", false);
        ImGui::Selectable("Wood.asset", false);
    }

    ImGui::End();
}
