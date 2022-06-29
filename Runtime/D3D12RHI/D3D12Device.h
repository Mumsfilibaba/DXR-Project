#pragma once
#include "D3D12RefCounted.h"

#include "Core/Containers/SharedRef.h"

#include <DXProgrammableCapture.h>

#if WIN10_BUILD_17134
    #include <dxgi1_6.h>
#endif

class CD3D12OfflineDescriptorHeap;
class CD3D12OnlineDescriptorHeap;
class CD3D12ComputePipelineState;
class CD3D12RootSignature;
class FD3D12CoreInterface;

#define D3D12_PIPELINE_STATE_STREAM_ALIGNMENT (sizeof(void*))
#define D3D12_ENABLE_PIX_MARKERS              (1)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

class FD3D12Device;
class FD3D12Adapter;
class FD3D12CoreInterface;

typedef FD3D12Device FD3D12Device;

typedef TSharedRef<FD3D12Device>  D3D12DeviceRef;
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

    bool Initialize();

    FD3D12Device*            CreateDevice();

    FD3D12AdapterInitializer GetInitializer()   const { return Initializer; }
    uint32                   GetAdapterIndex()  const { return AdapterIndex; }
    
    bool                     AllowTearing()     const { return bAllowTearing; }

    FD3D12CoreInterface*     GetCoreInterface() const { return CoreInterface; }

    IDXGIAdapter1* GetDXGIAdapter()  const { return Adapter.Get(); }
    IDXGIFactory2* GetDXGIFactory()  const { return Factory.Get(); }
    IDXGIFactory5* GetDXGIFactory5() const { return Factory5.Get(); }
#if WIN10_BUILD_17134
    IDXGIFactory6* GetDXGIFactory6() const { return Factory6.Get(); }
#endif

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
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Device

class FD3D12Device : public FD3D12RefCounted
{
public:
    FD3D12Device(FD3D12CoreInterface* InCoreInterface, bool bInEnableDebugLayer, bool bInEnableGPUValidation, bool bInEnableDRED);
    ~FD3D12Device();

    bool Initialize();

    int32 GetMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount);

    String GetAdapterName() const;

    FORCEINLINE FD3D12CoreInterface* GetCoreInterface() const { return CoreInterface; }

    FORCEINLINE ID3D12Device* GetD3D12Device() const { return Device.Get(); }

    FORCEINLINE ID3D12Device5* GeD3D12Device5() const { return DXRDevice.Get(); }

public:

    FORCEINLINE IDXGraphicsAnalysis* GetGraphicsAnalysisInterface() const
    {
        return GraphicsAnalysisInterface.Get();
    }

    FORCEINLINE IDXGIFactory2* GetFactory() const
    {
        return Factory.Get();
    }

    FORCEINLINE IDXGIAdapter1* GetAdapter() const
    {
        return Adapter.Get();
    }

    FORCEINLINE bool CanAllowTearing() const
    {
        return bAllowTearing;
    }

    FORCEINLINE D3D12_RAYTRACING_TIER GetRayTracingTier() const
    {
        return RayTracingTier;
    }

    FORCEINLINE D3D12_SAMPLER_FEEDBACK_TIER GetSamplerFeedbackTier() const
    {
        return SamplerFeedBackTier;
    }

    FORCEINLINE D3D12_VARIABLE_SHADING_RATE_TIER GetVariableRateShadingTier() const
    {
        return VariableShadingRateTier;
    }

    FORCEINLINE D3D12_MESH_SHADER_TIER GetMeshShaderTier() const
    {
        return MeshShaderTier;
    }

    FORCEINLINE uint32 GetVariableRateShadingTileSize() const
    {
        return VariableShadingRateTileSize;
    }

