#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Buffer.h"
#include "D3D12Views.h"

#include "RHI/RHIRayTracing.h"

#include "Core/Containers/Array.h"

class CD3D12CommandList;
class CMaterial;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SD3D12ShaderBindingTableEntry

struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) SD3D12ShaderBindingTableEntry
{
    char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
    D3D12_GPU_DESCRIPTOR_HANDLE	RootDescriptorTables[4] = { 0, 0, 0, 0 };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ShaderBindingTableBuilder

class CD3D12ShaderBindingTableBuilder : public CD3D12DeviceChild
{
public:
    CD3D12ShaderBindingTableBuilder(FD3D12Device* InDevice);
    ~CD3D12ShaderBindingTableBuilder() = default;

    void PopulateEntry( CD3D12RayTracingPipelineState* PipelineState
                      , CD3D12RootSignature* RootSignature
                      , CD3D12OnlineDescriptorHeap* ResourceHeap
                      , CD3D12OnlineDescriptorHeap* SamplerHeap
                      , SD3D12ShaderBindingTableEntry& OutShaderBindingEntry
                      , const SRayTracingShaderResources& Resources);

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12AccelerationStructure

class CD3D12AccelerationStructure : public CD3D12DeviceChild
{
public:

    CD3D12AccelerationStructure(FD3D12Device* InDevice);
    ~CD3D12AccelerationStructure() = default;
    
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        Check(ResultBuffer != nullptr);
        return ResultBuffer->GetGPUVirtualAddress();
    }

    CD3D12Resource* GetD3D12Resource() const { return ResultBuffer.Get(); }

    CD3D12Resource* GetD3D12ScratchBuffer() const { return ScratchBuffer.Get(); }

protected:
    D3D12ResourceRef ResultBuffer;
    D3D12ResourceRef ScratchBuffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayTracingGeometry

class CD3D12RayTracingGeometry : public CRHIRayTracingGeometry, public CD3D12AccelerationStructure
{
public:
    CD3D12RayTracingGeometry(FD3D12Device* InDevice, const CRHIRayTracingGeometryInitializer& Initializer);
    ~CD3D12RayTracingGeometry() = default;

    bool Build(class CD3D12CommandContext& CmdContext, CD3D12VertexBuffer* InVertexBuffer, CD3D12IndexBuffer* InIndexBuffer, bool bUpdate);

    CD3D12VertexBuffer* GetVertexBuffer() const { return VertexBuffer.Get(); }

    CD3D12IndexBuffer* GetIndexBuffer() const { return IndexBuffer.Get(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIRayTracingGeometry

    virtual void* GetRHIBaseBVHBuffer() override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(static_cast<CD3D12AccelerationStructure*>(this)); }

    virtual void SetName(const String& InName) override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

private:
    TSharedRef<CD3D12VertexBuffer> VertexBuffer;
    TSharedRef<CD3D12IndexBuffer>  IndexBuffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayTracingScene

class CD3D12RayTracingScene : public CRHIRayTracingScene, public CD3D12AccelerationStructure
{
public:

    CD3D12RayTracingScene(FD3D12Device* InDevice, const CRHIRayTracingSceneInitializer& Initializer);
    ~CD3D12RayTracingScene() = default;

    bool Build(class CD3D12CommandContext& CmdContext, const TArrayView<const CRHIRayTracingGeometryInstance>& InInstances, bool bUpdate);

    bool BuildBindingTable( class CD3D12CommandContext& CmdContext
                          , CD3D12RayTracingPipelineState* PipelineState
                          , CD3D12OnlineDescriptorHeap* ResourceHeap
                          , CD3D12OnlineDescriptorHeap* SamplerHeap
                          , const SRayTracingShaderResources* RayGenLocalResources
                          , const SRayTracingShaderResources* MissLocalResources
                          , const SRayTracingShaderResources* HitGroupResources
                          , uint32 NumHitGroupResources);


    D3D12_GPU_VIRTUAL_ADDRESS_RANGE            GetRayGenShaderRecord() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissShaderTable()    const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupTable()      const;

    CD3D12Resource* GetInstanceuffer() const { return InstanceBuffer.Get(); }

    CD3D12Resource* GetBindingTable()  const { return BindingTable.Get(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIRayTracingScene

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }

    virtual CRHIDescriptorHandle GetBindlessHandle() const override final { return CRHIDescriptorHandle(); }

    virtual void* GetRHIBaseBVHBuffer() override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(static_cast<CD3D12AccelerationStructure*>(this)); }

    virtual void SetName(const String& InName) override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

private:
    TArray<CRHIRayTracingGeometryInstance> Instances;

    TSharedRef<CD3D12ShaderResourceView>   View;

    D3D12ResourceRef                       InstanceBuffer;
    D3D12ResourceRef                       BindingTable;

    uint32 BindingTableStride = 0;
    uint32 NumHitGroups       = 0;

    // TODO: Maybe move these somewhere else
    CD3D12ShaderBindingTableBuilder ShaderBindingTableBuilder;
    ID3D12DescriptorHeap*           BindingTableHeaps[2] = { nullptr, nullptr };
};