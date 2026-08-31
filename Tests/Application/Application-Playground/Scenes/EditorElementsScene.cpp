#include <Core/Containers/SharedPtr.h>
#include <Core/Math/Math.h>
#include <Application/Application.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Button.h>
#include <Application/Elements/CheckBox.h>
#include <Application/Elements/Expander.h>
#include <Application/Elements/Histogram.h>
#include <Application/Elements/NumericEntry.h>
#include <Application/Elements/ProgressBar.h>
#include <Application/Elements/PropertyTable.h>
#include <Application/Elements/SearchBox.h>
#include <Application/Elements/Separator.h>
#include <Application/Elements/Slider.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Elements/TileView.h>
#include <Application/Elements/TreeView.h>
#include <Application/Input/Keys.h>
#include <Application/Menus/DragDropService.h>
#include <Application/Menus/MenuStack.h>
#include <Application/Style/UIStyle.h>

#include "PlaygroundScene.h"
#include "ScenePanel.h"

/** @brief The type id the chips in the drag and drop panel carry, which one of its targets insists on. */
static const CHAR* AssetPayloadId = "PLAYGROUND_ASSET";

/** @brief The type id the second chip carries, which the asset-only target turns away. */
static const CHAR* ActorPayloadId = "PLAYGROUND_ACTOR";

static TSharedPtr<FTextBlock> MakeReadout(const FPlaygroundFonts& Fonts, const String& InText)
{
    FTextBlock::FDesc Desc;
    Desc.Text            = InText;
    Desc.Font            = Fonts.Monospace;
    Desc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    return FTextBlock::Create(Desc);
}

static TSharedPtr<FVisualElement> MakeFrame(const TSharedPtr<FVisualElement>& Content, int32 MinHeight)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FBorder::FDesc Desc;
    Desc.BackgroundColor = Style.Colors.WindowBackground;
    Desc.BorderColor     = Style.Colors.Border;
    Desc.BorderThickness = Style.Metrics.BorderThickness;
    Desc.CornerRadius    = Style.Metrics.CornerRadius;
    Desc.Padding         = FMargin(4);
    Desc.MinHeight       = MinHeight;
    Desc.Content         = Content;

    return FBorder::Create(Desc);
}

static IntVector2 ClientToScreen(const TSharedPtr<FVisualElement>& Element, const IntVector2& ClientPosition)
{
    const FRectangle ScreenBounds = FMenuStack::GetScreenBounds(Element);
    if (ScreenBounds.IsEmpty())
    {
        return ClientPosition;
    }

    return ScreenBounds.Position + (ClientPosition - Element->GetContentRectangle().Position);
}

class FDragChip final : public FVisualElement
{
public:
    static constexpr int32 DragThreshold = 4;

    static TSharedPtr<FDragChip> Create(const String& InLabel, const String& InTypeId, const TSharedPtr<IFontFace>& InFont)
    {
        TSharedPtr<FDragChip> NewChip = MakeSharedPtr<FDragChip>();
        NewChip->Label  = InLabel;
        NewChip->TypeId = InTypeId;
        NewChip->Font   = InFont;
        return NewChip;
    }

    FDragChip()
        : FVisualElement()
        , Label()
        , TypeId()
        , Font(nullptr)
        , PressPosition()
        , bIsPressed(false)
        , bHasBegunDrag(false)
    {
    }

    virtual IntVector2 ComputeDesiredSize() const override
    {
        const int32 TextWidth = Font ? Font->MeasureWidth(StringView(Label.Data(), Label.Length())) : 0;
        return IntVector2(TextWidth + 24, FUIStyle::GetDefault().Metrics.RowHeight);
    }

    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override
    {
        const FUIStyle&    Style = FUIStyle::GetDefault();
        const FCornerRadii Radii(Style.Metrics.CornerRadius);

        const FFloatColor Fill = bHasBegunDrag ? Style.Colors.ControlPressed : Style.Colors.ControlNormal;
        OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Fill, Radii);
        OutCommandList.AddBoxOutline(LayerId, AllottedGeometry.Bounds, Style.Colors.Accent, Style.Metrics.BorderThickness, Radii);

