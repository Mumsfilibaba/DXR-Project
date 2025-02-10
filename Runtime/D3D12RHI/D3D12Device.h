#pragma once
#include "Core/Containers/SharedRef.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12SamplerState.h"
#include "D3D12RHI/D3D12PipelineState.h"
#include "D3D12RHI/D3D12Queue.h"

#include <DXProgrammableCapture.h>
#if WIN10_BUILD_17134
    #include <dxgi1_6.h>
#endif

class FD3D12Device;
class FD3D12Adapter;
class FD3D12RHI;
class FD3D12RootSignature;
class FD3D12ComputePipelineState;
class FD3D12OnlineDescriptorHeap;
class FD3D12OfflineDescriptorHeap;
class FD3D12QueryHeapManager;

typedef TSharedRef<FD3D12Device>  FD3D12DeviceRef;
typedef TSharedRef<FD3D12Adapter> FD3D12AdapterRef;

/* D3D12 Feature Support */

extern D3D12RHI_API bool GD3D12ForceBinding;
extern D3D12RHI_API bool GD3D12SupportPipelineCache;
extern D3D12RHI_API bool GD3D12SupportTightAlignment;
extern D3D12RHI_API bool GD3D12SupportGPUUploadHeaps;
extern D3D12RHI_API bool GD3D12SupportBindless;

extern D3D12RHI_API D3D12_RESOURCE_BINDING_TIER      GD3D12ResourceBindingTier;
extern D3D12RHI_API D3D12_RAYTRACING_TIER            GD3D12RayTracingTier;
extern D3D12RHI_API D3D12_VARIABLE_SHADING_RATE_TIER GD3D12VariableRateShadingTier;
extern D3D12RHI_API D3D12_MESH_SHADER_TIER           GD3D12MeshShaderTier;
extern D3D12RHI_API D3D12_SAMPLER_FEEDBACK_TIER      GD3D12SamplerFeedbackTier;
extern D3D12RHI_API D3D12_VIEW_INSTANCING_TIER       GD3D12ViewInstancingTier;
extern D3D12RHI_API D3D_SHADER_MODEL                 GD3D12HighestShaderModel;

class FD3D12Adapter
{
public:
    FD3D12Adapter();

    bool Initialize();

    bool IsDebugLayerEnabled() const { return bEnableDebugLayer; }
    bool IsTearingSupported()  const { return bAllowTearing; }

    FString GetDescription() const { return WideToChar(FStringViewWide(AdapterDesc.Description)); }

    FORCEINLINE uint32 GetAdapterIndex() const
    { 
        return AdapterIndex;
    }

    FORCEINLINE IDXGraphicsAnalysis* GetGraphicsAnalysis() const
    {
        return DXGraphicsAnalysis.Get();
    }

    FORCEINLINE IDXGIAdapter1* GetDXGIAdapter() const 
    {
        return Adapter.Get();
    }

    FORCEINLINE IDXGIAdapter3* GetDXGIAdapter3() const { return Adapter3.Get(); }

    FORCEINLINE IDXGIFactory2* GetDXGIFactory() const
    {
        return Factory.Get();
    }

    FORCEINLINE IDXGIFactory5* GetDXGIFactory5() const { return Factory5.Get(); }
#if WIN10_BUILD_17134
    FORCEINLINE IDXGIFactory6* GetDXGIFactory6() const { return Factory6.Get(); }
#endif

private:
    uint32 AdapterIndex;
    
    bool bAllowTearing;
    bool bEnableDebugLayer;

    TComPtr<IDXGIAdapter1> Adapter;
    TComPtr<IDXGIAdapter3> Adapter3;

    TComPtr<IDXGraphicsAnalysis> DXGraphicsAnalysis;
    
    TComPtr<IDXGIFactory2> Factory;
    TComPtr<IDXGIFactory5> Factory5;
#if WIN10_BUILD_17134
    TComPtr<IDXGIFactory6> Factory6;
#endif

    DXGI_ADAPTER_DESC1 AdapterDesc;
};

struct FD3D12DefaultDescriptors
{
    FD3D12ConstantBufferViewRef  DefaultCBV;
    FD3D12ShaderResourceViewRef  DefaultSRV;
    FD3D12UnorderedAccessViewRef DefaultUAV;
    FD3D12RenderTargetViewRef    DefaultRTV;
    FD3D12SamplerStateRef        DefaultSampler;
};

class FD3D12Device
{
public:
    FD3D12Device(FD3D12Adapter* InAdapter);
    ~FD3D12Device();

    bool Initialize();

    ID3D12CommandQueue*            GetD3D12CommandQueue(ED3D12CommandQueueType QueueType);
    FD3D12Queue*                   GetQueue(ED3D12CommandQueueType QueueType);
    FD3D12CommandAllocatorManager* GetCommandAllocatorManager(ED3D12CommandQueueType QueueType);

    FD3D12UploadHeapAllocator& GetUploadAllocator() { return *UploadAllocator; }

    FD3D12RootSignatureManager& GetRootSignatureManager() { return *RootSignatureManager; }
    FD3D12PipelineStateManager& GetPipelineStateManager() { return *PipelineStateManager; }

    FD3D12OnlineDescriptorHeap& GetGlobalResourceHeap() { return *GlobalResourceHeap; }
    FD3D12OnlineDescriptorHeap& GetGlobalSamplerHeap()  { return *GlobalSamplerHeap; }
    
    FD3D12QueryHeapManager* GetQueryHeapManager(EQueryType QueryType);

