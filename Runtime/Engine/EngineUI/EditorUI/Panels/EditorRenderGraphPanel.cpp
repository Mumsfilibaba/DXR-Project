#include "Engine/EngineUI/EditorUI/Panels/EditorRenderGraphPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/Overlay.h"
#include "Application/Elements/Spacer.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Graph/GraphCanvas.h"
#include "Application/Graph/GraphModel.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"
#include "RendererCore/Interfaces/IRendererModule.h"

// How far apart the header row holds its controls
constexpr int32 RENDERGRAPH_HEADER_SPACING = 8;

// The inset the header row is held off the panel edge by
constexpr int32 RENDERGRAPH_HEADER_INSET = 6;

// The legend box, at the sizes the ImGui window draws it at
constexpr int32 RENDERGRAPH_LEGEND_WIDTH         = 286;
constexpr int32 RENDERGRAPH_LEGEND_PADDING       = 8;
constexpr int32 RENDERGRAPH_LEGEND_SWATCH_WIDTH  = 14;
constexpr int32 RENDERGRAPH_LEGEND_SWATCH_HEIGHT = 8;
constexpr int32 RENDERGRAPH_LEGEND_LABEL_GAP     = 6;
constexpr int32 RENDERGRAPH_LEGEND_INSET         = 10;
constexpr int32 RENDERGRAPH_LEGEND_ROWS          = 4;
constexpr int32 RENDERGRAPH_LEGEND_ROW_SPACING   = 4;
constexpr float RENDERGRAPH_LEGEND_CIRCLE_RADIUS = 4.5f;
constexpr float RENDERGRAPH_LEGEND_CORNER_RADIUS = 5.0f;
constexpr float RENDERGRAPH_LEGEND_SWATCH_RADIUS = 2.0f;

static FFloatColor MakeGraphColor(int32 Red, int32 Green, int32 Blue, int32 Alpha = 255)
{
    constexpr float Scale = 1.0f / 255.0f;
    return FFloatColor(static_cast<float>(Red) * Scale, static_cast<float>(Green) * Scale,
        static_cast<float>(Blue) * Scale, static_cast<float>(Alpha) * Scale);
}

static void ResizeToUnassigned(TArray<int32>& Slots, int32 Size)
{
    Slots.Clear();
    Slots.Reserve(Size);

    for (int32 Index = 0; Index < Size; ++Index)
    {
        Slots.Add(-1);
    }
}

namespace RenderGraphColors
{
    static const FFloatColor NodeTitle   = MakeGraphColor(60, 60, 65);
    static const FFloatColor NodeBody    = MakeGraphColor(45, 45, 48);
    static const FFloatColor NodeBorder  = MakeGraphColor(90, 90, 95);
    static const FFloatColor NodeText    = MakeGraphColor(235, 235, 235);

    static const FFloatColor MutedTitle  = MakeGraphColor(42, 42, 46);
    static const FFloatColor MutedBody   = MakeGraphColor(32, 32, 35);
    static const FFloatColor MutedBorder = MakeGraphColor(62, 62, 66);
    static const FFloatColor MutedText   = MakeGraphColor(140, 140, 145);

    static const FFloatColor InputPin    = MakeGraphColor(100, 180, 255);
    static const FFloatColor OutputPin   = MakeGraphColor(255, 180, 90);
    static const FFloatColor PinOutline  = MakeGraphColor(20, 20, 20);

    static const FFloatColor Background  = MakeGraphColor(28, 28, 28);
    static const FFloatColor Link        = MakeGraphColor(170, 170, 180, 200);

    static const FFloatColor LegendFill   = MakeGraphColor(20, 20, 22, 225);
    static const FFloatColor LegendBorder = MakeGraphColor(90, 90, 95);
    static const FFloatColor LegendText   = MakeGraphColor(220, 220, 220);
}

class FRenderGraphLegend final : public FVisualElement
{
public:
    static TSharedPtr<FRenderGraphLegend> Create(const TSharedPtr<IFontFace>& InFont)
    {
        TSharedPtr<FRenderGraphLegend> NewLegend = MakeSharedPtr<FRenderGraphLegend>();
        NewLegend->Font = InFont;
        return NewLegend;
    }

    FRenderGraphLegend()
        : Font(nullptr)
    {
    }

    virtual ~FRenderGraphLegend() = default;

