#include "Engine/EngineUI/Editor/EditorRenderGraphWidget.h"
#include "Engine/EngineUI/Editor/EditorNodeGraph.h"
#include "Core/Math/Math.h"
#include "ImGuiPlugin/ImGuiCore.h"
#include "RendererCore/Interfaces/IRendererModule.h"

static constexpr int32 LoadPinBit = 1 << 30;

static int32 MakePinId(int32 PassIndex, int32 AccessIndex)
{
    return (PassIndex << 16) | AccessIndex;
}

static void DrawCanvasLegend()
{
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    if (!DrawList)
    {
        return;
    }

    constexpr float Padding     = 8.0f;
    constexpr float SwatchWidth = 14.0f;
    const float RowHeight       = ImGui::GetTextLineHeightWithSpacing();
    const float BoxWidth        = 286.0f;
    const float BoxHeight       = Padding * 2.0f + RowHeight * 4.0f;

    const ImVec2 WindowPos  = ImGui::GetWindowPos();
    const ImVec2 ContentMin = ImGui::GetWindowContentRegionMin();
    const ImVec2 ContentMax = ImGui::GetWindowContentRegionMax();
    const ImVec2 BoxMin     = ImVec2(WindowPos.x + ContentMin.x + 10.0f, WindowPos.y + ContentMax.y - 10.0f - BoxHeight);
    const ImVec2 BoxMax     = ImVec2(BoxMin.x + BoxWidth, BoxMin.y + BoxHeight);

    DrawList->AddRectFilled(BoxMin, BoxMax, IM_COL32(20, 20, 22, 225), 5.0f);
    DrawList->AddRect(BoxMin, BoxMax, IM_COL32(90, 90, 95, 255), 5.0f);

    const CHAR* Labels[] =
    {
        "Resource read (input)",
        "Resource write (output)",
        "Raster / Compute / Copy pass",
        "Culled or disabled pass",
    };

    const EditorNodeGraph::FNodeColors NormalNode = EditorNodeGraph::GetNodeColors(EditorNodeGraph::ENodeStyle::Normal);
    const EditorNodeGraph::FNodeColors MutedNode  = EditorNodeGraph::GetNodeColors(EditorNodeGraph::ENodeStyle::Muted);

    const ImU32 Colors[] =
    {
        EditorNodeGraph::GetPinColor(EditorNodeGraph::EPinKind::Input),
        EditorNodeGraph::GetPinColor(EditorNodeGraph::EPinKind::Output),
        NormalNode.Title,
        MutedNode.Title,
    };

    const ImU32 Outlines[] =
    {
        0,
        0,
        NormalNode.Border,
        MutedNode.Border,
    };

    for (int32 Index = 0; Index < 4; ++Index)
    {
        const float  CenterY   = BoxMin.y + Padding + RowHeight * (static_cast<float>(Index) + 0.5f);
        const ImVec2 SwatchMin = ImVec2(BoxMin.x + Padding, CenterY - 4.0f);
        const ImVec2 SwatchMax = ImVec2(SwatchMin.x + SwatchWidth, CenterY + 4.0f);

        if (Index < 2)
        {
            DrawList->AddCircleFilled(ImVec2(SwatchMin.x + SwatchWidth * 0.5f, CenterY), 4.5f, Colors[Index]);
        }
        else
        {
            DrawList->AddRectFilled(SwatchMin, SwatchMax, Colors[Index], 2.0f);
            DrawList->AddRect(SwatchMin, SwatchMax, Outlines[Index], 2.0f);
        }

        DrawList->AddText(ImVec2(BoxMin.x + Padding + SwatchWidth + 6.0f, CenterY - ImGui::GetTextLineHeight() * 0.5f), IM_COL32(220, 220, 220, 255), Labels[Index]);
    }
}

FEditorRenderGraphWidget::FEditorRenderGraphWidget()
    : Snapshot()
    , PositionsByPassKey()
    , NodePositions()
    , ImGuiDelegateHandle()
    , bVisible(false)
    , bShowCulled(true)
    , bForceAutoLayout(false)
    , bNeedsFitView(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorRenderGraphWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorRenderGraphWidget::~FEditorRenderGraphWidget()
{
    SetVisible(false);

    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorRenderGraphWidget::SetVisible(bool bInVisible)
{
    if (bVisible == bInVisible)
    {
        return;
    }

    bVisible = bInVisible;
    if (bVisible)
    {
        bNeedsFitView = true;
    }

    SyncCaptureEnabled();
}

void FEditorRenderGraphWidget::SyncCaptureEnabled() const
{
#if EDITOR_BUILD
    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        RendererModule->SetRenderGraphDebugCaptureEnabled(bVisible);
    }
#endif
}

void FEditorRenderGraphWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    DrawWindow();
}

