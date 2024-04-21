#include "MetalSamplerState.h"
#include "MetalDeviceContext.h"

FMetalSamplerState::FMetalSamplerState(FMetalDeviceContext* InDeviceContext, const FRHISamplerStateInfo& InSamplerInfo)
    : FRHISamplerState(InDesc)
    , FMetalObject(InDeviceContext)
    , FMetalRefCounted()
    , SamplerState(nullptr)
{
}

FMetalSamplerState::~FMetalSamplerState()
{
    NSSafeRelease(SamplerState);
}

bool FMetalSamplerState::Initialize()
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

    id<MTLDevice> Device = GetDeviceContext()->GetMTLDevice();
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
