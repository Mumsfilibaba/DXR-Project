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
    
    virtual FString GetDebugName() const override final
    {
        return DebugName;
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

class FNullRHITexture : public FRHITexture
{
public:
    FNullRHITexture(const FRHITextureDesc& InTextureDesc)
        : FRHITexture(InTextureDesc)
        , ShaderResourceView(new FNullRHIShaderResourceView(this))
        , UnorderedAccessView(new FNullRHIUnorderedAccessView(this))
    {
    }

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
    virtual FRHIShaderResourceView* GetShaderResourceView()  const override final { return nullptr; }
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return nullptr; }
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }

private:
    TSharedRef<FNullRHIShaderResourceView>  ShaderResourceView;
    TSharedRef<FNullRHIUnorderedAccessView> UnorderedAccessView;
};

struct FNullRHIRayTracingGeometry : public FRHIGeometryAccelerationStructure
{
    FNullRHIRayTracingGeometry(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
        : FRHIGeometryAccelerationStructure(InGeometryDesc)
    {
    }

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
};

class FNullRHIRayTracingScene : public FRHISceneAccelerationStructure
{
public:
    FNullRHIRayTracingScene(const FRHISceneAccelerationStructureDesc& InSceneDesc)
        : FRHISceneAccelerationStructure(InSceneDesc)
        , View(new FNullRHIShaderResourceView(this))
    {
    }

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

private:
    TSharedRef<FNullRHIShaderResourceView> View;
};

struct FNullRHISamplerState : public FRHISamplerState
{
    FNullRHISamplerState(const FRHISamplerStateDesc& InSamplerDesc)
        : FRHISamplerState(InSamplerDesc)
    {
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};

class FNullRHISwapChain : public FRHISwapChain
{
public:
    FNullRHISwapChain(const FRHISwapChainDesc& InSwapChainDesc)
        : FRHISwapChain(InSwapChainDesc)
        , BackBuffer(nullptr)
    { 
        FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(Desc.ColorFormat, Desc.Width, Desc.Height, 1, 1, ETextureUsageFlags::Presentable | ETextureUsageFlags::RenderTarget);
        BackBuffer = new FNullRHITexture(BackBufferDesc);
    }

    bool Resize(uint32 InWidth, uint32 InHeight)
    {
        Desc.Width  = uint16(InWidth);
        Desc.Height = uint16(InHeight);
        return true;
    }

    virtual FRHITexture* GetBackBuffer() const override final { return BackBuffer.Get(); }

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
    virtual bool IsSignaled() const override final { return true; }
    virtual bool Wait(uint64 TimeoutNs) const override final { (void)TimeoutNs; return true; }
    virtual void SetDebugName(const FString& InName) override final { DebugName = InName; }
    virtual FString GetDebugName() const override final { return DebugName; }

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
    virtual void* GetRHINativeHandle() const override final { return nullptr; }
};

struct FNullRHIComputePipelineState : public FRHIComputePipelineState
{
    virtual void* GetRHINativeHandle() const override final { return nullptr; }
};

struct FNullRHIRayTracingPipelineState : public FRHIRayTracingPipelineState
{
    virtual void* GetRHINativeHandle() const override final { return nullptr; }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