    virtual IntVector2 ComputeDesiredSize() const override final
    {
        return IntVector2(RENDERGRAPH_LEGEND_WIDTH, (RENDERGRAPH_LEGEND_PADDING * 2) + (GetRowHeight() * RENDERGRAPH_LEGEND_ROWS));
    }

    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override final
    {
        static const CHAR* const Labels[RENDERGRAPH_LEGEND_ROWS] =
        {
            "Resource read (input)",
            "Resource write (output)",
            "Raster / Compute / Copy pass",
            "Culled or disabled pass",
        };

        const FFloatColor Swatches[RENDERGRAPH_LEGEND_ROWS] =
        {
            RenderGraphColors::InputPin,
            RenderGraphColors::OutputPin,
            RenderGraphColors::NodeTitle,
            RenderGraphColors::MutedTitle,
        };

        const FFloatColor SwatchOutlines[] =
        {
            RenderGraphColors::NodeBorder,
            RenderGraphColors::MutedBorder,
        };

        const FRectangle&  Bounds  = AllottedGeometry.Bounds;
        const FCornerRadii Corners(RENDERGRAPH_LEGEND_CORNER_RADIUS);

        OutCommandList.AddBox(LayerId, Bounds, RenderGraphColors::LegendFill, Corners);
        OutCommandList.AddBoxOutline(LayerId, Bounds, RenderGraphColors::LegendBorder, 1.0f, Corners);

        if (!Font)
        {
            return LayerId;
        }

        const int32 RowHeight  = GetRowHeight();
        const int32 LineHeight = Font->GetLineHeight();
        const int32 SwatchLeft = Bounds.Position.X + RENDERGRAPH_LEGEND_PADDING;
        const int32 LabelLeft  = SwatchLeft + RENDERGRAPH_LEGEND_SWATCH_WIDTH + RENDERGRAPH_LEGEND_LABEL_GAP;

        for (int32 Row = 0; Row < RENDERGRAPH_LEGEND_ROWS; ++Row)
        {
            const int32 CenterY = Bounds.Position.Y + RENDERGRAPH_LEGEND_PADDING + (RowHeight * Row) + (RowHeight / 2);
            if (Row < 2)
            {
                const Vector2 Center(static_cast<float>(SwatchLeft) + (static_cast<float>(RENDERGRAPH_LEGEND_SWATCH_WIDTH) * 0.5f), static_cast<float>(CenterY));
                OutCommandList.AddCircleFilled(LayerId + 1, Center, RENDERGRAPH_LEGEND_CIRCLE_RADIUS, Swatches[Row]);
            }
            else
            {
                const FRectangle Swatch(IntVector2(SwatchLeft, CenterY - (RENDERGRAPH_LEGEND_SWATCH_HEIGHT / 2)),
                    RENDERGRAPH_LEGEND_SWATCH_WIDTH, RENDERGRAPH_LEGEND_SWATCH_HEIGHT);

                const FCornerRadii SwatchCorners(RENDERGRAPH_LEGEND_SWATCH_RADIUS);
                OutCommandList.AddBox(LayerId + 1, Swatch, Swatches[Row], SwatchCorners);
                OutCommandList.AddBoxOutline(LayerId + 1, Swatch, SwatchOutlines[Row - 2], 1.0f, SwatchCorners);
            }

            const FRectangle LabelBounds(IntVector2(LabelLeft, CenterY - (LineHeight / 2)),
                Bounds.GetRight() - LabelLeft - RENDERGRAPH_LEGEND_PADDING, LineHeight);

            OutCommandList.AddText(LayerId + 1, LabelBounds, Labels[Row], Font.Get(), RenderGraphColors::LegendText);
        }

        return LayerId + 1;
    }

private:
    int32 GetRowHeight() const
    {
        return (Font ? Font->GetLineHeight() : 0) + RENDERGRAPH_LEGEND_ROW_SPACING;
    }

    TSharedPtr<IFontFace> Font;
};

FEditorRenderGraphPanel::FEditorRenderGraphPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "RenderGraph", "Render Graph")
    , Canvas(nullptr)
    , Model(nullptr)
    , GraphNameText(nullptr)
    , StatisticsText(nullptr)
    , PositionsByPassName()
    , BuiltPasses()
    , BuiltSignature()
    , bIsCapturing(false)
    , bShowCulled(true)
    , bNeedsFitView(false)
{
}

FEditorRenderGraphPanel::~FEditorRenderGraphPanel()
{
}

