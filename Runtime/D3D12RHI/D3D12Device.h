#pragma once
#include "D3D12RefCounted.h"
#include "D3D12Descriptors.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandListManager.h"
#include "D3D12CommandAllocator.h"
#include "D3D12DeferredDeletionQueue.h"

#include "Core/Containers/SharedRef.h"

#if WIN10_BUILD_17134
    #include <dxgi1_6.h>
#endif

#include <DXProgrammableCapture.h>

class FD3D12Device;
class FD3D12Adapter;
class FD3D12Interface;
class FD3D12RootSignature;
class FD3D12ComputePipelineState;
class FD3D12OnlineDescriptorHeap;
class FD3D12OfflineDescriptorHeap;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<FD3D12Device>  FD3D12DeviceRef;
typedef TSharedRef<FD3D12Adapter> FD3D12AdapterRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RayTracingDesc

struct FD3D12RayTracingDesc
{
    FD3D12RayTracingDesc()
        : Tier(D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
    { }

    bool IsSupported() const
    {
        return (Tier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
    }

    D3D12_RAYTRACING_TIER Tier;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12VariableRateShadingDesc

struct FD3D12VariableRateShadingDesc
{
    FD3D12VariableRateShadingDesc()
        : Tier(D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
        , ShadingRateImageTileSize(0)
    { }

    bool IsSupported() const 
    {
        return (Tier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED);
    }

    D3D12_VARIABLE_SHADING_RATE_TIER Tier;
    uint32 ShadingRateImageTileSize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12MeshShadingDesc

struct FD3D12MeshShadingDesc
{
    FD3D12MeshShadingDesc()
        : Tier(D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
    { }

    bool IsSupported() const
    {
        return (Tier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED);
    }

    D3D12_MESH_SHADER_TIER Tier;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12SamplerFeedbackDesc

struct FD3D12SamplerFeedbackDesc
{
    FD3D12SamplerFeedbackDesc()
        : Tier(D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED)
    { }

    bool IsSupported() const
    {
        return (Tier != D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED);
    }

    D3D12_SAMPLER_FEEDBACK_TIER Tier;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Adapter

class FD3D12Adapter 
    : public FD3D12RefCounted
{
public:
    FD3D12Adapter(FD3D12Interface* InD3D12Interface);
    ~FD3D12Adapter() = default;

    bool           Initialize();

    inline FString GetDescription()  const { return WideToChar(FStringViewWide(AdapterDesc.Description)); }

    inline bool    IsDebugLayerEnabled() const { return bEnableDebugLayer; }
    inline bool    SupportsTearing()     const { return bAllowTearing; }

    FORCEINLINE FD3D12Interface*     GetD3D12Interface() const { return D3D12Interface; }

    FORCEINLINE IDXGraphicsAnalysis* GetGraphicsAnalysis() const { return DXGraphicsAnalysis.Get(); }

    FORCEINLINE IDXGIAdapter1*       GetDXGIAdapter()  const { return Adapter.Get(); }
    FORCEINLINE uint32               GetAdapterIndex() const { return AdapterIndex; }

    FORCEINLINE IDXGIFactory2*       GetDXGIFactory()  const { return Factory.Get(); }
    FORCEINLINE IDXGIFactory5*       GetDXGIFactory5() const { return Factory5.Get(); }

#if WIN10_BUILD_17134
    FORCEINLINE IDXGIFactory6*       GetDXGIFactory6() const { return Factory6.Get(); }
#endif

private:
    FD3D12Interface* D3D12Interface;

    uint32 AdapterIndex;

    bool   bAllowTearing;
    bool   bEnableDebugLayer;

    TComPtr<IDXGIAdapter1> Adapter;
    DXGI_ADAPTER_DESC1     AdapterDesc;
    
    TComPtr<IDXGIFactory2> Factory;
    TComPtr<IDXGIFactory5> Factory5;
#if WIN10_BUILD_17134
    TComPtr<IDXGIFactory6> Factory6;
#endif

    TComPtr<IDXGraphicsAnalysis> DXGraphicsAnalysis;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Device

class FD3D12Device 
    : public FD3D12RefCounted
{
public:
    FD3D12Device(FD3D12Adapter* InAdapter);
    ~FD3D12Device();

    bool Initialize();

    inline ID3D12CommandQueue* GetD3D12CommandQueue(ED3D12CommandQueueType QueueType)
    {
        FD3D12CommandListManager* CommandListManager = GetCommandListManager(QueueType);
        CHECK(CommandListManager != nullptr);
        return CommandListManager->GetD3D12CommandQueue();
    }

    FD3D12CommandListManager* GetCommandListManager(ED3D12CommandQueueType QueueType);

    inline FD3D12CommandListManager&      GetDirectCommandListManager()  { return DirectCommandListManager; }
    inline FD3D12CommandListManager&      GetCopyCommandListManager()    { return CopyCommandListManager; }
    inline FD3D12CommandListManager&      GetComputeCommandListManager() { return ComputeCommandListManager; }

    inline FD3D12CommandAllocatorManager& GetCopyCommandAllocatorManager() { return CopyCommandAllocatorManager; }
    
    inline FD3D12DeferredDeletionQueue&   GetDeferredDeletionQueue() { return DeferredDeletionQueue; }
    inline FD3D12RootSignatureCache&      GetRootSignatureCache()    { return RootSignatureCache; }
    
    inline FD3D12DescriptorHeap*          GetGlobalResourceHeap() const { return GlobalResourceHeap.Get(); }
    inline FD3D12DescriptorHeap*          GetGlobalSamplerHeap()  const { return GlobalSamplerHeap.Get(); }
    
    inline const FD3D12RayTracingDesc&          GetRayTracingDesc()          const { return RayTracingDesc; }
    inline const FD3D12VariableRateShadingDesc& GetVariableRateShadingDesc() const { return VariableRateShadingDesc; }
    inline const FD3D12MeshShadingDesc&         GetMeshShadingDesc()         const { return MeshShadingDesc; }
    inline const FD3D12SamplerFeedbackDesc&     GetSamplerFeedbackDesc()     const { return SamplerFeedbackDesc; }
    
    inline uint32 GetNodeCount() const { return NodeCount; }
    inline uint32 GetNodeMask()  const { return NodeMask; }

    int32 GetMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount);

    FORCEINLINE FD3D12Adapter* GetAdapter()      const { return Adapter; }
    
    FORCEINLINE ID3D12Device*  GetD3D12Device()  const { return Device.Get(); }

#if WIN10_BUILD_14393
    FORCEINLINE ID3D12Device1* GetD3D12Device1() const { return Device1.Get(); }
#endif
#if WIN10_BUILD_15063
    FORCEINLINE ID3D12Device2* GetD3D12Device2() const { return Device2.Get(); }
#endif
#if WIN10_BUILD_16299
    FORCEINLINE ID3D12Device3* GetD3D12Device3() const { return Device3.Get(); }
#endif
#if WIN10_BUILD_17134
    FORCEINLINE ID3D12Device4* GetD3D12Device4() const { return Device4.Get(); }
#endif
#if WIN10_BUILD_17763
    FORCEINLINE ID3D12Device5* GetD3D12Device5() const { return Device5.Get(); }
#endif
#if WIN10_BUILD_18362
    FORCEINLINE ID3D12Device6* GetD3D12Device6() const { return Device6.Get(); }
#endif
#if WIN10_BUILD_19041
    FORCEINLINE ID3D12Device7* GetD3D12Device7() const { return Device7.Get(); }
#endif
#if WIN10_BUILD_20348
    FORCEINLINE ID3D12Device8* GetD3D12Device8() const { return Device8.Get(); }
#endif
#if WIN11_BUILD_22000
    FORCEINLINE ID3D12Device9* GetD3D12Device9() const { return Device9.Get(); }
#endif

private:
    bool CreateDevice();
    bool CreateQueues();

    FD3D12DescriptorHeapRef       GlobalResourceHeap;
    FD3D12DescriptorHeapRef       GlobalSamplerHeap;

    FD3D12RootSignatureCache      RootSignatureCache;

    FD3D12CommandListManager      DirectCommandListManager;
    FD3D12CommandListManager      CopyCommandListManager;
    FD3D12CommandListManager      ComputeCommandListManager;

    FD3D12CommandAllocatorManager CopyCommandAllocatorManager;

    FD3D12DeferredDeletionQueue   DeferredDeletionQueue;

    FD3D12RayTracingDesc          RayTracingDesc;
    FD3D12MeshShadingDesc         MeshShadingDesc;
    FD3D12SamplerFeedbackDesc     SamplerFeedbackDesc;
    FD3D12VariableRateShadingDesc VariableRateShadingDesc;
    
    uint32            NodeMask;
    uint32            NodeCount;
    
    D3D_FEATURE_LEVEL MinFeatureLevel;
    D3D_FEATURE_LEVEL ActiveFeatureLevel;
    
    FD3D12Adapter*         Adapter;

    TComPtr<ID3D12Device>  Device;
#if WIN10_BUILD_14393
    TComPtr<ID3D12Device1> Device1;
#endif
#if WIN10_BUILD_15063
    TComPtr<ID3D12Device2> Device2;
#endif
#if WIN10_BUILD_16299
    TComPtr<ID3D12Device3> Device3;
#endif
#if WIN10_BUILD_17134
    TComPtr<ID3D12Device4> Device4;
#endif
#if WIN10_BUILD_17763
    TComPtr<ID3D12Device5> Device5;
#endif
#if WIN10_BUILD_18362
    TComPtr<ID3D12Device6> Device6;
#endif
#if WIN10_BUILD_19041
    TComPtr<ID3D12Device7> Device7;
#endif
#if WIN10_BUILD_20348
    TComPtr<ID3D12Device8> Device8;
#endif
#if WIN11_BUILD_22000
    TComPtr<ID3D12Device9> Device9;
#endif
};
