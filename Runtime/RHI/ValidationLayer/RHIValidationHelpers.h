#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"
#include "RHI/RHITypes.h"

namespace RHIValidationHelpers
{
    inline constexpr uint64 IndirectArgumentOffsetAlignment = sizeof(uint32);

    NODISCARD constexpr bool IsRangeValid(uint64 TotalSize, uint64 Offset, uint64 Size)
    {
        return Size > 0 && Offset <= TotalSize && Size <= TotalSize - Offset;
    }

    NODISCARD constexpr bool IsSubresourceRangeValid(uint32 TotalCount, uint32 First, uint32 Count)
    {
        return Count > 0 && First <= TotalCount && Count <= TotalCount - First;
    }

    NODISCARD constexpr bool DoRangesOverlap(uint64 FirstOffset, uint64 FirstSize, uint64 SecondOffset, uint64 SecondSize)
    {
        if (!IsRangeValid(UINT64_MAX, FirstOffset, FirstSize) || !IsRangeValid(UINT64_MAX, SecondOffset, SecondSize))
        {
            return false;
        }

        const uint64 FirstEnd  = FirstOffset + FirstSize;
        const uint64 SecondEnd = SecondOffset + SecondSize;
        
        return FirstOffset < SecondEnd && SecondOffset < FirstEnd;
    }

    NODISCARD constexpr bool IsIndirectCommandRangeValid(uint64 TotalSize, uint64 Offset, uint64 RecordSize, uint32 CommandCount)
    {
        if (RecordSize == 0 || CommandCount == 0 || (Offset % IndirectArgumentOffsetAlignment) != 0 || Offset > TotalSize)
        {
            return false;
        }

        const uint64 RemainingSize = TotalSize - Offset;
        return uint64(CommandCount) <= RemainingSize / RecordSize;
    }

    NODISCARD constexpr bool AreGeometrySegmentsValid(const FRHIRayTracingGeometrySegment* Segments, uint32 NumSegments, uint32 NumIndices)
    {
        if (NumSegments > 0 && (!Segments || NumIndices == 0))
        {
            return false;
        }

        for (uint32 Index = 0; Index < NumSegments; ++Index)
        {
            const FRHIRayTracingGeometrySegment& Segment = Segments[Index];
            if (Segment.FirstIndex > NumIndices || Segment.NumIndices > NumIndices - Segment.FirstIndex || (Segment.NumIndices % 3) != 0)
            {
                return false;
            }
        }

        return true;
    }
}
