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
        , MinColumnWidth(120.0f)
        , CrossingReductionPasses(4)
        , bSourcesFollowPreviousNode(false)
    {
    }

    /** @brief The gap between one column of nodes and the next. */
    float ColumnSpacing;

    /** @brief The gap between one node and the one under it. */
    float RowSpacing;

    /** @brief The width a column falls back to when every node in it measured narrower than this. */
    float MinColumnWidth;

    /** @brief How many barycenter and transpose sweeps to run, which trades tidiness for time. */
    int32 CrossingReductionPasses;

    /**
     * @brief True to move a node nothing leads into no further left than the node before it. A graph read in
     * submission order then keeps a late, disconnected node near where it was submitted instead of in column zero.
     */
    bool bSourcesFollowPreviousNode;
};

struct FLayerEdge
{
    int32 UpperPosition;
    int32 LowerPosition;
};

struct APPLICATION_API FLayeredGraphLayout
{
    /**
     * @brief Assigns each node a graph-space position using a layered, crossing-reduced layout: longest-path
     * layering, dummy nodes on edges that skip layers, barycenter ordering with adjacent-swap transposition, and a
     * vertical settle toward each node's neighbours.
     *
     * @param NodeSizes    The measured size of each node, indexed as the edges refer to them.
     * @param Edges        The directed edges between nodes.
     * @param Settings     The spacing and pass-count knobs.
     * @param OutPositions Receives one position per node, in the same order as NodeSizes.
     * @param SeedKeys     Optional, one per node. Nodes start each layer ordered by this key rather than by index,
     *                     and it breaks ties between equal barycenters.
     */
    static void Layout(
        TArrayView<const Vector2>          NodeSizes,
        TArrayView<const FGraphLayoutEdge> Edges,
        const FGraphLayoutSettings&        Settings,
        TArray<Vector2>&                   OutPositions,
        TArrayView<const int32>            SeedKeys = TArrayView<const int32>());

    /**
     * @brief Counts how many pairs of edges between two layers cross, as the inversions left in the lower ends
     * once the edges are ordered by their upper ends. Edges sharing an end do not cross.
     *
     * @param SortedEdges    The edges, ordered by upper position and then by lower position.
     * @param LowerLayerSize How many nodes the lower layer holds, which bounds every lower position.
     * @return The number of crossings.
     */
    NODISCARD static int64 CountCrossings(TArrayView<const FLayerEdge> SortedEdges, int32 LowerLayerSize);
};
