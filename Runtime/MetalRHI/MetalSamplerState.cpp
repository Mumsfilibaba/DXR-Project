#include "MetalSamplerState.h"
#include "MetalDeviceContext.h"

FMetalSamplerState::FMetalSamplerState(FMetalDeviceContext* InDeviceContext, const FRHISamplerStateInfo& InSamplerInfo)
    : FRHISamplerState(InSamplerInfo)
    , FMetalDeviceChild(InDeviceContext)
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
    SamplerDesc.rAddressMode          = ConvertSamplerMode(Info.AddressU);
    SamplerDesc.sAddressMode          = ConvertSamplerMode(Info.AddressV);
    SamplerDesc.tAddressMode          = ConvertSamplerMode(Info.AddressW);
    SamplerDesc.minFilter             = ConvertSamplerFilterToMinFilter(Info.Filter);
    SamplerDesc.magFilter             = ConvertSamplerFilterToMagFilter(Info.Filter);
    SamplerDesc.mipFilter             = ConvertSamplerFilterToMipmapMode(Info.Filter);
    SamplerDesc.lodMinClamp           = Info.MinLOD;
    SamplerDesc.lodMaxClamp           = Info.MaxLOD;
    SamplerDesc.lodAverage            = YES;
    SamplerDesc.maxAnisotropy         = Info.MaxAnisotropy;
    SamplerDesc.compareFunction       = ConvertComparisonFunc(Info.ComparisonFunc);
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
