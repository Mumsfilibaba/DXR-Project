#include "MetalRHI/MetalViews.h"

FMetalView::FMetalView(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
{
}

FMetalView::~FMetalView() = default;

FMetalShaderResourceViewRHI::FMetalShaderResourceViewRHI(FMetalDevice* InDevice, FRHIResource* InResource)
    : FRHIShaderResourceView(InResource)
    , FMetalView(InDevice)
{
}

FMetalShaderResourceViewRHI::~FMetalShaderResourceViewRHI() = default;

FMetalUnorderedAccessViewRHI::FMetalUnorderedAccessViewRHI(FMetalDevice* InDevice, FRHIResource* InResource)
    : FRHIUnorderedAccessView(InResource)
    , FMetalView(InDevice)
{
}

FMetalUnorderedAccessViewRHI::~FMetalUnorderedAccessViewRHI() = default;

FMetalRenderTargetViewRHI::FMetalRenderTargetViewRHI(FMetalDevice* InDevice, const FRHIRenderTargetViewDesc& InDesc)
    : FRHIRenderTargetView(InDesc.Texture)
    , FMetalView(InDevice)
    , MipLevel(InDesc.MipLevel)
    , ArrayIndex(InDesc.ArrayIndex)
{
}

FMetalRenderTargetViewRHI::~FMetalRenderTargetViewRHI() = default;

FMetalDepthStencilViewRHI::FMetalDepthStencilViewRHI(FMetalDevice* InDevice, const FRHIDepthStencilViewDesc& InDesc)
    : FRHIDepthStencilView(InDesc.Texture)
    , FMetalView(InDevice)
    , MipLevel(InDesc.MipLevel)
    , ArrayIndex(InDesc.ArrayIndex)
{
}

FMetalDepthStencilViewRHI::~FMetalDepthStencilViewRHI() = default;

void* FMetalShaderResourceViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}

FRHIDescriptorHandle FMetalShaderResourceViewRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

void* FMetalUnorderedAccessViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}

FRHIDescriptorHandle FMetalUnorderedAccessViewRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

void* FMetalRenderTargetViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}

void* FMetalDepthStencilViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}
