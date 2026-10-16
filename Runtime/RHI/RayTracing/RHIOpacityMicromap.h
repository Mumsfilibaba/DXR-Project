#pragma once
#include "Core/Containers/ArrayView.h"
#include "RHI/RHIResources.h"
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

struct FRHIOpacityMicromapDesc
{
    EAccelerationStructureBuildFlags Flags            = EAccelerationStructureBuildFlags::None;
    EOpacityMicromapFormat           Format           = EOpacityMicromapFormat::OC1_2State;
    uint32                           SubdivisionLevel = 0;
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

class FRHIOpacityMicromap : public FRHIResource
{
protected:
    explicit FRHIOpacityMicromap(const FRHIOpacityMicromapDesc& InDesc)
        : FRHIResource(ERHIResourceType::OpacityMicromap)
        , Flags(InDesc.Flags)
    {
    }

    virtual ~FRHIOpacityMicromap() = default;

public:

    /** @return D3D12: ID3D12Resource*. Vulkan: VkMicromapEXT. Metal and Null do not implement opacity micromaps. */
    virtual void* GetRHINativeResource() const = 0;

    NODISCARD EAccelerationStructureBuildFlags GetFlags() const
    {
        return Flags;
    }

protected:
    EAccelerationStructureBuildFlags Flags;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
