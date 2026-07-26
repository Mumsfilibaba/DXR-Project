#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RayTracing/RHIAccelerationStructure.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHIBuffer;

struct FRHIRayTracingAccelerationStructureClusterLimits
{
    uint32 MaxTrianglesPerCluster = 0;
    uint32 MaxVerticesPerCluster  = 0;
    uint32 MaxGeometryIndex       = 0;
    uint32 MaxClusterCount        = 0;
};

enum class ERayTracingAccelerationStructureOperationType : uint8
{
    BuildClusterAccelerationStructureFromTriangles  = 0,
    BuildClusterTemplatesFromTriangles              = 1,
    InstantiateClusterTemplates                     = 2,
    BuildGeometryAccelerationStructureFromClusters  = 3,
    MoveClusterObjects                              = 4,
    PartitionedSceneAccelerationStructure           = 5,
};

enum class ERayTracingAccelerationStructureOperationFlags : uint8
{
    None         = 0,
    PerformBuild = FLAG(1),
};

ENUM_CLASS_OPERATORS(ERayTracingAccelerationStructureOperationFlags);

struct FRHIRayTracingAccelerationStructureOperationInputs
{
    ERayTracingAccelerationStructureOperationType    OperationType    = ERayTracingAccelerationStructureOperationType::BuildClusterAccelerationStructureFromTriangles;
    uint32                                           MaxArgumentCount = 0;
    FRHIRayTracingAccelerationStructureClusterLimits ClusterLimits    = { };
};

struct FRHIRayTracingAccelerationStructurePrebuildInfo
{
    uint64 ResultSizeInBytes        = 0;
    uint64 ScratchSizeInBytes       = 0;
    uint64 UpdateScratchSizeInBytes = 0;
};

struct FRHIRayTracingAccelerationStructureOperationDesc
{
    ERayTracingAccelerationStructureOperationType    OperationType        = ERayTracingAccelerationStructureOperationType::BuildClusterAccelerationStructureFromTriangles;
    FRHIBuffer*                                      ArgumentBuffer       = nullptr;
    uint64                                           ArgumentBufferOffset = 0;
    uint32                                           ArgumentCount        = 0;
    uint32                                           ArgumentStride       = 0;
    FRHIBuffer*                                      DestinationBuffer    = nullptr;
    uint64                                           DestinationOffset    = 0;
    FRHIBuffer*                                      ScratchBuffer        = nullptr;
    uint64                                           ScratchOffset        = 0;
    FRHIRayTracingAccelerationStructureClusterLimits ClusterLimits        = { };
};

struct FRHIClusterAccelerationStructureDesc
{
    EAccelerationStructureBuildFlags                 Flags         = EAccelerationStructureBuildFlags::None;
    FRHIRayTracingAccelerationStructureClusterLimits ClusterLimits = { };
};

struct FRHIClusterTemplateDesc
{
    EAccelerationStructureBuildFlags                 Flags         = EAccelerationStructureBuildFlags::None;
    FRHIRayTracingAccelerationStructureClusterLimits ClusterLimits = { };
};

class FRHIClusterAccelerationStructure : public FRHIRayTracingAccelerationStructure
{
protected:
    explicit FRHIClusterAccelerationStructure(const FRHIClusterAccelerationStructureDesc& InDesc)
        : FRHIRayTracingAccelerationStructure(ERHIResourceType::ClusterAccelerationStructure, ERayTracingAccelerationStructureType::Cluster, InDesc.Flags)
    {
    }

    virtual ~FRHIClusterAccelerationStructure() = default;
};

class FRHIClusterTemplate : public FRHIResource
{
protected:
    explicit FRHIClusterTemplate(const FRHIClusterTemplateDesc& InDesc)
        : FRHIResource(ERHIResourceType::ClusterTemplate)
    {
        UNREFERENCED_VARIABLE(InDesc);
    }

    virtual ~FRHIClusterTemplate() = default;

public:
    virtual void* GetRHINativeResource() const = 0;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
