#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Buffer.h"
#include "D3D12ResourceViews.h"

#include "RHI/RHIRayTracing.h"

#include "Core/Containers/Array.h"

class FD3D12CommandList;
class CMaterial;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ShaderBindingTableEntry

struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) FD3D12ShaderBindingTableEntry
{
    char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
    D3D12_GPU_DESCRIPTOR_HANDLE	RootDescriptorTables[4] = { 0, 0, 0, 0 };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ShaderBindingTableBuilder

class FD3D12ShaderBindingTableBuilder : public FD3D12DeviceChild
{
public:
    FD3D12ShaderBindingTableBuilder(FD3D12Device* InDevice);
    ~FD3D12ShaderBindingTableBuilder() = default;

    void PopulateEntry( FD3D12RayTracingPipelineState* PipelineState
                      , FD3D12RootSignature* RootSignature
                      , FD3D12OnlineDescriptorHeap* ResourceHeap
                      , FD3D12OnlineDescriptorHeap* SamplerHeap
                      , FD3D12ShaderBindingTableEntry& OutShaderBindingEntry
                      , const FRayTracingShaderResources& Resources);

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
// FD3D12AccelerationStructure

class FD3D12AccelerationStructure : public FD3D12DeviceChild
{
public:

    FD3D12AccelerationStructure(FD3D12Device* InDevice);
    ~FD3D12AccelerationStructure() = default;
    
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        Check(ResultBuffer != nullptr);
        return ResultBuffer->GetGPUVirtualAddress();
    }

    FD3D12Resource* GetD3D12Resource() const { return ResultBuffer.Get(); }

    FD3D12Resource* GetD3D12ScratchBuffer() const { return ScratchBuffer.Get(); }

protected:
    FD3D12ResourceRef ResultBuffer;
    FD3D12ResourceRef ScratchBuffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RayTracingGeometry

class FD3D12RayTracingGeometry : public FRHIRayTracingGeometry, public FD3D12AccelerationStructure
{
public:
    FD3D12RayTracingGeometry(FD3D12Device* InDevice, const FRHIRayTracingGeometryInitializer& Initializer);
    ~FD3D12RayTracingGeometry() = default;

    bool Build(class FD3D12CommandContext& CmdContext, FD3D12VertexBuffer* InVertexBuffer, FD3D12IndexBuffer* InIndexBuffer, bool bUpdate);

    FD3D12VertexBuffer* GetVertexBuffer() const { return VertexBuffer.Get(); }

    FD3D12IndexBuffer* GetIndexBuffer() const { return IndexBuffer.Get(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIRayTracingGeometry

    virtual void* GetRHIBaseBVHBuffer() override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(static_cast<FD3D12AccelerationStructure*>(this)); }

    virtual void SetName(const FString& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

private:
    TSharedRef<FD3D12VertexBuffer> VertexBuffer;
    TSharedRef<FD3D12IndexBuffer>  IndexBuffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RayTracingScene

class FD3D12RayTracingScene : public FRHIRayTracingScene, public FD3D12AccelerationStructure
{
public:

    FD3D12RayTracingScene(FD3D12Device* InDevice, const FRHIRayTracingSceneInitializer& Initializer);
    ~FD3D12RayTracingScene() = default;

    bool Build(class FD3D12CommandContext& CmdContext, const TArrayView<const FRHIRayTracingGeometryInstance>& InInstances, bool bUpdate);

    bool BuildBindingTable( class FD3D12CommandContext& CmdContext
                          , FD3D12RayTracingPipelineState* PipelineState
                          , FD3D12OnlineDescriptorHeap* ResourceHeap
                          , FD3D12OnlineDescriptorHeap* SamplerHeap
                          , const FRayTracingShaderResources* RayGenLocalResources
                          , const FRayTracingShaderResources* MissLocalResources
                          , const FRayTracingShaderResources* HitGroupResources
                          , uint32 NumHitGroupResources);


    D3D12_GPU_VIRTUAL_ADDRESS_RANGE            GetRayGenShaderRecord() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissShaderTable()    const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupTable()      const;

    FD3D12Resource* GetInstanceuffer() const { return InstanceBuffer.Get(); }

    FD3D12Resource* GetBindingTable()  const { return BindingTable.Get(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIRayTracingScene

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void* GetRHIBaseBVHBuffer() override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(static_cast<FD3D12AccelerationStructure*>(this)); }

    virtual void SetName(const FString& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

private:
    TArray<FRHIRayTracingGeometryInstance> Instances;

    TSharedRef<FD3D12ShaderResourceView>   View;

    FD3D12ResourceRef                       InstanceBuffer;
    FD3D12ResourceRef                       BindingTable;

    uint32 BindingTableStride = 0;
    uint32 NumHitGroups       = 0;

    // TODO: Maybe move these somewhere else
    FD3D12ShaderBindingTableBuilder ShaderBindingTableBuilder;
    ID3D12DescriptorHeap*           BindingTableHeaps[2] = { nullptr, nullptr };
};