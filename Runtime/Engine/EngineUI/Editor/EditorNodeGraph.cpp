#include "Engine/EngineUI/Editor/EditorNodeGraph.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Math/Math.h"

namespace EditorNodeGraphPrivate
{
    struct FPinRecord
    {
        ImVec2                    Pos    = ImVec2(0.0f, 0.0f);
        EditorNodeGraph::EPinKind Kind   = EditorNodeGraph::EPinKind::Input;
        const CHAR*               Label  = nullptr;
        bool                      bValid = false;
    };

    struct FPendingPin
    {
        int32                     PinId = 0;
        EditorNodeGraph::EPinKind Kind  = EditorNodeGraph::EPinKind::Input;
        const CHAR*               Label = nullptr;
    };

    struct FNodeState
    {
        ImVec2*                      InOutPos    = nullptr;
        ImVec2                       ScreenMin   = ImVec2(0.0f, 0.0f);
        float                        NodeWidth   = 180.0f;
        float                        TitleHeight = 24.0f;
        int32                        NodeId      = 0;
        String                       Title;
        EditorNodeGraph::ENodeStyle  Style       = EditorNodeGraph::ENodeStyle::Normal;
        bool                         bActive     = false;
        TArray<FPendingPin>          PendingPins;
    };

    struct FCanvasState
    {
        ImDrawList*             DrawList        = nullptr;
        ImVec2                  CanvasOrigin    = ImVec2(0.0f, 0.0f);
        ImVec2                  CanvasSize      = ImVec2(0.0f, 0.0f);
        ImVec2                  Pan             = ImVec2(0.0f, 0.0f);
        float                   Zoom            = 1.0f;
        bool                    bActive         = false;
        bool                    bNodesAboveLinks = false;
        int32                   HoveredNodeId   = -1;
        int32                   ActiveNodeId    = -1;
        FNodeState              CurrentNode;
        TMap<int32, FPinRecord> PinPositions;
    };

    static FCanvasState GCanvas;

    static bool   GRequestResetView = false;
    static bool   GRequestFitView   = false;
    static ImVec2 GFitWorldMin      = ImVec2(0.0f, 0.0f);
    static ImVec2 GFitWorldMax      = ImVec2(0.0f, 0.0f);

    // Nodes always grow to fit their title and pin labels, so no text is ever hidden.
    static constexpr float MinNodeWidthWorld = 160.0f;
    static constexpr float PinGroupGapWorld  = 8.0f;

    static ImVec2 WorldToScreen(const ImVec2& World)
    {
        return ImVec2(
            GCanvas.CanvasOrigin.x + (World.x + GCanvas.Pan.x) * GCanvas.Zoom,
            GCanvas.CanvasOrigin.y + (World.y + GCanvas.Pan.y) * GCanvas.Zoom);
    }

    static ImVec2 ScreenToWorld(const ImVec2& Screen)
    {
        return ImVec2(
            (Screen.x - GCanvas.CanvasOrigin.x) / GCanvas.Zoom - GCanvas.Pan.x,
            (Screen.y - GCanvas.CanvasOrigin.y) / GCanvas.Zoom - GCanvas.Pan.y);
    }

    static float WrapGridOffset(float Value, float Step)
    {
        if (Step <= 0.0f)
        {
            return 0.0f;
        }

        float Result = Value - Step * static_cast<float>(static_cast<int32>(Value / Step));
        if (Result < 0.0f)
        {
            Result += Step;
        }

        return Result;
    }

    static void DrawGrid()
    {
        if (!GCanvas.DrawList)
        {
            return;
        }

        const float GridStep = 32.0f * GCanvas.Zoom;
        if (GridStep < 4.0f)
        {
            return;
        }

        const ImU32  GridColor = IM_COL32(48, 48, 48, 255);
        const ImVec2 Min       = GCanvas.CanvasOrigin;
        const ImVec2 Max       = ImVec2(Min.x + GCanvas.CanvasSize.x, Min.y + GCanvas.CanvasSize.y);

        const float OffsetX = WrapGridOffset(GCanvas.Pan.x * GCanvas.Zoom, GridStep);
        const float OffsetY = WrapGridOffset(GCanvas.Pan.y * GCanvas.Zoom, GridStep);

        for (float X = OffsetX; X < GCanvas.CanvasSize.x; X += GridStep)
        {
            GCanvas.DrawList->AddLine(ImVec2(Min.x + X, Min.y), ImVec2(Min.x + X, Max.y), GridColor);
        }

        for (float Y = OffsetY; Y < GCanvas.CanvasSize.y; Y += GridStep)
        {
            GCanvas.DrawList->AddLine(ImVec2(Min.x, Min.y + Y), ImVec2(Max.x, Min.y + Y), GridColor);
        }
    }

