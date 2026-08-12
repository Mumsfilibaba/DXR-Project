#pragma once
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"
#include "RHI/RayTracing/RHIRayTracingTypes.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHISceneAccelerationStructure;
class FRHIGeometryAccelerationStructure;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHIBuffer;
class FRHISamplerState;

typedef TSharedRef<FRHISceneAccelerationStructure>    FRHISceneAccelerationStructureRef;
typedef TSharedRef<FRHIGeometryAccelerationStructure> FRHIGeometryAccelerationStructureRef;

struct FRHIGeometryAccelerationStructureDesc
{
    constexpr FRHIGeometryAccelerationStructureDesc() noexcept = default;

    constexpr FRHIGeometryAccelerationStructureDesc(
        FRHIBuffer*                      InVertexBuffer,
        uint32                           InNumVerticies,
        FRHIBuffer*                      InIndexBuffer,
        uint32                           InNumIndices,
        EIndexFormat                     InIndexFormat,
        EAccelerationStructureBuildFlags InFlags) noexcept
        : VertexBuffer(InVertexBuffer)
        , NumVertices(InNumVerticies)
        , IndexBuffer(InIndexBuffer)
        , NumIndices(InNumIndices)
        , IndexFormat(InIndexFormat)
        , Flags(InFlags)
    {
    }

    NODISCARD constexpr bool PreferFastTrace()     const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastTrace); }
    NODISCARD constexpr bool PreferFastBuild()     const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastBuild); }
    NODISCARD constexpr bool AllowUpdate()         const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowUpdate); }
    NODISCARD constexpr bool AllowCompaction()     const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowCompaction); }
    NODISCARD constexpr bool IsClusteredGeometry() const noexcept { return bIsClusteredGeometry; }

    constexpr bool operator==(const FRHIGeometryAccelerationStructureDesc& Other) const noexcept = default;

    FRHIBuffer*                      VertexBuffer         = nullptr;
    uint32                           NumVertices          = 0;
    FRHIBuffer*                      IndexBuffer          = nullptr;
    uint32                           NumIndices           = 0;
    EIndexFormat                     IndexFormat          = EIndexFormat::Unknown;
    EAccelerationStructureBuildFlags Flags                = EAccelerationStructureBuildFlags::None;
    ERayTracingGeometryType          GeometryType         = ERayTracingGeometryType::Triangles;
    bool                             bIsClusteredGeometry = false;
};

struct FRHISceneAccelerationStructureDesc
{
    FRHISceneAccelerationStructureDesc() noexcept = default;

    FRHISceneAccelerationStructureDesc(
        const TArrayView<const FRHIGeometryAccelerationStructureInstance>& InInstances,
        EAccelerationStructureBuildFlags                                   InFlags) noexcept
        : Instances(InInstances)
        , Flags(InFlags)
    {
    }

    NODISCARD constexpr bool PreferFastTrace() const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastTrace); }
    NODISCARD constexpr bool PreferFastBuild() const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastBuild); }
    NODISCARD constexpr bool AllowUpdate()     const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowUpdate); }
    NODISCARD constexpr bool AllowCompaction() const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowCompaction); }
    NODISCARD constexpr bool IsPartitioned()   const noexcept { return bIsPartitioned; }

    bool operator==(const FRHISceneAccelerationStructureDesc& Other) const noexcept = default;

    TArray<FRHIGeometryAccelerationStructureInstance> Instances      = { };
    EAccelerationStructureBuildFlags                  Flags          = EAccelerationStructureBuildFlags::None;
    bool                                              bIsPartitioned = false;
};

class FRHIRayTracingAccelerationStructure : public FRHIResource
{
protected:
    FRHIRayTracingAccelerationStructure(ERHIResourceType InResourceType, ERayTracingAccelerationStructureType InAccelerationStructureType, EAccelerationStructureBuildFlags InFlags)
        : FRHIResource(InResourceType)
        , AccelerationStructureType(InAccelerationStructureType)
        , Flags(InFlags)
    {
    }

    virtual ~FRHIRayTracingAccelerationStructure() = default;

public:

    /** @return D3D12: ID3D12Resource*. Vulkan: VkAccelerationStructureKHR or VkBuffer. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeResource() const = 0;

    virtual void SetDebugName(const String& InName) = 0;
    virtual void GetDebugName(String& OutDebugName) const = 0;

    NODISCARD EAccelerationStructureBuildFlags GetFlags() const
    {
        return Flags;
    }

    NODISCARD ERayTracingAccelerationStructureType GetAccelerationStructureType() const
    {
        return AccelerationStructureType;
    }

protected:
    ERayTracingAccelerationStructureType AccelerationStructureType;
    EAccelerationStructureBuildFlags     Flags;
};

class FRHIGeometryAccelerationStructure : public FRHIRayTracingAccelerationStructure
{
protected:
    explicit FRHIGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
        : FRHIRayTracingAccelerationStructure(ERHIResourceType::GeometryAccelerationStructure, ERayTracingAccelerationStructureType::Geometry, InGeometryDesc.Flags)
        , GeometryType(InGeometryDesc.GeometryType)
    {
    }

    virtual ~FRHIGeometryAccelerationStructure() = default;

public:
    NODISCARD ERayTracingGeometryType GetGeometryType() const
    {
        return GeometryType;
    }

protected:
    ERayTracingGeometryType GeometryType;
};

class FRHISceneAccelerationStructure : public FRHIRayTracingAccelerationStructure
{
protected:
    explicit FRHISceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc)
        : FRHIRayTracingAccelerationStructure(ERHIResourceType::SceneAccelerationStructure, ERayTracingAccelerationStructureType::Scene, InSceneDesc.Flags)
    {
    }

    virtual ~FRHISceneAccelerationStructure() = default;

public:
    virtual FRHIShaderResourceView* GetShaderResourceView() const = 0;
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const = 0;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
