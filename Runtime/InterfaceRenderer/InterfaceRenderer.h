#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Time/Timer.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIResourceViews.h"

#include "Application/InputHandler.h"
#include "Application/IApplicationRenderer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FInterfaceRenderer

class FInterfaceRenderer final
    : public IApplicationRenderer
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

    TArray<FDrawableTexture*> RenderedImages;

    FRHITexture2DRef             FontTexture;
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIGraphicsPipelineStateRef PipelineStateNoBlending;
    FRHIPixelShaderRef           PShader;
    
    FRHIVertexBufferRef          VertexBuffer;
    FRHIIndexBufferRef           IndexBuffer;

    FRHISamplerStateRef          LinearSampler;
    FRHISamplerStateRef          PointSampler;
};
