#pragma once
#include "VulkanTexture.h"
#include "VulkanQueue.h"
#include "VulkanSemaphore.h"
#include "VulkanSurface.h"
#include "VulkanSwapChain.h"

#include "RHI/RHIViewport.h"

#include "Core/Containers/ArrayView.h"
#include "Core/Containers/SharedRef.h"

#include "CoreApplication/Interface/PlatformWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanViewport

class CVulkanViewport final : public CRHIViewport, public CVulkanDeviceObject
{
public:

    static TSharedRef<CVulkanViewport> CreateViewport(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CPlatformWindow* InWindow, EFormat InFormat, uint32 InWidth, uint32 InHeight);

    FORCEINLINE CVulkanSwapChain* GetSwapChain() const
    {
        return SwapChain.Get();
    }

    FORCEINLINE CVulkanQueue* GetQueue() const
    {
        return Queue.Get();
    }

    FORCEINLINE CVulkanSurface* GetSurface() const
    {
        return Surface.Get();
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIViewport Interface

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final;

    virtual bool Present(bool bVerticalSync) override final;

    virtual void SetName(const String& InName) override final;

    virtual CRHIRenderTargetView* GetRenderTargetView() const override final;

    virtual CRHITexture2D* GetBackBuffer() const override final;

    virtual bool IsValid() const override final;

private:

    CVulkanViewport(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CPlatformWindow* InWindow, EFormat InFormat, uint32 InWidth, uint32 InHeight);
	~CVulkanViewport();

    bool Initialize();

    TSharedRef<CPlatformWindow> Window;

    CVulkanSurfaceRef   Surface;
    CVulkanSwapChainRef SwapChain;
    CVulkanQueueRef     Queue;

    TSharedRef<CVulkanTexture2D>        BackBuffer;
    TSharedRef<CVulkanRenderTargetView> BackBufferView;

    TInlineArray<CVulkanSemaphore, NUM_BACK_BUFFERS> RenderSemaphores;
};

