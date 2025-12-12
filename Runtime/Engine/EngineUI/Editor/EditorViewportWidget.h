#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "RHI/RHIResources.h"
#include "Application/Widgets/ViewportWidget.h"

class FEditorViewportWidget
{
public:
    FEditorViewportWidget();
    ~FEditorViewportWidget();

    void Draw();
    
    void SetViewportWidget(const TSharedPtr<FViewportWidget>& ViewportWidget);
	void SetViewportImage(FRHITextureRef InViewportImage);
    
    FIntVector2 GetViewportSize() const;

    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

    bool IsVisible() const
    {
        return bVisible;
    }

private:
    TSharedPtr<FViewportWidget> ViewportWidget;
	FIntVector2                 CachedViewportSize;
	FImGuiTexture               ViewportImage;
    FDelegateHandle             ImGuiDelegateHandle;
    bool                        bVisible;
};