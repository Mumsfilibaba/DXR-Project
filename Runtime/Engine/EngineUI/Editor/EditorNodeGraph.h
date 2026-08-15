#pragma once
#include "Core/Containers/ArrayView.h"
#include "ImGuiPlugin/ImGuiCore.h"
#include "RendererCore/Debug/RenderGraphDebug.h"

struct ENGINE_API EditorNodeGraph
{
    enum class EPinKind : uint8
    {
        Input,
        Output
    };

    enum class EStyleFlags : uint8
    {
        None            = 0,
        NodesAboveLinks = 1 << 0
    };

    enum class ENodeStyle : uint8
    {
        Normal,

        /** Drawn dimmer, for nodes that describe work the graph will not run */
        Muted
    };

    struct FNodeColors
    {
        ImU32 Title  = 0;
        ImU32 Body   = 0;
        ImU32 Border = 0;
        ImU32 Text   = 0;
    };

    struct FNodeLayoutInput
    {
        int32 PassIndex = -1;
        float Width     = 0.0f;
        float Height    = 0.0f;
    };

    static void BeginCanvas(const CHAR* Id, const ImVec2& Size, EStyleFlags StyleFlags = EStyleFlags::NodesAboveLinks);
    static void EndCanvas();
    static void RequestResetView();
    static void RequestFitView(const ImVec2& WorldMin, const ImVec2& WorldMax);

    static bool BeginNode(int32 NodeId, ImVec2* InOutPos, ENodeStyle Style = ENodeStyle::Normal);
    static void NodeTitle(const CHAR* Title);
    static void EndNode();

    static void Pin(int32 PinId, EPinKind Kind, const CHAR* Label);
    static void Link(int32 FromOutputPinId, int32 ToInputPinId);

    static FNodeColors GetNodeColors(ENodeStyle Style);
    static ImU32       GetPinColor(EPinKind Kind);

    static void LayoutLeftToRight(TArrayView<ImVec2> Positions, float NodeSpacingX, float NodeSpacingY);
    static void LayoutLayered(const FRenderGraphDebugSnapshot& Snapshot, TArrayView<const int32> VisiblePassIndices,
        TArrayView<const FNodeLayoutInput> MeasuredNodes, TArrayView<ImVec2> OutPositions, float ColumnGap = 80.0f, float RowGap = 24.0f);
};