bool FEditorRenderGraphPanel::Initialize()
{
    Model = MakeSharedPtr<FGraphModel>();

    FGraphCanvas::FDesc CanvasDesc;
    CanvasDesc.Font                        = FEditorStyle::GetFonts().Body;
    CanvasDesc.BackgroundColor             = RenderGraphColors::Background;
    CanvasDesc.LinkColor                   = RenderGraphColors::Link;
    CanvasDesc.GridSpacing                 = 32;
    CanvasDesc.bIsViewer                   = true;
    CanvasDesc.NodeStyle.Body              = RenderGraphColors::NodeBody;
    CanvasDesc.NodeStyle.Border            = RenderGraphColors::NodeBorder;
    CanvasDesc.NodeStyle.Text              = RenderGraphColors::NodeText;
    CanvasDesc.NodeStyle.MutedBody         = RenderGraphColors::MutedBody;
    CanvasDesc.NodeStyle.MutedBorder       = RenderGraphColors::MutedBorder;
    CanvasDesc.NodeStyle.MutedText         = RenderGraphColors::MutedText;
    CanvasDesc.NodeStyle.PinOutline        = RenderGraphColors::PinOutline;
    CanvasDesc.NodeStyle.CornerRadius      = 6.0f;
    CanvasDesc.NodeStyle.MutedTintOpacity  = 1.0f;
    CanvasDesc.NodeStyle.bStackPinRows     = true;

    Canvas = FGraphCanvas::Create(CanvasDesc);
    if (!Canvas)
    {
        return false;
    }

    Canvas->SetModel(Model);

    TSharedPtr<FVisualElement> HeaderRow = BuildHeaderRow();
    if (!HeaderRow)
    {
        return false;
    }

    TSharedPtr<FOverlay> CanvasArea = FOverlay::Create();
    CanvasArea->AddSlot(Canvas);
    CanvasArea->AddSlot(FRenderGraphLegend::Create(FEditorStyle::GetFonts().Body))
        .SetHorizontalAlignment(EHorizontalAlignment::Left)
        .SetVerticalAlignment(EVerticalAlignment::Bottom)
        .SetPadding(FMargin(RENDERGRAPH_LEGEND_INSET, RENDERGRAPH_LEGEND_INSET, RENDERGRAPH_LEGEND_INSET, RENDERGRAPH_LEGEND_INSET));

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(HeaderRow).SetPadding(FMargin(RENDERGRAPH_HEADER_INSET, RENDERGRAPH_HEADER_INSET, RENDERGRAPH_HEADER_INSET, RENDERGRAPH_HEADER_INSET));
    Column->AddSlot(CanvasArea).SetFillCoefficient(1.0f);

    Content = Column;
    return true;
}

TSharedPtr<FVisualElement> FEditorRenderGraphPanel::BuildHeaderRow()
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FTextBlock::FDesc NameDesc;
    NameDesc.Font = FEditorStyle::GetFonts().Body;
    NameDesc.Text = "Graph: (none)";

    GraphNameText = FTextBlock::Create(NameDesc);

    FTextBlock::FDesc StatisticsDesc;
    StatisticsDesc.Font            = FEditorStyle::GetFonts().Body;
    StatisticsDesc.ColorAndOpacity = Style.Colors.TextDisabled;
    StatisticsDesc.Text            = "| Passes: 0  Culled: 0  Disabled: 0  Textures: 0  Buffers: 0";

    StatisticsText = FTextBlock::Create(StatisticsDesc);

    FCheckBox::FDesc CulledDesc;
    CulledDesc.Font           = FEditorStyle::GetFonts().Body;
    CulledDesc.Text           = "Show culled/disabled";
    CulledDesc.InitialState   = bShowCulled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    CulledDesc.OnStateChanged = FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
    {
        bShowCulled = State == ECheckBoxState::Checked;
    });

    FButton::FDesc LayoutDesc;
    LayoutDesc.Font      = FEditorStyle::GetFonts().Body;
    LayoutDesc.Text      = "Auto Layout";
    LayoutDesc.OnClicked = FOnClicked::CreateLambda([this]()
    {
        RunAutoLayout();
    });

    FButton::FDesc ResetDesc;
    ResetDesc.Font      = FEditorStyle::GetFonts().Body;
    ResetDesc.Text      = "Reset View";
    ResetDesc.OnClicked = FOnClicked::CreateLambda([this]()
    {
        bNeedsFitView = true;
    });

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(GraphNameText).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FSpacer::CreateHorizontal(RENDERGRAPH_HEADER_SPACING));
    Row->AddSlot(StatisticsText).SetFillCoefficient(1.0f).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FCheckBox::Create(CulledDesc)).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FSpacer::CreateHorizontal(RENDERGRAPH_HEADER_SPACING));
    Row->AddSlot(FButton::Create(LayoutDesc)).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FSpacer::CreateHorizontal(RENDERGRAPH_HEADER_SPACING));
    Row->AddSlot(FButton::Create(ResetDesc)).SetVerticalAlignment(EVerticalAlignment::Center);

    return Row;
}

