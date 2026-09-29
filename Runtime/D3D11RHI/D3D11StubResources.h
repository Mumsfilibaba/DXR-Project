#pragma once
#include "RHI/RHI.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"
#include "D3D11RHI/D3D11Core.h"

// ------------------------------------------------------------------------------------------------
// Placeholder objects that let the engine boot on D3D11RHI before the real resource types exist.
// Each one is removed once its D3D11 implementation lands.
// ------------------------------------------------------------------------------------------------

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FD3D11StubBufferRHI : public FRHIBuffer
{
public:
    FD3D11StubBufferRHI(const FRHIBufferDesc& InBufferDesc)
        : FRHIBuffer(InBufferDesc)
    {
    }

    virtual ~FD3D11StubBufferRHI();

    virtual void* GetRHINativeResource() const override final { return nullptr; }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final { return nullptr; }
    virtual void  Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final { }

    virtual void SetDebugName(const String& InDebugName) override final { DebugName = InDebugName; }
    virtual void GetDebugName(String& OutDebugName) const override final { OutDebugName = DebugName; }

private:
    String DebugName;
};

struct FD3D11StubShaderResourceViewRHI : public FRHIShaderResourceView
{
    FD3D11StubShaderResourceViewRHI(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc)
        : FRHIShaderResourceView(InResource, InRHIDesc)
    {
    }

    virtual ~FD3D11StubShaderResourceViewRHI();

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
};

struct FD3D11StubUnorderedAccessViewRHI : public FRHIUnorderedAccessView
{
    FD3D11StubUnorderedAccessViewRHI(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc)
        : FRHIUnorderedAccessView(InResource, InRHIDesc)
    {
    }

    virtual ~FD3D11StubUnorderedAccessViewRHI();

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
};

struct FD3D11StubRenderTargetViewRHI : public FRHIRenderTargetView
{
    FD3D11StubRenderTargetViewRHI(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InRHIDesc)
        : FRHIRenderTargetView(InResource, InRHIDesc)
    {
    }

    virtual ~FD3D11StubRenderTargetViewRHI();

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
};

struct FD3D11StubDepthStencilViewRHI : public FRHIDepthStencilView
{
    FD3D11StubDepthStencilViewRHI(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InRHIDesc)
        : FRHIDepthStencilView(InResource, InRHIDesc)
    {
    }

    virtual ~FD3D11StubDepthStencilViewRHI();

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
};

class FD3D11StubTextureRHI : public FRHITexture
{
public:
    FD3D11StubTextureRHI(const FRHITextureDesc& InTextureDesc)
        : FRHITexture(InTextureDesc)
    {
        if (Desc.IsShaderResourceTexture() && !Desc.IsNoDefaultSRV())
        {
            ShaderResourceView = new FD3D11StubShaderResourceViewRHI(this, GetDefaultShaderResourceViewDescForTexture(Desc));
        }

        if (Desc.IsUnorderedAccessTexture() && !Desc.IsNoDefaultUAV())
        {
            UnorderedAccessView = new FD3D11StubUnorderedAccessViewRHI(this, GetDefaultUnorderedAccessViewDescForTexture(Desc));
        }

        if (Desc.IsRenderTarget() && !Desc.IsNoDefaultRTV())
        {
            RenderTargetView = new FD3D11StubRenderTargetViewRHI(this, GetDefaultRenderTargetViewDescForTexture(Desc));
        }

        if (Desc.IsDepthStencil() && !Desc.IsNoDefaultDSV())
        {
            DepthStencilView = new FD3D11StubDepthStencilViewRHI(this, GetDefaultDepthStencilViewDescForTexture(Desc));
        }
    }

    virtual ~FD3D11StubTextureRHI();

    virtual void* GetRHINativeResource() const override final { return nullptr; }