    static void DrawPinRow(const FPendingPin& Pending, const ImVec2& NodeMin, const ImVec2& NodeMax, float RowCenterY, ImU32 LabelColor)
    {
        const bool   bIsInput    = (Pending.Kind == EditorNodeGraph::EPinKind::Input);
        const float  TextOffsetY = -ImGui::GetFontSize() * 0.5f;
        const ImVec2 PinCenter   = ImVec2(bIsInput ? NodeMin.x : NodeMax.x, RowCenterY);

        if (Pending.Label)
        {
            const float LabelX = bIsInput
                ? (PinCenter.x + 10.0f * GCanvas.Zoom)
                : (PinCenter.x - 10.0f * GCanvas.Zoom - ImGui::CalcTextSize(Pending.Label).x);

            GCanvas.DrawList->AddText(
                ImVec2(LabelX, PinCenter.y + TextOffsetY),
                LabelColor,
                Pending.Label);
        }

        const float PinRadius = 4.5f * GCanvas.Zoom;
        GCanvas.DrawList->AddCircleFilled(PinCenter, PinRadius, EditorNodeGraph::GetPinColor(Pending.Kind));
        GCanvas.DrawList->AddCircle(PinCenter, PinRadius, IM_COL32(20, 20, 20, 255));

        FPinRecord& Record = GCanvas.PinPositions.FindOrAdd(Pending.PinId);
        Record.Pos    = PinCenter;
        Record.Kind   = Pending.Kind;
        Record.Label  = Pending.Label;
        Record.bValid = true;
    }

    static int32 FindMeasuredIndex(TArrayView<const EditorNodeGraph::FNodeLayoutInput> MeasuredNodes, int32 PassIndex)
    {
        for (int32 Index = 0; Index < MeasuredNodes.Size(); ++Index)
        {
            if (MeasuredNodes[Index].PassIndex == PassIndex)
            {
                return Index;
            }
        }

        return -1;
    }
}

void EditorNodeGraph::RequestResetView()
{
    EditorNodeGraphPrivate::GRequestResetView = true;
    EditorNodeGraphPrivate::GRequestFitView   = false;
}

void EditorNodeGraph::RequestFitView(const ImVec2& WorldMin, const ImVec2& WorldMax)
{
    EditorNodeGraphPrivate::GRequestFitView   = true;
    EditorNodeGraphPrivate::GRequestResetView = false;
    EditorNodeGraphPrivate::GFitWorldMin      = WorldMin;
    EditorNodeGraphPrivate::GFitWorldMax      = WorldMax;
}

EditorNodeGraph::FNodeColors EditorNodeGraph::GetNodeColors(ENodeStyle Style)
{
    FNodeColors Colors;
    if (Style == ENodeStyle::Muted)
    {
        Colors.Title  = IM_COL32(42, 42, 46, 255);
        Colors.Body   = IM_COL32(32, 32, 35, 255);
        Colors.Border = IM_COL32(62, 62, 66, 255);
        Colors.Text   = IM_COL32(140, 140, 145, 255);
        return Colors;
    }

    Colors.Title  = IM_COL32(60, 60, 65, 255);
    Colors.Body   = IM_COL32(45, 45, 48, 255);
    Colors.Border = IM_COL32(90, 90, 95, 255);
    Colors.Text   = IM_COL32(235, 235, 235, 255);

    return Colors;
}

ImU32 EditorNodeGraph::GetPinColor(EPinKind Kind)
{
    return (Kind == EPinKind::Input) ? IM_COL32(100, 180, 255, 255) : IM_COL32(255, 180, 90, 255);
}

