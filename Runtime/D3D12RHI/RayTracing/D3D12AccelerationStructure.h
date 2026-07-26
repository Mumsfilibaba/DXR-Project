#pragma once
#include "Core/Containers/Array.h"
#include "RHI/RHIRayTracing.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12PipelineState.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/RayTracing/D3D12ShaderBindingTable.h"

class FD3D12CommandList;
class FD3D12CommandContext;
class FD3D12OnlineDescriptorHeap;

typedef TSharedRef<class FD3D12OpacityMicromapRHI>               FD3D12OpacityMicromapRHIRef;
typedef TSharedRef<class FD3D12GeometryAccelerationStructureRHI> FD3D12GeometryAccelerationStructureRHIRef;
typedef TSharedRef<class FD3D12SceneAccelerationStructureRHI>    FD3D12SceneAccelerationStructureRHIRef;

class FD3D12AccelerationStructure : public FD3D12DeviceChild
{
public:
    FD3D12AccelerationStructure(FD3D12Device* InDevice);
    virtual ~FD3D12AccelerationStructure();
    
    bool CompactInPlace(FD3D12CommandContext& CmdContext, uint64 CompactedSize);

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
    void UpdateAccelerationStructureMemoryStat();

    FD3D12ResourceStorage ResultResourceStorage;
    FD3D12ResourceStorage ScratchResourceStorage;
    uint64                TrackedASMemory;
};

class FD3D12GeometryAccelerationStructureRHI : public FRHIGeometryAccelerationStructure, public FD3D12AccelerationStructure
{
public:
    FD3D12GeometryAccelerationStructureRHI(FD3D12Device* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc);
    virtual ~FD3D12GeometryAccelerationStructureRHI();
    
    // FRHIGeometryAccelerationStructure Interface
    virtual void* GetRHINativeResource() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;
    
    bool Build(FD3D12CommandContext& CmdContext, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc);
    
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

#if D3D12_ENABLE_OPACITY_MICROMAPS
class FD3D12OpacityMicromapRHI : public FRHIOpacityMicromap, public FD3D12AccelerationStructure
{
public:
    FD3D12OpacityMicromapRHI(FD3D12Device* InDevice, const FRHIOpacityMicromapDesc& InDesc);
    virtual ~FD3D12OpacityMicromapRHI();

    // FRHIOpacityMicromap Interface
    virtual void* GetRHINativeResource() const override final;

    bool Build(FD3D12CommandContext& CmdContext, const FRHIOpacityMicromapBuildDesc& BuildDesc);

private:
    EOpacityMicromapFormat Format;
    uint32                 SubdivisionLevel;
};
#endif

class FD3D12SceneAccelerationStructureRHI : public FRHISceneAccelerationStructure , public FD3D12AccelerationStructure
{
public:
    FD3D12SceneAccelerationStructureRHI(FD3D12Device* InDevice, const FRHISceneAccelerationStructureDesc& InSceneDesc);
    virtual ~FD3D12SceneAccelerationStructureRHI();
    
    // FRHISceneAccelerationStructure Interface
    virtual void* GetRHINativeResource() const override final;

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final;
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Build(FD3D12CommandContext& CmdContext, const FRHISceneAccelerationStructureBuildDesc& BuildDesc);

    FD3D12Resource* GetInstanceBuffer() const
    {
        return InstanceBuffer.Get();
    }

private:
    FD3D12ShaderResourceViewRHIRef                    View;
    FD3D12ResourceRef                                 InstanceBuffer;
    TArray<FRHIGeometryAccelerationStructureInstance> Instances;
};
