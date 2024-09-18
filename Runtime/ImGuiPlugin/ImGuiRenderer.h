#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "RHI/RHIShader.h"
#include <imgui.h>

struct ImDrawData;
class FRHICommandList;

struct FImGuiViewport
{
    FImGuiViewport()
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

    FRHIBufferRef   VertexBuffer;
    FRHIBufferRef   IndexBuffer;
    int32           VertexCount;
    int32           IndexCount;
    FRHIViewportRef Viewport;
    bool            bDidResize;
    uint16          Width;
    uint16          Height;
};

class FImGuiRenderer
{
public:
    FImGuiRenderer();
    ~FImGuiRenderer();

    bool Initialize();
    void Render(FRHICommandList& CmdList);
    void RenderViewport(FRHICommandList& CmdList, ImDrawData* DrawData, FImGuiViewport& ViewportData, bool bClear);

private:
    void PrepareDrawData(FRHICommandList& CmdList, ImDrawData* DrawData);
    void RenderDrawData(FRHICommandList& CmdList, ImDrawData* DrawData);
    void SetupRenderState(FRHICommandList& CmdList, ImDrawData* DrawData, FImGuiViewport& ViewportData);
    
    TArray<FImGuiTexture*>       RenderedImages;
    FRHITextureRef               FontTexture;
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIGraphicsPipelineStateRef PipelineStateNoBlending;
    FRHIPixelShaderRef           PShader;
    FRHIBufferRef                VertexBuffer;
    FRHIBufferRef                IndexBuffer;
    FRHISamplerStateRef          LinearSampler;
    FRHISamplerStateRef          PointSampler;
};
