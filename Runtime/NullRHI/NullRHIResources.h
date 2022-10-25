#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

struct FNullRHIBuffer
    : public FRHIBuffer
{
    explicit FNullRHIBuffer(const FRHIBufferDesc& InDesc)
        : FRHIBuffer(InDesc)
    { }

    virtual void* GetRHIBaseBuffer()         override final { return this; }
    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final{ return FRHIDescriptorHandle(); }
};


struct FNullRHIShaderResourceView
    : public FRHIShaderResourceView
{
    explicit FNullRHIShaderResourceView(FRHIResource* InResource)
        : FRHIShaderResourceView(InResource)
    { }
};

struct FNullRHIUnorderedAccessView
    : public FRHIUnorderedAccessView
{
    explicit FNullRHIUnorderedAccessView(FRHIResource* InResource)
        : FRHIUnorderedAccessView(InResource)
    { }
};


class FNullRHITexture
    : public FRHITexture
{
public:
    explicit FNullRHITexture(const FRHITextureDesc& InDesc)
        : FRHITexture(InDesc)
        , ShaderResourceView(dbg_new FNullRHIShaderResourceView(this))
        , UnorderedAccessView(dbg_new FNullRHIUnorderedAccessView(this))
    { }

    virtual void* GetRHIBaseTexture()        override final { return reinterpret_cast<void*>(this); }
    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final { return nullptr; }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }

private:
    TSharedRef<FNullRHIShaderResourceView>  ShaderResourceView;
    TSharedRef<FNullRHIUnorderedAccessView> UnorderedAccessView;
};


struct FNullRHIRayTracingGeometry 
    : public FRHIRayTracingGeometry
{
    explicit FNullRHIRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc)
        : FRHIRayTracingGeometry(InDesc)
    { }

    virtual void* GetRHIBaseBVHBuffer()             { return nullptr; }
    virtual void* GetRHIBaseAccelerationStructure() { return reinterpret_cast<void*>(this); }
};

class FNullRHIRayTracingScene 
    : public FRHIRayTracingScene
{
public:
    explicit FNullRHIRayTracingScene(const FRHIRayTracingSceneDesc& InDesc)
        : FRHIRayTracingScene(InDesc)
        , View(dbg_new FNullRHIShaderResourceView(this))
    { }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const override final { return FRHIDescriptorHandle(); }

    virtual void* GetRHIBaseBVHBuffer()             override final { return nullptr; }
    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(this); }

private:
    TSharedRef<FNullRHIShaderResourceView> View;
};


struct FNullRHISamplerState 
    : public FRHISamplerState
{
    explicit FNullRHISamplerState(const FRHISamplerStateDesc& InDesc)
        : FRHISamplerState(InDesc)
    { }

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};


class FNullRHIViewport 
    : public FRHIViewport
{
public:
    FNullRHIViewport(const FRHIViewportDesc& InDesc)
        : FRHIViewport(InDesc)
        , BackBuffer(nullptr)
    { 
        FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(
            Desc.ColorFormat,
            Desc.Width,
            Desc.Height,
            1,
            1,
            ETextureUsageFlags::Presentable | ETextureUsageFlags::RenderTarget);

        BackBuffer = dbg_new FNullRHITexture(BackBufferDesc);
    }

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final
    {
        Desc.Width  = uint16(InWidth);
        Desc.Height = uint16(InHeight);
        return true;
    }

    virtual FRHITexture* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    TSharedRef<FNullRHITexture> BackBuffer;
};


struct FNullRHITimestampQuery 
    : public FRHITimestampQuery
{
    FNullRHITimestampQuery() = default;

    virtual void GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const override final { OutQuery = FRHITimestamp(); }

    virtual uint64 GetFrequency() const override final { return 1; }
};


struct FNullRHIInputLayoutState
    : public FRHIVertexInputLayout
{
};


struct FNullRHIDepthStencilState
    : public FRHIDepthStencilState
{
};


struct FNullRHIRasterizerState
    : public FRHIRasterizerState
{
};


struct FNullRHIBlendState
    : public FRHIBlendState
{
};


struct FNullRHIGraphicsPipelineState
    : public FRHIGraphicsPipelineState
{
};


struct FNullRHIComputePipelineState
    : public FRHIComputePipelineState
{
};


struct FNullRHIRayTracingPipelineState
    : public FRHIRayTracingPipelineState
{
};


#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
