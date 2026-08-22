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

void* FRHIValidationSwapChain::GetRHINativeBackBufferResourceFromIndex(uint32 Index) const
{
    return SwapChain->GetRHINativeBackBufferResourceFromIndex(Index);
}

void* FRHIValidationSwapChain::GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 Index) const
{
    return SwapChain->GetRHINativeBackBufferRenderTargetViewFromIndex(Index);
}

void* FRHIValidationSwapChain::GetRHINativeBackBufferUnorderedAccessViewFromIndex(uint32 Index) const
{
    return SwapChain->GetRHINativeBackBufferUnorderedAccessViewFromIndex(Index);
}

FRHITexture* FRHIValidationSwapChain::GetBackBuffer() const
{
    return SwapChain->GetBackBuffer();
}

FRHITexture* FRHIValidationSwapChain::GetBackBufferResourceFromIndex(uint32 Index) const
{
    return SwapChain->GetBackBufferResourceFromIndex(Index);
}

uint32 FRHIValidationSwapChain::GetNumBackBufferResources() const
{
    return SwapChain->GetNumBackBufferResources();
}

FRHIRenderTargetView* FRHIValidationSwapChain::GetBackBufferRenderTargetView() const
{
    return SwapChain->GetBackBufferRenderTargetView();
}

FRHIUnorderedAccessView* FRHIValidationSwapChain::GetBackBufferUnorderedAccessView() const
{
    return SwapChain->GetBackBufferUnorderedAccessView();
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