        if (Font && !Label.IsEmpty())
        {
            FRectangle TextBounds  = AllottedGeometry.Bounds;
            TextBounds.Position.X += 12;
            TextBounds.Position.Y += Font->GetTextBandOffset(AllottedGeometry.Bounds.Height);
            TextBounds.Height      = Font->GetTextBandHeight();

            OutCommandList.AddText(LayerId, TextBounds, Label, Font.Get(), Style.Colors.Text);
        }

        return LayerId + 1;
    }

    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override
    {
        if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
        {
            return FEventResponse::Unhandled();
        }

        bIsPressed    = true;
        bHasBegunDrag = false;
        PressPosition = CursorEvent.GetClientPosition();

        if (FApplication::IsInitialized())
        {
            FApplication::Get().CaptureMouse(AsSharedPtr());
        }

        return FEventResponse::Handled();
    }

    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override
    {
        if (!bIsPressed)
        {
            return FEventResponse::Unhandled();
        }

        const IntVector2 ClientPosition = CursorEvent.GetClientPosition();
        const IntVector2 ScreenPosition = ClientToScreen(AsSharedPtr(), ClientPosition);

        if (!bHasBegunDrag)
        {
            const IntVector2 Travel = ClientPosition - PressPosition;
            if ((Math::Abs(Travel.X) < DragThreshold) && (Math::Abs(Travel.Y) < DragThreshold))
            {
                return FEventResponse::Handled();
            }

            FDragDropPayload Payload;
            Payload.TypeId      = TypeId;
            Payload.DisplayText = Label;

            bHasBegunDrag = true;
            FDragDropService::Get().BeginDrag(Payload, ScreenPosition);
        }
        else
        {
            FDragDropService::Get().UpdateDrag(ScreenPosition);
        }

        return FEventResponse::Handled();
    }

    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override
    {
        if (!bIsPressed || CursorEvent.GetKey() != Keys::MouseButtonLeft)
        {
            return FEventResponse::Unhandled();
        }

        bIsPressed = false;

        if (FApplication::IsInitialized())
        {
            FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
        }

        if (bHasBegunDrag)
        {
            bHasBegunDrag = false;
            FDragDropService::Get().EndDrag(ClientToScreen(AsSharedPtr(), CursorEvent.GetClientPosition()));
        }

        return FEventResponse::Handled();
    }

private:
    String                Label;
    String                TypeId;
    TSharedPtr<IFontFace> Font;
    IntVector2            PressPosition;
    bool                  bIsPressed;
    bool                  bHasBegunDrag;
};

static TSharedPtr<FTreeItem> MakeBranch(const String& Label, const CHAR* const* ChildLabels, int32 NumChildren)
{
    TSharedPtr<FTreeItem> Branch = FTreeItem::Create(Label);
    for (int32 Index = 0; Index < NumChildren; ++Index)
    {
        Branch->AddChild(FTreeItem::Create(ChildLabels[Index]));
    }

    return Branch;
}