void FEditorRenderGraphPanel::Release()
{
    SetCaptureEnabled(false);

    Canvas.Reset();
    Model.Reset();
    GraphNameText.Reset();
    StatisticsText.Reset();
    PositionsByPassName.Clear();
    BuiltPasses.Clear();
    BuiltSignature.Clear();

    FEditorPanel::Release();
}

void FEditorRenderGraphPanel::SetCaptureEnabled(bool bEnabled)
{
    if (bIsCapturing == bEnabled)
    {
        return;
    }

    bIsCapturing = bEnabled;

#if EDITOR_BUILD
    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        RendererModule->SetRenderGraphDebugCaptureEnabled(bEnabled);
    }
#endif
}

void FEditorRenderGraphPanel::RunAutoLayout()
{
    Canvas->AutoLayout();

    for (const FBuiltPass& Pass : BuiltPasses)
    {
        if (const FGraphNode* Node = Model->FindNode(Pass.NodeId))
        {
            PositionsByPassName.FindOrAdd(Pass.Name) = Node->Position;
        }
    }

    bNeedsFitView = true;
}

void FEditorRenderGraphPanel::Tick(float /*DeltaTime*/)
{
    if (IsVisible() != bIsCapturing)
    {
        SetCaptureEnabled(IsVisible());
        bNeedsFitView = IsVisible();
    }

    if (!IsVisible())
    {
        return;
    }

#if EDITOR_BUILD
    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        FRenderGraphDebugSnapshot Snapshot;
        if (RendererModule->CopyLatestRenderGraphDebugSnapshot(Snapshot))
        {
            RefreshStatistics(Snapshot);

            if (RebuildModel(Snapshot))
            {
                RunAutoLayout();
            }
        }
    }
#endif

    if (bNeedsFitView)
    {
        Canvas->FitToNodes();
        bNeedsFitView = Canvas->GetContentRectangle().IsEmpty();
    }
}

#if EDITOR_BUILD

bool FEditorRenderGraphPanel::IsPassInactive(const FRenderGraphDebugPass& Pass)
{
    return Pass.bCulled || !Pass.bEnabled;
}

String FEditorRenderGraphPanel::BuildNodeTitle(const FRenderGraphDebugPass& Pass)
{
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

    return Title;
}

bool FEditorRenderGraphPanel::HasIncomingLink(const FRenderGraphDebugSnapshot& Snapshot, int32 PassIndex, int32 AccessIndex)
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

String FEditorRenderGraphPanel::BuildSignature(const FRenderGraphDebugSnapshot& Snapshot) const
{
    String Signature = String::Printf("%d|%d|", bShowCulled ? 1 : 0, Snapshot.Links.Size());

    for (const FRenderGraphDebugPass& Pass : Snapshot.Passes)
    {
        if (!bShowCulled && IsPassInactive(Pass))
        {
            continue;
        }

        Signature.Append(String::Printf("%s;%d;%d,", *Pass.Name, Pass.Accesses.Size(), IsPassInactive(Pass) ? 1 : 0));
    }

    return Signature;
}

