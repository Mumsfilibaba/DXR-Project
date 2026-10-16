#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "RHI/RHIRayTracing.h"
#include "RHI/RayTracing/RHIShaderBindingTable.h"
#include "VulkanRHI/VulkanMemoryManager.h"
#include "VulkanRHI/RayTracing/VulkanRayTracingPipeline.h"

typedef TSharedRef<class FVulkanShaderBindingTableRHI> FVulkanShaderBindingTableRHIRef;

class FVulkanShaderBindingTableRHI : public FRHIShaderBindingTable, public FVulkanDeviceChild
{
public:
    FVulkanShaderBindingTableRHI(FVulkanDevice* InDevice, const FRHIShaderBindingTableDesc& InDesc);
    virtual ~FVulkanShaderBindingTableRHI();

    // FRHIShaderBindingTable Interface
    virtual void* GetRHINativeResource() const override final;
    virtual FRHIShaderBindingTableAddressInfo GetAddressInfo() const override final;

    bool Initialize();
    void SetRecord(const FRHIShaderRecordDesc& RecordDesc);
    void Build(class FVulkanCommandContext& CmdContext);
    void ClearTableRecords();

    VkStridedDeviceAddressRegionKHR GetRayGenRegion()   const;
    VkStridedDeviceAddressRegionKHR GetMissRegion()     const;
    VkStridedDeviceAddressRegionKHR GetHitGroupRegion() const;
    VkStridedDeviceAddressRegionKHR GetCallableRegion() const;

    FVulkanRayTracingPipelineStateRHI* GetPipeline() const
    {
        return Pipeline.Get();
    }

private:
    uint64 GetRegionBaseOffset(ERayTracingShaderRecordKind RecordKind) const;
    void   PopulateRecord(uint8* OutRecord, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, uint32 ShaderIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings);

    FVulkanRayTracingPipelineStateRHIRef Pipeline;
    uint32                               NumRayGen;
    uint32                               NumMiss;
    uint32                               NumCallable;
    uint32                               NumHitGroup;
    uint32                               HandleSize;
    uint32                               RecordStride;
    uint64                               RayGenOffset;
    uint64                               MissOffset;
    uint64                               HitGroupOffset;
    uint64                               CallableOffset;
    TArray<uint8>                        CpuShadow;
    FVulkanMemoryLocation                TableLocation;
};
