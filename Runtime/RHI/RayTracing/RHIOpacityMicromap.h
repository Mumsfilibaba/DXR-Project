#pragma once
#include "Core/Containers/ArrayView.h"
#include "RHI/RHIResources.h"
#include "RHI/RayTracing/RHIAccelerationStructure.h"
#include "RHI/RayTracing/RHIRayTracingTypes.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHIBuffer;

typedef TSharedRef<class FRHIOpacityMicromap> FRHIOpacityMicromapRef;

enum class EOpacityMicromapSpecialIndex : int8
{
    FullyTransparent        = -1,
    FullyOpaque             = -2,
    FullyUnknownTransparent = -3,
    FullyUnknownOpaque      = -4,
};

enum class EOpacityMicromapFormat : uint8
{
    OC1_2State = 1,
    OC1_4State = 2,
};

struct FRHIOpacityMicromapTriangleDesc
{
    constexpr FRHIOpacityMicromapTriangleDesc() noexcept = default;

    constexpr FRHIOpacityMicromapTriangleDesc(uint32 InDataOffset, uint16 InSubdivisionLevel, EOpacityMicromapFormat InFormat) noexcept
        : DataOffset(InDataOffset)
        , SubdivisionLevel(InSubdivisionLevel)
        , Format(static_cast<uint16>(InFormat))
    {
    }

    uint32 DataOffset       = 0;
    uint16 SubdivisionLevel = 0;
    uint16 Format           = static_cast<uint16>(EOpacityMicromapFormat::OC1_2State);
};

static_assert(sizeof(FRHIOpacityMicromapTriangleDesc) == 8, "FRHIOpacityMicromapTriangleDesc must match D3D12_RAYTRACING_OPACITY_MICROMAP_DESC and VkMicromapTriangleKHR");

struct FRHIOpacityMicromapHistogramEntry
{
    uint32                 Count            = 0;
    uint32                 SubdivisionLevel = 0;
    EOpacityMicromapFormat Format           = EOpacityMicromapFormat::OC1_2State;
};

struct FRHIOpacityMicromapBuildDesc
{
    FRHIBuffer* OMMDescriptorBuffer        = nullptr;
    uint64      OMMDescriptorBufferOffset  = 0;
    uint32      OMMDescriptorStrideInBytes = 0;

    FRHIBuffer* OpacityMicroTriangleDataBuffer       = nullptr;
    uint64      OpacityMicroTriangleDataBufferOffset = 0;

    /** When empty, the build uses a single uniform entry from NumOpacityMicromaps and the OMM's create-time format/subdivision. */
    TArrayView<const FRHIOpacityMicromapHistogramEntry> HistogramEntries;

    uint32 NumOpacityMicromaps = 0;
    bool   bUpdate             = false;
};

struct FRHIOpacityMicromapDesc
{
    /**
     * @brief Describe a micromap deserialized from data SerializeAccelerationStructure wrote for a micromap on this driver
     * @param InSerializedData The data, which only has to live until the micromap is created
     */
    NODISCARD static FORCEINLINE FRHIOpacityMicromapDesc CreateSerialized(const void* InSerializedData, uint64 InSerializedSizeInBytes, EAccelerationStructureBuildFlags InFlags) noexcept
    {
        FRHIOpacityMicromapDesc Desc;
        Desc.Flags                 = InFlags | EAccelerationStructureBuildFlags::Deserialize;
        Desc.SerializedData        = InSerializedData;
        Desc.SerializedSizeInBytes = InSerializedSizeInBytes;
        return Desc;
    }

    FRHIOpacityMicromapDesc() noexcept
        : OMMDescriptorBuffer(nullptr)
        , OMMDescriptorBufferOffset(0)
        , OMMDescriptorStrideInBytes(0)
        , OpacityMicroTriangleDataBuffer(nullptr)
        , OpacityMicroTriangleDataBufferOffset(0)
        , HistogramEntries(nullptr)
        , NumHistogramEntries(0)
        , NumOpacityMicromaps(0)
    {
    }

