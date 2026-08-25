#pragma once
#include "RHI/RHISwapChain.h"

class FRHIValidationStateTracker;

class RHI_API FRHIValidationSwapChain final : public FRHISwapChain
{
    friend class FRHIValidationCommandContext;

public:
    FRHIValidationSwapChain(FRHISwapChain* InSwapChain, FRHIValidationStateTracker* InStateTracker);
    virtual ~FRHIValidationSwapChain() = default;

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

    NODISCARD FRHISwapChain* GetRHI() const
    {
        return SwapChain.Get();
    }

    NODISCARD bool IsAcquired() const
    {
        return bAcquired;
    }

    NODISCARD bool AreContentsUndefined() const
    {
        return bContentsUndefined;
    }

private:
    void OnAcquired();
    void OnResized();

    void OnPresented()
    {
        bAcquired = false;
    }

    void SyncDesc()
    {
        Desc = SwapChain->GetDesc();
    }

    void OnBackBufferWritten()
    {
        bContentsUndefined = false;
    }

    FRHISwapChainRef            SwapChain;
    FRHIValidationStateTracker* StateTracker;
    bool                        bAcquired;
    bool                        bContentsUndefined;
};
