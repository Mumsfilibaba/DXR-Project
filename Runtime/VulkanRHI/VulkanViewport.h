#pragma once
#include "VulkanTexture.h"

#include "RHI/RHIViewport.h"

#include "Core/Containers/ArrayView.h"
#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanViewport

class CVulkanViewport : public CRHIViewport
{
public:
    CVulkanViewport(EFormat InFormat, uint32 InWidth, uint32 InHeight)
        : CRHIViewport(InFormat, InWidth, InHeight)
        , BackBuffer(dbg_new TVulkanTexture<CVulkanTexture2D>(InFormat, Width, Height, 1, 1, 0, SClearValue()))
        , BackBufferView(dbg_new CVulkanRenderTargetView())
    {
    }

    ~CVulkanViewport() = default;

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final
    {
        Width = InWidth;
        Height = InHeight;
        return true;
    }

    virtual bool Present(bool bVerticalSync) override final
    {
        return true;
    }

    virtual void SetName(const String& InName) override final
    {
        CRHIObject::SetName(InName);
    }

    virtual CRHIRenderTargetView* GetRenderTargetView() const override final
    {
        return BackBufferView.Get();
    }

    virtual CRHITexture2D* GetBackBuffer() const override final
    {
        return BackBuffer.Get();
    }

    virtual bool IsValid() const override final
    {
        return true;
    }

private:
    TSharedRef<CVulkanTexture2D>        BackBuffer;
    TSharedRef<CVulkanRenderTargetView> BackBufferView;
};

