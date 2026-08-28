#include "Application/Graph/GraphLayout.h"
#include "Core/Math/Math.h"

// The width a column falls back to when every node in it measured narrower than this
constexpr float GRAPH_LAYOUT_MIN_COLUMN_WIDTH = 120.0f;

// How many adjacent swaps one transpose pass will try before it gives up on the column
constexpr int32 GRAPH_LAYOUT_TRANSPOSE_PASSES = 8;

void FGraphLayout::LayoutLayered(
    TArrayView<const Vector2>          NodeSizes,
    TArrayView<const FGraphLayoutEdge> Edges,
    const FGraphLayoutSettings&        Settings,
    TArray<Vector2>&                   OutPositions)
{
    const int32 NumNodes = NodeSizes.Size();

    OutPositions.Clear();
    OutPositions.Resize(NumNodes);

    if (NumNodes <= 0)
    {
        return;
    }

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
    Rank.Resize(NumNodes);

    for (int32 Index = 0; Index < NumNodes; ++Index)
    {
        Rank[Index] = 0;
    }

    bool bChanged = true;
    for (int32 Iteration = 0; Iteration < NumNodes && bChanged; ++Iteration)
    {
        bChanged = false;
        for (int32 Index = 0; Index < NumNodes; ++Index)
        {
            for (const int32 Successor : Successors[Index])
            {
                const int32 Candidate = Rank[Index] + 1;
                if (Candidate > Rank[Successor])
                {
                    Rank[Successor] = Candidate;
                    bChanged        = true;
                }
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

    auto InputOrderLess = [&](int32 A, int32 B) -> bool
    {
        const bool bIsARealNode = A < NumNodes;
        const bool bIsBRealNode = B < NumNodes;

        if (bIsARealNode != bIsBRealNode)
        {
            return bIsARealNode;
        }

        return A < B;
    };

    for (int32 Column = 0; Column < Columns.Size(); ++Column)
    {
        Columns[Column].SortWithPredicate(InputOrderLess);
    }

    auto OrderIndexInColumn = [&](int32 Index) -> int32
    {
        const TArray<int32>& Order = Columns[Rank[Index]];
        for (int32 OrderIndex = 0; OrderIndex < Order.Size(); ++OrderIndex)
        {
            if (Order[OrderIndex] == Index)
            {
                return OrderIndex;
            }
        }

        return 0;
    };

    auto Barycenter = [&](int32 Index, bool bUsePredecessors) -> float
    {
        const TArray<int32>& Neighbors = bUsePredecessors ? Predecessors[Index] : Successors[Index];
        if (Neighbors.IsEmpty())
        {
            return static_cast<float>(OrderIndexInColumn(Index));
        }

        float Sum = 0.0f;
        for (const int32 Neighbor : Neighbors)
        {
            Sum += static_cast<float>(OrderIndexInColumn(Neighbor));
        }

        return Sum / static_cast<float>(Neighbors.Size());
    };

    auto CountCrossings = [&](int32 LeftColumn, int32 RightColumn) -> int32
    {
        const TArray<int32>& LeftOrder  = Columns[LeftColumn];
        const TArray<int32>& RightOrder = Columns[RightColumn];

        TArray<int32> LeftPosition;
        LeftPosition.Resize(NumLayoutNodes);

        TArray<int32> RightPosition;
        RightPosition.Resize(NumLayoutNodes);

        for (int32 Index = 0; Index < LeftOrder.Size(); ++Index)
        {
            LeftPosition[LeftOrder[Index]] = Index;
        }

        for (int32 Index = 0; Index < RightOrder.Size(); ++Index)
        {
            RightPosition[RightOrder[Index]] = Index;
        }

        TArray<int32> EdgeLeft;
        TArray<int32> EdgeRight;

        for (const int32 From : LeftOrder)
        {
            for (const int32 To : Successors[From])
            {
                if (Rank[To] == RightColumn)
                {
                    EdgeLeft.Add(LeftPosition[From]);
                    EdgeRight.Add(RightPosition[To]);
                }
            }
        }

        int32 Crossings = 0;
        for (int32 First = 0; First < EdgeLeft.Size(); ++First)
        {
            for (int32 Second = First + 1; Second < EdgeLeft.Size(); ++Second)
            {
                if ((EdgeLeft[First] - EdgeLeft[Second]) * (EdgeRight[First] - EdgeRight[Second]) < 0)
                {
                    Crossings++;
                }
            }
        }

        return Crossings;
    };

    auto Transpose = [&](int32 Column, int32 FixedNeighborColumn, bool bNeighborIsLeft)
    {
        TArray<int32>& Order = Columns[Column];

        bool bImproved = true;
        for (int32 Pass = 0; Pass < GRAPH_LAYOUT_TRANSPOSE_PASSES && bImproved; ++Pass)
        {
            bImproved = false;
            for (int32 Index = 0; Index + 1 < Order.Size(); ++Index)
            {
                const int32 LeftColumn  = bNeighborIsLeft ? FixedNeighborColumn : Column;
                const int32 RightColumn = bNeighborIsLeft ? Column : FixedNeighborColumn;
                const int32 Before      = CountCrossings(LeftColumn, RightColumn);

                Order.Swap(Index, Index + 1);

                if (CountCrossings(LeftColumn, RightColumn) < Before)
                {
                    bImproved = true;
                }
                else
                {
                    Order.Swap(Index, Index + 1);
                }
            }
        }
    };

    for (int32 Sweep = 0; Sweep < Settings.CrossingReductionPasses; ++Sweep)
    {
        for (int32 Column = 1; Column < Columns.Size(); ++Column)
        {
            Columns[Column].SortWithPredicate([&](int32 A, int32 B)
            {
                const float BarycenterA = Barycenter(A, true);
                const float BarycenterB = Barycenter(B, true);

                return BarycenterA == BarycenterB ? InputOrderLess(A, B) : BarycenterA < BarycenterB;
            });

            Transpose(Column, Column - 1, true);
        }

        for (int32 Column = Columns.Size() - 2; Column >= 0; --Column)
        {
            Columns[Column].SortWithPredicate([&](int32 A, int32 B)
            {
                const float BarycenterA = Barycenter(A, false);
                const float BarycenterB = Barycenter(B, false);

                return BarycenterA == BarycenterB ? InputOrderLess(A, B) : BarycenterA < BarycenterB;
            });

            Transpose(Column, Column + 1, false);
        }
    }

    auto NodeHeight = [&](int32 Index) -> float
    {
        return Index < NumNodes ? NodeSizes[Index].Y : 0.0f;
    };

    TArray<float> ColumnWidths;
    ColumnWidths.Resize(Columns.Size());

    for (int32 Column = 0; Column < Columns.Size(); ++Column)
    {
        float Width = GRAPH_LAYOUT_MIN_COLUMN_WIDTH;
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

    auto NeighborAverageY = [&](int32 Index) -> float
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

    auto EnforceNonOverlap = [&](const TArray<int32>& Order)
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
