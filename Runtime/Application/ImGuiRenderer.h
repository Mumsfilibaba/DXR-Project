#pragma once
#include "Widget.h"
#include "DrawableTexture.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Time/Stopwatch.h"
#include "Core/Containers/Map.h"
#include "CoreApplication/Generic/GenericWindow.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"

struct ImDrawData;
class FRHICommandList;

struct FViewportBuffers
{
    FViewportBuffers()
        : VertexBuffer(nullptr)
        , VertexCount(0)
        , IndexBuffer(nullptr)
        , IndexCount(0)
    {
    }

    TSharedRef<FRHIBuffer> VertexBuffer;
    TSharedRef<FRHIBuffer> IndexBuffer;

	int32 VertexCount;
	int32 IndexCount;
};

class APPLICATION_API FImGuiRenderer
{
public:
    bool Initialize();

    void Render(FRHICommandList& CmdList);

    void RenderViewport(FRHICommandList& CmdList, ImDrawData* DrawData, FViewport* InViewport, bool bClear);

public:
    void RenderDrawData(FRHICommandList& CmdList, ImDrawData* DrawData);

    void SetupRenderState(FRHICommandList& CmdList, ImDrawData* DrawData, FViewportBuffers& Buffers);
    
    TArray<FDrawableTexture*> RenderedImages;

    TMap<FViewport*, FViewportBuffers> ViewportBuffers;

    FRHITextureRef               FontTexture;
    
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIGraphicsPipelineStateRef PipelineStateNoBlending;

    FRHIPixelShaderRef           PShader;
    
    FRHIBufferRef                VertexBuffer;
    FRHIBufferRef                IndexBuffer;

    FRHISamplerStateRef          LinearSampler;
    FRHISamplerStateRef          PointSampler;
};