void EditorNodeGraph::BeginCanvas(const CHAR* Id, const ImVec2& Size, EStyleFlags StyleFlags)
{
    using namespace EditorNodeGraphPrivate;

    GCanvas                  = FCanvasState();
    GCanvas.bActive          = true;
    GCanvas.bNodesAboveLinks = (static_cast<uint8>(StyleFlags) & static_cast<uint8>(EStyleFlags::NodesAboveLinks)) != 0;

    ImGui::PushID(Id);
    ImGui::BeginChild(Id, Size, ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    GCanvas.DrawList     = ImGui::GetWindowDrawList();
    GCanvas.CanvasOrigin = ImGui::GetCursorScreenPos();
    GCanvas.CanvasSize   = ImGui::GetContentRegionAvail();

    if (GCanvas.CanvasSize.x < 1.0f)
    {
        GCanvas.CanvasSize.x = Size.x;
    }
    if (GCanvas.CanvasSize.y < 1.0f)
    {
        GCanvas.CanvasSize.y = Size.y;
    }

    ImGuiStorage* Storage = ImGui::GetStateStorage();
    const ImGuiID PanXId  = ImGui::GetID("PanX");
    const ImGuiID PanYId  = ImGui::GetID("PanY");
    const ImGuiID ZoomId  = ImGui::GetID("Zoom");

    if (GRequestFitView)
    {
        constexpr float Zoom = 1.0f;
        const ImVec2 Center((GFitWorldMin.x + GFitWorldMax.x) * 0.5f, (GFitWorldMin.y + GFitWorldMax.y) * 0.5f);

        Storage->SetFloat(PanXId, GCanvas.CanvasSize.x / (2.0f * Zoom) - Center.x);
        Storage->SetFloat(PanYId, GCanvas.CanvasSize.y / (2.0f * Zoom) - Center.y);
        Storage->SetFloat(ZoomId, Zoom);

        GRequestFitView = false;
    }
    else if (GRequestResetView)
    {
        Storage->SetFloat(PanXId, 40.0f);
        Storage->SetFloat(PanYId, 40.0f);
        Storage->SetFloat(ZoomId, 1.0f);

        GRequestResetView = false;
    }

    GCanvas.Pan.x = Storage->GetFloat(PanXId, 40.0f);
    GCanvas.Pan.y = Storage->GetFloat(PanYId, 40.0f);
    GCanvas.Zoom  = Math::Clamp(Storage->GetFloat(ZoomId, 1.0f), 0.25f, 2.5f);

    const ImVec2 CanvasMax = ImVec2(GCanvas.CanvasOrigin.x + GCanvas.CanvasSize.x, GCanvas.CanvasOrigin.y + GCanvas.CanvasSize.y);
    GCanvas.DrawList->AddRectFilled(GCanvas.CanvasOrigin, CanvasMax, IM_COL32(28, 28, 28, 255));

    DrawGrid();

    // Reserve space without owning mouse input so node hit-tests can win.
    ImGui::Dummy(GCanvas.CanvasSize);

    GCanvas.DrawList->PushClipRect(GCanvas.CanvasOrigin, CanvasMax, true);
    if (GCanvas.bNodesAboveLinks)
    {
        // Links use channel 0 while nodes and later overlays use channel 1.
        GCanvas.DrawList->ChannelsSplit(2);
        GCanvas.DrawList->ChannelsSetCurrent(1);
    }
}

void EditorNodeGraph::EndCanvas()
{
    using namespace EditorNodeGraphPrivate;

    // Submit the canvas interaction item after all nodes. A node under the mouse has already
    // claimed hover/active state, while blank canvas space is now owned by this item instead of
    // falling through to ImGui's window-move behavior.

    ImGui::SetCursorScreenPos(GCanvas.CanvasOrigin);
    ImGui::InvisibleButton("##CanvasInteract", GCanvas.CanvasSize,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);

    const bool bCanvasHovered = ImGui::IsItemHovered();
    if (bCanvasHovered && ImGui::GetIO().MouseWheel != 0.0f)
    {
        const ImVec2 Mouse  = ImGui::GetIO().MousePos;
        const ImVec2 Before = ScreenToWorld(Mouse);

        GCanvas.Zoom = Math::Clamp(GCanvas.Zoom + ImGui::GetIO().MouseWheel * 0.1f, 0.25f, 2.5f);

        const ImVec2 After  = ScreenToWorld(Mouse);
        GCanvas.Pan.x += After.x - Before.x;
        GCanvas.Pan.y += After.y - Before.y;
    }

    const bool bPanDrag = ImGui::IsItemActive() &&
        (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || (ImGui::GetIO().KeyAlt && ImGui::IsMouseDragging(ImGuiMouseButton_Left)));

    if (bPanDrag)
    {
        GCanvas.Pan.x += ImGui::GetIO().MouseDelta.x / GCanvas.Zoom;
        GCanvas.Pan.y += ImGui::GetIO().MouseDelta.y / GCanvas.Zoom;
    }

    ImGuiStorage* Storage = ImGui::GetStateStorage();
    Storage->SetFloat(ImGui::GetID("PanX"), GCanvas.Pan.x);
    Storage->SetFloat(ImGui::GetID("PanY"), GCanvas.Pan.y);
    Storage->SetFloat(ImGui::GetID("Zoom"), GCanvas.Zoom);

    if (GCanvas.DrawList)
    {
        if (GCanvas.bNodesAboveLinks)
        {
            GCanvas.DrawList->ChannelsMerge();
        }

        GCanvas.DrawList->PopClipRect();
    }

    ImGui::EndChild();
    ImGui::PopID();

    GCanvas = FCanvasState();
}

bool EditorNodeGraph::BeginNode(int32 NodeId, ImVec2* InOutPos, ENodeStyle Style)
{
    using namespace EditorNodeGraphPrivate;

    if (!GCanvas.bActive || !InOutPos)
    {
        return false;
    }

    GCanvas.CurrentNode             = FNodeState();
    GCanvas.CurrentNode.bActive     = true;
    GCanvas.CurrentNode.NodeId      = NodeId;
    GCanvas.CurrentNode.Style       = Style;
    GCanvas.CurrentNode.InOutPos    = InOutPos;
    GCanvas.CurrentNode.NodeWidth   = MinNodeWidthWorld * GCanvas.Zoom;
    GCanvas.CurrentNode.TitleHeight = 22.0f * GCanvas.Zoom;
    GCanvas.CurrentNode.ScreenMin   = WorldToScreen(*InOutPos);

    ImGui::PushID(NodeId);
    return true;
}

void EditorNodeGraph::NodeTitle(const CHAR* Title)
{
    using namespace EditorNodeGraphPrivate;

    if (!GCanvas.CurrentNode.bActive)
    {
        return;
    }

    GCanvas.CurrentNode.Title = Title ? Title : "";
}

void EditorNodeGraph::EndNode()
{
    using namespace EditorNodeGraphPrivate;

    if (!GCanvas.CurrentNode.bActive || !GCanvas.DrawList)
    {
        if (GCanvas.CurrentNode.bActive)
        {
            ImGui::PopID();
        }

        GCanvas.CurrentNode = FNodeState();
        return;
    }

    int32 InputCount  = 0;
    int32 OutputCount = 0;

    for (const FPendingPin& Pending : GCanvas.CurrentNode.PendingPins)
    {
        if (Pending.Kind == EditorNodeGraph::EPinKind::Input)
        {
            ++InputCount;
        }
        else
        {
            ++OutputCount;
        }
    }

    const float TitlePad = 16.0f * GCanvas.Zoom;
    const float PinPad   = 28.0f * GCanvas.Zoom;

    float ContentWidth = MinNodeWidthWorld * GCanvas.Zoom;
    ContentWidth = Math::Max(ContentWidth, ImGui::CalcTextSize(*GCanvas.CurrentNode.Title).x + TitlePad);
    for (const FPendingPin& Pending : GCanvas.CurrentNode.PendingPins)
    {
        if (Pending.Label)
        {
            ContentWidth = Math::Max(ContentWidth, ImGui::CalcTextSize(Pending.Label).x + PinPad);
        }
    }

    GCanvas.CurrentNode.NodeWidth = ContentWidth;

    // Every pin owns a row, inputs first and outputs below them, so labels never share a line.
    const float PinRowHeight = 18.0f * GCanvas.Zoom;
    const float GroupGap     = (InputCount > 0 && OutputCount > 0) ? PinGroupGapWorld * GCanvas.Zoom : 0.0f;
    const int32 PinRows      = InputCount + OutputCount;
    const float BodyHeight   = Math::Max(PinRows * PinRowHeight + GroupGap + 12.0f * GCanvas.Zoom, 28.0f * GCanvas.Zoom);
    const float NodeHeight   = GCanvas.CurrentNode.TitleHeight + BodyHeight;

    // Recompute screen min after width is known so drag uses the final rect.
    if (GCanvas.CurrentNode.InOutPos)
    {
        GCanvas.CurrentNode.ScreenMin = WorldToScreen(*GCanvas.CurrentNode.InOutPos);
    }

    const ImVec2 Min = GCanvas.CurrentNode.ScreenMin;
    const ImVec2 Max = ImVec2(Min.x + GCanvas.CurrentNode.NodeWidth, Min.y + NodeHeight);

    ImGui::SetCursorScreenPos(Min);
    ImGui::InvisibleButton("##NodeDrag", ImVec2(Max.x - Min.x, Max.y - Min.y));

    if (ImGui::IsItemHovered())
    {
        GCanvas.HoveredNodeId = GCanvas.CurrentNode.NodeId;
    }

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !ImGui::GetIO().KeyAlt)
    {
        GCanvas.ActiveNodeId = GCanvas.CurrentNode.NodeId;
        if (GCanvas.CurrentNode.InOutPos)
        {
            GCanvas.CurrentNode.InOutPos->x += ImGui::GetIO().MouseDelta.x / GCanvas.Zoom;
            GCanvas.CurrentNode.InOutPos->y += ImGui::GetIO().MouseDelta.y / GCanvas.Zoom;
            GCanvas.CurrentNode.ScreenMin = WorldToScreen(*GCanvas.CurrentNode.InOutPos);
        }
    }

    const ImVec2      DrawMin = GCanvas.CurrentNode.ScreenMin;
    const ImVec2      DrawMax = ImVec2(DrawMin.x + GCanvas.CurrentNode.NodeWidth, DrawMin.y + NodeHeight);
    const FNodeColors Colors  = GetNodeColors(GCanvas.CurrentNode.Style);

    GCanvas.DrawList->AddRectFilled(DrawMin, DrawMax, Colors.Body, 6.0f * GCanvas.Zoom);
    GCanvas.DrawList->AddRectFilled(DrawMin, ImVec2(DrawMax.x, DrawMin.y + GCanvas.CurrentNode.TitleHeight),
        Colors.Title, 6.0f * GCanvas.Zoom, ImDrawFlags_RoundCornersTop);

    GCanvas.DrawList->AddRect(DrawMin, DrawMax, Colors.Border, 6.0f * GCanvas.Zoom);

    if (!GCanvas.CurrentNode.Title.IsEmpty())
    {
        GCanvas.DrawList->AddText(ImVec2(DrawMin.x + 8.0f * GCanvas.Zoom, DrawMin.y + 3.0f * GCanvas.Zoom),
            Colors.Text, *GCanvas.CurrentNode.Title);
    }

    const ImU32 PinLabelColor = (GCanvas.CurrentNode.Style == ENodeStyle::Muted)
        ? IM_COL32(130, 130, 135, 255)
        : IM_COL32(210, 210, 210, 255);

    const float FirstRowY = DrawMin.y + GCanvas.CurrentNode.TitleHeight + 10.0f * GCanvas.Zoom;

    int32 Row = 0;
    for (const FPendingPin& Pending : GCanvas.CurrentNode.PendingPins)
    {
        if (Pending.Kind == EditorNodeGraph::EPinKind::Input)
        {
            DrawPinRow(Pending, DrawMin, DrawMax, FirstRowY + Row * PinRowHeight, PinLabelColor);
            ++Row;
        }
    }

    const float OutputStartY = FirstRowY + Row * PinRowHeight + GroupGap;
    if (GroupGap > 0.0f)
    {
        const float SeparatorY = OutputStartY - GroupGap * 0.5f - PinRowHeight * 0.5f;
        GCanvas.DrawList->AddLine(ImVec2(DrawMin.x + 6.0f * GCanvas.Zoom, SeparatorY),
            ImVec2(DrawMax.x - 6.0f * GCanvas.Zoom, SeparatorY), Colors.Border);
    }

    Row = 0;
    for (const FPendingPin& Pending : GCanvas.CurrentNode.PendingPins)
    {
        if (Pending.Kind == EditorNodeGraph::EPinKind::Output)
        {
            DrawPinRow(Pending, DrawMin, DrawMax, OutputStartY + Row * PinRowHeight, PinLabelColor);
            ++Row;
        }
    }

    ImGui::PopID();
    GCanvas.CurrentNode = FNodeState();
}

