#pragma once
#include "Core/Containers/SharedPtr.h"
#include "RHI/RHIShader.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include <imgui.h>

struct ImDrawData;
class FWindow;
class FRHICommandList;

struct FImGuiViewport
{
    FImGuiViewport()
        : Viewport(nullptr)
        , Window(nullptr)
        , VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
        , VertexCount(0)
        , IndexCount(0)
        , bDidResize(false)
        , Width(0)
        , Height(0)
    {
    }

    FRHIViewportRef     Viewport;
    TSharedPtr<FWindow> Window;
    FRHIBufferRef       VertexBuffer;
    FRHIBufferRef       IndexBuffer;
    int32               VertexCount;
    int32               IndexCount;
    bool                bDidResize;
    uint16              Width;
    uint16              Height;
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
    static void StaticCreateWindow(ImGuiViewport* InViewport);
    static void StaticDestroyWindow(ImGuiViewport* Viewport);
    static void StaticSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size);
    static void StaticRenderWindow(ImGuiViewport* Viewport, void* CmdList);
    static void StaticSwapBuffers(ImGuiViewport* Viewport, void* CmdList);

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
