#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "RHI/RHITypes.h"

#define RHI_DEFAULT_GEOMETRY_INSTANCE_MASK (0xff)

class FRHIGeometryAccelerationStructure;

enum class ERHIType : uint32;

struct FRayPayload
{
    Vector3 Color;
    float   HitT;
};

struct FRayIntersectionAttributes
{
    float Attrib0;
    float Attrib1;
};

// -------------------------------------------------------------------------------------------
// Acceleration-structure build flags
// -------------------------------------------------------------------------------------------

enum class EAccelerationStructureBuildFlags : uint8
{
    None            = 0,
    AllowUpdate     = FLAG(1),
    PreferFastTrace = FLAG(2),
    PreferFastBuild = FLAG(3),
    AllowCompaction = FLAG(4),

    /** Hint for streaming / scratch-tight scenarios */
    MinimizeMemory = FLAG(5),
};

ENUM_CLASS_OPERATORS(EAccelerationStructureBuildFlags);

// -------------------------------------------------------------------------------------------
// Ray-tracing instance flags
// -------------------------------------------------------------------------------------------

enum class ERayTracingInstanceFlags : uint8
{
    None                  = 0,
    CullDisable           = FLAG(1),
    FrontCounterClockwise = FLAG(2),
    ForceOpaque           = FLAG(3),
    ForceNonOpaque        = FLAG(4),
};

ENUM_CLASS_OPERATORS(ERayTracingInstanceFlags);

// -------------------------------------------------------------------------------------------
// Shader-binding-table record kind
// -------------------------------------------------------------------------------------------

enum class ERayTracingShaderRecordKind : uint8
{
    RayGeneration = 0,
    Miss          = 1,
    Callable      = 2,
    HitGroup      = 3,
};

NODISCARD constexpr const CHAR* ToString(ERayTracingShaderRecordKind Kind)
{
    switch (Kind)
    {
        case ERayTracingShaderRecordKind::RayGeneration: return "RayGeneration";
        case ERayTracingShaderRecordKind::Miss:          return "Miss";
        case ERayTracingShaderRecordKind::Callable:      return "Callable";
        case ERayTracingShaderRecordKind::HitGroup:      return "HitGroup";
        default:                                         return "Unknown";
    }
}

// -------------------------------------------------------------------------------------------
// Acceleration-structure kind
// -------------------------------------------------------------------------------------------

enum class ERayTracingAccelerationStructureType : uint8
{
    Geometry         = 0,
    Scene            = 1,
    Cluster          = 2,
    PartitionedScene = 3,
};

NODISCARD constexpr const CHAR* ToString(ERayTracingAccelerationStructureType Type)
{
    switch (Type)
    {
        case ERayTracingAccelerationStructureType::Geometry:         return "Geometry";
        case ERayTracingAccelerationStructureType::Scene:            return "Scene";
        case ERayTracingAccelerationStructureType::Cluster:          return "Cluster";
        case ERayTracingAccelerationStructureType::PartitionedScene: return "PartitionedScene";
        default:                                                     return "Unknown";
    }
}

// -------------------------------------------------------------------------------------------
// Post-build info & copy modes (compaction / serialization / tools)
// -------------------------------------------------------------------------------------------

enum class EAccelerationStructurePostBuildInfoType : uint8
{
    CompactedSize      = 0,
    CurrentSize        = 1,
    Serialization      = 2,
    ToolsVisualization = 3,
};

NODISCARD constexpr const CHAR* ToString(EAccelerationStructurePostBuildInfoType Type)
{
    switch (Type)
    {
        case EAccelerationStructurePostBuildInfoType::CompactedSize:      return "CompactedSize";
        case EAccelerationStructurePostBuildInfoType::CurrentSize:        return "CurrentSize";
        case EAccelerationStructurePostBuildInfoType::Serialization:      return "Serialization";
        case EAccelerationStructurePostBuildInfoType::ToolsVisualization: return "ToolsVisualization";
        default:                                                          return "Unknown";
    }
}

