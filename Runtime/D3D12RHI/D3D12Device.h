#pragma once
#include "Core/Containers/SharedRef.h"
#include "D3D12RHI/D3D12Capabilities.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Heap.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12SamplerState.h"
#include "D3D12RHI/D3D12PipelineState.h"
#include "D3D12RHI/D3D12Queue.h"

#include <DXProgrammableCapture.h>
#if DXGI_1_6
    #include <dxgi1_6.h>
#endif

class FD3D12Device;
class FD3D12Adapter;
class FD3D12DeviceRHI;
class FD3D12RootSignature;
class FD3D12ComputePipelineStateRHI;
class FD3D12OnlineDescriptorHeap;
class FD3D12OfflineDescriptorHeap;
class FD3D12BindlessDescriptorHeap;
class FD3D12QueryHeap;
class FD3D12QueryHeapManager;
class FD3D12ResidencyManager;
class FD3D12LinearAllocator;
class FD3D12DynamicConstantsAllocator;
class FD3D12BufferAllocator;
class FD3D12TextureAllocator;
class FD3D12UploadHeapAllocator;
class FD3D12CommandContext;

typedef TSharedRef<FD3D12Device>  FD3D12DeviceRef;
typedef TSharedRef<FD3D12Adapter> FD3D12AdapterRef;

class FD3D12Adapter
{
public:
    FD3D12Adapter();
    ~FD3D12Adapter();

    bool Initialize();

    bool IsDebugLayerEnabled() const { return bEnableDebugLayer; }
    bool IsTearingSupported()  const { return bAllowTearing; }

    String GetDescription() const { return WideToChar(WStringView(AdapterDesc.Description)); }

    FORCEINLINE uint32 GetAdapterIndex() const
    { 
        return AdapterIndex;
    }

    FORCEINLINE IDXGraphicsAnalysis* GetGraphicsAnalysis() const
    {
        return GraphicsAnalysisInterface.Get();
    }

    FORCEINLINE IDXGIAdapter1* GetDXGIAdapter() const 
    {
        return Adapter.Get();
    }

    FORCEINLINE IDXGIAdapter3* GetDXGIAdapter3() const
    {
        return Adapter3.Get();
    }

    FORCEINLINE IDXGIFactory2* GetDXGIFactory() const
    {
        return Factory.Get();
    }

    FORCEINLINE IDXGIFactory5* GetDXGIFactory5() const { return Factory5.Get(); }
#if DXGI_1_6
    FORCEINLINE IDXGIFactory6* GetDXGIFactory6() const { return Factory6.Get(); }
#endif

private:
    TComPtr<IDXGIAdapter1>       Adapter;
    TComPtr<IDXGIAdapter3>       Adapter3;
    TComPtr<IDXGraphicsAnalysis> GraphicsAnalysisInterface;
    TComPtr<IDXGIFactory2>       Factory;
    TComPtr<IDXGIFactory5>       Factory5;
#if DXGI_1_6
    TComPtr<IDXGIFactory6>       Factory6;
#endif

    DXGI_ADAPTER_DESC1 AdapterDesc;
    uint32             AdapterIndex;
    bool               bAllowTearing     : 1;
    bool               bEnableDebugLayer : 1;
};

struct FD3D12DefaultDescriptors
{
    FD3D12ConstantBufferViewRef     DefaultCBV;
    FD3D12ShaderResourceViewRHIRef  DefaultSRV;
    FD3D12UnorderedAccessViewRHIRef DefaultUAV;
    FD3D12RenderTargetViewRHIRef    DefaultRTV;
    FD3D12SamplerStateRHIRef        DefaultSampler;
};

struct ED3D12CommandSignatureType
{
    enum Type : uint8
    {
        Draw = 0,
        DrawIndexed,
        Dispatch,
        DispatchMesh,
        DispatchRays,
        Count
    };
};