    virtual FRHIDescriptorHandle     GetBindlessSRVHandle()   const override final { return FRHIDescriptorHandle(); }
    virtual FRHIDescriptorHandle     GetBindlessUAVHandle()   const override final { return FRHIDescriptorHandle(); }
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final { return ShaderResourceView.Get(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final { return RenderTargetView.Get(); }
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const override final { return DepthStencilView.Get(); }

    virtual void SetDebugName(const String& InDebugName) override final { DebugName = InDebugName; }
    virtual void GetDebugName(String& OutDebugName) const override final { OutDebugName = DebugName; }

private:
    TSharedRef<FD3D11StubShaderResourceViewRHI>  ShaderResourceView;
    TSharedRef<FD3D11StubUnorderedAccessViewRHI> UnorderedAccessView;
    TSharedRef<FD3D11StubRenderTargetViewRHI>    RenderTargetView;
    TSharedRef<FD3D11StubDepthStencilViewRHI>    DepthStencilView;
    String                                       DebugName;
};

struct FD3D11StubSamplerStateRHI : public FRHISamplerState
{
    FD3D11StubSamplerStateRHI(const FRHISamplerStateDesc& InSamplerDesc)
        : FRHISamplerState(InSamplerDesc)
    {
    }

    virtual ~FD3D11StubSamplerStateRHI();

    virtual void* GetRHINativeSampler() const override final { return nullptr; }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
};

struct FD3D11StubQueryRHI : public FRHIQuery
{
    FD3D11StubQueryRHI(EQueryType InQueryType)
        : FRHIQuery(InQueryType)
    {
    }

    virtual ~FD3D11StubQueryRHI();
};

class FD3D11StubFenceRHI : public FRHIFence
{
public:
    virtual void* GetRHINativeFence() const override final { return nullptr; }
    virtual bool  IsSignaled() const override final { return true; }
    virtual bool  Wait(uint64 TimeoutNs) const override final { return true; }

    virtual void SetDebugName(const String& InName) override final { DebugName = InName; }
    virtual void GetDebugName(String& OutDebugName) const override final { OutDebugName = DebugName; }

private:
    String DebugName;
};

class FD3D11StubInputLayoutRHI : public FRHIInputLayout
{
public:
    FD3D11StubInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
        : FRHIInputLayout()
        , InputElements(InInputElements)
    {
    }

    virtual ~FD3D11StubInputLayoutRHI();

    virtual void* GetRHINativeState() const override final { return nullptr; }

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const override final
    {
        return (Index < static_cast<uint32>(InputElements.Size())) ? &InputElements[Index] : nullptr;
    }

    virtual uint32 GetNumInputElementDescs() const override final { return InputElements.Size(); }

private:
    TArray<FRHIInputElementDesc> InputElements;
};

class FD3D11StubDepthStencilStateRHI : public FRHIDepthStencilState
{
public:
    FD3D11StubDepthStencilStateRHI(const FRHIDepthStencilStateDesc& InDesc)
        : FRHIDepthStencilState(InDesc)
    {
    }

    virtual ~FD3D11StubDepthStencilStateRHI();

    virtual void* GetRHINativeState() const override final { return nullptr; }
};

class FD3D11StubRasterizerStateRHI : public FRHIRasterizerState
{
public:
    FD3D11StubRasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc)
        : FRHIRasterizerState(InDesc)
    {
    }

    virtual ~FD3D11StubRasterizerStateRHI();

    virtual void* GetRHINativeState() const override final { return nullptr; }
};

class FD3D11StubBlendStateRHI : public FRHIBlendState
{
public:
    FD3D11StubBlendStateRHI(const FRHIBlendStateDesc& InDesc)
        : FRHIBlendState(InDesc)
    {
    }

    virtual ~FD3D11StubBlendStateRHI();

    virtual void* GetRHINativeState() const override final { return nullptr; }
};

template<typename BasePipelineType>
class TD3D11StubPipelineStateRHI final : public BasePipelineType
{
public:
    virtual void* GetRHINativeState() const override final { return nullptr; }

    virtual void SetDebugName(const String& InDebugName) override final { DebugName = InDebugName; }
    virtual void GetDebugName(String& OutDebugName) const override final { OutDebugName = DebugName; }

private:
    String DebugName;
};

template<typename BaseShaderType>
class TD3D11StubShaderRHI final : public BaseShaderType
{
public:
    virtual void* GetRHINativeHandle()  override final { return nullptr; }
    virtual void* GetRHIBaseInterface() override final { return this; }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
