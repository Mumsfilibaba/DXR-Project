#pragma once
#include "Core/Containers/Function.h"
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

typedef TSharedRef<class FRHIRayTracingAccelerationStructure> FRHIRayTracingAccelerationStructureRef;
typedef TSharedRef<FRHISceneAccelerationStructure>            FRHISceneAccelerationStructureRef;
typedef TSharedRef<FRHIGeometryAccelerationStructure>         FRHIGeometryAccelerationStructureRef;

struct FRHIGeometryAccelerationStructureDesc
{
    /**
     * @brief Describe a geometry structure deserialized from data SerializeAccelerationStructure wrote for a geometry structure on this driver
     * @param InOpacityMicromaps The micromaps the data links, one for each micromap GetSerializedOpacityMicromaps returned for it: those micromaps or
     * deserialized copies of them
     */
    NODISCARD static FORCEINLINE FRHIGeometryAccelerationStructureDesc CreateSerialized(
        const void*                                   InSerializedData,
        uint64                                        InSerializedSizeInBytes,
        EAccelerationStructureBuildFlags              InFlags,
        const TArrayView<FRHIOpacityMicromap* const>& InOpacityMicromaps = TArrayView<FRHIOpacityMicromap* const>()) noexcept
    {
        FRHIGeometryAccelerationStructureDesc Desc;
        Desc.Flags                         = InFlags | EAccelerationStructureBuildFlags::Deserialize;
        Desc.SerializedData                = InSerializedData;
        Desc.SerializedSizeInBytes         = InSerializedSizeInBytes;
        Desc.SerializedOpacityMicromaps    = InOpacityMicromaps.Data();
        Desc.NumSerializedOpacityMicromaps = static_cast<uint32>(InOpacityMicromaps.Size());
        return Desc;
    }

    constexpr FRHIGeometryAccelerationStructureDesc() noexcept
        : VertexBuffer(nullptr)
        , NumVertices(0)
        , IndexBuffer(nullptr)
        , NumIndices(0)
        , IndexFormat(EIndexFormat::Unknown)
        , Segments(nullptr)
        , NumSegments(0)
    {
    }

    constexpr FRHIGeometryAccelerationStructureDesc(
        FRHIBuffer*                      InVertexBuffer,
        uint32                           InNumVerticies,
        FRHIBuffer*                      InIndexBuffer,
        uint32                           InNumIndices,
        EIndexFormat                     InIndexFormat,
        EAccelerationStructureBuildFlags InFlags) noexcept
        : Flags(InFlags)
        , VertexBuffer(InVertexBuffer)
        , NumVertices(InNumVerticies)
        , IndexBuffer(InIndexBuffer)
        , NumIndices(InNumIndices)
        , IndexFormat(InIndexFormat)
        , Segments(nullptr)
        , NumSegments(0)
    {
    }

