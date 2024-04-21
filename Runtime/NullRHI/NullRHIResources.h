#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FNullRHIBuffer : public FRHIBuffer
{
    FNullRHIBuffer(const FRHIBufferDesc& InDesc)
        : FRHIBuffer(InDesc)
    {
    }

    virtual void* GetRHIBaseBuffer() override final { return this; }
    
    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final{ return FRHIDescriptorHandle(); }
};


struct FNullRHIShaderResourceView : public FRHIShaderResourceView
{
    FNullRHIShaderResourceView(FRHIResource* InResource)
        : FRHIShaderResourceView(InResource)
    {
    }
};

struct FNullRHIUnorderedAccessView : public FRHIUnorderedAccessView
{
    FNullRHIUnorderedAccessView(FRHIResource* InResource)
        : FRHIUnorderedAccessView(InResource)
    {
    }
};


class FNullRHITexture : public FRHITexture
{
public:
    FNullRHITexture(const FRHITextureDesc& InDesc)
        : FRHITexture(InDesc)
        , ShaderResourceView(new FNullRHIShaderResourceView(this))
        , UnorderedAccessView(new FNullRHIUnorderedAccessView(this))
    {
    }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(this); }
    
    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final { return nullptr; }
    
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }

private:
    TSharedRef<FNullRHIShaderResourceView>  ShaderResourceView;
    TSharedRef<FNullRHIUnorderedAccessView> UnorderedAccessView;
};


struct FNullRHIRayTracingGeometry : public FRHIRayTracingGeometry
{
    FNullRHIRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc)
        : FRHIRayTracingGeometry(InDesc)
    {
    }

    virtual void* GetRHIBaseBVHBuffer() { return nullptr; }

    virtual void* GetRHIBaseAccelerationStructure() { return reinterpret_cast<void*>(this); }
};

class FNullRHIRayTracingScene : public FRHIRayTracingScene
{
public:
    FNullRHIRayTracingScene(const FRHIRayTracingSceneDesc& InDesc)
        : FRHIRayTracingScene(InDesc)
        , View(new FNullRHIShaderResourceView(this))
    {
    }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void* GetRHIBaseBVHBuffer() override final { return nullptr; }
    
    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(this); }

private:
    TSharedRef<FNullRHIShaderResourceView> View;
};


struct FNullRHISamplerState : public FRHISamplerState
{
    FNullRHISamplerState(const FRHISamplerStateDesc& InDesc)
        : FRHISamplerState(InDesc)
    {
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};


class FNullRHIViewport : public FRHIViewport
{
public:
    FNullRHIViewport(const FRHIViewportInfo& InViewportInfo)
        : FRHIViewport(InViewportInfo)
        , BackBuffer(nullptr)
    { 
        FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(Info.ColorFormat, Info.Width, Info.Height, 1, 1, ETextureUsageFlags::Presentable | ETextureUsageFlags::RenderTarget);
        BackBuffer = new FNullRHITexture(BackBufferDesc);
    }

    bool Resize(uint32 InWidth, uint32 InHeight)
    {
        Info.Width  = uint16(InWidth);
        Info.Height = uint16(InHeight);
        return true;
    }

    virtual FRHITexture* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    TSharedRef<FNullRHITexture> BackBuffer;
};


struct FNullRHIQuery : public FRHIQuery
{
    FNullRHIQuery() = default;

    virtual void GetTimestampFromIndex(FTimingQuery& OutQuery, uint32 Index) const override final { OutQuery = FTimingQuery(); }

    virtual uint64 GetFrequency() const override final { return 1; }
};


struct FNullRHIInputLayoutState : public FRHIVertexInputLayout
{
};


class FNullRHIDepthStencilState : public FRHIDepthStencilState
{
public:
    FNullRHIDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
        : FRHIDepthStencilState()
        , Initializer(InInitializer)
    {
    }

    virtual FRHIDepthStencilStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

private:
    FRHIDepthStencilStateInitializer Initializer;
};


class FNullRHIRasterizerState : public FRHIRasterizerState
{
public:
    FNullRHIRasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
        : FRHIRasterizerState()
        , Initializer(InInitializer)
    {
    }

    virtual FRHIRasterizerStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

private:
    FRHIRasterizerStateInitializer Initializer;
};


struct FNullRHIBlendState : public FRHIBlendState
{
public:
    FNullRHIBlendState(const FRHIBlendStateInitializer& InInitializer)
        : FRHIBlendState()
        , Initializer(InInitializer)
    {
    }

    virtual FRHIBlendStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

private:
    FRHIBlendStateInitializer Initializer;
};


struct FNullRHIGraphicsPipelineState : public FRHIGraphicsPipelineState
{
};


struct FNullRHIComputePipelineState : public FRHIComputePipelineState
{
};


struct FNullRHIRayTracingPipelineState : public FRHIRayTracingPipelineState
{
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
