#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Time/Timer.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIResourceViews.h"

#include "Canvas/InputHandler.h"
#include "Canvas/IApplicationRenderer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FInterfaceRenderer

class FInterfaceRenderer final : public IApplicationRenderer
{
public:

    static FInterfaceRenderer* Make();

     /** @brief: Initialize the context */
    virtual bool InitContext(InterfaceContext Context) override final;

     /** @brief: Start the update of the UI, after the call to this function, calls to UI window's tick are valid */
    virtual void BeginTick() override final;

     /** @brief: End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid  */
    virtual void EndTick() override final;

     /** @brief: Render all the UI for this frame */
    virtual void Render(class FRHICommandList& Commandlist) override final;

private:

    FInterfaceRenderer() = default;
    ~FInterfaceRenderer() = default;

    TArray<FDrawableImage*> RenderedImages;

    FRHITexture2DRef             FontTexture;
    TSharedRef<FRHIGraphicsPipelineState> PipelineState;
    TSharedRef<FRHIGraphicsPipelineState> PipelineStateNoBlending;
    TSharedRef<FRHIPixelShader>           PShader;
    TSharedRef<FRHIVertexBuffer>          VertexBuffer;
    TSharedRef<FRHIIndexBuffer>           IndexBuffer;
    FRHISamplerStateRef          PointSampler;
};
