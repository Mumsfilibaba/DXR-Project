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

struct FViewportData
{
    FViewportData()
        : VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
        , VertexCount(0)
        , IndexCount(0)
        , Viewport(nullptr)
        , bDidResize(false)
        , Width(0)
        , Height(0)
    {
    }

    FRHIBufferRef VertexBuffer;
    FRHIBufferRef IndexBuffer;

    int32 VertexCount;
    int32 IndexCount;

    FRHIViewportRef Viewport;
    
    bool   bDidResize;
    uint16 Width;
    uint16 Height;
};

class APPLICATION_API FImGuiRenderer
{
public:
    ~FImGuiRenderer();
    
    bool Initialize();
    void Render(FRHICommandList& CmdList);
    void RenderViewport(FRHICommandList& CmdList, ImDrawData* DrawData, FViewportData& ViewportData, bool bClear);

private:
    void PrepareDrawData(FRHICommandList& CmdList, ImDrawData* DrawData);
    void RenderDrawData(FRHICommandList& CmdList, ImDrawData* DrawData);
    void SetupRenderState(FRHICommandList& CmdList, ImDrawData* DrawData, FViewportData& ViewportData);
    
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
