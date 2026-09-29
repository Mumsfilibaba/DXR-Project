#pragma once
#include "RHI/RHIResources.h"
#include "D3D11RHI/D3D11DeviceChild.h"

typedef TSharedRef<class FD3D11ShaderResourceViewRHI>  FD3D11ShaderResourceViewRHIRef;
typedef TSharedRef<class FD3D11UnorderedAccessViewRHI> FD3D11UnorderedAccessViewRHIRef;
typedef TSharedRef<class FD3D11RenderTargetViewRHI>    FD3D11RenderTargetViewRHIRef;
typedef TSharedRef<class FD3D11DepthStencilViewRHI>    FD3D11DepthStencilViewRHIRef;

class FD3D11ShaderResourceViewRHI : public FRHIShaderResourceView, public FD3D11DeviceChild
{
public:
    FD3D11ShaderResourceViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc);
    virtual ~FD3D11ShaderResourceViewRHI();

    // FRHIShaderResourceView Interface
    virtual void* GetRHINativeHandle() const override final { return View.Get(); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    bool Initialize(ID3D11Resource* InResource, const D3D11_SHADER_RESOURCE_VIEW_DESC& InDesc);
    void ReleaseView()
    {
        View.Reset();
    }

    FORCEINLINE ID3D11ShaderResourceView* GetD3D11View() const
    {
        return View.Get();
    }

private:
    TComPtr<ID3D11ShaderResourceView> View;
};

class FD3D11UnorderedAccessViewRHI : public FRHIUnorderedAccessView, public FD3D11DeviceChild
{
public:
    FD3D11UnorderedAccessViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc);
    virtual ~FD3D11UnorderedAccessViewRHI();

    // FRHIUnorderedAccessView Interface
    virtual void* GetRHINativeHandle() const override final { return View.Get(); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    bool Initialize(ID3D11Resource* InResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC& InDesc);
    void ReleaseView()
    {
        View.Reset();
    }

    FORCEINLINE ID3D11UnorderedAccessView* GetD3D11View() const
    {
        return View.Get();
    }

private:
    TComPtr<ID3D11UnorderedAccessView> View;
};

class FD3D11RenderTargetViewRHI : public FRHIRenderTargetView, public FD3D11DeviceChild
{
public:
    FD3D11RenderTargetViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIRenderTargetViewDesc& InRHIDesc);
    virtual ~FD3D11RenderTargetViewRHI();

    // FRHIRenderTargetView Interface
    virtual void* GetRHINativeHandle() const override final { return View.Get(); }

    bool Initialize(ID3D11Resource* InResource, const D3D11_RENDER_TARGET_VIEW_DESC& InDesc);
    void ReleaseView()
    {
        View.Reset();
    }

    FORCEINLINE ID3D11RenderTargetView* GetD3D11View() const
    {
        return View.Get();
    }

private:
    TComPtr<ID3D11RenderTargetView> View;
};

class FD3D11DepthStencilViewRHI : public FRHIDepthStencilView, public FD3D11DeviceChild
{
public:
    FD3D11DepthStencilViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIDepthStencilViewDesc& InRHIDesc);
    virtual ~FD3D11DepthStencilViewRHI();

    // FRHIDepthStencilView Interface
    virtual void* GetRHINativeHandle() const override final { return View.Get(); }

    bool Initialize(ID3D11Resource* InResource, const D3D11_DEPTH_STENCIL_VIEW_DESC& InDesc);
    void ReleaseView()
    {
        View.Reset();
    }

    FORCEINLINE ID3D11DepthStencilView* GetD3D11View() const
    {
        return View.Get();
    }

    NODISCARD FORCEINLINE bool HasStencilFormat() const { return IsStencilFormat(D3D11Desc.Format); }

private:
    D3D11_DEPTH_STENCIL_VIEW_DESC   D3D11Desc;
    TComPtr<ID3D11DepthStencilView> View;
};
