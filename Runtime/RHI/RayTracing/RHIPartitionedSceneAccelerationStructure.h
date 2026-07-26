#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RayTracing/RHIAccelerationStructure.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHIBuffer;

struct FRHIRayTracingAccelerationStructurePartitionedSceneInputs
{
    uint32                           MaxInstanceCount  = 0;
    uint32                           MaxPartitionCount = 0;
    EAccelerationStructureBuildFlags Flags             = EAccelerationStructureBuildFlags::None;
};

struct FRHIRayTracingAccelerationStructurePartitionedSceneWriteInstanceArgs
{
    uint32                             InstanceIndex  = 0;
    uint32                             PartitionIndex = 0;
    FRHIGeometryAccelerationStructure* Geometry       = nullptr;
    Matrix3x4                          Transform      = { };
};

struct FRHIRayTracingAccelerationStructurePartitionedSceneUpdateInstanceArgs
{
    uint32    InstanceIndex = 0;
    Matrix3x4 Transform     = { };
};

struct FRHIRayTracingAccelerationStructurePartitionedSceneTranslatePartitionArgs
{
    uint32  PartitionIndex = 0;
    Vector3 Translation    = { };
};

class FRHIPartitionedSceneAccelerationStructure : public FRHIRayTracingAccelerationStructure
{
protected:
    explicit FRHIPartitionedSceneAccelerationStructure(EAccelerationStructureBuildFlags InFlags)
        : FRHIRayTracingAccelerationStructure(ERHIResourceType::SceneAccelerationStructure, ERayTracingAccelerationStructureType::PartitionedScene, InFlags)
    {
    }

    virtual ~FRHIPartitionedSceneAccelerationStructure() = default;

public:
    virtual FRHIShaderResourceView* GetShaderResourceView() const = 0;
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const = 0;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
