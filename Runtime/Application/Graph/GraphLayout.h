#pragma once
#include "Core/CoreTypes.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Math/Vector2.h"

struct FGraphLayoutEdge
{
    FGraphLayoutEdge()
        : FromNodeIndex(-1)
        , ToNodeIndex(-1)
    {
    }

    FGraphLayoutEdge(int32 InFromNodeIndex, int32 InToNodeIndex)
        : FromNodeIndex(InFromNodeIndex)
        , ToNodeIndex(InToNodeIndex)
    {
    }

    int32 FromNodeIndex;
    int32 ToNodeIndex;
};

struct FGraphLayoutSettings
{
    FGraphLayoutSettings()
        : ColumnSpacing(80.0f)
        , RowSpacing(24.0f)
        , CrossingReductionPasses(4)
    {
    }

    /** @brief The gap between one column of nodes and the next. */
    float ColumnSpacing;

    /** @brief The gap between one node and the one under it. */
    float RowSpacing;

    /** @brief How many barycenter and transpose sweeps to run, which trades tidiness for time. */
    int32 CrossingReductionPasses;
};

struct APPLICATION_API FGraphLayout
{
    /**
     * @brief Assigns each node a graph-space position using a layered, crossing-reduced layout.
     *
     * @param NodeSizes    The measured size of each node, indexed as the edges refer to them.
     * @param Edges        The directed edges between nodes.
     * @param Settings     The spacing and pass-count knobs.
     * @param OutPositions Receives one position per node, in the same order as NodeSizes.
     */
    static void LayoutLayered(
        TArrayView<const Vector2>          NodeSizes,
        TArrayView<const FGraphLayoutEdge> Edges,
        const FGraphLayoutSettings&        Settings,
        TArray<Vector2>&                   OutPositions);
};
