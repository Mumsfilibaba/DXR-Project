#pragma once
#include "Core/Containers/Array.h"
#include "RHI/RHIRayTracing.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12SamplerState.h"
#include "D3D12RHI/D3D12PipelineState.h"
#include "D3D12RHI/RayTracing/D3D12RayTracingPipeline.h"
#include "D3D12RHI/D3D12RootSignature.h"

class FD3D12CommandContext;
class FD3D12LocalDescriptorHeap;

enum class ED3D12PendingLocalTableHeap : uint8
{
    Resource, // CBV/SRV/UAV heap
    Sampler,  // Sampler heap
};

struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) FD3D12ShaderBindingTableEntry
{
    CHAR                      ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES] = {};
    D3D12_GPU_VIRTUAL_ADDRESS RootDescriptors[D3D12_MAX_LOCAL_RECORD_ENTRIES]         = {};
};

struct FD3D12PendingLocalTable
{
    static constexpr uint32 MaxDescriptors = D3D12_MAX_LOCAL_TABLE_DESCRIPTORS;

    uint64                        RecordByteOffset                     = 0;     // Record offset within CpuShadow
    uint32                        TableParamSlot                       = 0;     // Index into FD3D12ShaderBindingTableEntry::RootDescriptors
    uint32                        NumDescriptors                       = 0;     // Contiguous descriptors, indexed by table slot
    ED3D12PendingLocalTableHeap   HeapKind                             = ED3D12PendingLocalTableHeap::Resource;
    bool                          bUsesUnorderedAccessViews            = false; // Resource heap only: SRV-typed or UAV-typed
    FD3D12ShaderResourceViewRHI*  ShaderResourceViews[MaxDescriptors]  = {};
    FD3D12UnorderedAccessViewRHI* UnorderedAccessViews[MaxDescriptors] = {};
    FD3D12SamplerStateRHI*        Samplers[MaxDescriptors]             = {};
};

class FD3D12ShaderBindingTable : public FRHIShaderBindingTable, public FD3D12DeviceChild
{
public:
    FD3D12ShaderBindingTable(FD3D12Device* InDevice, const FRHIShaderBindingTableDesc& InDesc);
    virtual ~FD3D12ShaderBindingTable();

    // FRHIShaderBindingTable Interface
    virtual void* GetRHINativeResource() const override final;

    void SetBindings(ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings);
    void Build(FD3D12CommandContext& CmdContext);
    void ResolveLocalDescriptorTables(FD3D12CommandContext& CmdContext, FD3D12LocalDescriptorHeap& ResourceHeap, FD3D12LocalDescriptorHeap& SamplerHeap);
    void ClearTableRecords();
    
    uint32 GetNumPendingLocalTableDescriptors() const;
    uint32 GetNumPendingLocalSamplerDescriptors() const;

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE            GetRayGenRecord()  const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissTable()     const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupTable() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetCallableTable() const;

    bool HasPendingLocalTables() const
    {
        return !PendingLocalTables.IsEmpty();
    }

    FD3D12Resource* GetResource() const
    {
        return TableResourceStorage.GetResource();
    }

    FD3D12RayTracingPipelineStateRHI* GetPipeline() const
    {
        return Pipeline.Get();
    }

private:
    uint32 GetSubTableBaseRecord(ERayTracingShaderRecordKind RecordKind) const;

    void PopulateRecord(FD3D12RootSignature* LocalRootSignature, uint8* OutRecord, uint64 RecordByteOffset, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const String& ExportName, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings);
    void UploadCpuShadow(FD3D12CommandContext& CmdContext);
    void RemovePendingTablesInRange(uint64 BeginByteOffset, uint64 EndByteOffset);

    FD3D12RayTracingPipelineStateRHIRef Pipeline;
    uint32                              NumRayGen;
    uint32                              NumMiss;
    uint32                              NumCallable;
    uint32                              NumHitGroup;
    uint32                              RecordStride;
    TArray<uint8>                       CpuShadow;
    FD3D12ResourceStorage               TableResourceStorage;
    TArray<FD3D12PendingLocalTable>     PendingLocalTables;
};
