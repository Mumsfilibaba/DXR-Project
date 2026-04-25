#pragma once
#include "Core/Containers/Array.h"
#include "RHI/RHIRayTracing.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "D3D12RHI/D3D12ResourceViews.h"

class FD3D12CommandList;
class FMaterial;

typedef TSharedRef<class FD3D12GeometryAccelerationStructureRHI> FD3D12GeometryAccelerationStructureRHIRef;
typedef TSharedRef<class FD3D12SceneAccelerationStructureRHI>    FD3D12SceneAccelerationStructureRHIRef;

struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) FD3D12ShaderBindingTableEntry
{
    CHAR ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
    D3D12_GPU_VIRTUAL_ADDRESS RootDescriptors[D3D12_MAX_LOCAL_ROOT_DESCRIPTORS] = {};
};

class FD3D12ShaderBindingTableBuilder : public FD3D12DeviceChild
{
public:
    FD3D12ShaderBindingTableBuilder(FD3D12Device* InDevice);

    void PopulateEntry(
        FD3D12RayTracingPipelineStateRHI* PipelineState,
        FD3D12RootSignature*              RootSignature,
        FD3D12ShaderBindingTableEntry&    OutShaderBindingEntry,
        const FRayTracingShaderResources& Resources);

    void Reset();
};

class FD3D12AccelerationStructure : public FD3D12DeviceChild
{
public:
    FD3D12AccelerationStructure(FD3D12Device* InDevice);
    virtual ~FD3D12AccelerationStructure() = default;

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        return ResultResourceStorage.GetGPUVirtualAddress();
    }

    FD3D12Resource* GetResource() const
    {
        return ResultResourceStorage.GetResource();
    }

    FD3D12Resource* GetScratchBuffer() const
    {
        return ScratchResourceStorage.GetResource();
    }

protected:
    FD3D12ResourceStorage ResultResourceStorage;
    FD3D12ResourceStorage ScratchResourceStorage;
};

class FD3D12GeometryAccelerationStructureRHI : public FRHIGeometryAccelerationStructure, public FD3D12AccelerationStructure
{
public:
    FD3D12GeometryAccelerationStructureRHI(FD3D12Device* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc);
    virtual ~FD3D12GeometryAccelerationStructureRHI() = default;
    
    bool Build(FD3D12CommandContext& CmdContext, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc);

    // FRHIGeometryAccelerationStructure Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetResource()); }
    
    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

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

    bool Build(FD3D12CommandContext& CmdContext, const FRHISceneAccelerationStructureBuildDesc& BuildDesc);
	bool BuildBindingTable(
        class FD3D12CommandContext& CmdContext, 
        FD3D12RayTracingPipelineStateRHI* PipelineState, 
        FD3D12OnlineDescriptorHeap* ResourceHeap, 
        FD3D12OnlineDescriptorHeap* SamplerHeap,
		const FRayTracingShaderResources* RayGenLocalResources, 
        const FRayTracingShaderResources* MissLocalResources, 
        const FRayTracingShaderResources* HitGroupResources, 
        uint32 NumHitGroupResources);

    // FRHISceneAccelerationStructure Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetResource()); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const override final { return FRHIDescriptorHandle(); }

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE            GetRayGenShaderRecord() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupTable()      const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissShaderTable()    const;

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
    FD3D12ShaderResourceViewRHIRef                    View;
    FD3D12ResourceRef                                 InstanceBuffer;
    FD3D12ResourceRef                                 BindingTable;
    uint32                                            BindingTableStride;
    uint32                                            NumHitGroups;
    // TODO: Maybe move these somewhere else
    FD3D12ShaderBindingTableBuilder                   ShaderBindingTableBuilder;
    ID3D12DescriptorHeap*                             BindingTableHeaps[2];
};
