#pragma once
#include "Core/Containers/ArrayView.h"
#include "Core/Mac/MacPlatformEvent.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "CoreApplication/Mac/CocoaWindow.h"
#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalDevice.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalSwapChainRHI> FMetalSwapChainRef;

@interface FMetalWindowView : FCocoaWindowView
@property (nonatomic, assign) BOOL IsOpaqueSurface;
@end

class FMetalSwapChainRHI : public FRHISwapChain, public FMetalDeviceChild
{
public:
    FMetalSwapChainRHI(FMetalDevice* InDevice, const FRHISwapChainDesc& SwapChainDesc);
    virtual ~FMetalSwapChainRHI();

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

    bool Initialize();
    bool Resize(uint32 InWidth, uint32 InHeight, EFormat Format, EColorSpace ColorSpace);
    bool Present(id<MTLCommandBuffer> CommandBuffer, bool bVerticalSync);
    bool SetHDRMetadata(const FRHIHDRMetadata& Metadata);
    void AcquireNextBackBuffer();

    id<CAMetalDrawable> GetDrawable();
    id<MTLTexture>      GetDrawableTexture();

    CAMetalLayer* GetMetalLayer() const
    {
        return MetalLayer;
    }

    FMetalWindowView* GetMetalView() const
    {
        return MetalView;
    }
    
private:
    bool RefreshBackBuffer();
    bool ApplyLayerColorSpace();
    bool ApplyHDRMetadata();

    FMetalTextureRef    BackBuffer;
    FMetalWindowView*   MetalView;
    CAMetalLayer*       MetalLayer;
    id<CAMetalDrawable> Drawable;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
