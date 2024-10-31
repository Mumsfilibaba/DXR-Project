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
    uint16              Width;
    uint16              Height;
};

class FImGuiRenderer
{
public:
    FImGuiRenderer();
    ~FImGuiRenderer();

    bool Initialize();
    void Render(FRHICommandList& CommandList);
    void RenderViewport(FRHICommandList& CommandList, ImDrawData* DrawData, FImGuiViewport& ViewportData, bool bClear);
    
    void OnCreateWindow(ImGuiViewport* InViewport);
    void OnDestroyWindow(ImGuiViewport* Viewport);
    void OnSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size);
    void OnRenderWindow(ImGuiViewport* Viewport, void* CommandList);
    void OnSwapBuffers(ImGuiViewport* Viewport, void* CommandList);
    
private:
    static FORCEINLINE FImGuiRenderer* Get()
    {
        FImGuiRenderer* ImGuiRenderer = reinterpret_cast<FImGuiRenderer*>(ImGui::GetIO().BackendRendererUserData);
        CHECK(ImGuiRenderer != nullptr);
        return ImGuiRenderer;
    }
    
    static void StaticOnCreateWindow(ImGuiViewport* InViewport);
    static void StaticOnDestroyWindow(ImGuiViewport* Viewport);
    static void StaticOnSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size);
    static void StaticOnRenderWindow(ImGuiViewport* Viewport, void* CommandList);
    static void StaticOnSwapBuffers(ImGuiViewport* Viewport, void* CommandList);

    void PrepareDrawData(FRHICommandList& CommandList, ImDrawData* DrawData);
    void RenderDrawData(FRHICommandList& CommandList, ImDrawData* DrawData);
    void SetupRenderState(FRHICommandList& CommandList, ImDrawData* DrawData, FImGuiViewport& ViewportData);
    
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