bool FEditorRenderGraphPanel::RebuildModel(const FRenderGraphDebugSnapshot& Snapshot)
{
    const String Signature = BuildSignature(Snapshot);
    if (Signature == BuiltSignature)
    {
        return false;
    }

    for (const FBuiltPass& Pass : BuiltPasses)
    {
        if (const FGraphNode* Node = Model->FindNode(Pass.NodeId))
        {
            PositionsByPassName.FindOrAdd(Pass.Name) = Node->Position;
        }
    }

    Canvas->ClearSelection();
    Model->Clear();

    BuiltPasses.Clear();
    BuiltPasses.Resize(Snapshot.Passes.Size());

    bool bHasNewPass = false;

    for (int32 PassIndex = 0; PassIndex < Snapshot.Passes.Size(); ++PassIndex)
    {
        const FRenderGraphDebugPass& Pass = Snapshot.Passes[PassIndex];
        if (!bShowCulled && IsPassInactive(Pass))
        {
            continue;
        }

        FBuiltPass& BuiltPass = BuiltPasses[PassIndex];
        BuiltPass.Name        = Pass.Name;

        ResizeToUnassigned(BuiltPass.AccessPinIds, Pass.Accesses.Size());
        ResizeToUnassigned(BuiltPass.LoadPinIds, Pass.Accesses.Size());

        const bool bIsInactive = IsPassInactive(Pass);

        FGraphNode Node;
        Node.Title     = BuildNodeTitle(Pass);
        Node.TitleTint = bIsInactive ? RenderGraphColors::MutedTitle : RenderGraphColors::NodeTitle;
        Node.bIsMuted  = bIsInactive;

        TArray<int32> AccessPinSlots;
        TArray<int32> LoadPinSlots;

        ResizeToUnassigned(AccessPinSlots, Pass.Accesses.Size());
        ResizeToUnassigned(LoadPinSlots, Pass.Accesses.Size());

        for (int32 AccessIndex = 0; AccessIndex < Pass.Accesses.Size(); ++AccessIndex)
        {
            const FRenderGraphDebugAccess& Access = Pass.Accesses[AccessIndex];
            if (Access.ResourceIndex < 0 || Access.ResourceIndex >= Snapshot.Resources.Size())
            {
                continue;
            }

            const FRenderGraphDebugResource& Resource = Snapshot.Resources[Access.ResourceIndex];
            const String                     TypeTag  = Resource.Kind == ERenderGraphDebugResourceKind::Buffer ? String("Buffer") : String("Texture");

            if (Access.bIsWrite && HasIncomingLink(Snapshot, PassIndex, AccessIndex))
            {
                LoadPinSlots[AccessIndex] = Node.Pins.Size();
                Node.Pins.Emplace(EGraphPinDirection::Input, Resource.Name, TypeTag, RenderGraphColors::InputPin);
            }

            AccessPinSlots[AccessIndex] = Node.Pins.Size();
            Node.Pins.Emplace(Access.bIsWrite ? EGraphPinDirection::Output : EGraphPinDirection::Input, Resource.Name, TypeTag,
                Access.bIsWrite ? RenderGraphColors::OutputPin : RenderGraphColors::InputPin);
        }

        if (const Vector2* StoredPosition = PositionsByPassName.Find(Pass.Name))
        {
            Node.Position = *StoredPosition;
        }
        else
        {
            bHasNewPass = true;
        }

        BuiltPass.NodeId = Model->AddNode(Node);

        const FGraphNode* AddedNode = Model->FindNode(BuiltPass.NodeId);
        if (!AddedNode)
        {
            continue;
        }

        for (int32 AccessIndex = 0; AccessIndex < Pass.Accesses.Size(); ++AccessIndex)
        {
            if (AccessPinSlots[AccessIndex] >= 0)
            {
                BuiltPass.AccessPinIds[AccessIndex] = AddedNode->Pins[AccessPinSlots[AccessIndex]].PinId;
            }

            if (LoadPinSlots[AccessIndex] >= 0)
            {
                BuiltPass.LoadPinIds[AccessIndex] = AddedNode->Pins[LoadPinSlots[AccessIndex]].PinId;
            }
        }
    }

    for (const FRenderGraphDebugLink& Link : Snapshot.Links)
    {
        if (!BuiltPasses.IsValidIndex(Link.FromPass) || !BuiltPasses.IsValidIndex(Link.ToPass))
        {
            continue;
        }

        const FBuiltPass& FromPass = BuiltPasses[Link.FromPass];
        const FBuiltPass& ToPass   = BuiltPasses[Link.ToPass];

        if (!FromPass.AccessPinIds.IsValidIndex(Link.FromAccess) || !ToPass.AccessPinIds.IsValidIndex(Link.ToAccess))
        {
            continue;
        }

        const int32 FromPinId = FromPass.AccessPinIds[Link.FromAccess];
        const int32 ToPinId   = ToPass.LoadPinIds[Link.ToAccess] >= 0 ? ToPass.LoadPinIds[Link.ToAccess] : ToPass.AccessPinIds[Link.ToAccess];

        if (FromPinId >= 0 && ToPinId >= 0)
        {
            Model->AddLink(FromPinId, ToPinId);
        }
    }

    BuiltSignature = Signature;
    return bHasNewPass;
}

void FEditorRenderGraphPanel::RefreshStatistics(const FRenderGraphDebugSnapshot& Snapshot)
{
    const FRenderGraphStatistics& Statistics = Snapshot.Statistics;

    GraphNameText->SetText(String::Printf("Graph: %s", Snapshot.GraphName.IsEmpty() ? "(none)" : *Snapshot.GraphName));

    StatisticsText->SetText(String::Printf("| Passes: %d  Culled: %d  Disabled: %d  Textures: %d  Buffers: %d",
        Statistics.NumPasses, Statistics.NumCulledPasses, Statistics.NumDisabledPasses,
        Statistics.NumTexturesAllocated, Statistics.NumBuffersAllocated));
}

#endif
