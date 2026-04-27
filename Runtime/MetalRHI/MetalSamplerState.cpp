#include "MetalRHI/MetalSamplerState.h"
#include "MetalRHI/MetalDevice.h"

FMetalSamplerStateRHI::FMetalSamplerStateRHI(FMetalDevice* InDevice, const FRHISamplerStateDesc& InSamplerDesc)
    : FRHISamplerState(InSamplerDesc)
    , FMetalDeviceChild(InDevice)
    , SamplerState(nullptr)
{
}

FMetalSamplerStateRHI::~FMetalSamplerStateRHI()
{
    [SamplerState release];
}

FRHIDescriptorHandle FMetalSamplerStateRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

void* FMetalSamplerStateRHI::GetRHINativeSampler() const
{
    return (__bridge void*)SamplerState;
}

bool FMetalSamplerStateRHI::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    MTLSamplerDescriptor* SamplerDesc = [[MTLSamplerDescriptor new] autorelease];
    SamplerDesc.rAddressMode          = ConvertSamplerMode(Desc.AddressU);
    SamplerDesc.sAddressMode          = ConvertSamplerMode(Desc.AddressV);
    SamplerDesc.tAddressMode          = ConvertSamplerMode(Desc.AddressW);
    SamplerDesc.minFilter             = ConvertSamplerFilterToMinFilter(Desc.Filter);
    SamplerDesc.magFilter             = ConvertSamplerFilterToMagFilter(Desc.Filter);
    SamplerDesc.mipFilter             = ConvertSamplerFilterToMipmapMode(Desc.Filter);
    SamplerDesc.lodMinClamp           = Desc.MinLOD;
    SamplerDesc.lodMaxClamp           = Desc.MaxLOD;
    SamplerDesc.lodAverage            = YES;
    SamplerDesc.maxAnisotropy         = Desc.MaxAnisotropy;
    SamplerDesc.compareFunction       = ConvertComparisonFunc(Desc.ComparisonFunc);
    SamplerDesc.borderColor           = MTLSamplerBorderColorOpaqueBlack;
    SamplerDesc.normalizedCoordinates = YES;

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
