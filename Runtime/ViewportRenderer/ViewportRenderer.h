#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Time/Timer.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIResourceViews.h"

#include "Application/InputHandler.h"
#include "Application/IViewportRenderer.h"

class FViewportRenderer final
    : public IViewportRenderer
{
    FViewportRenderer() = default;
    ~FViewportRenderer() = default;

public:
    static FViewportRenderer* Make();

     /** @brief - Initialize the context */
    virtual bool InitContext(InterfaceContext Context) override final;

     /** @brief - Start the update of the UI, after the call to this function, calls to UI window's tick are valid */
    virtual void BeginTick() override final;

     /** @brief - End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid  */
    virtual void EndTick() override final;

     /** @brief - Render all the UI for this frame */
    virtual void Render(class FRHICommandList& Commandlist) override final;

private:
    TArray<FDrawableTexture*>    RenderedImages;

    FRHITextureRef             FontTexture;
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIGraphicsPipelineStateRef PipelineStateNoBlending;
    FRHIPixelShaderRef           PShader;
    
    FRHIBufferRef                VertexBuffer;
    FRHIBufferRef                IndexBuffer;

    FRHISamplerStateRef          LinearSampler;
    FRHISamplerStateRef          PointSampler;
};