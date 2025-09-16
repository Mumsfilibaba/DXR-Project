#pragma once
#include "Engine/Engine.h"

class ENGINE_API FEditorEngine : public FEngine
{
public:
    FEditorEngine();
    virtual ~FEditorEngine();

    virtual bool Init() override final;
	virtual void Release() override final;

    virtual void Tick(float DeltaTime) override final;
    virtual void RenderFrame() override final;

private:
    bool CreateViewportRenderTarget();

	TSharedPtr<class FDockspaceWidget> DockspaceWidget;
    FRHITextureRef                     ViewportImage;
    FIntVector2                        ViewportImageSize;
};