void FEditorRenderGraphWidget::BuildVisiblePassIndices(TArray<int32>& OutVisiblePassIndices) const
{
    OutVisiblePassIndices.Clear();
    OutVisiblePassIndices.Reserve(Snapshot.Passes.Size());

    for (int32 PassIndex = 0; PassIndex < Snapshot.Passes.Size(); ++PassIndex)
    {
        const FRenderGraphDebugPass& Pass = Snapshot.Passes[PassIndex];
        if (!bShowCulled && (Pass.bCulled || !Pass.bEnabled))
        {
            continue;
        }

        OutVisiblePassIndices.Add(PassIndex);
    }
}

bool FEditorRenderGraphWidget::HasIncomingLink(int32 PassIndex, int32 AccessIndex) const
{
    for (const FRenderGraphDebugLink& Link : Snapshot.Links)
    {
        if (Link.ToPass == PassIndex && Link.ToAccess == AccessIndex)
        {
            return true;
        }
    }

    return false;
}

void FEditorRenderGraphWidget::EstimateNodeSizes(TArrayView<const int32> VisiblePassIndices, TArray<EditorNodeGraph::FNodeLayoutInput>& OutMeasured) const
{
    OutMeasured.Clear();
    OutMeasured.Reserve(VisiblePassIndices.Size());

    constexpr float MinWidth     = 160.0f;
    constexpr float TitlePad     = 16.0f;
    constexpr float PinPad       = 28.0f;
    constexpr float TitleHeight  = 22.0f;
    constexpr float PinRowHeight = 18.0f;
    constexpr float PinGroupGap  = 8.0f;

    for (const int32 PassIndex : VisiblePassIndices)
    {
        const FRenderGraphDebugPass& Pass = Snapshot.Passes[PassIndex];

        String Title = Pass.Name;
        if (IsEnumFlagSet(Pass.Flags, ERenderGraphPassFlags::Raster))
        {
            Title.Append(" [Raster]");
        }
        else if (IsEnumFlagSet(Pass.Flags, ERenderGraphPassFlags::Compute))
        {
            Title.Append(" [Compute]");
        }
        else if (IsEnumFlagSet(Pass.Flags, ERenderGraphPassFlags::Copy))
        {
            Title.Append(" [Copy]");
        }

        if (Pass.bCulled)
        {
            Title.Append(" (Culled)");
        }
        else if (!Pass.bEnabled)
        {
            Title.Append(" (Disabled)");
        }

        float Width = MinWidth;
        Width = Math::Max(Width, ImGui::CalcTextSize(*Title).x + TitlePad);

        int32 InputCount  = 0;
        int32 OutputCount = 0;

        for (int32 AccessIndex = 0; AccessIndex < Pass.Accesses.Size(); ++AccessIndex)
        {
            const FRenderGraphDebugAccess& Access = Pass.Accesses[AccessIndex];
            if (Access.ResourceIndex < 0 || Access.ResourceIndex >= Snapshot.Resources.Size())
            {
                continue;
            }

            const FRenderGraphDebugResource& Resource = Snapshot.Resources[Access.ResourceIndex];
            Width = Math::Max(Width, ImGui::CalcTextSize(*Resource.Name).x + PinPad);
            if (Access.bIsWrite)
            {
                ++OutputCount;

                // A write that something links into also gets a load pin on the input side.
                if (HasIncomingLink(PassIndex, AccessIndex))
                {
                    ++InputCount;
                }
            }
            else
            {
                ++InputCount;
            }
        }

        const int32 PinRows  = InputCount + OutputCount;
        const float GroupGap = (InputCount > 0 && OutputCount > 0) ? PinGroupGap : 0.0f;
        const float Height   = TitleHeight + Math::Max(PinRows * PinRowHeight + GroupGap + 12.0f, 28.0f);

        EditorNodeGraph::FNodeLayoutInput& Measured = OutMeasured.Emplace();
        Measured.PassIndex = PassIndex;
        Measured.Width     = Width;
        Measured.Height    = Height;
    }
}