static TSharedPtr<FVisualElement> MakeTreeViewRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "nothing selected yet");

    const CHAR* LightNames[]    = { "Directional Light", "Point Light 01", "Point Light 02", "Spot Light" };
    const CHAR* GeometryNames[] = { "Sponza", "Street Light", "Floor Plane", "Sky Sphere" };
    const CHAR* CameraNames[]   = { "Main Camera", "Cutscene Camera" };

    TArray<TSharedPtr<FTreeItem>> Roots;

    TSharedPtr<FTreeItem> World = FTreeItem::Create("World");
    World->AddChild(MakeBranch("Lights", LightNames, static_cast<int32>(ARRAY_COUNT(LightNames))));
    World->AddChild(MakeBranch("Geometry", GeometryNames, static_cast<int32>(ARRAY_COUNT(GeometryNames))));
    World->AddChild(MakeBranch("Cameras", CameraNames, static_cast<int32>(ARRAY_COUNT(CameraNames))));
    Roots.Add(World);

    FTreeView::FDesc TreeDesc;
    TreeDesc.Font               = Fonts.Body;
    TreeDesc.OnSelectionChanged = FOnTreeSelectionChanged::CreateLambda([Readout](const TArray<TSharedPtr<FTreeItem>>& Selection)
    {
        if (Selection.IsEmpty())
        {
            Readout->SetText("nothing selected");
            return;
        }

        Readout->SetText(String::Printf("%d selected, first is '%s'", Selection.Size(), Selection[0]->Label.Data()));
    });
    TreeDesc.OnItemActivated = FOnTreeItemActivated::CreateLambda([Readout](const TSharedPtr<FTreeItem>& Item)
    {
        Readout->SetText(String::Printf("activated '%s'", Item->Label.Data()));
    });
    TreeDesc.OnExpansionChanged = FOnTreeItemExpansionChanged::CreateLambda([Readout](const TSharedPtr<FTreeItem>& Item, bool bIsExpanded)
    {
        Readout->SetText(String::Printf("'%s' is now %s", Item->Label.Data(), bIsExpanded ? "open" : "closed"));
    });

    TSharedPtr<FTreeView> TreeView = FTreeView::Create(TreeDesc);
    TreeView->SetRootItems(Roots);
    TreeView->ExpandAll();

    FSearchBox::FDesc SearchDesc;
    SearchDesc.HintText      = "Filter the tree";
    SearchDesc.Font          = Fonts.Body;
    SearchDesc.OnTextChanged = FOnSearchTextChanged::CreateLambda([TreeView, Readout](const String& SearchText)
    {
        TreeView->SetFilterText(SearchText);
        Readout->SetText(SearchText.IsEmpty() ? String("filter cleared") : String::Printf("filtering on '%s'", SearchText.Data()));
    });

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FSearchBox::Create(SearchDesc)).SetPadding(FMargin(0, 0, 0, 6));
    Column->AddSlot(MakeFrame(TreeView, 180));
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeTileViewRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "click a tile, double click to open it");

    const CHAR* AssetNames[] =
    {
        "Brick_Albedo", "Brick_Normal", "Brick_Roughness", "Sponza.obj",
        "Street_Light.obj", "Sky_Cubemap", "Water_Caustics", "Terrain_Height",
        "Grass_Albedo", "Grass_Normal", "Metal_Plate", "Wood_Planks",
    };

    TArray<FTileItem> Items;
    for (const CHAR* AssetName : AssetNames)
    {
        FTileItem Item;
        Item.Label = AssetName;
        Items.Add(Item);
    }

    FTileView::FDesc TileDesc;
    TileDesc.Font               = Fonts.Body;
    TileDesc.OnSelectionChanged = FOnTileSelectionChanged::CreateLambda([Readout](const TArray<int32>& SelectedIndices)
    {
        Readout->SetText(SelectedIndices.IsEmpty() ? String("nothing selected") : String::Printf("%d tile(s) selected", SelectedIndices.Size()));
    });
    TileDesc.OnItemActivated = FOnTileActivated::CreateLambda([Readout](int32 Index)
    {
        Readout->SetText(String::Printf("opened tile %d", Index));
    });

    TSharedPtr<FTileView> TileView = FTileView::Create(TileDesc);
    TileView->SetItems(Items);

    FSlider::FDesc ZoomDesc;
    ZoomDesc.SetRange(56.0f, 128.0f).SetValue(80.0f);
    ZoomDesc.StepSize       = 8.0f;
    ZoomDesc.OnValueChanged = FOnSliderValueChanged::CreateLambda([TileView](float NewValue)
    {
        const int32 Side = static_cast<int32>(NewValue);
        TileView->SetTileSize(IntVector2(Side, Side + 12));
    });

    FTextBlock::FDesc ZoomLabel;
    ZoomLabel.Text            = "Tile size";
    ZoomLabel.Font            = Fonts.Body;
    ZoomLabel.ColorAndOpacity = FUIStyle::GetDefault().Colors.Text;

    TSharedPtr<FHorizontalBox> ZoomRow = FHorizontalBox::Create();
    ZoomRow->AddSlot(FTextBlock::Create(ZoomLabel)).SetVerticalAlignment(EVerticalAlignment::Center).SetPadding(FMargin(0, 0, 12, 0));
    ZoomRow->AddSlot(FSlider::Create(ZoomDesc)).SetFillCoefficient(1.0f).SetVerticalAlignment(EVerticalAlignment::Center);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(ZoomRow).SetPadding(FMargin(0, 0, 0, 6));
    Column->AddSlot(MakeFrame(TileView, 200));
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeExpanderRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "click a header to open or close its section");

    struct FSample
    {
        const CHAR* Label;
        const CHAR* Body;
        bool        bStartsOpen;
    };

    const FSample Samples[] =
    {
        { "Deferred Rendering", "The section a panel opens on, which is the one the reader came for.", true  },
        { "Shadows",            "A closed section still measures its content, so opening it needs no second pass.", false },
        { "Post Processing",    "Sections nest, so a group of settings can carry groups of its own.", false },
    };

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    for (const FSample& Sample : Samples)
    {
        FTextBlock::FDesc BodyDesc;
        BodyDesc.Text            = Sample.Body;
        BodyDesc.Font            = Fonts.Body;
        BodyDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

        FExpander::FDesc Desc;
        Desc.Label          = Sample.Label;
        Desc.Font           = Fonts.Body;
        Desc.Content        = FTextBlock::Create(BodyDesc);
        Desc.bIsExpanded    = Sample.bStartsOpen;
        Desc.OnStateChanged = FOnExpanderStateChanged::CreateLambda([Readout, Label = String(Sample.Label)](bool bIsExpanded)
        {
            Readout->SetText(String::Printf("'%s' is now %s", Label.Data(), bIsExpanded ? "open" : "closed"));
        });

        Column->AddSlot(FExpander::Create(Desc)).SetPadding(FMargin(0, 0, 0, 4));
    }

    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeVectorRow(const FPlaygroundFonts& Fonts, const TSharedPtr<FTextBlock>& Readout,
    const String& RowName, float InitialValue, float Step)
{
    struct FAxis
    {
        const CHAR* Label;
        FFloatColor Color;
    };

    const FAxis Axes[] =
    {
        { "X", FFloatColor(0.62f, 0.20f, 0.22f, 1.0f) },
        { "Y", FFloatColor(0.30f, 0.52f, 0.24f, 1.0f) },
        { "Z", FFloatColor(0.24f, 0.38f, 0.62f, 1.0f) },
    };

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    for (const FAxis& Axis : Axes)
    {
        FNumericEntryFloat::FDesc Desc;
        Desc.Label          = Axis.Label;
        Desc.LabelColor     = Axis.Color;
        Desc.Value          = InitialValue;
        Desc.Step           = Step;
        Desc.Font           = Fonts.Monospace;
        Desc.OnValueChanged = FNumericEntryFloat::FOnValueChanged::CreateLambda(
            [Readout, RowName, AxisName = String(Axis.Label)](float NewValue)
        {
            Readout->SetText(String::Printf("%s.%s is now %.3f", RowName.Data(), AxisName.Data(), NewValue));
        });

        Row->AddSlot(FNumericEntryFloat::Create(Desc)).SetFillCoefficient(1.0f).SetPadding(FMargin(0, 0, 4, 0));
    }

    return Row;
}

