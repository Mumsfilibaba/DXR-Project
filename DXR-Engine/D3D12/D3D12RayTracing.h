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
    Byte ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
    D3D12_GPU_DESCRIPTOR_HANDLE	ConstantBufferTable      = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE	ShaderResourceViewTable  = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE	UnorderedAccessViewTable = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE	SamplerStateTable        = { 0 };
};

class D3D12RayTracingScene : public RayTracingScene, public D3D12DeviceChild
{
public:
    D3D12RayTracingScene(D3D12Device* InDevice, UInt32 InFlags);
    ~D3D12RayTracingScene() = default;

    Bool Build(class D3D12CommandContext& CmdContext, TArrayView<RayTracingGeometryInstance> InInstances, Bool Update);

    virtual void SetName(const std::string& InName) override
    {
        Resource::SetName(InName);
        ResultBuffer->SetName(InName);
    }

    virtual Bool IsValid() const override { return ResultBuffer != nullptr; }

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE            GetRayGenerationShaderRecord() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissShaderTable()           const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupTable()             const;
    
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        Assert(ResultBuffer != nullptr);
        return ResultBuffer->GetGPUVirtualAddress();
    }

    D3D12ShaderResourceView* GetShaderResourceView() const { return ShaderResourceView.Get(); }

private:
    TArray<RayTracingGeometryInstance> Instances;
    TRef<D3D12ShaderResourceView>      ShaderResourceView;
    
    TRef<D3D12Resource> ResultBuffer;
    TRef<D3D12Resource> ScratchBuffer;
    TRef<D3D12Resource> InstanceBuffer;
    TRef<D3D12Resource> BindingTable;

    UInt32 BindingTableStride = 0;
    UInt32 NumHitGroups       = 0;
};