bool FEditorRenderGraphWidget::TryRequestFitViewToNodes()
{
    if (!bNeedsFitView)
    {
        return false;
    }

    TArray<int32> VisiblePassIndices;
    BuildVisiblePassIndices(VisiblePassIndices);
    if (VisiblePassIndices.IsEmpty() || NodePositions.IsEmpty())
    {
        return false;
    }

    TArray<EditorNodeGraph::FNodeLayoutInput> Measured;
    EstimateNodeSizes(MakeArrayView(VisiblePassIndices), Measured);

    ImVec2 WorldMin(FLT_MAX, FLT_MAX);
    ImVec2 WorldMax(-FLT_MAX, -FLT_MAX);
    bool   bHasBounds = false;

    for (const EditorNodeGraph::FNodeLayoutInput& Node : Measured)
    {
        if (Node.PassIndex < 0 || Node.PassIndex >= NodePositions.Size())
        {
            continue;
        }

        const ImVec2& Pos = NodePositions[Node.PassIndex];
        WorldMin.x = Math::Min(WorldMin.x, Pos.x);
        WorldMin.y = Math::Min(WorldMin.y, Pos.y);
        WorldMax.x = Math::Max(WorldMax.x, Pos.x + Node.Width);
        WorldMax.y = Math::Max(WorldMax.y, Pos.y + Node.Height);
        bHasBounds = true;
    }

    if (!bHasBounds)
    {
        return false;
    }

    EditorNodeGraph::RequestFitView(WorldMin, WorldMax);
    bNeedsFitView = false;
    return true;
}

void FEditorRenderGraphWidget::EnsureNodeLayout(bool bForce)
{
    TArray<int32> VisiblePassIndices;
    BuildVisiblePassIndices(VisiblePassIndices);

    bool bNeedsLayout = bForce || PositionsByPassKey.IsEmpty();
    if (!bNeedsLayout)
    {
        for (const int32 PassIndex : VisiblePassIndices)
        {
            if (!PositionsByPassKey.Contains(Snapshot.Passes[PassIndex].Name))
            {
                bNeedsLayout = true;
                break;
            }
        }
    }

    NodePositions.Clear();
    NodePositions.Resize(Snapshot.Passes.Size());

    if (bNeedsLayout)
    {
        TArray<EditorNodeGraph::FNodeLayoutInput> Measured;
        EstimateNodeSizes(MakeArrayView(VisiblePassIndices), Measured);

        TArray<ImVec2> LaidOut;
        LaidOut.Resize(VisiblePassIndices.Size());

        EditorNodeGraph::LayoutLayered(Snapshot, MakeArrayView(VisiblePassIndices),
            MakeArrayView(Measured), MakeArrayView(LaidOut));

        if (bForce)
        {
            PositionsByPassKey.Clear();
        }

        for (int32 VisibleIndex = 0; VisibleIndex < VisiblePassIndices.Size(); ++VisibleIndex)
        {
            const int32 PassIndex = VisiblePassIndices[VisibleIndex];
            PositionsByPassKey.FindOrAdd(Snapshot.Passes[PassIndex].Name) = LaidOut[VisibleIndex];
        }
    }

    for (int32 PassIndex = 0; PassIndex < Snapshot.Passes.Size(); ++PassIndex)
    {
        if (const ImVec2* Stored = PositionsByPassKey.Find(Snapshot.Passes[PassIndex].Name))
        {
            NodePositions[PassIndex] = *Stored;
        }
        else
        {
            NodePositions[PassIndex] = ImVec2(0.0f, static_cast<float>(PassIndex) * 100.0f);
        }
    }
}

