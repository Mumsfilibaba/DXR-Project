#include "RHI/ValidationLayer/RHIValidationSwapChain.h"
#include "RHI/ValidationLayer/RHIValidationStateTracker.h"

static const FRHISwapChainDesc& GetSwapChainDesc(FRHISwapChain* SwapChain)
{
    CHECK(SwapChain != nullptr);
    return SwapChain->GetDesc();
}

FRHIValidationSwapChain::FRHIValidationSwapChain(FRHISwapChain* InSwapChain, FRHIValidationStateTracker* InStateTracker)
    : FRHISwapChain(GetSwapChainDesc(InSwapChain))
    , SwapChain(InSwapChain)
    , StateTracker(InStateTracker)
    , bAcquired(false)
    , bContentsUndefined(true)
{
    CHECK(InStateTracker != nullptr);
}

void* FRHIValidationSwapChain::GetRHINativeHandle() const
{
    return SwapChain->GetRHINativeHandle();
}

void* FRHIValidationSwapChain::GetRHINativeResourceFromIndex(uint32 Index) const
{
    return SwapChain->GetRHINativeResourceFromIndex(Index);
}

void* FRHIValidationSwapChain::GetRHINativeRenderTargetViewFromIndex(uint32 Index) const
{
    return SwapChain->GetRHINativeRenderTargetViewFromIndex(Index);
}

void* FRHIValidationSwapChain::GetRHINativeUnorderedAccessViewFromIndex(uint32 Index) const
{
    return SwapChain->GetRHINativeUnorderedAccessViewFromIndex(Index);
}

void* FRHIValidationSwapChain::GetRHINativeShaderResourceViewFromIndex(uint32 Index) const
{
    return SwapChain->GetRHINativeShaderResourceViewFromIndex(Index);
}

FRHITexture* FRHIValidationSwapChain::GetBackBuffer() const
{
    return SwapChain->GetBackBuffer();
}

FRHIRenderTargetView* FRHIValidationSwapChain::GetRenderTargetView() const
{
    return SwapChain->GetRenderTargetView();
}

FRHIUnorderedAccessView* FRHIValidationSwapChain::GetUnorderedAccessView() const
{
    return SwapChain->GetUnorderedAccessView();
}

FRHIShaderResourceView* FRHIValidationSwapChain::GetShaderResourceView() const
{
    return SwapChain->GetShaderResourceView();
}

uint32 FRHIValidationSwapChain::GetNumResources() const
{
    return SwapChain->GetNumResources();
}

bool FRHIValidationSwapChain::IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const
{
    return SwapChain->IsFormatSupported(Format, ColorSpace);
}

bool FRHIValidationSwapChain::QueryDisplayHDRInfo(FRHIDisplayHDRInfo& OutInfo) const
{
    return SwapChain->QueryDisplayHDRInfo(OutInfo);
}

void FRHIValidationSwapChain::OnAcquired()
{
    bAcquired          = true;
    bContentsUndefined = true;

    StateTracker->SetResourceState(GetBackBuffer(), ERHIResourceState::Undefined);
}

void FRHIValidationSwapChain::OnResized()
{
    bAcquired          = false;
    bContentsUndefined = true;

    SyncDesc();

    StateTracker->SetResourceState(GetBackBuffer(), ERHIResourceState::Undefined);
}