static TSharedPtr<FVisualElement> MakePropertyTableRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "drag across a field to scrub it, or drag the column divider");

    FPropertyTable::FDesc TableDesc;
    TableDesc.Font = Fonts.Body;

    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(TableDesc);
    Table->AddHeaderRow("Transform");
    Table->AddRow("Location", MakeVectorRow(Fonts, Readout, "Location", 0.0f, 0.05f)).ToolTipText = "Where the actor sits, in world units.";
    Table->AddRow("Rotation", MakeVectorRow(Fonts, Readout, "Rotation", 0.0f, 0.5f)).ToolTipText  = "Pitch, yaw and roll, in degrees.";
    Table->AddRow("Scale", MakeVectorRow(Fonts, Readout, "Scale", 1.0f, 0.01f)).ToolTipText       = "A uniform scale of one leaves the mesh as authored.";

    Table->AddHeaderRow("Static Mesh");

    FCheckBox::FDesc ShadowDesc;
    ShadowDesc.SetFont(Fonts.Body);
    ShadowDesc.InitialState   = ECheckBoxState::Checked;
    ShadowDesc.OnStateChanged = FOnCheckStateChanged::CreateLambda([Readout](ECheckBoxState NewState)
    {
        Readout->SetText(String::Printf("Cast Shadows is now %s", NewState == ECheckBoxState::Checked ? "on" : "off"));
    });
    Table->AddRow("Cast Shadows", FCheckBox::Create(ShadowDesc));

    FCheckBox::FDesc DoubleSidedDesc;
    DoubleSidedDesc.SetFont(Fonts.Body);
    DoubleSidedDesc.InitialState   = ECheckBoxState::Unchecked;
    DoubleSidedDesc.OnStateChanged = FOnCheckStateChanged::CreateLambda([Readout](ECheckBoxState NewState)
    {
        Readout->SetText(String::Printf("Double Sided is now %s", NewState == ECheckBoxState::Checked ? "on" : "off"));
    });
    Table->AddRow("Double Sided", FCheckBox::Create(DoubleSidedDesc));

    FNumericEntryInt::FDesc SlotsDesc;
    SlotsDesc.Label          = "N";
    SlotsDesc.Value          = 3;
    SlotsDesc.MinValue       = 0;
    SlotsDesc.MaxValue       = 32;
    SlotsDesc.Font           = Fonts.Monospace;
    SlotsDesc.OnValueChanged = FNumericEntryInt::FOnValueChanged::CreateLambda([Readout](int32 NewValue)
    {
        Readout->SetText(String::Printf("Material Slots is now %d, clamped to 0 through 32", NewValue));
    });

    // Nested under the mesh heading, which is what the indent level is for
    Table->AddRow("Material Slots", FNumericEntryInt::Create(SlotsDesc)).IndentLevel = 1;

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(MakeFrame(Table, 0));
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeProgressBarRow(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    struct FSample
    {
        const CHAR* Label;
        float       Percent;
        FFloatColor FillColor;
    };

    const FSample Samples[] =
    {
        { "Local VRAM     2048 / 8192 MB", 0.25f, Style.Colors.Accent                        },
        { "Non-local VRAM 5734 / 8192 MB", 0.70f, FFloatColor(0.80f, 0.66f, 0.24f, 1.0f)     },
        { "Shader cache   7823 / 8192 MB", 0.95f, FFloatColor(0.85f, 0.35f, 0.25f, 1.0f)     },
    };

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    for (const FSample& Sample : Samples)
    {
        FProgressBar::FDesc Desc;
        Desc.Percent     = Sample.Percent;
        Desc.OverlayText = Sample.Label;
        Desc.Font        = Fonts.Monospace;
        Desc.FillColor   = Sample.FillColor;

        Column->AddSlot(FProgressBar::Create(Desc)).SetPadding(FMargin(0, 3, 0, 3));
    }

    FProgressBar::FDesc DrivenDesc;
    DrivenDesc.Percent     = 0.4f;
    DrivenDesc.OverlayText = "40%";
    DrivenDesc.Font        = Fonts.Monospace;

    TSharedPtr<FProgressBar> Driven = FProgressBar::Create(DrivenDesc);

    FSlider::FDesc SliderDesc;
    SliderDesc.SetRange(0.0f, 1.0f).SetValue(0.4f);
    SliderDesc.OnValueChanged = FOnSliderValueChanged::CreateLambda([Driven](float NewValue)
    {
        Driven->SetPercent(NewValue);
        Driven->SetOverlayText(String::Printf("%.0f%%", NewValue * 100.0f));
        Driven->SetFillColor(NewValue > 0.85f ? FFloatColor(0.85f, 0.35f, 0.25f, 1.0f) : FUIStyle::GetDefault().Colors.Accent);
    });

    Column->AddSlot(FSeparator::CreateHorizontal()).SetPadding(FMargin(0, 8, 0, 8));
    Column->AddSlot(Driven).SetPadding(FMargin(0, 0, 0, 6));
    Column->AddSlot(FSlider::Create(SliderDesc));
    return Column;
}