public:

    FORCEINLINE HRESULT CreateRootSignature(uint32 NodeMask, const void* BlobWithRootSignature, SIZE_T BlobLengthInBytes, REFIID Riid, void** RootSignature)
    {
        return Device->CreateRootSignature(NodeMask, BlobWithRootSignature, BlobLengthInBytes, Riid, RootSignature);
    }

    FORCEINLINE HRESULT CreateCommitedResource(const D3D12_HEAP_PROPERTIES* HeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC* Desc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* OptimizedClearValue, REFIID RiidResource, void** Resource)
    {
        return Device->CreateCommittedResource(HeapProperties, HeapFlags, Desc, InitialResourceState, OptimizedClearValue, RiidResource, Resource);
    }

    FORCEINLINE HRESULT CreatePipelineState(const D3D12_PIPELINE_STATE_STREAM_DESC* Desc, REFIID Riid, void** PipelineState)
    {
        return DXRDevice->CreatePipelineState(Desc, Riid, PipelineState);
    }

    FORCEINLINE void CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC* Desc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        Device->CreateConstantBufferView(Desc, DestDescriptor);
    }

    FORCEINLINE void CreateRenderTargetView(ID3D12Resource* Resource, const D3D12_RENDER_TARGET_VIEW_DESC* Desc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        Device->CreateRenderTargetView(Resource, Desc, DestDescriptor);
    }

    FORCEINLINE void CreateDepthStencilView(ID3D12Resource* Resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* Desc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        Device->CreateDepthStencilView(Resource, Desc, DestDescriptor);
    }

    FORCEINLINE void CreateShaderResourceView(ID3D12Resource* Resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* Desc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        Device->CreateShaderResourceView(Resource, Desc, DestDescriptor);
    }

    FORCEINLINE void CreateUnorderedAccessView(ID3D12Resource* Resource, ID3D12Resource* CounterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* Desc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        Device->CreateUnorderedAccessView(Resource, CounterResource, Desc, DestDescriptor);
    }

    FORCEINLINE void CreateSampler(const D3D12_SAMPLER_DESC* Desc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        Device->CreateSampler(Desc, DestDescriptor);
    }

    FORCEINLINE void CopyDescriptors(uint32 NumDestDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* DestDescriptorRangeStarts, const uint32* DestDescriptorRangeSizes, uint32 NumSrcDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* SrcDescriptorRangeStarts, const uint32* SrcDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
    {
        Device->CopyDescriptors(NumDestDescriptorRanges, DestDescriptorRangeStarts, DestDescriptorRangeSizes, NumSrcDescriptorRanges, SrcDescriptorRangeStarts, SrcDescriptorRangeSizes, DescriptorHeapsType);
    }

    FORCEINLINE void CopyDescriptorsSimple(uint32 NumDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart, D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart, D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
    {
        Device->CopyDescriptorsSimple(NumDescriptors, DestDescriptorRangeStart, SrcDescriptorRangeStart, DescriptorHeapsType);
    }

    FORCEINLINE void GetRaytracingAccelerationStructurePrebuildInfo(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* Desc, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* Info)
    {
        DXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(Desc, Info);
    }

    FORCEINLINE uint32 GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType)
    {
        return Device->GetDescriptorHandleIncrementSize(DescriptorHeapType);
    }

private:
    class FD3D12CoreInterface*   CoreInterface;

    TComPtr<IDXGIFactory2>       Factory;
    TComPtr<IDXGIAdapter1>       Adapter;

    TComPtr<ID3D12Device>        Device;
    TComPtr<ID3D12Device5>       DXRDevice;
    TComPtr<IDXGraphicsAnalysis> GraphicsAnalysisInterface;

    D3D_FEATURE_LEVEL MinFeatureLevel    = D3D_FEATURE_LEVEL_12_0;
    D3D_FEATURE_LEVEL ActiveFeatureLevel = D3D_FEATURE_LEVEL_11_0;

    D3D12_RAYTRACING_TIER            RayTracingTier              = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    D3D12_SAMPLER_FEEDBACK_TIER      SamplerFeedBackTier         = D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED;
    D3D12_MESH_SHADER_TIER           MeshShaderTier              = D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
    D3D12_VARIABLE_SHADING_RATE_TIER VariableShadingRateTier     = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
    uint32                           VariableShadingRateTileSize = 0;

    HMODULE DXGILib  = 0;
    HMODULE D3D12Lib = 0;
    HMODULE PIXLib   = 0;

    uint32 AdapterID = 0;

    bool bAllowTearing        = false;
    bool bEnableDebugLayer    = false;
    bool bEnableGPUValidation = false;
    bool bEnableDRED          = false;
};