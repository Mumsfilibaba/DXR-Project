#pragma once
#include "RenderLayer/RayTracing.h"
#include "RenderLayer/GeometryInstance.h"

#include "D3D12DeviceChild.h"
#include "D3D12Buffer.h"
#include "D3D12Views.h"

#include "Core.h"

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
    TRef<D3D12Resource> ResultBuffer;
    TRef<D3D12Resource> ScratchBuffer;
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

    D3D12ShaderResourceView* GetShaderResourceView() const { return View.Get(); }

private:
    TArray<RayTracingGeometryInstance>  Instances;
    TRef<D3D12ShaderResourceView> View;
    TRef<D3D12Resource> ResultBuffer;
    TRef<D3D12Resource> ScratchBuffer;
    TRef<D3D12Resource> InstanceBuffer;
    TRef<D3D12Resource> BindingTable;
    UInt32 BindingTableStride = 0;
    UInt32 NumHitGroups       = 0;
};