    /**
     * @brief Describe a micromap built from the inputs of BuildDesc when it's created
     * @param InBuildDesc The build inputs, whose histogram only has to live until the micromap is created
     */
    FRHIOpacityMicromapDesc(EOpacityMicromapFormat InFormat, uint32 InSubdivisionLevel, const FRHIOpacityMicromapBuildDesc& InBuildDesc, EAccelerationStructureBuildFlags InFlags = EAccelerationStructureBuildFlags::None) noexcept
        : Flags(InFlags)
        , Format(InFormat)
        , SubdivisionLevel(InSubdivisionLevel)
        , OMMDescriptorBuffer(InBuildDesc.OMMDescriptorBuffer)
        , OMMDescriptorBufferOffset(InBuildDesc.OMMDescriptorBufferOffset)
        , OMMDescriptorStrideInBytes(InBuildDesc.OMMDescriptorStrideInBytes)
        , OpacityMicroTriangleDataBuffer(InBuildDesc.OpacityMicroTriangleDataBuffer)
        , OpacityMicroTriangleDataBufferOffset(InBuildDesc.OpacityMicroTriangleDataBufferOffset)
        , HistogramEntries(InBuildDesc.HistogramEntries.Data())
        , NumHistogramEntries(static_cast<uint32>(InBuildDesc.HistogramEntries.Size()))
        , NumOpacityMicromaps(InBuildDesc.NumOpacityMicromaps)
    {
    }

    NODISCARD constexpr bool IsSerialized() const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::Deserialize); }

    NODISCARD FRHIOpacityMicromapBuildDesc GetBuildDesc() const noexcept
    {
        FRHIOpacityMicromapBuildDesc BuildDesc;
        BuildDesc.OMMDescriptorBuffer                  = OMMDescriptorBuffer;
        BuildDesc.OMMDescriptorBufferOffset            = OMMDescriptorBufferOffset;
        BuildDesc.OMMDescriptorStrideInBytes           = OMMDescriptorStrideInBytes;
        BuildDesc.OpacityMicroTriangleDataBuffer       = OpacityMicroTriangleDataBuffer;
        BuildDesc.OpacityMicroTriangleDataBufferOffset = OpacityMicroTriangleDataBufferOffset;
        BuildDesc.HistogramEntries                     = TArrayView<const FRHIOpacityMicromapHistogramEntry>(HistogramEntries, NumHistogramEntries);
        BuildDesc.NumOpacityMicromaps                  = NumOpacityMicromaps;
        return BuildDesc;
    }

    EAccelerationStructureBuildFlags Flags            = EAccelerationStructureBuildFlags::None;
    EOpacityMicromapFormat           Format           = EOpacityMicromapFormat::OC1_2State;
    uint32                           SubdivisionLevel = 0;

    union
    {
        struct
        {
            FRHIBuffer*                              OMMDescriptorBuffer;
            uint64                                   OMMDescriptorBufferOffset;
            uint32                                   OMMDescriptorStrideInBytes;
            FRHIBuffer*                              OpacityMicroTriangleDataBuffer;
            uint64                                   OpacityMicroTriangleDataBufferOffset;
            const FRHIOpacityMicromapHistogramEntry* HistogramEntries;
            uint32                                   NumHistogramEntries;
            uint32                                   NumOpacityMicromaps;
        };

        struct
        {
            const void* SerializedData;
            uint64      SerializedSizeInBytes;
        };
    };
};

class FRHIOpacityMicromap : public FRHIRayTracingAccelerationStructure
{
protected:
    explicit FRHIOpacityMicromap(const FRHIOpacityMicromapDesc& InDesc)
        : FRHIRayTracingAccelerationStructure(ERHIResourceType::OpacityMicromap, ERayTracingAccelerationStructureType::OpacityMicromap, InDesc.Flags)
    {
    }

    virtual ~FRHIOpacityMicromap() = default;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
