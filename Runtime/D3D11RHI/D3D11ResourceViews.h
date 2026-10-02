#pragma once
#include "RHI/RHIResources.h"
#include "D3D11RHI/D3D11DeviceChild.h"

typedef TSharedRef<class FD3D11ShaderResourceViewRHI>  FD3D11ShaderResourceViewRHIRef;
typedef TSharedRef<class FD3D11UnorderedAccessViewRHI> FD3D11UnorderedAccessViewRHIRef;
typedef TSharedRef<class FD3D11RenderTargetViewRHI>    FD3D11RenderTargetViewRHIRef;
typedef TSharedRef<class FD3D11DepthStencilViewRHI>    FD3D11DepthStencilViewRHIRef;

struct FD3D11SubresourceRange
{
    static constexpr uint32 AllSubresources = TNumericLimits<uint16>::Max();

    FD3D11SubresourceRange() = default;

    FD3D11SubresourceRange(FRHIResource* InResource, uint32 InFirstMip = 0, uint32 InNumMips = AllSubresources, uint32 InFirstSlice = 0, uint32 InNumSlices = AllSubresources)
        : Resource(InResource)
        , FirstMip(InFirstMip)
        , NumMips(InNumMips == UINT32_MAX ? AllSubresources : InNumMips)
        , FirstSlice(InFirstSlice)
        , NumSlices(InNumSlices)
    {
    }

    bool Overlaps(const FD3D11SubresourceRange& Other) const
    {
        if (!Resource || Resource != Other.Resource)
        {
            return false;
        }

        const bool bMipsOverlap   = FirstMip < Other.FirstMip + Other.NumMips && Other.FirstMip < FirstMip + NumMips;
        const bool bSlicesOverlap = FirstSlice < Other.FirstSlice + Other.NumSlices && Other.FirstSlice < FirstSlice + NumSlices;
        return bMipsOverlap && bSlicesOverlap;
    }

    FRHIResource* Resource   = nullptr;
    uint32        FirstMip   = 0;
    uint32        NumMips    = AllSubresources;
    uint32        FirstSlice = 0;
    uint32        NumSlices  = AllSubresources;
};

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

    FORCEINLINE const FD3D11SubresourceRange& GetSubresourceRange() const
    {
        return SubresourceRange;
    }

    FORCEINLINE D3D11_SRV_DIMENSION GetViewDimension() const
    {
        return ViewDimension;
    }

private:
    TComPtr<ID3D11ShaderResourceView> View;
    FD3D11SubresourceRange            SubresourceRange;
    D3D11_SRV_DIMENSION               ViewDimension;
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

    FORCEINLINE const FD3D11SubresourceRange& GetSubresourceRange() const
    {
        return SubresourceRange;
    }

private:
    TComPtr<ID3D11UnorderedAccessView> View;
    FD3D11SubresourceRange             SubresourceRange;
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

    FORCEINLINE const FD3D11SubresourceRange& GetSubresourceRange() const
    {
        return SubresourceRange;
    }

private:
    TComPtr<ID3D11RenderTargetView> View;
    FD3D11SubresourceRange          SubresourceRange;
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

    FORCEINLINE const FD3D11SubresourceRange& GetSubresourceRange() const
    {
        return SubresourceRange;
    }

    NODISCARD FORCEINLINE bool HasStencilFormat() const { return IsStencilFormat(D3D11Desc.Format); }

private:
    D3D11_DEPTH_STENCIL_VIEW_DESC   D3D11Desc;
    TComPtr<ID3D11DepthStencilView> View;
    FD3D11SubresourceRange          SubresourceRange;
};
