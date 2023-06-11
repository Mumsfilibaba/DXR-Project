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

#define INITIALIZER_ATTRIBUTE(Type, Name) \
    Type Name;                            \
    auto& Set##Name(Type In##Name)        \
    {                                     \
        Name = In##Name;                  \
        return *this;                     \
    }

struct FViewportInitializer
{
    FViewportInitializer()
        : Window(nullptr)
        , Width(0)
        , Height(0)
    {
    }

    INITIALIZER_ATTRIBUTE(FGenericWindow*, Window);

    INITIALIZER_ATTRIBUTE(int32, Width);
    INITIALIZER_ATTRIBUTE(int32, Height);
};

class FViewport
{
public:
    FViewport()
        : RHIViewport(nullptr)
        , Window(nullptr)
    {
    }

    ~FViewport() = default;

    bool InitializeRHI(const FViewportInitializer& Initializer);
    void ReleaseRHI();

    FRHIViewport* GetRHIViewport() const
    {
        return RHIViewport.Get();
    }

    FGenericWindow* GetWindow() const
    {
        return Window.Get();
    }

private:
    TSharedRef<FRHIViewport>   RHIViewport;
    TSharedRef<FGenericWindow> Window;
};

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

class APPLICATION_API FViewportRenderer
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
