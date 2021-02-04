#pragma once
#include "RenderLayer/RayTracing.h"

#include "D3D12DeviceChild.h"
#include "D3D12Buffer.h"
#include "D3D12Views.h"

#include "Core.h"

class D3D12CommandListHandle;
class Material;

class D3D12RayTracingGeometry : public RayTracingGeometry, public D3D12DeviceChild
{
public:
    D3D12RayTracingGeometry(D3D12Device* InDevice);
    ~D3D12RayTracingGeometry();

    bool BuildAccelerationStructure(
        D3D12CommandListHandle* CommandList, 
        TSharedRef<D3D12VertexBuffer>& InVertexBuffer, 
        UInt32 InVertexCount, 
        TSharedRef<D3D12IndexBuffer>& InIndexBuffer, 
        UInt32 InIndexCount);

    void SetName(const std::string& Name);

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

private:
    TSharedRef<D3D12VertexBuffer> VertexBuffer = nullptr;
    TSharedRef<D3D12IndexBuffer>  IndexBuffer  = nullptr;
    D3D12StructuredBuffer* ResultBuffer        = nullptr;
    D3D12StructuredBuffer* ScratchBuffer       = nullptr;
    
    UInt32 VertexCount = 0;
    UInt32 IndexCount  = 0;

    Bool IsDirty = true;
};

/*
* D3D12RayTracingGeometryInstance
*/

class D3D12RayTracingGeometryInstance
{
public:
    D3D12RayTracingGeometryInstance(TSharedPtr<D3D12RayTracingGeometry>& InGeometry, Material* InMaterial, XMFLOAT3X4 InTransform, UInt32 InHitGroupIndex, UInt32 InInstanceID)
        : Geometry(InGeometry)
        , Material(InMaterial)
        , Transform(InTransform)
        , HitGroupIndex(InHitGroupIndex)
        , InstanceID(InInstanceID)
    {
    }

    ~D3D12RayTracingGeometryInstance()
    {
    }

public:
    TSharedPtr<D3D12RayTracingGeometry> Geometry;
    Material* Material;

    XMFLOAT3X4 Transform;
    UInt32     HitGroupIndex;
    UInt32     InstanceID;
};

/*
* BindingTableEntry
*/

struct BindingTableEntry
{
public:
    BindingTableEntry()
        : ShaderExportName()
        //, DescriptorTable0(nullptr)
        //, DescriptorTable1(nullptr)
    {
    }

    //BindingTableEntry(std::string InShaderExportName, TSharedPtr<D3D12DescriptorTable> InDescriptorTable0, TSharedPtr<D3D12DescriptorTable> InDescriptorTable1)
    //    : ShaderExportName(InShaderExportName)
    //    , DescriptorTable0(InDescriptorTable0)
    //    , DescriptorTable1(InDescriptorTable1)
    //{
    //}

    std::string ShaderExportName;

    //TSharedPtr<D3D12DescriptorTable> DescriptorTable0;
    //TSharedPtr<D3D12DescriptorTable> DescriptorTable1;
};

/*
* D3D12RayTracingScene - Equal to the Top-Level AccelerationStructure
*/

class D3D12RayTracingScene : public D3D12DeviceChild
{
public:
    D3D12RayTracingScene(D3D12Device* InDevice);
    ~D3D12RayTracingScene();

    bool Initialize(class D3D12RayTracingPipelineState* PipelineState);

    bool BuildAccelerationStructure(D3D12CommandListHandle* CommandList,
        TArray<D3D12RayTracingGeometryInstance>& InInstances,
        TArray<BindingTableEntry>& InBindingTableEntries,
        UInt32 InNumHitGroups);

    void SetName(const std::string& Name);

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE            GetRayGenerationShaderRecord() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissShaderTable()           const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupTable()             const;
    
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

    FORCEINLINE D3D12ShaderResourceView* GetShaderResourceView() const
    {
        return View.Get();
    }

    FORCEINLINE bool NeedsBuild() const
    {
        return IsDirty;
    }

private:
    D3D12StructuredBuffer* ResultBuffer   = nullptr;
    D3D12StructuredBuffer* ScratchBuffer  = nullptr;
    D3D12StructuredBuffer* InstanceBuffer = nullptr;
    D3D12StructuredBuffer* BindingTable   = nullptr;
    UInt32 BindingTableStride = 0;
    UInt32 NumHitGroups       = 0;

    TSharedRef<D3D12ShaderResourceView> View;
    
    TArray<D3D12RayTracingGeometryInstance> Instances;
    TArray<BindingTableEntry>               BindingTableEntries;

    TComPtr<ID3D12StateObjectProperties> PipelineStateProperties;

    bool IsDirty = true;
};