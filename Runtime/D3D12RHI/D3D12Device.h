#pragma once
#include "D3D12RefCounted.h"

#include "Core/Containers/SharedRef.h"

#if WIN10_BUILD_17134
    #include <dxgi1_6.h>
#endif

#include <DXProgrammableCapture.h>

class FD3D12Device;
class FD3D12Adapter;
class FD3D12CoreInterface;
class CD3D12RootSignature;
class CD3D12ComputePipelineState;
class CD3D12OnlineDescriptorHeap;
class CD3D12OfflineDescriptorHeap;

#define D3D12_PIPELINE_STATE_STREAM_ALIGNMENT (sizeof(void*))
#define D3D12_ENABLE_PIX_MARKERS              (1)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<FD3D12Device>  FD3D12DeviceRef;
typedef TSharedRef<FD3D12Adapter> FD3D12AdapterRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12DeviceRemovedHandlerRHI

void D3D12DeviceRemovedHandlerRHI(FD3D12Device* Device);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12AdapterInitializer

struct FD3D12AdapterInitializer
{
    FD3D12AdapterInitializer()
        : bEnableDebugLayer(false)
        , bEnableGPUValidation(false)
        , bEnableDRED(false)
        , bEnablePIX(false)
        , bPreferDGPU(true)
    { }

    FD3D12AdapterInitializer( bool bInEnableDebugLayer
                            , bool bInEnableGPUValidation
                            , bool bInEnableDRED
                            , bool bInEnablePIX
                            , bool bInPreferDGPU)
        : bEnableDebugLayer(bInEnableDebugLayer)
        , bEnableGPUValidation(bInEnableGPUValidation)
        , bEnableDRED(bInEnableDRED)
        , bEnablePIX(bInEnablePIX)
        , bPreferDGPU(bInPreferDGPU)
    { }

    bool operator==(const FD3D12AdapterInitializer& RHS) const
    {
        return (bEnableDebugLayer    == RHS.bEnableDebugLayer)
            && (bEnableGPUValidation == RHS.bEnableGPUValidation)
            && (bEnableDRED          == RHS.bEnableDRED)
            && (bEnablePIX           == RHS.bEnablePIX)
            && (bPreferDGPU          == RHS.bPreferDGPU);
    }

