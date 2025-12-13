#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include "RHI/RHIResources.h"
#include "Application/Application.h"
#include <imgui.h>
#include <imgui_internal.h>

FEditorViewportWidget::FEditorViewportWidget()
    : CachedViewportSize(0, 0)
    , ViewportImage()
    , ImGuiDelegateHandle()
    , bVisible(true)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorViewportWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorViewportWidget::~FEditorViewportWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FEditorViewportWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    const ImGuiWindowFlags ViewportFlags = 
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("Viewport", &bVisible, ViewportFlags))
    {
        // Cache the new size of the viewport
        const ImVec2 Size = ImGui::GetContentRegionAvail();
        CachedViewportSize = FIntVector2(int32(Size.x), int32(Size.y));

        // Draw the viewport texture
        ImGui::Image(&ViewportImage, Size);
    }

    ImGui::End(); // Viewport Window

    ImGui::PopStyleVar();
}

void FEditorViewportWidget::SetViewportWidget(const TSharedPtr<FViewportWidget>& InViewportWidget)
{
    ViewportWidget = InViewportWidget;
}

void FEditorViewportWidget::SetViewportImage(FRHITextureRef InViewportImage)
{
	if (InViewportImage)
	{
		ViewportImage.Texture        = InViewportImage;
		ViewportImage.View           = MakeSharedRef<FRHIShaderResourceView>(InViewportImage->GetShaderResourceView());
		ViewportImage.ResourceState  = EResourceAccess::RenderTarget;
		ViewportImage.bSamplerLinear = false;
		ViewportImage.bAllowBlending = false;
	}
}

FIntVector2 FEditorViewportWidget::GetViewportSize() const
{
    if (CachedViewportSize.X > 0 && CachedViewportSize.Y > 0)
    {
        return CachedViewportSize;
    }

    if (ImGuiWindow* ViewportWindow = ImGui::FindWindowByName("Viewport"))
    {
        const ImVec2 Size = ViewportWindow->ContentRegionRect.GetSize();
        return FIntVector2(int32(Size.x), int32(Size.y));
    }

    return FIntVector2(1920, 1080);
}
