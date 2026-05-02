#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FNullBufferRHI : public FRHIBuffer
{
public:
    FNullBufferRHI(const FRHIBufferDesc& InBufferDesc)
        : FRHIBuffer(InBufferDesc)
    {
    }

    virtual void* GetRHINativeResource() const override final
    {
        return nullptr;
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }

    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final
    {
        return nullptr;
    }

    virtual void Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final
    {
    }

    virtual void SetDebugName(const FString& InDebugName) override final
    {
        DebugName = InDebugName;
    }

    virtual void GetDebugName(FString& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    FString DebugName;
};

struct FNullShaderResourceViewRHI : public FRHIShaderResourceView
{
    FNullShaderResourceViewRHI(FRHIResource* InResource)
        : FRHIShaderResourceView(InResource)
    {
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }
};

struct FNullUnorderedAccessViewRHI : public FRHIUnorderedAccessView
{
    FNullUnorderedAccessViewRHI(FRHIResource* InResource)
        : FRHIUnorderedAccessView(InResource)
    {
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }
};

struct FNullRenderTargetViewRHI : public FRHIRenderTargetView
{
    FNullRenderTargetViewRHI(FRHIResource* InResource)
        : FRHIRenderTargetView(InResource)
    {
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }
};

struct FNullDepthStencilViewRHI : public FRHIDepthStencilView
{
    FNullDepthStencilViewRHI(FRHIResource* InResource)
        : FRHIDepthStencilView(InResource)
    {
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }
};

class FNullTextureRHI : public FRHITexture
{
public:
    FNullTextureRHI(const FRHITextureDesc& InTextureDesc)
        : FRHITexture(InTextureDesc)
        , ShaderResourceView(nullptr)
        , UnorderedAccessView(nullptr)
        , RenderTargetView(nullptr)
        , DepthStencilView(nullptr)
    {
        if (Desc.IsShaderResourceTexture() && !Desc.IsNoDefaultSRV())
        {
            ShaderResourceView = new FNullShaderResourceViewRHI(this);
        }

        if (Desc.IsUnorderedAccessTexture() && !Desc.IsNoDefaultUAV())
        {
            UnorderedAccessView = new FNullUnorderedAccessViewRHI(this);
        }

        if (Desc.IsRenderTarget() && !Desc.IsNoDefaultRTV())
        {
            RenderTargetView = new FNullRenderTargetViewRHI(this);
        }

        if (Desc.IsDepthStencil() && !Desc.IsNoDefaultDSV())
        {
            DepthStencilView = new FNullDepthStencilViewRHI(this);
        }
    }

    virtual void* GetRHINativeResource() const override final
    {
        return nullptr;
    }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final
    {
        return ShaderResourceView.Get();
    }

    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final
    {
        return UnorderedAccessView.Get();
    }

    virtual FRHIRenderTargetView* GetRenderTargetView() const override final
    {
        return RenderTargetView.Get();
    }

    virtual FRHIDepthStencilView* GetDepthStencilView() const override final
    {
        return DepthStencilView.Get();
    }

    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final
    {
        return FRHIDescriptorHandle();
    }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final
    {
        return FRHIDescriptorHandle();
    }

private:
    TSharedRef<FNullShaderResourceViewRHI>  ShaderResourceView;
    TSharedRef<FNullUnorderedAccessViewRHI> UnorderedAccessView;
    TSharedRef<FNullRenderTargetViewRHI>    RenderTargetView;
    TSharedRef<FNullDepthStencilViewRHI>    DepthStencilView;
};

class FNullRayTracingGeometryRHI : public FRHIGeometryAccelerationStructure
{
public:
    FNullRayTracingGeometryRHI(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
        : FRHIGeometryAccelerationStructure(InGeometryDesc)
    {
    }

    virtual void* GetRHINativeResource() const override final
    {
        return nullptr;
    }

    virtual void SetDebugName(const FString& InName) override final
    {
        DebugName = InName;
    }

    virtual void GetDebugName(FString& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    FString DebugName;
};

class FNullRayTracingSceneRHI : public FRHISceneAccelerationStructure
{
public:
    FNullRayTracingSceneRHI(const FRHISceneAccelerationStructureDesc& InSceneDesc)
        : FRHISceneAccelerationStructure(InSceneDesc)
        , View(new FNullShaderResourceViewRHI(this))
    {
    }

    virtual void* GetRHINativeResource() const override final
    {
        return nullptr;
    }
    
    virtual FRHIShaderResourceView* GetShaderResourceView() const override final
    {
        return View.Get();
    }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }

    virtual void SetDebugName(const FString& InName) override final
    {
        DebugName = InName;
    }

    virtual void GetDebugName(FString& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    TSharedRef<FNullShaderResourceViewRHI> View;
    FString                                DebugName;
};

struct FNullSamplerStateRHI : public FRHISamplerState
{
    FNullSamplerStateRHI(const FRHISamplerStateDesc& InSamplerDesc)
        : FRHISamplerState(InSamplerDesc)
    {
    }

    virtual void* GetRHINativeSampler() const override final
    {
        return nullptr;
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }
};

class FNullSwapChainRHI : public FRHISwapChain
{
public:
    static constexpr uint32 kNumBackBuffers = 2;

    FNullSwapChainRHI(const FRHISwapChainDesc& InSwapChainDesc)
        : FRHISwapChain(InSwapChainDesc)
        , BackBufferIndex(0)
    {
        if (Desc.ColorSpace == EColorSpace::Unknown)
        {
            Desc.ColorSpace = EColorSpace::RGB_Full_G22_None_P709;
        }
        AllocateBackBuffers();
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeBackBufferResourceFromIndex(uint32 /*Index*/) const override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 /*Index*/) const override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeBackBufferUnorderedAccessViewFromIndex(uint32 /*Index*/) const override final
    {
        return nullptr;
    }

    virtual FRHITexture* GetBackBuffer() const override final
    {
        return BackBuffers[BackBufferIndex].Get();
    }

    virtual FRHITexture* GetBackBufferResourceFromIndex(uint32 Index) const override final
    {
        return (Index < kNumBackBuffers) ? BackBuffers[Index].Get() : nullptr;
    }

    virtual uint32 GetNumBackBufferResources() const override final
    {
        return kNumBackBuffers;
    }

    virtual FRHIRenderTargetView* GetBackBufferRenderTargetView() const override final
    {
        FNullTextureRHI* Texture = BackBuffers[BackBufferIndex].Get();
        return Texture ? Texture->GetRenderTargetView() : nullptr;
    }

    virtual FRHIUnorderedAccessView* GetBackBufferUnorderedAccessView() const override final
    {
        FNullTextureRHI* Texture = BackBuffers[BackBufferIndex].Get();
        return Texture ? Texture->GetUnorderedAccessView() : nullptr;
    }

    virtual bool IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const override final
    {
        return Format != EFormat::Unknown && ColorSpace == EColorSpace::RGB_Full_G22_None_P709;
    }

    uint32 GetCurrentBackBufferIndex() const
    {
        return BackBufferIndex;
    }

    bool Present(bool /*bVerticalSync*/)
    {
        BackBufferIndex = (BackBufferIndex + 1u) % kNumBackBuffers;
        return true;
    }

    bool Resize(uint32 InWidth, uint32 InHeight, EFormat NewFormat = EFormat::Unknown, EColorSpace /*NewColorSpace*/ = EColorSpace::Unknown)
    {
        Desc.Width  = uint16(InWidth);
        Desc.Height = uint16(InHeight);

        if (NewFormat != EFormat::Unknown)
        {
            Desc.ColorFormat = NewFormat;
        }

        AllocateBackBuffers();
        BackBufferIndex = 0;
        return true;
    }

private:
    void AllocateBackBuffers()
    {
        const FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(
            Desc.ColorFormat,
            Desc.Width,
            Desc.Height,
            1,
            1,
            ETextureUsageFlags::Presentable | ETextureUsageFlags::RenderTarget);

        for (uint32 Index = 0; Index < kNumBackBuffers; ++Index)
        {
            BackBuffers[Index] = new FNullTextureRHI(BackBufferDesc);
        }
    }

    TSharedRef<FNullTextureRHI> BackBuffers[kNumBackBuffers];
    uint32                      BackBufferIndex;
};

struct FNullQueryRHI : public FRHIQuery
{
    FNullQueryRHI(EQueryType InQueryType)
        : FRHIQuery(InQueryType)
    {
    }
};

class FNullFenceRHI : public FRHIFence
{
public:
    FNullFenceRHI()
        : FRHIFence()
        , DebugName()
    {
    }

    // FRHIFence Interface
    virtual void* GetRHINativeFence() const override final
    {
        return nullptr;
    }

    virtual bool IsSignaled() const override final
    {
        return true;
    }

    virtual bool Wait(uint64 /*TimeoutNs*/) const override final
    {
        return true;
    }

    virtual void SetDebugName(const FString& InName) override final
    {
        DebugName = InName;
    }

    virtual void GetDebugName(FString& OutDebugName) const override final
    {
        OutDebugName = DebugName;
    }

private:
    FString DebugName;
};

class FNullInputLayoutRHI : public FRHIInputLayout
{
public:
    FNullInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
        : FRHIInputLayout()
        , InputElements(InInputElements)
    {
    }

    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const override final
    {
        return &InputElements[Index];
    }

    virtual uint32 GetNumInputElementDescs() const override final
    {
        return InputElements.Size();
    }

private:
    TArray<FRHIInputElementDesc> InputElements;
};

class FNullDepthStencilStateRHI : public FRHIDepthStencilState
{
public:
    FNullDepthStencilStateRHI(const FRHIDepthStencilStateDesc& InDesc)
        : FRHIDepthStencilState()
        , Desc(InDesc)
    {
    }

    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }

    virtual FRHIDepthStencilStateDesc GetDesc() const override final
    {
        return Desc;
    }

private:
    FRHIDepthStencilStateDesc Desc;
};

class FNullRasterizerStateRHI : public FRHIRasterizerState
{
public:
    FNullRasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc)
        : FRHIRasterizerState()
        , Desc(InDesc)
    {
    }

    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }

    virtual FRHIRasterizerStateDesc GetDesc() const override final
    {
        return Desc;
    }

private:
    FRHIRasterizerStateDesc Desc;
};

struct FNullBlendStateRHI : public FRHIBlendState
{
public:
    FNullBlendStateRHI(const FRHIBlendStateDesc& InDesc)
        : FRHIBlendState()
        , Desc(InDesc)
    {
    }

    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }

    virtual FRHIBlendStateDesc GetDesc() const override final
    {
        return Desc;
    }

private:
    FRHIBlendStateDesc Desc;
};

struct FNullGraphicsPipelineStateRHI : public FRHIGraphicsPipelineState
{
    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }
};

struct FNullComputePipelineStateRHI : public FRHIComputePipelineState
{
    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }
};

struct FNullRayTracingPipelineStateRHI : public FRHIRayTracingPipelineState
{
    virtual void* GetRHINativeState() const override final
    {
        return nullptr;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
