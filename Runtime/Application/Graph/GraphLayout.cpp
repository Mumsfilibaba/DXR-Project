#include "Application/Graph/GraphLayout.h"
#include "Core/Algorithms/Algorithm.h"
#include "Core/Math/Math.h"

static int32 FenwickPrefixSum(const TArray<int32>& Tree, int32 Index)
{
    int32 Sum = 0;
    for (; Index > 0; Index -= Index & -Index)
    {
        Sum += Tree[Index];
    }

    return Sum;
}

static void FenwickAdd(TArray<int32>& Tree, int32 Index, int32 Value)
{
    for (; Index < Tree.Size(); Index += Index & -Index)
    {
        Tree[Index] += Value;
    }
}

// How many adjacent swaps one transpose pass will try before it gives up on the column
constexpr int32 GRAPH_LAYOUT_TRANSPOSE_PASSES = 8;

int64 FLayeredGraphLayout::CountCrossings(TArrayView<const FLayerEdge> SortedEdges, int32 LowerLayerSize)
{
    TArray<int32> Tree;
    Tree.Reset(LowerLayerSize + 1, 0);

    int64 Crossings = 0;
    for (int32 Index = 0; Index < SortedEdges.Size(); ++Index)
    {
        const int32 Lower = SortedEdges[Index].LowerPosition + 1;
        Crossings += Index - FenwickPrefixSum(Tree, Lower);
        FenwickAdd(Tree, Lower, 1);
    }

    return Crossings;
}

