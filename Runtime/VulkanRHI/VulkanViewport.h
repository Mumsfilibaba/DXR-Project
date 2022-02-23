#pragma once
#include "VulkanTexture.h"
#include "VulkanCommandQueue.h"
#include "VulkanSemaphore.h"
#include "VulkanSurface.h"

#include "RHI/RHIViewport.h"

#include "Core/Containers/ArrayView.h"
#include "Core/Containers/SharedRef.h"

#include "CoreApplication/Interface/PlatformWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanViewport

class CVulkanViewport : public CRHIViewport, public CVulkanDeviceObject
{
public:

    static TSharedRef<CVulkanViewport> CreateViewport(CVulkanDevice* InDevice, CVulkanCommandQueue* InQueue, CPlatformWindow* InWindow, EFormat InFormat, uint32 InWidth, uint32 InHeight);

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final;

    virtual bool Present(bool bVerticalSync) override final;

    virtual void SetName(const String& InName) override final;

    virtual CRHIRenderTargetView* GetRenderTargetView() const override final;

    virtual CRHITexture2D* GetBackBuffer() const override final;

    virtual bool IsValid() const override final;

private:

    CVulkanViewport(CVulkanDevice* InDevice, CVulkanCommandQueue* InQueue, CPlatformWindow* InWindow, EFormat InFormat, uint32 InWidth, uint32 InHeight);
	~CVulkanViewport();

    bool Initialize();

    TSharedRef<CPlatformWindow>     Window;
	TSharedRef<CVulkanSurface>      Surface;
    TSharedRef<CVulkanCommandQueue> Queue;

    TArray<TSharedRef<CVulkanTexture2D>>        BackBuffers;
    TArray<TSharedRef<CVulkanRenderTargetView>> BackBufferViews;
    TArray<CVulkanSemaphore>                    ImageSemaphores;
    TArray<CVulkanSemaphore>                    RenderSemaphores;
};

