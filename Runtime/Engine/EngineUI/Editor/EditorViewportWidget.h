#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "RHI/RHIResources.h"
#include "Application/Widgets/ViewportWidget.h"
#include "RendererCore/Interfaces/IRendererModule.h"

class FEditorViewportWidget
{
public:
    FEditorViewportWidget();
    ~FEditorViewportWidget();

    void Draw();
    
    void SetViewportWidget(const TSharedPtr<FViewportWidget>& ViewportWidget);
    void SetViewportImage(FRHITextureRef InViewportImage);
    
    FIntVector2 GetViewportSize() const;

    FSceneRenderView::EDebugView GetDebugView() const;
    FSceneRenderView::EDebugView GetSecondaryDebugView() const;

    bool IsVisible() const
    {
        return bVisible;
    }
    
    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

private:
    TSharedPtr<FViewportWidget>  ViewportWidget;
    FIntVector2                  CachedViewportSize;
    FImGuiTexture                ViewportImage;
    FDelegateHandle              ImGuiDelegateHandle;
    bool                         bVisible;
    bool                         bViewportInputActive;
    FSceneRenderView::EDebugView DebugView;
    FSceneRenderView::EDebugView SecondaryDebugView;
};
