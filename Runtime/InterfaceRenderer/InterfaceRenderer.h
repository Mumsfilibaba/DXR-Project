#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Time/Timer.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIResourceViews.h"

#include "Application/InputHandler.h"
#include "Application/IInterfaceRenderer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CInterfaceRenderer

class CInterfaceRenderer final : public IInterfaceRenderer
{
public:

    static CInterfaceRenderer* Make();

    /* Init the context */
    virtual bool InitContext(InterfaceContext Context) override final;

    /* Start the update of the UI, after the call to this function, calls to UI window's tick are valid */
    virtual void BeginTick() override final;

    /* End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid  */
    virtual void EndTick() override final;

    /* Render all the UI for this frame */
    virtual void Render(class CRHICommandList& Commandlist) override final;

private:

    CInterfaceRenderer() = default;
    ~CInterfaceRenderer() = default;

    TArray<SInterfaceImage*> RenderedImages;

    TSharedRef<CRHITexture2D>             FontTexture;
    TSharedRef<CRHIGraphicsPipelineState> PipelineState;
    TSharedRef<CRHIGraphicsPipelineState> PipelineStateNoBlending;
    TSharedRef<CRHIPixelShader>           PShader;
    TSharedRef<CRHIBuffer>          VertexBuffer;
    TSharedRef<CRHIBuffer>           IndexBuffer;
    TSharedRef<CRHISamplerState>          PointSampler;
};
