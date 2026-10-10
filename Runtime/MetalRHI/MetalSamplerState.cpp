#include "MetalRHI/MetalSamplerState.h"
#include "MetalRHI/MetalBindlessDescriptors.h"
#include "MetalRHI/MetalDevice.h"

FMetalSamplerStateRHI::FMetalSamplerStateRHI(FMetalDevice* InDevice, const FRHISamplerStateDesc& InSamplerDesc)
    : FRHISamplerState(InSamplerDesc)
    , FMetalDeviceChild(InDevice)
    , SamplerState(nullptr)
    , BindlessHandle()
{
}

FMetalSamplerStateRHI::~FMetalSamplerStateRHI()
{
    if (BindlessHandle.IsValid())
    {
        if (FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager())
        {
            BindlessManager->Free(BindlessHandle);
        }

        BindlessHandle = FRHIDescriptorHandle();
    }

    [SamplerState release];
}

FRHIDescriptorHandle FMetalSamplerStateRHI::GetBindlessHandle() const
{
    if (BindlessHandle.IsValid())
    {
        return BindlessHandle;
    }

    FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();

    if (!BindlessManager || !BindlessManager->IsEnabled() || !SamplerState)
    {
        return FRHIDescriptorHandle();
    }

    BindlessHandle = BindlessManager->Allocate(EDescriptorType::Sampler);

    if (!BindlessHandle.IsValid())
    {
        return FRHIDescriptorHandle();
    }

    BindlessManager->WriteSampler(BindlessHandle, SamplerState, true);
    return BindlessHandle;
}

void* FMetalSamplerStateRHI::GetRHINativeSampler() const
{
    return (__bridge void*)SamplerState;
}

bool FMetalSamplerStateRHI::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    MTLSamplerDescriptor* SamplerDesc = [[MTLSamplerDescriptor new] autorelease];
    SamplerDesc.sAddressMode = MetalRHI::ConvertSamplerMode(Desc.AddressU);
    SamplerDesc.tAddressMode = MetalRHI::ConvertSamplerMode(Desc.AddressV);
    SamplerDesc.rAddressMode = MetalRHI::ConvertSamplerMode(Desc.AddressW);
    SamplerDesc.minFilter    = MetalRHI::ConvertSamplerFilterToMinFilter(Desc.Filter);
    SamplerDesc.magFilter    = MetalRHI::ConvertSamplerFilterToMagFilter(Desc.Filter);
    SamplerDesc.mipFilter    = MetalRHI::ConvertSamplerFilterToMipmapMode(Desc.Filter);
    SamplerDesc.lodMinClamp  = Desc.MinLOD;
    SamplerDesc.lodMaxClamp  = Desc.MaxLOD;
    SamplerDesc.lodAverage   = YES;

    if (Desc.MipLODBias != 0.0f)
    {
        if (GMetalSupportsSamplerLODBias)
        {
        #if METAL_SDK_HAS_MACOS_26
            if (@available(macOS 26.0, *))
            {
                SamplerDesc.lodBias = Math::Clamp(Desc.MipLODBias, -16.0f, 15.999f);
            }
        #endif
        }
        else
        {
            METAL_WARNING("Sampler MipLODBias %.3f is ignored, Metal applies a sampler LOD bias only from macOS 26", Desc.MipLODBias);
        }
    }

    SamplerDesc.maxAnisotropy = MetalRHI::IsAnisotropySampler(Desc.Filter)
        ? Math::Clamp<NSUInteger>(Desc.MaxAnisotropy, 1, 16)
        : 1;

    SamplerDesc.compareFunction = MetalRHI::IsComparisonSampler(Desc.Filter)
        ? MetalRHI::ConvertCompareFunction(Desc.ComparisonFunc)
        : MTLCompareFunctionNever;

    SamplerDesc.borderColor            = MetalRHI::ConvertBorderColor(Desc.BorderColor);
    SamplerDesc.normalizedCoordinates  = YES;
    SamplerDesc.supportArgumentBuffers = YES;

    id<MTLDevice> Device = GetDevice()->GetMTLDevice();
    CHECK(Device != nil);

    id<MTLSamplerState> NewSamplerState = [Device newSamplerStateWithDescriptor:SamplerDesc];
    if (!NewSamplerState)
    {
        LOG_ERROR("Failed to create SamplerState");
        return false;
    }
    else
    {
        SamplerState = NewSamplerState;
    }

    return true;
}
