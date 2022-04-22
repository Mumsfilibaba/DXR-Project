#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Time/Timer.h"

#include "RHI/RHIResources.h"
#include "RHi/RHIPipeline.h"

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

    CInterfaceRenderer()  = default;
    ~CInterfaceRenderer() = default;

    TArray<SInterfaceImage*> RenderedImages;

    CRHITexture2DRef             FontTexture;
    CRHIGraphicsPipelineStateRef PipelineState;
    CRHIGraphicsPipelineStateRef PipelineStateNoBlending;
    CRHIPixelShaderRef           PShader;
    CRHIVertexBufferRef          VertexBuffer;
    CRHIIndexBufferRef           IndexBuffer;
    CRHISamplerStateRef          PointSampler;
};