    FD3D12OfflineDescriptorHeap& GetResourceOfflineDescriptorHeap()     { return *ResourceOfflineDescriptorHeap; }
    FD3D12OfflineDescriptorHeap& GetRenderTargetOfflineDescriptorHeap() { return *RenderTargetOfflineDescriptorHeap; }
    FD3D12OfflineDescriptorHeap& GetDepthStencilOfflineDescriptorHeap() { return *DepthStencilOfflineDescriptorHeap; }
    FD3D12OfflineDescriptorHeap& GetSamplerOfflineDescriptorHeap()      { return *SamplerOfflineDescriptorHeap; }

    FD3D12DefaultDescriptors& GetDefaultDescriptors() { return DefaultDescriptors; }

    int32 QueryMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount);

    D3D_FEATURE_LEVEL GetFeatureLevel() const { return ActiveFeatureLevel; }
    uint32 GetNodeMask()  const { return NodeMask; }
    uint32 GetNodeCount() const { return NodeCount; }

    FORCEINLINE FD3D12Adapter* GetAdapter() const
    {
        return Adapter;
    }

    FORCEINLINE ID3D12Device* GetD3D12Device() const
    {
        return D3D12Device.Get();
    }

#if WIN10_BUILD_14393
    FORCEINLINE ID3D12Device1* GetD3D12Device1() const { return D3D12Device1.Get(); }
#endif
#if WIN10_BUILD_15063
    FORCEINLINE ID3D12Device2* GetD3D12Device2() const { return D3D12Device2.Get(); }
#endif
#if WIN10_BUILD_16299
    FORCEINLINE ID3D12Device3* GetD3D12Device3() const { return D3D12Device3.Get(); }
#endif
#if WIN10_BUILD_17134
    FORCEINLINE ID3D12Device4* GetD3D12Device4() const { return D3D12Device4.Get(); }
#endif
#if WIN10_BUILD_17763
    FORCEINLINE ID3D12Device5* GetD3D12Device5() const { return D3D12Device5.Get(); }
#endif
#if WIN10_BUILD_18362
    FORCEINLINE ID3D12Device6* GetD3D12Device6() const { return D3D12Device6.Get(); }
#endif
#if WIN10_BUILD_19041
    FORCEINLINE ID3D12Device7* GetD3D12Device7() const { return D3D12Device7.Get(); }
#endif
#if WIN10_BUILD_20348
    FORCEINLINE ID3D12Device8* GetD3D12Device8() const { return D3D12Device8.Get(); }
#endif
#if WIN11_BUILD_22000
    FORCEINLINE ID3D12Device9* GetD3D12Device9() const { return D3D12Device9.Get(); }
#endif

private:
    bool CreateDevice();
    bool CreateCommandManagers();
    bool CreateDefaultResources();
    void QueryFeatureSupport();

    FD3D12Adapter* const Adapter;

    FD3D12OnlineDescriptorHeap*    GlobalResourceHeap;
    FD3D12OnlineDescriptorHeap*    GlobalSamplerHeap;

    FD3D12OfflineDescriptorHeap*   ResourceOfflineDescriptorHeap;
    FD3D12OfflineDescriptorHeap*   RenderTargetOfflineDescriptorHeap;
    FD3D12OfflineDescriptorHeap*   DepthStencilOfflineDescriptorHeap;
    FD3D12OfflineDescriptorHeap*   SamplerOfflineDescriptorHeap;

    FD3D12UploadHeapAllocator*     UploadAllocator;
    
    FD3D12RootSignatureManager*    RootSignatureManager;
    FD3D12PipelineStateManager*    PipelineStateManager;

    FD3D12Queue*                   DirectQueue;
    FD3D12Queue*                   CopyQueue;
    FD3D12Queue*                   ComputeQueue;

    FD3D12CommandAllocatorManager* DirectCommandAllocatorManager;
    FD3D12CommandAllocatorManager* CopyCommandAllocatorManager;
    FD3D12CommandAllocatorManager* ComputeCommandAllocatorManager;

    FD3D12QueryHeapManager*        TimingQueryHeapManager;
    FD3D12QueryHeapManager*        OcclusionQueryHeapManager;

    FD3D12DefaultDescriptors       DefaultDescriptors;


    D3D_FEATURE_LEVEL              MinFeatureLevel;
    D3D_FEATURE_LEVEL              ActiveFeatureLevel;

    TComPtr<ID3D12Device> D3D12Device;

#if WIN10_BUILD_14393
    TComPtr<ID3D12Device1> D3D12Device1;
#endif
#if WIN10_BUILD_15063
    TComPtr<ID3D12Device2> D3D12Device2;
#endif
#if WIN10_BUILD_16299
    TComPtr<ID3D12Device3> D3D12Device3;
#endif
#if WIN10_BUILD_17134
    TComPtr<ID3D12Device4> D3D12Device4;
#endif
#if WIN10_BUILD_17763
    TComPtr<ID3D12Device5> D3D12Device5;
#endif
#if WIN10_BUILD_18362
    TComPtr<ID3D12Device6> D3D12Device6;
#endif
#if WIN10_BUILD_19041
    TComPtr<ID3D12Device7> D3D12Device7;
#endif
#if WIN10_BUILD_20348
    TComPtr<ID3D12Device8> D3D12Device8;
#endif
#if WIN11_BUILD_22000
    TComPtr<ID3D12Device9> D3D12Device9;
#endif

    uint32 NodeMask;
    uint32 NodeCount;
};