void EditorNodeGraph::Pin(int32 PinId, EPinKind Kind, const CHAR* Label)
{
    using namespace EditorNodeGraphPrivate;

    if (!GCanvas.CurrentNode.bActive)
    {
        return;
    }

    FPendingPin& Pending = GCanvas.CurrentNode.PendingPins.Emplace();
    Pending.PinId = PinId;
    Pending.Kind  = Kind;
    Pending.Label = Label;
}

void EditorNodeGraph::Link(int32 FromOutputPinId, int32 ToInputPinId)
{
    using namespace EditorNodeGraphPrivate;

    if (!GCanvas.DrawList)
    {
        return;
    }

    const FPinRecord* From = GCanvas.PinPositions.Find(FromOutputPinId);
    const FPinRecord* To   = GCanvas.PinPositions.Find(ToInputPinId);

    if (!From || !To || !From->bValid || !To->bValid)
    {
        return;
    }

    const ImVec2 P0 = From->Pos;
    const ImVec2 P3 = To->Pos;
    const float  Dx = Math::Max(40.0f * GCanvas.Zoom, Math::Abs(P3.x - P0.x) * 0.5f);
    const ImVec2 P1 = ImVec2(P0.x + Dx, P0.y);
    const ImVec2 P2 = ImVec2(P3.x - Dx, P3.y);

    if (GCanvas.bNodesAboveLinks)
    {
        GCanvas.DrawList->ChannelsSetCurrent(0);
    }

    GCanvas.DrawList->AddBezierCubic(P0, P1, P2, P3, IM_COL32(170, 170, 180, 200), 2.0f * GCanvas.Zoom);

    if (GCanvas.bNodesAboveLinks)
    {
        GCanvas.DrawList->ChannelsSetCurrent(1);
    }
}

