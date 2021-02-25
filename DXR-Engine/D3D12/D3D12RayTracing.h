#pragma once
#include "RenderLayer/RayTracing.h"

#include "D3D12DeviceChild.h"
#include "D3D12Buffer.h"
#include "D3D12Views.h"

class D3D12CommandListHandle;
class Material;

class D3D12RayTracingGeometry : public RayTracingGeometry, public D3D12DeviceChild
{
public:
    D3D12RayTracingGeometry(D3D12Device* InDevice, UInt32 InFlags);
    ~D3D12RayTracingGeometry() = default;

    Bool Build(class D3D12CommandContext& CmdContext, Bool Update);

    virtual void SetName(const std::string& InName) override
    {
        Resource::SetName(InName);
        ResultBuffer->SetName(InName);
    }

    virtual Bool IsValid() const override { return ResultBuffer != nullptr; }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const 
    { 
        Assert(ResultBuffer != nullptr);
        return ResultBuffer->GetGPUVirtualAddress();
    }

    TRef<D3D12VertexBuffer> VertexBuffer;
    TRef<D3D12IndexBuffer>  IndexBuffer;
    TRef<D3D12Resource>     ResultBuffer;
    TRef<D3D12Resource>     ScratchBuffer;
};

struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) D3D12ShaderBindingTableEntry
{
    Char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
    D3D12_GPU_DESCRIPTOR_HANDLE	RootDescriptorTables[4] = { 0, 0, 0, 0 };
};

class D3D12ShaderBindingTableBuilder : public D3D12DeviceChild
{
public:
    D3D12ShaderBindingTableBuilder(D3D12Device* InDevice);
    ~D3D12ShaderBindingTableBuilder() = default;

    void PopulateEntry(
        D3D12RayTracingPipelineState* PipelineState,
        D3D12RootSignature* RootSignature,
        D3D12OnlineDescriptorHeap* ResourceHeap,
        D3D12OnlineDescriptorHeap* SamplerHeap,
        D3D12ShaderBindingTableEntry& OutShaderBindingEntry, 
        const RayTracingShaderResources& Resources);

    void CopyDescriptors();

    void Reset();

private:
    UInt32 CPUHandleSizes[1024];
    D3D12_CPU_DESCRIPTOR_HANDLE ResourceHandles[1024];
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerHandles[1024];
    // Online resources
    D3D12_CPU_DESCRIPTOR_HANDLE GPUResourceHandles[1024];
    D3D12_CPU_DESCRIPTOR_HANDLE GPUSamplerHandles[1024];
    UInt32 GPUResourceHandleSizes[1024];
    UInt32 GPUSamplerHandleSizes[1024];
    UInt32 CPUResourceIndex = 0;
    UInt32 CPUSamplerIndex  = 0;
    UInt32 GPUResourceIndex = 0;
    UInt32 GPUSamplerIndex  = 0;
};

class D3D12RayTracingScene : public RayTracingScene, public D3D12DeviceChild
{
public:
    D3D12RayTracingScene(D3D12Device* InDevice, UInt32 InFlags);
    ~D3D12RayTracingScene() = default;

    Bool Build(class D3D12CommandContext& CmdContext, const RayTracingGeometryInstance* Instances, UInt32 NumInstances, Bool Update);
    
    Bool BuildBindingTable(
        class D3D12CommandContext& CmdContext, 
        D3D12RayTracingPipelineState* PipelineState,
        D3D12OnlineDescriptorHeap* ResourceHeap,
        D3D12OnlineDescriptorHeap* SamplerHeap,
        const RayTracingShaderResources* RayGenLocalResources,
        const RayTracingShaderResources* MissLocalResources,
        const RayTracingShaderResources* HitGroupResources,
        UInt32 NumHitGroupResources);

    virtual void SetName(const std::string& InName) override;

    virtual Bool IsValid() const override { return ResultBuffer != nullptr; }

    virtual ShaderResourceView* GetShaderResourceView() const { return View.Get(); }

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE            GetRayGenShaderRecord() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissShaderTable()    const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupTable()      const;
    
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        Assert(ResultBuffer != nullptr);
        return ResultBuffer->GetGPUVirtualAddress();
    }
    
    D3D12Resource* GetResultBuffer()  const { return ResultBuffer.Get(); }
    D3D12Resource* GetInstanceuffer() const { return InstanceBuffer.Get(); }
    D3D12Resource* GetBindingTable()  const { return BindingTable.Get(); }

private:
    TArray<RayTracingGeometryInstance> Instances;
    TRef<D3D12ShaderResourceView>      View;
    
    TRef<D3D12Resource> ResultBuffer;
    TRef<D3D12Resource> ScratchBuffer;
    TRef<D3D12Resource> InstanceBuffer;
    TRef<D3D12Resource> BindingTable;

    UInt32 BindingTableStride = 0;
    UInt32 NumHitGroups       = 0;

    // TODO: Maybe move these somewere else
    D3D12ShaderBindingTableBuilder ShaderBindingTableBuilder;
    ID3D12DescriptorHeap* BindingTableHeaps[2] = { nullptr, nullptr };
};