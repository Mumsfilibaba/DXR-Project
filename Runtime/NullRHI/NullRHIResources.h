#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FNullRHIBuffer : public FRHIBuffer
{
public:
    FNullRHIBuffer(const FRHIBufferDesc& InBufferDesc)
        : FRHIBuffer(InBufferDesc)
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

struct FNullRHIRenderTargetView : public FRHIRenderTargetView
{
    FNullRHIRenderTargetView(FRHIResource* InResource)
        : FRHIRenderTargetView(InResource)
    {
    }
};

struct FNullRHIDepthStencilView : public FRHIDepthStencilView
{
    FNullRHIDepthStencilView(FRHIResource* InResource)
        : FRHIDepthStencilView(InResource)
    {
    }
};

class FNullRHITexture : public FRHITexture
{
public:
    FNullRHITexture(const FRHITextureDesc& InTextureDesc)
        : FRHITexture(InTextureDesc)
        , ShaderResourceView(nullptr)
        , UnorderedAccessView(nullptr)
        , RenderTargetView(nullptr)
        , DepthStencilView(nullptr)
    {
        if (Desc.IsShaderResourceTexture() && !Desc.IsNoDefaultSRV())
        {
            ShaderResourceView = new FNullRHIShaderResourceView(this);
        }

        if (Desc.IsUnorderedAccessTexture() && !Desc.IsNoDefaultUAV())
        {
            UnorderedAccessView = new FNullRHIUnorderedAccessView(this);
        }

        if (Desc.IsRenderTarget() && !Desc.IsNoDefaultRTV())
        {
            RenderTargetView = new FNullRHIRenderTargetView(this);
        }

        if (Desc.IsDepthStencil() && !Desc.IsNoDefaultDSV())
        {
            DepthStencilView = new FNullRHIDepthStencilView(this);
        }
    }

    virtual void* GetRHINativeHandle() const override final
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
    TSharedRef<FNullRHIShaderResourceView>  ShaderResourceView;
    TSharedRef<FNullRHIUnorderedAccessView> UnorderedAccessView;
    TSharedRef<FNullRHIRenderTargetView>    RenderTargetView;
    TSharedRef<FNullRHIDepthStencilView>    DepthStencilView;
};

struct FNullRHIRayTracingGeometry : public FRHIGeometryAccelerationStructure
{
    FNullRHIRayTracingGeometry(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
        : FRHIGeometryAccelerationStructure(InGeometryDesc)
    {
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }
};

class FNullRHIRayTracingScene : public FRHISceneAccelerationStructure
{
public:
    FNullRHIRayTracingScene(const FRHISceneAccelerationStructureDesc& InSceneDesc)
        : FRHISceneAccelerationStructure(InSceneDesc)
        , View(new FNullRHIShaderResourceView(this))
    {
    }

    virtual void* GetRHINativeHandle() const override final
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

private:
    TSharedRef<FNullRHIShaderResourceView> View;
};

struct FNullRHISamplerState : public FRHISamplerState
{
    FNullRHISamplerState(const FRHISamplerStateDesc& InSamplerDesc)
        : FRHISamplerState(InSamplerDesc)
    {
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }
};

class FNullRHISwapChain : public FRHISwapChain
{
public:
    FNullRHISwapChain(const FRHISwapChainDesc& InSwapChainDesc)
        : FRHISwapChain(InSwapChainDesc)
        , BackBuffer(nullptr)
    { 
        FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(
            Desc.ColorFormat, 
            Desc.Width, 
            Desc.Height, 
            1, 
            1, 
            ETextureUsageFlags::Presentable | ETextureUsageFlags::RenderTarget);
        BackBuffer = new FNullRHITexture(BackBufferDesc);
    }
    
    virtual FRHITexture* GetBackBuffer() const override final 
    {
        return BackBuffer.Get();
    }
    
    virtual FRHIRenderTargetView* GetBackBufferRenderTargetView() const override final
    {
        return BackBuffer ? BackBuffer->GetRenderTargetView() : nullptr;
    }

    bool Resize(uint32 InWidth, uint32 InHeight)
    {
        Desc.Width  = uint16(InWidth);
        Desc.Height = uint16(InHeight);
        return true;
    }

private:
    TSharedRef<FNullRHITexture> BackBuffer;
};

struct FNullRHIQuery : public FRHIQuery
{
    FNullRHIQuery(EQueryType InQueryType)
        : FRHIQuery(InQueryType)
    {
    }
};

class FNullRHIGpuFence : public FRHIFence
{
public:
    FNullRHIGpuFence()
        : FRHIFence()
        , DebugName()
    {
    }

    // FRHIFence Interface
    virtual bool IsSignaled() const override final
    {
        return true;
    }

    virtual bool Wait(uint64 TimeoutNs) const override final
    {
        UNREFERENCED_VARIABLE(TimeoutNs);
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

class FNullRHIInputLayout : public FRHIInputLayout
{
public:
    FNullRHIInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
        : FRHIInputLayout()
        , InputElements(InInputElements)
    {
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

class FNullRHIDepthStencilState : public FRHIDepthStencilState
{
public:
    FNullRHIDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
        : FRHIDepthStencilState()
        , Desc(InDesc)
    {
    }

    virtual FRHIDepthStencilStateDesc GetDesc() const override final
    {
        return Desc;
    }

private:
    FRHIDepthStencilStateDesc Desc;
};

class FNullRHIRasterizerState : public FRHIRasterizerState
{
public:
    FNullRHIRasterizerState(const FRHIRasterizerStateDesc& InDesc)
        : FRHIRasterizerState()
        , Desc(InDesc)
    {
    }

    virtual FRHIRasterizerStateDesc GetDesc() const override final
    {
        return Desc;
    }

private:
    FRHIRasterizerStateDesc Desc;
};

struct FNullRHIBlendState : public FRHIBlendState
{
public:
    FNullRHIBlendState(const FRHIBlendStateDesc& InDesc)
        : FRHIBlendState()
        , Desc(InDesc)
    {
    }

    virtual FRHIBlendStateDesc GetDesc() const override final
    {
        return Desc;
    }

private:
    FRHIBlendStateDesc Desc;
};

struct FNullRHIGraphicsPipelineState : public FRHIGraphicsPipelineState
{
    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }
};

struct FNullRHIComputePipelineState : public FRHIComputePipelineState
{
    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }
};

struct FNullRHIRayTracingPipelineState : public FRHIRayTracingPipelineState
{
    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
