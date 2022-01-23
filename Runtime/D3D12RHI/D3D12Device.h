#pragma once
#include "D3D12Core.h"
#include <dxgi1_6.h>

#include <DXProgrammableCapture.h>

class CD3D12OfflineDescriptorHeap;
class CD3D12OnlineDescriptorHeap;
class CD3D12RHIComputePipelineState;
class CD3D12RootSignature;

#define D3D12_PIPELINE_STATE_STREAM_ALIGNMENT (sizeof(void*))
#define D3D12_ENABLE_PIX_MARKERS              (1)

void RHID3D12DeviceRemovedHandler(class CD3D12Device* Device);

class CD3D12Device
{
public:
    CD3D12Device(bool bInEnableDebugLayer, bool bInEnableGPUValidation, bool bInEnableDRED);
    ~CD3D12Device();

    bool Init();

    /* Retrieve the multi sample quality for a certain format and sample count */
    int32 GetMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount);

    /* Retrieves the adapter name for the graphics card that is represented by this device */
    CString GetAdapterName() const;

public:

    FORCEINLINE ID3D12Device* GetDevice() const
    {
        return Device.Get();
    }

    FORCEINLINE ID3D12Device5* GetDXRDevice() const
    {
        return DXRDevice.Get();
    }

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
    TComPtr<IDXGIFactory2> Factory;
    TComPtr<IDXGIAdapter1> Adapter;
    TComPtr<ID3D12Device>  Device;
    TComPtr<ID3D12Device5> DXRDevice;

    TComPtr<IDXGraphicsAnalysis> GraphicsAnalysisInterface;

    D3D_FEATURE_LEVEL MinFeatureLevel = D3D_FEATURE_LEVEL_12_0;
    D3D_FEATURE_LEVEL ActiveFeatureLevel = D3D_FEATURE_LEVEL_11_0;

    D3D12_RAYTRACING_TIER            RayTracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    D3D12_SAMPLER_FEEDBACK_TIER      SamplerFeedBackTier = D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED;
    D3D12_MESH_SHADER_TIER           MeshShaderTier = D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
    D3D12_VARIABLE_SHADING_RATE_TIER VariableShadingRateTier = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
    uint32 VariableShadingRateTileSize = 0;

    HMODULE DXGILib = 0;
    HMODULE D3D12Lib = 0;
    HMODULE PIXLib = 0;

    uint32 AdapterID = 0;

    bool bAllowTearing = false;
    bool bEnableDebugLayer = false;
    bool bEnableGPUValidation = false;
    bool bEnableDRED = false;
};