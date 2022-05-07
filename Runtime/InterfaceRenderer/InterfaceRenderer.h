#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Time/Timer.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIResourceViews.h"

#include "Canvas/InputHandler.h"
#include "Canvas/ICanvasRenderer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CInterfaceRenderer

class CInterfaceRenderer final : public ICanvasRenderer
{
public:

    static CInterfaceRenderer* Make();

     /** @brief: Init the context */
    virtual bool InitContext(InterfaceContext Context) override final;

     /** @brief: Start the update of the UI, after the call to this function, calls to UI window's tick are valid */
    virtual void BeginTick() override final;

     /** @brief: End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid  */
    virtual void EndTick() override final;

     /** @brief: Render all the UI for this frame */
    virtual void Render(class CRHICommandList& Commandlist) override final;

private:

    CInterfaceRenderer() = default;
    ~CInterfaceRenderer() = default;

    TArray<SCanvasImage*> RenderedImages;

    TSharedRef<CRHITexture2D>             FontTexture;
    TSharedRef<CRHIGraphicsPipelineState> PipelineState;
    TSharedRef<CRHIGraphicsPipelineState> PipelineStateNoBlending;
    TSharedRef<CRHIPixelShader>           PShader;
    TSharedRef<CRHIVertexBuffer>          VertexBuffer;
    TSharedRef<CRHIIndexBuffer>           IndexBuffer;
    TSharedRef<CRHISamplerState>          PointSampler;
};
