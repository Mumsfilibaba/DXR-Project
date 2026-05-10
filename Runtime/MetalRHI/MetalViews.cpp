#include "MetalRHI/MetalViews.h"

static void ResolveRTVMipAndSlice(const FRHIRenderTargetViewDesc& InDesc, uint8& OutMipLevel, uint16& OutArrayIndex)
{
    OutMipLevel   = 0;
    OutArrayIndex = 0;

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            OutMipLevel = InDesc.Texture1D.MipLevel;
            break;

        case EViewDimension::Texture1DArray:
            OutMipLevel   = InDesc.Texture1DArray.MipLevel;
            OutArrayIndex = InDesc.Texture1DArray.FirstArraySlice;
            break;

        case EViewDimension::Texture2D:
            OutMipLevel = InDesc.Texture2D.MipLevel;
            break;

        case EViewDimension::Texture2DArray:
            OutMipLevel   = InDesc.Texture2DArray.MipLevel;
            OutArrayIndex = InDesc.Texture2DArray.FirstArraySlice;
            break;

        case EViewDimension::TextureCube:
            OutMipLevel = InDesc.TextureCube.MipLevel;
            break;

        case EViewDimension::TextureCubeArray:
            OutMipLevel   = InDesc.TextureCubeArray.MipLevel;
            OutArrayIndex = InDesc.TextureCubeArray.FirstCube;
            break;

        case EViewDimension::Texture3D:
            OutMipLevel   = InDesc.Texture3D.MipLevel;
            OutArrayIndex = InDesc.Texture3D.FirstWSlice;
            break;

        default:
            break;
    }
}

static void ResolveDSVMipAndSlice(const FRHIDepthStencilViewDesc& InDesc, uint8& OutMipLevel, uint16& OutArrayIndex)
{
    OutMipLevel   = 0;
    OutArrayIndex = 0;

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            OutMipLevel = InDesc.Texture1D.MipLevel;
            break;
        
        case EViewDimension::Texture1DArray:
            OutMipLevel   = InDesc.Texture1DArray.MipLevel;
            OutArrayIndex = InDesc.Texture1DArray.FirstArraySlice;
            break;

        case EViewDimension::Texture2D:
            OutMipLevel = InDesc.Texture2D.MipLevel;
            break;

        case EViewDimension::Texture2DArray:
            OutMipLevel   = InDesc.Texture2DArray.MipLevel;
            OutArrayIndex = InDesc.Texture2DArray.FirstArraySlice;
            break;
        
        case EViewDimension::TextureCube:
            OutMipLevel = InDesc.TextureCube.MipLevel;
            break;
        
        case EViewDimension::TextureCubeArray: 
            OutMipLevel   = InDesc.TextureCubeArray.MipLevel; 
            OutArrayIndex = InDesc.TextureCubeArray.FirstCube;
            break;
        
        default: 
            break;
    }
}

FMetalView::FMetalView(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
{
}

FMetalView::~FMetalView() = default;

FMetalShaderResourceViewRHI::FMetalShaderResourceViewRHI(FMetalDevice* InDevice, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc)
    : FRHIShaderResourceView(InResource, InRHIDesc)
    , FMetalView(InDevice)
{
}

FMetalShaderResourceViewRHI::~FMetalShaderResourceViewRHI() = default;

FMetalUnorderedAccessViewRHI::FMetalUnorderedAccessViewRHI(FMetalDevice* InDevice, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc)
    : FRHIUnorderedAccessView(InResource, InRHIDesc)
    , FMetalView(InDevice)
{
}

FMetalUnorderedAccessViewRHI::~FMetalUnorderedAccessViewRHI() = default;

FMetalRenderTargetViewRHI::FMetalRenderTargetViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIRenderTargetViewDesc& InDesc)
    : FRHIRenderTargetView(InTexture, InDesc)
    , FMetalView(InDevice)
    , MipLevel(0)
    , ArrayIndex(0)
{
    ResolveRTVMipAndSlice(InDesc, MipLevel, ArrayIndex);
}

FMetalRenderTargetViewRHI::~FMetalRenderTargetViewRHI() = default;

FMetalDepthStencilViewRHI::FMetalDepthStencilViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIDepthStencilViewDesc& InDesc)
    : FRHIDepthStencilView(InTexture, InDesc)
    , FMetalView(InDevice)
    , MipLevel(0)
    , ArrayIndex(0)
    , Flags(InDesc.Flags)
{
    ResolveDSVMipAndSlice(InDesc, MipLevel, ArrayIndex);
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