void FEditorRenderGraphWidget::DrawWindow()
{
    const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing;
    if (!ImGui::Begin("Render Graph", &bVisible, Flags))
    {
        ImGui::End();
        if (!bVisible)
        {
            SyncCaptureEnabled();
        }

        return;
    }

    if (!bVisible)
    {
        SyncCaptureEnabled();
        ImGui::End();
        return;
    }

#if EDITOR_BUILD
    IRendererModule* RendererModule = IRendererModule::Get();
    if (RendererModule)
    {
        RendererModule->CopyLatestRenderGraphDebugSnapshot(Snapshot);
    }
#endif

    ImGui::Text("Graph: %s", Snapshot.GraphName.IsEmpty() ? "(none)" : *Snapshot.GraphName);
    ImGui::SameLine();

    ImGui::TextDisabled("| Passes: %d  Culled: %d  Disabled: %d  Textures: %d  Buffers: %d",
        Snapshot.Statistics.NumPasses, Snapshot.Statistics.NumCulledPasses, Snapshot.Statistics.NumDisabledPasses,
        Snapshot.Statistics.NumTexturesAllocated, Snapshot.Statistics.NumBuffersAllocated);

    ImGui::Checkbox("Show culled/disabled", &bShowCulled);
    ImGui::SameLine();

    if (ImGui::Button("Auto Layout"))
    {
        bForceAutoLayout = true;
        bNeedsFitView    = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset View"))
    {
        bNeedsFitView = true;
    }

    const bool bRunAutoLayout = bForceAutoLayout;
    bForceAutoLayout = false;

    EnsureNodeLayout(bRunAutoLayout);
    TryRequestFitViewToNodes();

    const ImVec2 CanvasSize = ImGui::GetContentRegionAvail();
    EditorNodeGraph::BeginCanvas("##RenderGraphCanvas", CanvasSize);

    for (int32 PassIndex = 0; PassIndex < Snapshot.Passes.Size(); ++PassIndex)
    {
        const FRenderGraphDebugPass& Pass = Snapshot.Passes[PassIndex];
        if (!bShowCulled && (Pass.bCulled || !Pass.bEnabled))
        {
            continue;
        }

        if (PassIndex >= NodePositions.Size())
        {
            break;
        }

        const bool bIsInactive = Pass.bCulled || !Pass.bEnabled;
        const EditorNodeGraph::ENodeStyle NodeStyle = bIsInactive ? EditorNodeGraph::ENodeStyle::Muted : EditorNodeGraph::ENodeStyle::Normal;

        if (!EditorNodeGraph::BeginNode(PassIndex, &NodePositions[PassIndex], NodeStyle))
        {
            continue;
        }

        String Title = Pass.Name;
        if (IsEnumFlagSet(Pass.Flags, ERenderGraphPassFlags::Raster))
        {
            Title.Append(" [Raster]");
        }
        else if (IsEnumFlagSet(Pass.Flags, ERenderGraphPassFlags::Compute))
        {
            Title.Append(" [Compute]");
        }
        else if (IsEnumFlagSet(Pass.Flags, ERenderGraphPassFlags::Copy))
        {
            Title.Append(" [Copy]");
        }

        if (Pass.bCulled)
        {
            Title.Append(" (Culled)");
        }
        else if (!Pass.bEnabled)
        {
            Title.Append(" (Disabled)");
        }

        EditorNodeGraph::NodeTitle(*Title);

        for (int32 AccessIndex = 0; AccessIndex < Pass.Accesses.Size(); ++AccessIndex)
        {
            const FRenderGraphDebugAccess& Access = Pass.Accesses[AccessIndex];
            if (Access.ResourceIndex < 0 || Access.ResourceIndex >= Snapshot.Resources.Size())
            {
                continue;
            }

            const FRenderGraphDebugResource& Resource = Snapshot.Resources[Access.ResourceIndex];
            const int32 PinId = MakePinId(PassIndex, AccessIndex);

            // A pass that loads what an earlier pass wrote needs somewhere on the input side for that
            // dependency to land, since the access itself is a write and only owns an output pin.
            if (Access.bIsWrite && HasIncomingLink(PassIndex, AccessIndex))
            {
                EditorNodeGraph::Pin(PinId | LoadPinBit, EditorNodeGraph::EPinKind::Input, *Resource.Name);
            }

            EditorNodeGraph::Pin(PinId, Access.bIsWrite ? EditorNodeGraph::EPinKind::Output : EditorNodeGraph::EPinKind::Input, *Resource.Name);
        }

        EditorNodeGraph::EndNode();

        PositionsByPassKey.FindOrAdd(Pass.Name) = NodePositions[PassIndex];
    }

    for (const FRenderGraphDebugLink& Link : Snapshot.Links)
    {
        if (Link.FromPass < 0 || Link.ToPass < 0 || Link.FromPass >= Snapshot.Passes.Size() || Link.ToPass >= Snapshot.Passes.Size())
        {
            continue;
        }

        const FRenderGraphDebugPass& FromPass = Snapshot.Passes[Link.FromPass];
        const FRenderGraphDebugPass& ToPass   = Snapshot.Passes[Link.ToPass];

        if (!bShowCulled && ((FromPass.bCulled || !FromPass.bEnabled) || (ToPass.bCulled || !ToPass.bEnabled)))
        {
            continue;
        }

        if (Link.ToAccess < 0 || Link.ToAccess >= ToPass.Accesses.Size())
        {
            continue;
        }

        int32 ToPinId = MakePinId(Link.ToPass, Link.ToAccess);
        if (ToPass.Accesses[Link.ToAccess].bIsWrite)
        {
            ToPinId |= LoadPinBit;
        }

        EditorNodeGraph::Link(MakePinId(Link.FromPass, Link.FromAccess), ToPinId);
    }

    DrawCanvasLegend();
    EditorNodeGraph::EndCanvas();
    ImGui::End();
}