enum class EAccelerationStructureCopyMode : uint8
{
    Clone                    = 0,
    Compact                  = 1,
    Serialize                = 2,
    Deserialize              = 3,
    ToolsVisualizationDecode = 4,
};

NODISCARD constexpr const CHAR* ToString(EAccelerationStructureCopyMode Mode)
{
    switch (Mode)
    {
        case EAccelerationStructureCopyMode::Clone:                    return "Clone";
        case EAccelerationStructureCopyMode::Compact:                  return "Compact";
        case EAccelerationStructureCopyMode::Serialize:                return "Serialize";
        case EAccelerationStructureCopyMode::Deserialize:              return "Deserialize";
        case EAccelerationStructureCopyMode::ToolsVisualizationDecode: return "ToolsVisualizationDecode";
        default:                                                       return "Unknown";
    }
}

// -------------------------------------------------------------------------------------------
// Geometry-acceleration-structure instance
// -------------------------------------------------------------------------------------------

struct FRHIGeometryAccelerationStructureInstance
{
    FRHIGeometryAccelerationStructureInstance() noexcept = default;

    FRHIGeometryAccelerationStructureInstance(FRHIGeometryAccelerationStructure* InGeometry, uint32 InInstanceIndex, uint32 InHitGroupIndex,
        ERayTracingInstanceFlags InFlags, uint32 InMask, const Matrix3x4& InTransform) noexcept
        : Geometry(InGeometry)
        , InstanceIndex(InInstanceIndex)
        , HitGroupIndex(InHitGroupIndex)
        , Flags(InFlags)
        , Mask(InMask)
        , Transform(InTransform)
    {
    }

    bool operator==(const FRHIGeometryAccelerationStructureInstance& Other) const noexcept = default;

    FRHIGeometryAccelerationStructure* Geometry      = nullptr;
    uint32                             InstanceIndex = 0;
    uint32                             HitGroupIndex = 0;
    ERayTracingInstanceFlags           Flags         = ERayTracingInstanceFlags::None;
    uint32                             Mask          = RHI_DEFAULT_GEOMETRY_INSTANCE_MASK;
    Matrix3x4                          Transform     = { };
};

// -------------------------------------------------------------------------------------------
// Serialization
// -------------------------------------------------------------------------------------------

struct FRHIAccelerationStructureSerializationInfo
{
    uint64 SerializedSizeInBytes   = 0;
    uint64 DeserializedSizeInBytes = 0;
};

struct FRHIAccelerationStructureSerializationHeader
{
    // 32 bytes covers all backends' driver-match payloads:
    //  - D3D12 D3D12_SERIALIZED_DATA_DRIVER_MATCHING_IDENTIFIER (GUID[16] + BYTE[16])
    //  - Vulkan VkAccelerationStructureVersionInfoKHR::pVersionData (2 * VK_UUID_SIZE = 32)
    static constexpr uint32 DRIVER_MATCHING_IDENTIFIER_SIZE = 32;

    // Bump VERSION whenever the on-disk layout changes so stale caches are rejected up front.
    static constexpr uint32 MAGIC             = 0x53415452u;
    static constexpr uint32 VERSION           = 1u;
    static constexpr uint32 MAGIC_AND_VERSION = MAGIC ^ (VERSION << 24);

    uint32                               MagicAndVersion           = 0;
    ERHIType                             RHIType                   = {};
    ERayTracingAccelerationStructureType AccelerationStructureType = ERayTracingAccelerationStructureType::Geometry;
    uint64                               SerializedSizeInBytes     = 0;
    uint64                               DeserializedSizeInBytes   = 0;

    uint8 DriverMatchingIdentifier[DRIVER_MATCHING_IDENTIFIER_SIZE] = { };

    /** @return true if the magic/version match this build (cheap pre-check before the driver-match call). */
    NODISCARD bool HasValidMagic() const
    {
        return MagicAndVersion == MAGIC_AND_VERSION;
    }
};
