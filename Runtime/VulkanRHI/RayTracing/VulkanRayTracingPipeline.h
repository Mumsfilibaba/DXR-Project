#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "RHI/RayTracing/RHIRayTracingPipelineState.h"
#include "VulkanRHI/VulkanPipelineState.h"

typedef TSharedRef<class FVulkanRayTracingPipelineStateRHI> FVulkanRayTracingPipelineStateRHIRef;

class FVulkanRayTracingPipelineStateRHI : public FRHIRayTracingPipelineState, public FVulkanPipeline
{
public:
    FVulkanRayTracingPipelineStateRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanRayTracingPipelineStateRHI();

    bool Initialize(const FRHIRayTracingPipelineStateDesc& InDesc);

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    // FRHIRayTracingPipelineState Interface
    virtual void GetExportName(ERayTracingShaderRecordKind Kind, uint32 RecordIndex, String& OutExportName) const override final;
    virtual uint32 GetNumExportNames(ERayTracingShaderRecordKind Kind) const override final;

    /** @return pointer to HandleSize bytes of the shader-group handle for the export, or nullptr. */
    const uint8* GetShaderGroupHandle(const String& ExportName) const;

    uint32 GetShaderGroupHandleSize() const
    {
        return HandleSize;
    }

    uint32 GetShaderGroupHandleAlignment() const
    {
        return HandleAlignment;
    }

    uint32 GetShaderGroupBaseAlignment() const
    {
        return BaseAlignment;
    }

    uint32 GetMaxShaderGroupStride() const
    {
        return MaxStride;
    }

private:
    TArray<String>& GetExportNameArray(ERayTracingShaderRecordKind Kind)
    {
        return ExportNames[uint32(Kind)];
    }

    const TArray<String>& GetExportNameArray(ERayTracingShaderRecordKind Kind) const
    {
        return ExportNames[uint32(Kind)];
    }

    TArray<uint8>  GroupHandleStorage;
    TArray<String> GroupNames;        
    TArray<String> ExportNames[4];
    uint32         HandleSize;
    uint32         HandleAlignment;
    uint32         BaseAlignment;
    uint32         MaxStride;
#if VULKAN_STORE_DEBUG_NAMES
    String         DebugName;
#endif
};
