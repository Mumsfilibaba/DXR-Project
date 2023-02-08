#pragma once
#include "DrawableTexture.h"
#include "Widget.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Time/Stopwatch.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"

class APPLICATION_API FViewportRenderer
{
public:

    /**
     * @brief            - Initialize the context
     * @param NewContext - Context to set to the renderer
     * @return           - Returns true if the initialization was successful
     */
    bool Initialize();

    /** 
     * @brief - Start the update of the UI, after the call to this function, calls to UI window's tick are valid
     */
    void BeginFrame();

    /**
     * @brief - End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid 
     */
    void EndFrame();

    /**
     * @brief               - Render all the UI for this frame
     * @param InCommandList - CommandList to record all draw-commands to
     */
    void Render(class FRHICommandList& InCommandlist);

public: 
    TArray<FDrawableTexture*>    RenderedImages;

    FRHITextureRef               FontTexture;
    
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIGraphicsPipelineStateRef PipelineStateNoBlending;

    FRHIPixelShaderRef           PShader;
    
    FRHIBufferRef                VertexBuffer;
    FRHIBufferRef                IndexBuffer;

    FRHISamplerStateRef          LinearSampler;
    FRHISamplerStateRef          PointSampler;
};