void EditorNodeGraph::LayoutLeftToRight(TArrayView<ImVec2> Positions, float NodeSpacingX, float NodeSpacingY)
{
    for (int32 Index = 0; Index < Positions.Size(); ++Index)
    {
        Positions[Index] = ImVec2(static_cast<float>(Index) * NodeSpacingX, static_cast<float>(Index) * NodeSpacingY * 0.15f);
    }
}

void EditorNodeGraph::LayoutLayered(const FRenderGraphDebugSnapshot& Snapshot, TArrayView<const int32> VisiblePassIndices,
    TArrayView<const FNodeLayoutInput> MeasuredNodes, TArrayView<ImVec2> OutPositions, float ColumnGap, float RowGap)
{
    using namespace EditorNodeGraphPrivate;

    const int32 NumVisible = VisiblePassIndices.Size();
    if (NumVisible <= 0 || OutPositions.Size() < NumVisible)
    {
        return;
    }

    TMap<int32, int32> PassToLocal;
    PassToLocal.Reserve(NumVisible);

    for (int32 Local = 0; Local < NumVisible; ++Local)
    {
        PassToLocal.FindOrAdd(VisiblePassIndices[Local]) = Local;
        OutPositions[Local] = ImVec2(0.0f, 0.0f);
    }

    TArray<TArray<int32>> Successors;
    Successors.Resize(NumVisible);

    TArray<TArray<int32>> Predecessors;
    Predecessors.Resize(NumVisible);

    for (const FRenderGraphDebugLink& LinkEdge : Snapshot.Links)
    {
        const int32* FromLocal = PassToLocal.Find(LinkEdge.FromPass);
        const int32* ToLocal   = PassToLocal.Find(LinkEdge.ToPass);

        if (!FromLocal || !ToLocal || *FromLocal == *ToLocal)
        {
            continue;
        }

        Successors[*FromLocal].AddUnique(*ToLocal);
        Predecessors[*ToLocal].AddUnique(*FromLocal);
    }

    // Longest-path ranks from sources (rank 0) toward sinks.
    TArray<int32> Rank;
    Rank.Resize(NumVisible);

    for (int32 Index = 0; Index < NumVisible; ++Index)
    {
        Rank[Index] = 0;
    }

    bool bChanged = true;
    for (int32 Iter = 0; Iter < NumVisible && bChanged; ++Iter)
    {
        bChanged = false;
        for (int32 Local = 0; Local < NumVisible; ++Local)
        {
            for (const int32 Succ : Successors[Local])
            {
                const int32 Candidate = Rank[Local] + 1;
                if (Candidate > Rank[Succ])
                {
                    Rank[Succ] = Candidate;
                    bChanged   = true;
                }
            }
        }
    }

    // Orphan sources inherit the previous visible pass's rank so late culled/disabled
    // passes do not all collapse into column 0 ahead of the live pipeline.

    for (int32 Local = 0; Local < NumVisible; ++Local)
    {
        if (!Predecessors[Local].IsEmpty())
        {
            continue;
        }

        if (Local > 0)
        {
            Rank[Local] = Math::Max(Rank[Local], Rank[Local - 1]);
        }
    }

    int32 MaxRank = 0;
    for (int32 Local = 0; Local < NumVisible; ++Local)
    {
        MaxRank = Math::Max(MaxRank, Rank[Local]);
    }

    // Split multi-rank edges through dummy nodes so crossing reduction sees every layer hop.
    TArray<int32> LongEdgeFrom;
    TArray<int32> LongEdgeTo;

    for (int32 Local = 0; Local < NumVisible; ++Local)
    {
        for (const int32 Succ : Successors[Local])
        {
            if (Rank[Succ] - Rank[Local] > 1)
            {
                LongEdgeFrom.Add(Local);
                LongEdgeTo.Add(Succ);
            }
        }
    }

    for (int32 EdgeIndex = 0; EdgeIndex < LongEdgeFrom.Size(); ++EdgeIndex)
    {
        const int32 From = LongEdgeFrom[EdgeIndex];
        const int32 To   = LongEdgeTo[EdgeIndex];

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

    for (int32 Local = 0; Local < NumLayoutNodes; ++Local)
    {
        Columns[Rank[Local]].Add(Local);
    }

    auto PassOrderLess = [&](int32 A, int32 B) -> bool
    {
        const bool bAReal = A < NumVisible;
        const bool bBReal = B < NumVisible;

        if (bAReal && bBReal)
        {
            return VisiblePassIndices[A] < VisiblePassIndices[B];
        }

        if (bAReal != bBReal)
        {
            return bAReal;
        }

        return A < B;
    };

    // Seed each column by builder submission order, not local index.
    for (int32 Column = 0; Column < Columns.Size(); ++Column)
    {
        Columns[Column].SortWithPredicate(PassOrderLess);
    }

    auto OrderIndexInColumn = [&](int32 Local) -> int32
    {
        const TArray<int32>& Order = Columns[Rank[Local]];
        for (int32 OrderIndex = 0; OrderIndex < Order.Size(); ++OrderIndex)
        {
            if (Order[OrderIndex] == Local)
            {
                return OrderIndex;
            }
        }

        return 0;
    };

    auto Barycenter = [&](int32 Local, bool bUsePredecessors) -> float
    {
        const TArray<int32>& Neighbors = bUsePredecessors ? Predecessors[Local] : Successors[Local];
        if (Neighbors.IsEmpty())
        {
            return static_cast<float>(OrderIndexInColumn(Local));
        }

        float Sum   = 0.0f;
        int32 Count = 0;

        for (const int32 Neighbor : Neighbors)
        {
            Sum += static_cast<float>(OrderIndexInColumn(Neighbor));
            ++Count;
        }

        return (Count > 0) ? (Sum / static_cast<float>(Count)) : static_cast<float>(OrderIndexInColumn(Local));
    };

    auto CountCrossings = [&](int32 LeftColumn, int32 RightColumn) -> int32
    {
        const TArray<int32>& LeftOrder  = Columns[LeftColumn];
        const TArray<int32>& RightOrder = Columns[RightColumn];

        TArray<int32> LeftPos;
        LeftPos.Resize(NumLayoutNodes);

        TArray<int32> RightPos;
        RightPos.Resize(NumLayoutNodes);

        for (int32 Index = 0; Index < LeftOrder.Size(); ++Index)
        {
            LeftPos[LeftOrder[Index]] = Index;
        }

        for (int32 Index = 0; Index < RightOrder.Size(); ++Index)
        {
            RightPos[RightOrder[Index]] = Index;
        }

        TArray<int32> EdgeLeft;
        TArray<int32> EdgeRight;

        for (const int32 From : LeftOrder)
        {
            for (const int32 To : Successors[From])
            {
                if (Rank[To] == RightColumn)
                {
                    EdgeLeft.Add(LeftPos[From]);
                    EdgeRight.Add(RightPos[To]);
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
                    ++Crossings;
                }
            }
        }
        return Crossings;
    };

    auto Transpose = [&](int32 Column, int32 FixedNeighborColumn, bool bNeighborIsLeft)
    {
        TArray<int32>& Order = Columns[Column];

        bool bImproved = true;
        for (int32 Pass = 0; Pass < 8 && bImproved; ++Pass)
        {
            bImproved = false;
            for (int32 Index = 0; Index + 1 < Order.Size(); ++Index)
            {
                const int32 LeftColumn  = bNeighborIsLeft ? FixedNeighborColumn : Column;
                const int32 RightColumn = bNeighborIsLeft ? Column : FixedNeighborColumn;
                const int32 Before      = CountCrossings(LeftColumn, RightColumn);

                Order.Swap(Index, Index + 1);

                const int32 After = CountCrossings(LeftColumn, RightColumn);
                if (After < Before)
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

    for (int32 Sweep = 0; Sweep < 4; ++Sweep)
    {
        for (int32 Column = 1; Column < Columns.Size(); ++Column)
        {
            TArray<int32>& Order = Columns[Column];
            Order.SortWithPredicate([&](int32 A, int32 B)
            {
                const float Ba = Barycenter(A, true);
                const float Bb = Barycenter(B, true);

                if (Ba == Bb)
                {
                    return PassOrderLess(A, B);
                }

                return Ba < Bb;
            });

            Transpose(Column, Column - 1, true);
        }

        for (int32 Column = Columns.Size() - 2; Column >= 0; --Column)
        {
            TArray<int32>& Order = Columns[Column];
            Order.SortWithPredicate([&](int32 A, int32 B)
            {
                const float Ba = Barycenter(A, false);
                const float Bb = Barycenter(B, false);

                if (Ba == Bb)
                {
                    return PassOrderLess(A, B);
                }

                return Ba < Bb;
            });

            Transpose(Column, Column + 1, false);
        }
    }

    auto NodeHeight = [&](int32 Local) -> float
    {
        if (Local >= NumVisible)
        {
            return 0.0f;
        }

        const int32 PassIndex = VisiblePassIndices[Local];
        const int32 Measured  = FindMeasuredIndex(MeasuredNodes, PassIndex);

        return (Measured >= 0) ? MeasuredNodes[Measured].Height : 80.0f;
    };

    TArray<float> ColumnWidths;
    ColumnWidths.Resize(Columns.Size());

    for (int32 Column = 0; Column < Columns.Size(); ++Column)
    {
        float Width = MinNodeWidthWorld;
        for (const int32 Local : Columns[Column])
        {
            if (Local >= NumVisible)
            {
                continue;
            }

            const int32 PassIndex = VisiblePassIndices[Local];
            const int32 Measured  = FindMeasuredIndex(MeasuredNodes, PassIndex);

            if (Measured >= 0)
            {
                Width = Math::Max(Width, MeasuredNodes[Measured].Width);
            }
        }

        ColumnWidths[Column] = Width;
    }

    TArray<float> NodeY;
    NodeY.Resize(NumLayoutNodes);

    for (int32 Column = 0; Column < Columns.Size(); ++Column)
    {
        float CursorY = 0.0f;
        for (const int32 Local : Columns[Column])
        {
            NodeY[Local] = CursorY;
            CursorY += NodeHeight(Local) + RowGap;
        }
    }

    auto NeighborAverageY = [&](int32 Local) -> float
    {
        float Sum   = 0.0f;
        int32 Count = 0;

        for (const int32 Neighbor : Predecessors[Local])
        {
            if (Rank[Neighbor] == Rank[Local] - 1)
            {
                Sum += NodeY[Neighbor];
                ++Count;
            }
        }

        for (const int32 Neighbor : Successors[Local])
        {
            if (Rank[Neighbor] == Rank[Local] + 1)
            {
                Sum += NodeY[Neighbor];
                ++Count;
            }
        }

        return (Count > 0) ? (Sum / static_cast<float>(Count)) : NodeY[Local];
    };

    auto EnforceNonOverlap = [&](TArrayView<const int32> Order)
    {
        if (Order.IsEmpty())
        {
            return;
        }

        float MinY = NodeY[Order[0]];
        for (int32 Index = 1; Index < Order.Size(); ++Index)
        {
            const int32 Previous = Order[Index - 1];
            const int32 Current  = Order[Index];
            const float FloorY   = NodeY[Previous] + NodeHeight(Previous) + RowGap;

            MinY = Math::Max(NodeY[Current], FloorY);
            NodeY[Current] = MinY;
        }
    };

    for (int32 Sweep = 0; Sweep < 4; ++Sweep)
    {
        for (int32 Column = 0; Column < Columns.Size(); ++Column)
        {
            for (const int32 Local : Columns[Column])
            {
                NodeY[Local] = NeighborAverageY(Local);
            }

            EnforceNonOverlap(MakeArrayView(Columns[Column]));
        }
    }

    float X = 0.0f;
    for (int32 Column = 0; Column < Columns.Size(); ++Column)
    {
        for (const int32 Local : Columns[Column])
        {
            if (Local < NumVisible)
            {
                OutPositions[Local] = ImVec2(X, NodeY[Local]);
            }
        }

        X += ColumnWidths[Column] + ColumnGap;
    }
}