static TSharedPtr<FVisualElement> MakeHistogramRow(const FPlaygroundFonts& Fonts)
{
    FHistogram::FDesc FrameDesc;
    FrameDesc.Label            = "CPU frame time (ms)";
    FrameDesc.Font             = Fonts.Monospace;
    FrameDesc.Capacity         = 120;
    FrameDesc.bAutoScale       = false;
    FrameDesc.MaxValue         = 33.3f;
    FrameDesc.WarningThreshold = 16.6f;

    TSharedPtr<FHistogram> FrameTimes = FHistogram::Create(FrameDesc);

    FHistogram::FDesc GpuDesc;
    GpuDesc.Label      = "GPU frame time (ms), auto-scaled";
    GpuDesc.Font       = Fonts.Monospace;
    GpuDesc.Capacity   = 120;
    GpuDesc.bAutoScale = true;

    TSharedPtr<FHistogram> GpuTimes = FHistogram::Create(GpuDesc);

    for (int32 Index = 0; Index < 90; ++Index)
    {
        const float Wave  = Math::Sin(static_cast<float>(Index) * 0.21f);
        const float Spike = ((Index % 23) == 0) ? 9.0f : 0.0f;

        FrameTimes->AddSample(11.0f + (Wave * 2.5f) + Spike);
        GpuTimes->AddSample(7.4f + (Wave * 1.2f) + (Spike * 0.4f));
    }

    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, String::Printf("latest %.2f ms, average %.2f ms, peak %.2f ms",
        FrameTimes->GetLatest(), FrameTimes->GetAverage(), FrameTimes->GetMaximum()));

    const auto RefreshReadout = [Readout, FrameTimes]()
    {
        Readout->SetText(String::Printf("latest %.2f ms, average %.2f ms, peak %.2f ms, %d sample(s) held",
            FrameTimes->GetLatest(), FrameTimes->GetAverage(), FrameTimes->GetMaximum(), FrameTimes->GetNumSamples()));
    };

    FButton::FDesc SpikeDesc;
    SpikeDesc.SetText("Add a spike").SetFont(Fonts.Body);
    SpikeDesc.OnClicked = FOnClicked::CreateLambda([FrameTimes, GpuTimes, RefreshReadout]()
    {
        FrameTimes->AddSample(28.0f);
        GpuTimes->AddSample(19.0f);
        RefreshReadout();
    });

    FButton::FDesc FillDesc;
    FillDesc.SetText("Add 40 samples").SetFont(Fonts.Body);
    FillDesc.OnClicked = FOnClicked::CreateLambda([FrameTimes, GpuTimes, RefreshReadout]()
    {
        for (int32 Index = 0; Index < 40; ++Index)
        {
            const float Wave = Math::Sin(static_cast<float>(Index) * 0.37f);
            FrameTimes->AddSample(12.0f + (Wave * 3.0f));
            GpuTimes->AddSample(8.0f + (Wave * 1.5f));
        }

        RefreshReadout();
    });

    FButton::FDesc ClearDesc;
    ClearDesc.SetText("Clear").SetFont(Fonts.Body);
    ClearDesc.OnClicked = FOnClicked::CreateLambda([FrameTimes, GpuTimes, RefreshReadout]()
    {
        FrameTimes->Clear();
        GpuTimes->Clear();
        RefreshReadout();
    });

    TSharedPtr<FHorizontalBox> Buttons = FHorizontalBox::Create();
    Buttons->AddSlot(FButton::Create(SpikeDesc)).SetPadding(FMargin(0, 0, 8, 0));
    Buttons->AddSlot(FButton::Create(FillDesc)).SetPadding(FMargin(0, 0, 8, 0));
    Buttons->AddSlot(FButton::Create(ClearDesc));
    Buttons->AddSlot(FSpacer::CreateHorizontal(0)).SetFillCoefficient(1.0f);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FrameTimes).SetPadding(FMargin(0, 0, 0, 8));
    Column->AddSlot(GpuTimes).SetPadding(FMargin(0, 0, 0, 8));
    Column->AddSlot(Buttons);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeDropTarget(const FPlaygroundFonts& Fonts, const TSharedPtr<FTextBlock>& Readout,
    const String& Title, const String& AcceptedTypeId)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FTextBlock::FDesc TitleDesc;
    TitleDesc.Text            = Title;
    TitleDesc.Font            = Fonts.Body;
    TitleDesc.ColorAndOpacity = Style.Colors.Text;

    FBorder::FDesc Desc;
    Desc.BackgroundColor = Style.Colors.PanelBackground;
    Desc.BorderColor     = Style.Colors.Border;
    Desc.BorderThickness = Style.Metrics.BorderThickness;
    Desc.CornerRadius    = Style.Metrics.CornerRadius;
    Desc.Padding         = FMargin(12);
    Desc.MinHeight       = 72;
    Desc.Content         = FTextBlock::Create(TitleDesc);

    TSharedPtr<FBorder> Target = FBorder::Create(Desc);

    FOnDragDropped OnDropped = FOnDragDropped::CreateLambda([Readout, Title](const FDragDropPayload& Payload, const IntVector2& ScreenPosition)
    {
        Readout->SetText(String::Printf("'%s' dropped on %s at %d, %d",
            Payload.DisplayText.Data(), Title.Data(), ScreenPosition.X, ScreenPosition.Y));
    });

    FOnDragOver OnOver;
    if (!AcceptedTypeId.IsEmpty())
    {
        OnOver = FOnDragOver::CreateLambda([AcceptedTypeId](const FDragDropPayload& Payload)
        {
            return Payload.TypeId == AcceptedTypeId;
        });
    }

    FDragDropService::Get().RegisterTarget(Target, OnDropped, OnOver);
    return Target;
}

