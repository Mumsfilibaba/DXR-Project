#pragma once
#include "RHI/RHIPipelineState.h"
#include "RHI/RayTracing/RHIRayTracingTypes.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHIRayTracingPipelineState;

enum class ERayTracingHitGroupType : uint8
{
    Unknown    = 0,
    Triangles  = 1,
    Procedural = 2
};

enum class ERayTracingPipelineFlags : uint8
{
    None                           = 0,
    AllowClusteredGeometry         = FLAG(1),
    AllowOpacityMicromap           = FLAG(2),
    AllowShaderExecutionReordering = FLAG(3),
    AllowInlineRayTracing          = FLAG(4),
    AllowStateObjectAdditions      = FLAG(5),
};

ENUM_CLASS_OPERATORS(ERayTracingPipelineFlags);

struct FRHIRayTracingHitGroupInfo
{
    FRHIRayTracingHitGroupInfo() noexcept = default;

    FRHIRayTracingHitGroupInfo(const String& InName, ERayTracingHitGroupType InType, TArrayView<FRHIRayTracingShader*> InRayTracingShaders) noexcept
        : Name(InName)
        , Shaders(InRayTracingShaders)
        , Type(InType)
    {
    }

    bool operator==(const FRHIRayTracingHitGroupInfo& Other) const noexcept = default;

    String                        Name;
    TArray<FRHIRayTracingShader*> Shaders;
    ERayTracingHitGroupType       Type = ERayTracingHitGroupType::Unknown;
};

struct FRHIRayTracingPipelineStateDesc
{
    FRHIRayTracingPipelineStateDesc() noexcept  = default;

    FRHIRayTracingPipelineStateDesc(const TArrayView<FRHIRayGenShader*>& InRayGenShaders, const TArrayView<FRHIRayCallableShader*>& InCallableShaders,
        const TArrayView<FRHIRayTracingHitGroupInfo>& InHitGroups, const TArrayView<FRHIRayMissShader*>& InMissShaders, uint32 InMaxAttributeSizeInBytes,
        uint32 InMaxPayloadSizeInBytes, uint32 InMaxRecursionDepth) noexcept
        : MaxAttributeSizeInBytes(InMaxAttributeSizeInBytes)
        , MaxPayloadSizeInBytes(InMaxPayloadSizeInBytes)
        , MaxRecursionDepth(InMaxRecursionDepth)
        , RayGenShaders(InRayGenShaders)
        , CallableShaders(InCallableShaders)
        , MissShaders(InMissShaders)
        , HitGroups(InHitGroups)
    {
    }

    bool operator==(const FRHIRayTracingPipelineStateDesc& Other) const noexcept = default;

    uint32                             MaxAttributeSizeInBytes       = 0;
    uint32                             MaxPayloadSizeInBytes         = 0;
    uint32                             MaxRecursionDepth             = 1;
    ERayTracingPipelineFlags           Flags                         = ERayTracingPipelineFlags::None;
    uint32                             MaxClusterTrianglesPerCluster = 0;
    uint32                             MaxClusterVerticesPerCluster  = 0;
    FRHIRayTracingPipelineState*       BasePipeline                  = nullptr;
    TArray<FRHIRayGenShader*>          RayGenShaders                 = {};
    TArray<FRHIRayCallableShader*>     CallableShaders               = {};
    TArray<FRHIRayMissShader*>         MissShaders                   = {};
    TArray<FRHIRayTracingHitGroupInfo> HitGroups                     = {};
};

class FRHIRayTracingPipelineState : public FRHIPipelineState
{
public:
    /** @brief Returns the flags the pipeline was created with. */
    ERayTracingPipelineFlags GetRayTracingPipelineFlags() const
    {
        return RayTracingPipelineFlags;
    }

    /** @brief Returns the backend export/group name for a sub-table record, or an empty string. */
    virtual void GetExportName(ERayTracingShaderRecordKind Kind, uint32 RecordIndex, String& OutExportName) const = 0;

    /** @brief Returns the number of registered export names for the given record kind. */
    virtual uint32 GetNumExportNames(ERayTracingShaderRecordKind Kind) const = 0;

protected:
    FRHIRayTracingPipelineState() = default;
    virtual ~FRHIRayTracingPipelineState() = default;

    ERayTracingPipelineFlags RayTracingPipelineFlags = ERayTracingPipelineFlags::None;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