void FLayeredGraphLayout::Layout(
    TArrayView<const Vector2>          NodeSizes,
    TArrayView<const FGraphLayoutEdge> Edges,
    const FGraphLayoutSettings&        Settings,
    TArray<Vector2>&                   OutPositions,
    TArrayView<const int32>            SeedKeys)
{
    const int32 NumNodes = NodeSizes.Size();

    OutPositions.Clear();
    OutPositions.Resize(NumNodes);

    if (NumNodes <= 0)
    {
        return;
    }

    const bool bHasSeedKeys = SeedKeys.Size() == NumNodes;

    TArray<TArray<int32>> Successors;
    Successors.Resize(NumNodes);

    TArray<TArray<int32>> Predecessors;
    Predecessors.Resize(NumNodes);

    for (const FGraphLayoutEdge& Edge : Edges)
    {
        const bool bIsValidEdge = Edge.FromNodeIndex >= 0
            && Edge.FromNodeIndex < NumNodes
            && Edge.ToNodeIndex >= 0
            && Edge.ToNodeIndex < NumNodes
            && Edge.FromNodeIndex != Edge.ToNodeIndex;

        if (bIsValidEdge)
        {
            Successors[Edge.FromNodeIndex].AddUnique(Edge.ToNodeIndex);
            Predecessors[Edge.ToNodeIndex].AddUnique(Edge.FromNodeIndex);
        }
    }

    TArray<int32> Rank;
    Rank.Reset(NumNodes, 0);

    TArray<int32> PendingPredecessors;
    PendingPredecessors.Resize(NumNodes);

    TArray<uint8> bIsRanked;
    bIsRanked.Reset(NumNodes, 0);

    TArray<int32> Ready;
    Ready.Reserve(NumNodes);

    for (int32 Index = 0; Index < NumNodes; ++Index)
    {
        PendingPredecessors[Index] = Predecessors[Index].Size();
        if (PendingPredecessors[Index] == 0)
        {
            Ready.Add(Index);
        }
    }

    int32 RankedCount        = 0;
    int32 NextCycleCandidate = 0;
    while (RankedCount < NumNodes)
    {
        if (Ready.IsEmpty())
        {
            while (bIsRanked[NextCycleCandidate])
            {
                ++NextCycleCandidate;
            }

            Ready.Add(NextCycleCandidate);
        }

        const int32 Node = Ready.Last();
        Ready.Pop();

        if (bIsRanked[Node])
        {
            continue;
        }

        bIsRanked[Node] = 1;
        ++RankedCount;

        for (const int32 Successor : Successors[Node])
        {
            if (bIsRanked[Successor])
            {
                continue;
            }

            Rank[Successor] = Math::Max(Rank[Successor], Rank[Node] + 1);
            if (--PendingPredecessors[Successor] == 0)
            {
                Ready.Add(Successor);
            }
        }
    }

    if (Settings.bSourcesFollowPreviousNode)
    {
        for (int32 Index = 1; Index < NumNodes; ++Index)
        {
            if (Predecessors[Index].IsEmpty())
            {
                Rank[Index] = Math::Max(Rank[Index], Rank[Index - 1]);
            }
        }
    }

    int32 MaxRank = 0;
    for (int32 Index = 0; Index < NumNodes; ++Index)
    {
        MaxRank = Math::Max(MaxRank, Rank[Index]);
    }

    TArray<FGraphLayoutEdge> LongEdges;
    for (int32 Index = 0; Index < NumNodes; ++Index)
    {
        for (const int32 Successor : Successors[Index])
        {
            if (Rank[Successor] - Rank[Index] > 1)
            {
                LongEdges.Add(FGraphLayoutEdge(Index, Successor));
            }
        }
    }

    for (const FGraphLayoutEdge& LongEdge : LongEdges)
    {
        const int32 From = LongEdge.FromNodeIndex;
        const int32 To   = LongEdge.ToNodeIndex;

        Successors[From].Remove(To);
        Predecessors[To].Remove(From);

        int32 Previous = From;
        for (int32 IntermediateRank = Rank[From] + 1; IntermediateRank < Rank[To]; ++IntermediateRank)
        {
            const int32 Dummy = Rank.Size();
            Rank.Add(IntermediateRank);
            Successors.Emplace();
            Predecessors.Emplace();

            Successors[Previous].AddUnique(Dummy);
            Predecessors[Dummy].AddUnique(Previous);
            Previous = Dummy;
        }

        Successors[Previous].AddUnique(To);
        Predecessors[To].AddUnique(Previous);
    }

    const int32 NumLayoutNodes = Rank.Size();

    TArray<TArray<int32>> Columns;
    Columns.Resize(MaxRank + 1);

    for (int32 Index = 0; Index < NumLayoutNodes; ++Index)
    {
        Columns[Rank[Index]].Add(Index);
    }

    const auto SeedLess = [&](int32 A, int32 B) -> bool
    {
        const bool bIsARealNode = A < NumNodes;
        const bool bIsBRealNode = B < NumNodes;

        if (bIsARealNode != bIsBRealNode)
        {
            return bIsARealNode;
        }

        if (bIsARealNode && bHasSeedKeys && SeedKeys[A] != SeedKeys[B])
        {
            return SeedKeys[A] < SeedKeys[B];
        }

        return A < B;
    };

    TArray<int32> Position;
    Position.Resize(NumLayoutNodes);

    const auto UpdatePositions = [&](int32 Column)
    {
        const TArray<int32>& Order = Columns[Column];
        for (int32 Index = 0; Index < Order.Size(); ++Index)
        {
            Position[Order[Index]] = Index;
        }
    };

    for (int32 Column = 0; Column < Columns.Size(); ++Column)
    {
        Algorithm::Sort(Columns[Column], SeedLess);
        UpdatePositions(Column);
    }

    TArray<float> Barycenters;
    Barycenters.Resize(NumLayoutNodes);

    const auto SortByBarycenter = [&](int32 Column, bool bUsePredecessors)
    {
        TArray<int32>& Order = Columns[Column];
        for (const int32 Node : Order)
        {
            const TArray<int32>& Neighbors = bUsePredecessors ? Predecessors[Node] : Successors[Node];
            if (Neighbors.IsEmpty())
            {
                Barycenters[Node] = static_cast<float>(Position[Node]);
                continue;
            }

            float Sum = 0.0f;
            for (const int32 Neighbor : Neighbors)
            {
                Sum += static_cast<float>(Position[Neighbor]);
            }

            Barycenters[Node] = Sum / static_cast<float>(Neighbors.Size());
        }

        Algorithm::Sort(Order, [&](int32 A, int32 B)
        {
            return Barycenters[A] == Barycenters[B] ? SeedLess(A, B) : Barycenters[A] < Barycenters[B];
        });

        UpdatePositions(Column);
    };

    TArray<FLayerEdge> ScratchEdges;

    const auto CrossingsBetween = [&](int32 LeftColumn, int32 RightColumn) -> int64
    {
        ScratchEdges.Clear();
        for (const int32 From : Columns[LeftColumn])
        {
            for (const int32 To : Successors[From])
            {
                if (Rank[To] == RightColumn)
                {
                    ScratchEdges.Add(FLayerEdge{ Position[From], Position[To] });
                }
            }
        }

        Algorithm::Sort(ScratchEdges, [](const FLayerEdge& A, const FLayerEdge& B)
        {
            return A.UpperPosition != B.UpperPosition ? A.UpperPosition < B.UpperPosition : A.LowerPosition < B.LowerPosition;
        });

        return CountCrossings(ScratchEdges, Columns[RightColumn].Size());
    };

    const auto Transpose = [&](int32 Column, int32 FixedNeighborColumn, bool bNeighborIsLeft)
    {
        TArray<int32>& Order = Columns[Column];

        const int32 LeftColumn  = bNeighborIsLeft ? FixedNeighborColumn : Column;
        const int32 RightColumn = bNeighborIsLeft ? Column : FixedNeighborColumn;

        const auto SwapAt = [&](int32 Index)
        {
            Order.Swap(Index, Index + 1);
            Position[Order[Index]]     = Index;
            Position[Order[Index + 1]] = Index + 1;
        };

        bool bImproved = true;
        for (int32 Pass = 0; Pass < GRAPH_LAYOUT_TRANSPOSE_PASSES && bImproved; ++Pass)
        {
            bImproved = false;
            for (int32 Index = 0; Index + 1 < Order.Size(); ++Index)
            {
                const int64 Before = CrossingsBetween(LeftColumn, RightColumn);

                SwapAt(Index);

                if (CrossingsBetween(LeftColumn, RightColumn) < Before)
                {
                    bImproved = true;
                }
                else
                {
                    SwapAt(Index);
                }
            }
        }
    };

    for (int32 Sweep = 0; Sweep < Settings.CrossingReductionPasses; ++Sweep)
    {
        for (int32 Column = 1; Column < Columns.Size(); ++Column)
        {
            SortByBarycenter(Column, true);
            Transpose(Column, Column - 1, true);
        }

        for (int32 Column = Columns.Size() - 2; Column >= 0; --Column)
        {
            SortByBarycenter(Column, false);
            Transpose(Column, Column + 1, false);
        }
    }

    const auto NodeHeight = [&](int32 Index) -> float
    {
        return Index < NumNodes ? NodeSizes[Index].Y : 0.0f;
    };

    TArray<float> ColumnWidths;
    ColumnWidths.Resize(Columns.Size());

    for (int32 Column = 0; Column < Columns.Size(); ++Column)
    {
        float Width = Settings.MinColumnWidth;
        for (const int32 Index : Columns[Column])
        {
            if (Index < NumNodes)
            {
                Width = Math::Max(Width, NodeSizes[Index].X);
            }
        }

        ColumnWidths[Column] = Width;
    }

    TArray<float> NodeY;
    NodeY.Resize(NumLayoutNodes);

    for (int32 Column = 0; Column < Columns.Size(); ++Column)
    {
        float CursorY = 0.0f;
        for (const int32 Index : Columns[Column])
        {
            NodeY[Index] = CursorY;
            CursorY += NodeHeight(Index) + Settings.RowSpacing;
        }
    }

    const auto NeighborAverageY = [&](int32 Index) -> float
    {
        float Sum   = 0.0f;
        int32 Count = 0;

        for (const int32 Neighbor : Predecessors[Index])
        {
            if (Rank[Neighbor] == Rank[Index] - 1)
            {
                Sum += NodeY[Neighbor];
                Count++;
            }
        }

        for (const int32 Neighbor : Successors[Index])
        {
            if (Rank[Neighbor] == Rank[Index] + 1)
            {
                Sum += NodeY[Neighbor];
                Count++;
            }
        }

        return Count > 0 ? (Sum / static_cast<float>(Count)) : NodeY[Index];
    };

    const auto EnforceNonOverlap = [&](const TArray<int32>& Order)
    {
        for (int32 Index = 1; Index < Order.Size(); ++Index)
        {
            const int32 Previous = Order[Index - 1];
            const int32 Current  = Order[Index];

            NodeY[Current] = Math::Max(NodeY[Current], NodeY[Previous] + NodeHeight(Previous) + Settings.RowSpacing);
        }
    };

    for (int32 Sweep = 0; Sweep < Settings.CrossingReductionPasses; ++Sweep)
    {
        for (int32 Column = 0; Column < Columns.Size(); ++Column)
        {
            for (const int32 Index : Columns[Column])
            {
                NodeY[Index] = NeighborAverageY(Index);
            }

            EnforceNonOverlap(Columns[Column]);
        }
    }

    float CursorX = 0.0f;
    for (int32 Column = 0; Column < Columns.Size(); ++Column)
    {
        for (const int32 Index : Columns[Column])
        {
            if (Index < NumNodes)
            {
                OutPositions[Index] = Vector2(CursorX, NodeY[Index]);
            }
        }

        CursorX += ColumnWidths[Column] + Settings.ColumnSpacing;
    }
}