static TSharedPtr<FVisualElement> MakeDragDropRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "drag a chip onto one of the areas below");

    TSharedPtr<FHorizontalBox> Chips = FHorizontalBox::Create();
    Chips->AddSlot(FDragChip::Create("Brick_Albedo", AssetPayloadId, Fonts.Body))
        .SetPadding(FMargin(0, 0, 8, 0))
        .SetHorizontalAlignment(EHorizontalAlignment::Left)
        .SetVerticalAlignment(EVerticalAlignment::Center);
    Chips->AddSlot(FDragChip::Create("Point Light 01", ActorPayloadId, Fonts.Body))
        .SetPadding(FMargin(0, 0, 8, 0))
        .SetHorizontalAlignment(EHorizontalAlignment::Left)
        .SetVerticalAlignment(EVerticalAlignment::Center);
    Chips->AddSlot(FSpacer::CreateHorizontal(0)).SetFillCoefficient(1.0f);

    TSharedPtr<FHorizontalBox> Targets = FHorizontalBox::Create();
    Targets->AddSlot(MakeDropTarget(Fonts, Readout, "Takes anything", String())).SetFillCoefficient(1.0f).SetPadding(FMargin(0, 0, 8, 0));
    Targets->AddSlot(MakeDropTarget(Fonts, Readout, "Assets only", AssetPayloadId)).SetFillCoefficient(1.0f);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(Chips).SetPadding(FMargin(0, 0, 0, 10));
    Column->AddSlot(Targets);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