    NODISCARD constexpr bool PreferFastTrace()     const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastTrace); }
    NODISCARD constexpr bool PreferFastBuild()     const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastBuild); }
    NODISCARD constexpr bool AllowUpdate()         const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowUpdate); }
    NODISCARD constexpr bool AllowCompaction()     const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowCompaction); }
    NODISCARD constexpr bool IsClusteredGeometry() const noexcept { return bIsClusteredGeometry; }
    NODISCARD constexpr bool IsSerialized()        const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::Deserialize); }

    NODISCARD constexpr FRHIGeometryAccelerationStructureBuildDesc GetBuildDesc() const noexcept
    {
        FRHIGeometryAccelerationStructureBuildDesc BuildDesc(VertexBuffer, NumVertices, IndexBuffer, NumIndices, IndexFormat, false);
        BuildDesc.Segments    = Segments;
        BuildDesc.NumSegments = NumSegments;
        return BuildDesc;
    }

    EAccelerationStructureBuildFlags Flags                = EAccelerationStructureBuildFlags::None;
    ERayTracingGeometryType          GeometryType         = ERayTracingGeometryType::Triangles;
    bool                             bIsClusteredGeometry = false;

    union
    {
        struct
        {
            FRHIBuffer*                          VertexBuffer;
            uint32                               NumVertices;
            FRHIBuffer*                          IndexBuffer;
            uint32                               NumIndices;
            EIndexFormat                         IndexFormat;
            const FRHIRayTracingGeometrySegment* Segments;
            uint32                               NumSegments;
        };

        struct
        {
            const void*                 SerializedData;
            uint64                      SerializedSizeInBytes;
            FRHIOpacityMicromap* const* SerializedOpacityMicromaps;
            uint32                      NumSerializedOpacityMicromaps;
        };
    };
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

    NODISCARD FRHISceneAccelerationStructureBuildDesc GetBuildDesc() const noexcept
    {
        return FRHISceneAccelerationStructureBuildDesc(Instances.Data(), static_cast<uint32>(Instances.Size()), false);
    }

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

    /** @return D3D12: ID3D12Resource*. Vulkan: VkAccelerationStructureKHR or VkBuffer. Metal: id<MTLAccelerationStructure>. Null: nullptr. */
    virtual void* GetRHINativeResource() const = 0;

    virtual void SetDebugName(const FString& InName) = 0;
    virtual void GetDebugName(FString& OutDebugName) const = 0;

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

    /**
     * @brief Find which of LinkedMicromaps each micromap address in the blocks of a serialized geometry structure is, laid out the same on D3D12 and Vulkan
     * @param GetAddress Returns the address a micromap is recorded by
     * @param OutOpacityMicromaps One micromap for each address in order, nullptr for a null address
     * @return Returns false when the blocks don't fit in the data or an address isn't one of LinkedMicromaps
     */
    static RHI_API bool FindSerializedOpacityMicromaps(
        const void*                                   SerializedData,
        uint64                                        SerializedSizeInBytes,
        const TArrayView<FRHIOpacityMicromap* const>& LinkedMicromaps,
        TFunctionRef<uint64(FRHIOpacityMicromap*)>    GetAddress,
        TArray<FRHIOpacityMicromap*>&                 OutOpacityMicromaps);

    /**
     * @brief Copy a serialized geometry structure with the micromap addresses in its blocks pointed at OpacityMicromaps
     * @param OpacityMicromaps One micromap for each address in order, nullptr for a null address
     * @param GetAddress Returns the address a micromap is recorded by
     * @return Returns false when the blocks don't fit in the data or hold a different number of addresses
     */
    static RHI_API bool LinkSerializedOpacityMicromaps(
        const void*                                   SerializedData,
        uint64                                        SerializedSizeInBytes,
        const TArrayView<FRHIOpacityMicromap* const>& OpacityMicromaps,
        TFunctionRef<uint64(FRHIOpacityMicromap*)>    GetAddress,
        TArray<uint8>&                                OutSerializedData);

    explicit FRHIGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
        : FRHIRayTracingAccelerationStructure(ERHIResourceType::GeometryAccelerationStructure, ERayTracingAccelerationStructureType::Geometry, InGeometryDesc.Flags)
        , GeometryType(InGeometryDesc.GeometryType)
    {
    }

    virtual ~FRHIGeometryAccelerationStructure() = default;

public:

    /**
     * @brief Find which of the micromaps the structure links each micromap address in data it serialized is
     * @param OutOpacityMicromaps One micromap for each address in order, nullptr for a null address, and empty when the structure links none
     * @return Returns false when the data's blocks don't fit in it or an address isn't a micromap the structure links
     */
    virtual bool GetSerializedOpacityMicromaps(const void* SerializedData, uint64 SerializedSizeInBytes, TArray<FRHIOpacityMicromap*>& OutOpacityMicromaps) const
    {
        OutOpacityMicromaps.Clear();
        return true;
    }

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