class FD3D12Device
{
public:
    FD3D12Device(FD3D12Adapter* InAdapter);
    ~FD3D12Device();

    bool Initialize();
    void BeginFrame(FD3D12CommandContext* InCommandContext);
    void EndFrame(FD3D12CommandContext* InCommandContext);
    void FinalizePendingDefragMoves();
    void CancelPendingDefragMoves(FD3D12ResourceBase* Owner);

    void WaitForGPU();
        
    bool ReallocateGlobalDescriptorHeap(ED3D12GlobalDescriptorHeapType HeapType);
    
    bool CreateCommittedResource(const D3D12_RESOURCE_DESC& Desc, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceRef& OutResource);
    bool CreatePlacedResource(FD3D12Heap* Heap, uint64 Offset, const D3D12_RESOURCE_DESC& Desc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceRef& OutResource);
#if D3D12_USE_RESOURCE_DESC1
    bool CreateCommittedResource2(const D3D12_RESOURCE_DESC1& Desc, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceRef& OutResource);
    bool CreatePlacedResource1(FD3D12Heap* Heap, uint64 Offset, const D3D12_RESOURCE_DESC1& Desc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceRef& OutResource);
#endif
    bool CreateHeap(const D3D12_HEAP_DESC& Desc, FD3D12HeapRef& OutHeap);
    
    bool SupportsSwapChainFormat(DXGI_FORMAT DXGIFormat, ESwapChainUsageFlags Usage) const;

    void RegisterDeviceRemovedEvent();
    void RegisterDebugMessageCallback();
    void UnregisterDebugMessageCallback();

    ID3D12CommandQueue*              GetD3D12CommandQueue(ED3D12CommandQueueType QueueType);
    FD3D12Queue*                     GetQueue(ED3D12CommandQueueType QueueType);
    FD3D12CommandAllocatorManager*   GetCommandAllocatorManager(ED3D12CommandQueueType QueueType);
    FD3D12QueryHeapManager*          GetQueryHeapManager(EQueryType QueryType);
    FD3D12QueryHeap*                 ObtainQueryHeap(D3D12_QUERY_HEAP_TYPE HeapType);
    void                             RecycleQueryHeap(FD3D12QueryHeap* Heap);
    int32                            QueryMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount);

    FD3D12RootSignatureManager&      GetRootSignatureManager()              const { return *RootSignatureManager; }
    FD3D12PipelineStateManager&      GetPipelineStateManager()              const { return *PipelineStateManager; }
    FD3D12OnlineDescriptorHeap&      GetGlobalResourceHeap()                const { return *GlobalResourceHeap; }
    FD3D12OnlineDescriptorHeap&      GetGlobalSamplerHeap()                 const { return *GlobalSamplerHeap; }
    FD3D12BindlessDescriptorHeap*    GetResourceBindlessHeap()              const { return ResourceBindlessHeap; }
    FD3D12BindlessDescriptorHeap*    GetSamplerBindlessHeap()               const { return SamplerBindlessHeap; }
    FD3D12OfflineDescriptorHeap&     GetResourceOfflineDescriptorHeap()     const { return *ResourceOfflineDescriptorHeap; }
    FD3D12OfflineDescriptorHeap&     GetRenderTargetOfflineDescriptorHeap() const { return *RenderTargetOfflineDescriptorHeap; }
    FD3D12OfflineDescriptorHeap&     GetDepthStencilOfflineDescriptorHeap() const { return *DepthStencilOfflineDescriptorHeap; }
    FD3D12OfflineDescriptorHeap&     GetSamplerOfflineDescriptorHeap()      const { return *SamplerOfflineDescriptorHeap; }
    const FD3D12DefaultDescriptors&  GetDefaultDescriptors()                const { return DefaultDescriptors; }
    FD3D12ResidencyManager*          GetResidencyManager()                  const { return ResidencyManager; }
    FD3D12LinearAllocator*           GetStagingBufferAllocator()            const { return StagingBufferAllocator; }
    FD3D12DynamicConstantsAllocator* GetDynamicConstantsAllocator()         const { return DynamicConstantsAllocator; }
    FD3D12BufferAllocator*           GetBufferAllocator()                   const { return BufferAllocator; }
    FD3D12TextureAllocator*          GetTextureAllocator()                  const { return TextureAllocator; }
    FD3D12UploadHeapAllocator*       GetUploadHeapAllocator()               const { return UploadHeapAllocator; }
    FD3D12Fence&                     GetFrameFence()                        const { return *FrameFence; }

    ID3D12CommandSignature* GetCommandSignature(ED3D12CommandSignatureType::Type Type) const
    {
        return CommandSignatures[Type].Get();
    }

    uint32            GetNodeCount()    const { return NodeCount; }
    uint32            GetNodeMask()     const { return NodeMask; }
    D3D_FEATURE_LEVEL GetFeatureLevel() const { return ActiveFeatureLevel; }

    FORCEINLINE FD3D12Adapter* GetAdapter() const
    {
        return Adapter;
    }

    FORCEINLINE ID3D12Device* GetD3D12Device() const
    {
        return D3D12Device.Get();
    }