FPlaygroundScene CreateEditorElementsScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Tree view and search box";
        Desc.Description = "Expand arrows, multi-select with chord and shift, arrow keys to walk it. Filtering opens a branch whose descendant matches.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeTreeViewRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Tile view";
        Desc.Description = "A wrapping icon grid that re-flows as the tile size and the width change, with labels elided to fit.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeTileViewRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Expanders";
        Desc.Description = "Collapsible titled sections. A closed one still measures its content, so opening it needs no second pass.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeExpanderRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Property table and numeric entries";
        Desc.Description = "Headings, indented rows and a draggable column divider. Drag across an axis field to scrub it, click one to type into it.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakePropertyTableRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Progress bars";
        Desc.Description = "Overlay text over a filled track. The driven bar turns red past 85%, which is how a budget bar warns rather than only reports.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeProgressBarRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Histograms";
        Desc.Description = "A rolling buffer drawn as a bar strip. The first is scaled against a fixed budget and warns past it, the second against its own peak.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeHistogramRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Drag and drop";
        Desc.Description = "The chips are sources and the areas are targets, one of which turns away everything that is not an asset. The ghost under the cursor is drawn by the host's desktop overlay, which the playground does not raise.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeDragDropRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Editor Elements", MakeSceneColumn("Editor Elements", Fonts, Panels));
}
