#pragma once
#include "ImGuiPlugin.h"
#include "RHI/RHIShader.h"

struct ImDrawData;
class FRHICommandList;

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
