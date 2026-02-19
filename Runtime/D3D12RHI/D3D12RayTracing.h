#pragma once
#include "Core/Containers/Array.h"
#include "RHI/RHIRayTracing.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "D3D12RHI/D3D12ResourceViews.h"

class FD3D12CommandList;
class FMaterial;

struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) FD3D12ShaderBindingTableEntry
{
    CHAR ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
    D3D12_GPU_DESCRIPTOR_HANDLE RootDescriptorTables[4] = { 0, 0, 0, 0 };
};

class FD3D12ShaderBindingTableBuilder : public FD3D12DeviceChild
{
public:
    FD3D12ShaderBindingTableBuilder(FD3D12Device* InDevice);

    void PopulateEntry(
        FD3D12RayTracingPipelineStateRHI*    PipelineState,
        FD3D12RootSignature*              RootSignature,
        FD3D12OnlineDescriptorHeap*       ResourceHeap,
        FD3D12OnlineDescriptorHeap*       SamplerHeap,
        FD3D12ShaderBindingTableEntry&    OutShaderBindingEntry,
        const FRayTracingShaderResources& Resources);

    void CopyDescriptors();
    void Reset();

private:
    uint32 CPUHandleSizes[1024];

    D3D12_CPU_DESCRIPTOR_HANDLE ResourceHandles[1024];
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerHandles[1024];

    // Online resources
    D3D12_CPU_DESCRIPTOR_HANDLE GPUResourceHandles[1024];
    D3D12_CPU_DESCRIPTOR_HANDLE GPUSamplerHandles[1024];

    uint32 GPUResourceHandleSizes[1024];
    uint32 GPUSamplerHandleSizes[1024];
    uint32 CPUResourceIndex = 0;
    uint32 CPUSamplerIndex  = 0;
    uint32 GPUResourceIndex = 0;
    uint32 GPUSamplerIndex  = 0;
};

class FD3D12AccelerationStructure : public FD3D12DeviceChild
{
public:
    FD3D12AccelerationStructure(FD3D12Device* InDevice);
    virtual ~FD3D12AccelerationStructure() = default;

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        return ResultBuffer->GetGPUVirtualAddress();
    }

    FD3D12Resource* GetResource() const
    {
        return ResultBuffer.Get();
    }

    FD3D12Resource* GetScratchBuffer() const
    {
        return ScratchBuffer.Get();
    }

protected:
    FD3D12ResourceRef ResultBuffer;
    FD3D12ResourceRef ScratchBuffer;
};

class FD3D12GeometryAccelerationStructureRHI : public FRHIGeometryAccelerationStructure, public FD3D12AccelerationStructure
{
public:
    FD3D12GeometryAccelerationStructureRHI(FD3D12Device* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc);
    virtual ~FD3D12GeometryAccelerationStructureRHI() = default;
    
    bool Build(FD3D12CommandContext& CmdContext, const FRHIGeometryAccelerationStructureBuildDesc& BuildInfo);

    // FRHIGeometryAccelerationStructure Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetResource()); }
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    FD3D12BufferRHI* GetVertexBuffer() const
    { 
        return VertexBuffer.Get();
    }

    FD3D12BufferRHI* GetIndexBuffer() const
    {
        return IndexBuffer.Get();
    }

private:
    TSharedRef<FD3D12BufferRHI> VertexBuffer;
    TSharedRef<FD3D12BufferRHI> IndexBuffer;
};

class FD3D12SceneAccelerationStructureRHI : public FRHISceneAccelerationStructure , public FD3D12AccelerationStructure
{
public:
    FD3D12SceneAccelerationStructureRHI(FD3D12Device* InDevice, const FRHISceneAccelerationStructureDesc& InSceneDesc);
    virtual ~FD3D12SceneAccelerationStructureRHI() = default;

    bool Build(FD3D12CommandContext& CmdContext, const FRHISceneAccelerationStructureBuildDesc& BuildInfo);
	bool BuildBindingTable(class FD3D12CommandContext& CmdContext, FD3D12RayTracingPipelineStateRHI* PipelineState, FD3D12OnlineDescriptorHeap* ResourceHeap, FD3D12OnlineDescriptorHeap* SamplerHeap,
		const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources);

    // FRHISceneAccelerationStructure Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetResource()); }
    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE GetRayGenShaderRecord() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissShaderTable() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupTable() const;

    FD3D12Resource* GetInstanceBuffer() const
    {
        return InstanceBuffer.Get();
    }

    FD3D12Resource* GetBindingTable() const
    {
        return BindingTable.Get();
    }

private:
    TArray<FRHIGeometryAccelerationStructureInstance> Instances;

    FD3D12ShaderResourceViewRHIRef View;
    FD3D12ResourceRef InstanceBuffer;
    FD3D12ResourceRef BindingTable;

    uint32 BindingTableStride = 0;
    uint32 NumHitGroups = 0;

    // TODO: Maybe move these somewhere else
    FD3D12ShaderBindingTableBuilder ShaderBindingTableBuilder;
    ID3D12DescriptorHeap* BindingTableHeaps[2] = { nullptr, nullptr };
};
