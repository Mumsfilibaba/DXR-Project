#pragma once
#include "RHI/RHIRayTracing.h"

#include "D3D12DeviceChild.h"
#include "D3D12RHIBuffer.h"
#include "D3D12RHIViews.h"

class CD3D12CommandList;
class CMaterial;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIRayTracingGeometry

class CD3D12RHIRayTracingGeometry : public CRHIRayTracingGeometry, public CD3D12DeviceObject
{
public:
    CD3D12RHIRayTracingGeometry(CD3D12Device* InDevice, uint32 InFlags);
    ~CD3D12RHIRayTracingGeometry() = default;

    bool Build(class CD3D12RHICommandContext& CmdContext, bool Update);

    virtual void SetName(const String& InName) override
    {
        CRHIObject::SetName(InName);
        ResultBuffer->SetName(InName);
    }

    virtual bool IsValid() const override
    {
        return ResultBuffer != nullptr;
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        Assert(ResultBuffer != nullptr);
        return ResultBuffer->GetGPUVirtualAddress();
    }

    TSharedRef<CD3D12RHIVertexBuffer> VertexBuffer;
    TSharedRef<CD3D12RHIIndexBuffer>  IndexBuffer;

    TSharedRef<CD3D12Resource> ResultBuffer;
    TSharedRef<CD3D12Resource> ScratchBuffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12ShaderBindingTableEntry

struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) SD3D12ShaderBindingTableEntry
{
    char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
    D3D12_GPU_DESCRIPTOR_HANDLE	RootDescriptorTables[4] = { 0, 0, 0, 0 };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12ShaderBindingTableBuilder

class CD3D12ShaderBindingTableBuilder : public CD3D12DeviceObject
{
public:
    CD3D12ShaderBindingTableBuilder(CD3D12Device* InDevice);
    ~CD3D12ShaderBindingTableBuilder() = default;

    void PopulateEntry(
        CD3D12RHIRayTracingPipelineState* PipelineState,
        CD3D12RootSignature* RootSignature,
        CD3D12OnlineDescriptorHeap* ResourceHeap,
        CD3D12OnlineDescriptorHeap* SamplerHeap,
        SD3D12ShaderBindingTableEntry& OutShaderBindingEntry,
        const SRayTracingShaderResources& Resources);

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
    uint32 CPUSamplerIndex = 0;
    uint32 GPUResourceIndex = 0;
    uint32 GPUSamplerIndex = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIRayTracingScene

class CD3D12RHIRayTracingScene : public CRHIRayTracingScene, public CD3D12DeviceObject
{
public:
    CD3D12RHIRayTracingScene(CD3D12Device* InDevice, uint32 InFlags);
    ~CD3D12RHIRayTracingScene() = default;

    bool Build(class CD3D12RHICommandContext& CmdContext, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool Update);

    bool BuildBindingTable(
        class CD3D12RHICommandContext& CmdContext,
        CD3D12RHIRayTracingPipelineState* PipelineState,
        CD3D12OnlineDescriptorHeap* ResourceHeap,
        CD3D12OnlineDescriptorHeap* SamplerHeap,
        const SRayTracingShaderResources* RayGenLocalResources,
        const SRayTracingShaderResources* MissLocalResources,
        const SRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources);

    virtual void SetName(const String& InName) override;

    virtual bool IsValid() const override
    {
        return ResultBuffer != nullptr;
    }

    virtual CRHIShaderResourceView* GetShaderResourceView() const
    {
        return View.Get();
    }

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE            GetRayGenShaderRecord() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissShaderTable() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupTable() const;

    FORCEINLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        Assert(ResultBuffer != nullptr);
        return ResultBuffer->GetGPUVirtualAddress();
    }

    FORCEINLINE CD3D12Resource* GetResultBuffer()  const
    {
        return ResultBuffer.Get();
    }

    FORCEINLINE CD3D12Resource* GetInstanceuffer() const
    {
        return InstanceBuffer.Get();
    }

    FORCEINLINE CD3D12Resource* GetBindingTable()  const
    {
        return BindingTable.Get();
    }

private:
    TArray<SRayTracingGeometryInstance>     Instances;
    TSharedRef<CD3D12ShaderResourceView> View;

    TSharedRef<CD3D12Resource> ResultBuffer;
    TSharedRef<CD3D12Resource> ScratchBuffer;
    TSharedRef<CD3D12Resource> InstanceBuffer;
    TSharedRef<CD3D12Resource> BindingTable;

    uint32 BindingTableStride = 0;
    uint32 NumHitGroups = 0;

    // TODO: Maybe move these somewhere else
    CD3D12ShaderBindingTableBuilder ShaderBindingTableBuilder;
    ID3D12DescriptorHeap* BindingTableHeaps[2] = { nullptr, nullptr };
};