    bool operator!=(const FD3D12AdapterInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    bool bEnableDebugLayer    : 1;
    bool bEnableGPUValidation : 1;
    bool bEnableDRED          : 1;
    bool bEnablePIX           : 1;
    bool bPreferDGPU          : 1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Adapter

class FD3D12Adapter : public FD3D12RefCounted
{
public:

    FD3D12Adapter(FD3D12CoreInterface* InCoreInterface, const FD3D12AdapterInitializer& InInitializer)
        : FD3D12RefCounted()
        , Initializer(InInitializer)
        , AdapterIndex(0)
        , bAllowTearing(false)
        , CoreInterface(InCoreInterface)
        , Factory(nullptr)
#if WIN10_BUILD_17134
        , Factory6(nullptr)
#endif
        , Adapter(nullptr)
    { }

    ~FD3D12Adapter() = default;

public:

    bool                     Initialize();

    FD3D12AdapterInitializer GetInitializer()   const { return Initializer; }
    uint32                   GetAdapterIndex()  const { return AdapterIndex; }
    
    String                   GetDescription() const { return WideToChar(WStringView(AdapterDesc.Description)); }

    bool                     IsDebugLayerEnabled() const { return Initializer.bEnableDebugLayer; }

    bool                     SupportsTearing()     const { return bAllowTearing; }

    FD3D12CoreInterface*     GetCoreInterface() const { return CoreInterface; }

    IDXGIAdapter1* GetDXGIAdapter()  const { return Adapter.Get(); }

    IDXGIFactory2* GetDXGIFactory()  const { return Factory.Get(); }
    IDXGIFactory5* GetDXGIFactory5() const { return Factory5.Get(); }
#if WIN10_BUILD_17134
    IDXGIFactory6* GetDXGIFactory6() const { return Factory6.Get(); }
#endif

    IDXGraphicsAnalysis* GetGraphicsAnalysis() const { return DXGraphicsAnalysis.Get(); }

private:
    FD3D12AdapterInitializer Initializer;
    uint32                   AdapterIndex;
    
    bool                     bAllowTearing;

    FD3D12CoreInterface*         CoreInterface;

    TComPtr<IDXGIAdapter1>       Adapter;
    
    TComPtr<IDXGIFactory2>       Factory;
    TComPtr<IDXGIFactory5>       Factory5;
#if WIN10_BUILD_17134
    TComPtr<IDXGIFactory6>       Factory6;
#endif

    TComPtr<IDXGraphicsAnalysis> DXGraphicsAnalysis;

    DXGI_ADAPTER_DESC1           AdapterDesc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Device

class FD3D12Device : public FD3D12RefCounted
{
public:
    
    FD3D12Device(FD3D12Adapter* InAdapter)
        : FD3D12RefCounted()
        , Adapter(InAdapter)
        , Device(nullptr)
        , Device1(nullptr)
        , Device2(nullptr)
        , Device3(nullptr)
        , Device4(nullptr)
        , Device5(nullptr)
        , Device6(nullptr)
        , Device7(nullptr)
        , Device8(nullptr)
        , Device9(nullptr)
        , MinFeatureLevel(D3D_FEATURE_LEVEL_12_0)
        , ActiveFeatureLevel(D3D_FEATURE_LEVEL_11_0)
    { }

    ~FD3D12Device();

public:
    
    bool           Initialize();

    int32          GetMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount);

    FD3D12Adapter* GetAdapter() const { return Adapter; }

    ID3D12Device*  GetD3D12Device()  const { return Device.Get(); }
#if WIN10_BUILD_14393
    ID3D12Device1* GetD3D12Device1() const { return Device1.Get(); }
#endif
#if WIN10_BUILD_15063
    ID3D12Device2* GetD3D12Device2() const { return Device2.Get(); }
#endif
#if WIN10_BUILD_17763 // TODO: Fix correctly
    ID3D12Device3* GetD3D12Device3() const { return Device3.Get(); }
    ID3D12Device4* GetD3D12Device4() const { return Device4.Get(); }
    ID3D12Device5* GetD3D12Device5() const { return Device5.Get(); }
#endif
#if WIN11_BUILD_22000 // TODO: Fix correctly
    ID3D12Device6* GetD3D12Device6() const { return Device6.Get(); }
    ID3D12Device7* GetD3D12Device7() const { return Device7.Get(); }
    ID3D12Device8* GetD3D12Device8() const { return Device8.Get(); }
    ID3D12Device9* GetD3D12Device9() const { return Device9.Get(); }
#endif

private:
    FD3D12Adapter*         Adapter;

    TComPtr<ID3D12Device>  Device;
#if WIN10_BUILD_14393
    TComPtr<ID3D12Device1> Device1;
#endif
#if WIN10_BUILD_15063
    TComPtr<ID3D12Device2> Device2;
#endif
#if WIN10_BUILD_17763 // TODO: Fix correctly
    TComPtr<ID3D12Device3> Device3;
    TComPtr<ID3D12Device4> Device4;
    TComPtr<ID3D12Device5> Device5;
#endif
#if WIN11_BUILD_22000 // TODO: Fix correctly
    TComPtr<ID3D12Device6> Device6;
    TComPtr<ID3D12Device7> Device7;
    TComPtr<ID3D12Device8> Device8;
    TComPtr<ID3D12Device9> Device9;
#endif

    D3D_FEATURE_LEVEL      MinFeatureLevel;
    D3D_FEATURE_LEVEL      ActiveFeatureLevel;
};