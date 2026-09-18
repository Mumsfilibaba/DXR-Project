#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/ArrayView.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12CommandContext.h"

class FD3D12CommandContext;

#if D3D12_ENABLE_COMPOSITION
class FD3D12Composition;
#endif

typedef TSharedRef<class FD3D12SwapChainRHI> FD3D12SwapChainRHIRef;

class FD3D12SwapChainRHI : public FRHISwapChain, public FD3D12DeviceChild
{
public:
    static EFormat GetDefaultBackBufferFormat();

public:
    FD3D12SwapChainRHI(FD3D12Device* InDevice, FD3D12CommandContext* InCommandContext, const FRHISwapChainDesc& InSwapChainDesc);
    virtual ~FD3D12SwapChainRHI();

    // FRHISwapChain Interface
    virtual void* GetRHINativeHandle()                                   const override final;
    virtual void* GetRHINativeResourceFromIndex(uint32 Index)            const override final;
    virtual void* GetRHINativeRenderTargetViewFromIndex(uint32 Index)    const override final;
    virtual void* GetRHINativeUnorderedAccessViewFromIndex(uint32 Index) const override final;
    virtual void* GetRHINativeShaderResourceViewFromIndex(uint32 Index)  const override final;

    virtual FRHITexture*             GetBackBuffer()          const override final;
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final;
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final;
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final;
    virtual uint32                   GetNumResources()        const override final;

    virtual bool IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const override final;
    virtual bool QueryDisplayHDRInfo(FRHIDisplayHDRInfo& OutInfo) const override final;

    bool Initialize(FD3D12CommandContext* InCommandContext);
    
    bool Resize(FD3D12CommandContext* InCommandContext, uint32 Width, uint32 Height, EFormat NewFormat, EColorSpace NewColorSpace);
    bool Present(bool bVerticalSync);
    bool SetHDRMetadata(const FRHIHDRMetadata& Metadata);
    void AcquireNextBackBuffer();

    FD3D12Resource* GetResourceAtIndex(uint32 Index) const
    {
        const int32 ResourceIndex = static_cast<int32>(Index);
        return BackBuffers.IsValidIndex(ResourceIndex) ? BackBuffers[ResourceIndex].Resource.Get() : nullptr;
    }

    uint32 GetBackBufferCount() const
    {
        return static_cast<uint32>(BackBuffers.Size());
    }

private:
    struct FBackBufferData
    {
        FD3D12ResourceRef       Resource;
        FD3D12OfflineDescriptor RenderTargetDescriptor;
        FD3D12OfflineDescriptor UnorderedAccessDescriptor;
        FD3D12OfflineDescriptor ShaderResourceDescriptor;
    };

    bool RetrieveBackBuffers();
    bool CreateBackBuffer();
    bool CreateBackBufferDescriptors();
    void ReleaseBackBufferResources();
    void SwapResources(uint32 Index);
    bool ApplyHDRMetadata();
    void ApplySettingsChanges();

    TComPtr<IDXGISwapChain3>      SwapChain;
    TComPtr<IDXGISwapChain4>      SwapChain4;
#if D3D12_ENABLE_COMPOSITION
    TSharedRef<FD3D12Composition> Composition;
#endif
    FD3D12CommandContext*         CommandContext;
    FD3D12TextureRHIRef           BackBuffer;
    TArray<FBackBufferData>       BackBuffers;
    HWND                          Hwnd;
    HANDLE                        SwapChainWaitableObject;
    DXGI_HDR_METADATA_TYPE        AppliedHDRMetadataType;
    DXGI_HDR_METADATA_HDR10       AppliedHDR10Metadata;
    EColorSpace                   CurrentColorSpace;
    uint32                        Flags;
    uint32                        NumBackBuffers;
    uint32                        ActiveFrameLatency;
    uint32                        BackBufferIndex;
};