#if D3D12_USE_ID3D12DEVICE_1
    FORCEINLINE ID3D12Device1*  GetD3D12Device1()  const { return D3D12Device1.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_2
    FORCEINLINE ID3D12Device2*  GetD3D12Device2()  const { return D3D12Device2.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_3
    FORCEINLINE ID3D12Device3*  GetD3D12Device3()  const { return D3D12Device3.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_4
    FORCEINLINE ID3D12Device4*  GetD3D12Device4()  const { return D3D12Device4.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_5
    FORCEINLINE ID3D12Device5*  GetD3D12Device5()  const { return D3D12Device5.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_6
    FORCEINLINE ID3D12Device6*  GetD3D12Device6()  const { return D3D12Device6.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_7
    FORCEINLINE ID3D12Device7*  GetD3D12Device7()  const { return D3D12Device7.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_8
    FORCEINLINE ID3D12Device8*  GetD3D12Device8()  const { return D3D12Device8.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_9
    FORCEINLINE ID3D12Device9*  GetD3D12Device9()  const { return D3D12Device9.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_10
    FORCEINLINE ID3D12Device10* GetD3D12Device10() const { return D3D12Device10.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_11
    FORCEINLINE ID3D12Device11* GetD3D12Device11() const { return D3D12Device11.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_12
    FORCEINLINE ID3D12Device12* GetD3D12Device12() const { return D3D12Device12.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_13
    FORCEINLINE ID3D12Device13* GetD3D12Device13() const { return D3D12Device13.Get(); }
#endif
#if D3D12_USE_ID3D12DEVICE_14
    FORCEINLINE ID3D12Device14* GetD3D12Device14() const { return D3D12Device14.Get(); }
#endif

private:
    bool CreateDevice();
    bool CreateCommandQueues();
    bool CreateDefaultResources();
    bool CreateCommandSignatures();
    void QueryDeviceFeatureSupport();

    FD3D12Adapter* const             Adapter;

    FD3D12OnlineDescriptorHeap*      GlobalResourceHeap;
    FD3D12OnlineDescriptorHeap*      GlobalSamplerHeap;
    FD3D12BindlessDescriptorHeap*    ResourceBindlessHeap;
    FD3D12BindlessDescriptorHeap*    SamplerBindlessHeap;
    FD3D12OfflineDescriptorHeap*     ResourceOfflineDescriptorHeap;
    FD3D12OfflineDescriptorHeap*     RenderTargetOfflineDescriptorHeap;
    FD3D12OfflineDescriptorHeap*     DepthStencilOfflineDescriptorHeap;
    FD3D12OfflineDescriptorHeap*     SamplerOfflineDescriptorHeap;

    FD3D12RootSignatureManager*      RootSignatureManager;
    FD3D12PipelineStateManager*      PipelineStateManager;
    FD3D12ResidencyManager*          ResidencyManager;
    FD3D12LinearAllocator*           StagingBufferAllocator;
    FD3D12DynamicConstantsAllocator* DynamicConstantsAllocator;
    FD3D12BufferAllocator*           BufferAllocator;
    FD3D12TextureAllocator*          TextureAllocator;
    FD3D12UploadHeapAllocator*       UploadHeapAllocator;

    FD3D12FenceRef                   FrameFence;
    FD3D12Queue*                     DirectQueue;
    FD3D12Queue*                     CopyQueue;
    FD3D12Queue*                     ComputeQueue;
    FD3D12CommandAllocatorManager*   DirectCommandAllocatorManager;
    FD3D12CommandAllocatorManager*   CopyCommandAllocatorManager;
    FD3D12CommandAllocatorManager*   ComputeCommandAllocatorManager;

    FD3D12QueryHeapManager*          TimingQueryHeapManager;
    FD3D12QueryHeapManager*          OcclusionQueryHeapManager;
    FD3D12QueryHeapManager*          PipelineStatsQueryHeapManager;

    FD3D12DefaultDescriptors         DefaultDescriptors;

    TComPtr<ID3D12CommandSignature>  CommandSignatures[ED3D12CommandSignatureType::Count];

    D3D_FEATURE_LEVEL                MinFeatureLevel;
    D3D_FEATURE_LEVEL                ActiveFeatureLevel;
    uint32                           NodeMask;
    uint32                           NodeCount;

    TComPtr<ID3D12Device>     D3D12Device;
#if D3D12_USE_ID3D12DEVICE_1
    TComPtr<ID3D12Device1>    D3D12Device1;
#endif
#if D3D12_USE_ID3D12DEVICE_2
    TComPtr<ID3D12Device2>    D3D12Device2;
#endif
#if D3D12_USE_ID3D12DEVICE_3
    TComPtr<ID3D12Device3>    D3D12Device3;
#endif
#if D3D12_USE_ID3D12DEVICE_4
    TComPtr<ID3D12Device4>    D3D12Device4;
#endif
#if D3D12_USE_ID3D12DEVICE_5
    TComPtr<ID3D12Device5>    D3D12Device5;
#endif
#if D3D12_USE_ID3D12DEVICE_6
    TComPtr<ID3D12Device6>    D3D12Device6;
#endif
#if D3D12_USE_ID3D12DEVICE_7
    TComPtr<ID3D12Device7>    D3D12Device7;
#endif
#if D3D12_USE_ID3D12DEVICE_8
    TComPtr<ID3D12Device8>    D3D12Device8;
#endif
#if D3D12_USE_ID3D12DEVICE_9
    TComPtr<ID3D12Device9>    D3D12Device9;
#endif
#if D3D12_USE_ID3D12DEVICE_10
    TComPtr<ID3D12Device10>   D3D12Device10;
#endif
#if D3D12_USE_ID3D12DEVICE_11
    TComPtr<ID3D12Device11>   D3D12Device11;
#endif
#if D3D12_USE_ID3D12DEVICE_12
    TComPtr<ID3D12Device12>   D3D12Device12;
#endif
#if D3D12_USE_ID3D12DEVICE_13
    TComPtr<ID3D12Device13>   D3D12Device13;
#endif
#if D3D12_USE_ID3D12DEVICE_14
    TComPtr<ID3D12Device14>   D3D12Device14;
#endif
#if D3D12_USE_DEBUG_MESSAGE_CALLBACK
    TComPtr<ID3D12InfoQueue1> DebugInfoQueue;
    DWORD                     DebugMessageCallbackCookie;
#endif

    HANDLE               DeviceRemovedEvent;
    HANDLE               DeviceRemovedWait;
    TComPtr<ID3D12Fence> DeviceRemovedFence;
};
