#pragma once
#include "Core/CoreTypes.h"

struct FUIPaintStats
{
    FUIPaintStats()
        : ElementWalkTime(0.0f)
        , GeometryBuildTime(0.0f)
        , BufferUploadTime(0.0f)
        , CommandCount(0)
        , VertexCount(0)
        , BatchCount(0)
    {
    }

    /** @return How much of a frame the three phases took together, in milliseconds. */
    NODISCARD FORCEINLINE float GetTotalTime() const
    {
        return ElementWalkTime + GeometryBuildTime + BufferUploadTime;
    }

    /** @brief How long the element tree took to walk and record its draw commands, in milliseconds. */
    float ElementWalkTime;

    /** @brief How long those commands took to turn into vertices and indices, in milliseconds. */
    float GeometryBuildTime;

    /** @brief How long the vertices and indices took to reach the GPU, in milliseconds. */
    float BufferUploadTime;

    /** @brief How many draw commands the walk recorded. */
    int32 CommandCount;

    /** @brief How many vertices the build produced. */
    int32 VertexCount;

    /** @brief How many draw calls the geometry collapsed into. */
    int32 BatchCount;
};
