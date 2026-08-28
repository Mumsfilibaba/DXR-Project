#include <Core/Containers/SharedPtr.h>
#include <Core/Math/Math.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Button.h>
#include <Application/Elements/CheckBox.h>
#include <Application/Elements/Overlay.h>
#include <Application/Elements/ScrollBox.h>
#include <Application/Elements/Separator.h>
#include <Application/Elements/Slider.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/SpinBox.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Style/UIStyle.h>

#include "PlaygroundScene.h"
#include "ScenePanel.h"

static TSharedPtr<FTextBlock> MakeReadout(const FPlaygroundFonts& Fonts, const String& InText)
{
    FTextBlock::FDesc Desc;
    Desc.Text            = InText;
    Desc.Font            = Fonts.Monospace;
    Desc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    return FTextBlock::Create(Desc);
}

static TSharedPtr<FVisualElement> MakeButtonRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock>     Readout    = MakeReadout(Fonts, "no clicks yet");
    TSharedPtr<int32>          ClickCount = MakeSharedPtr<int32>(0);
    TSharedPtr<FHorizontalBox> Row        = FHorizontalBox::Create();

    const CHAR* Labels[] = { "Save", "Cancel", "Bordered", "Disabled" };
    for (int32 Index = 0; Index < static_cast<int32>(ARRAY_COUNT(Labels)); ++Index)
    {
        FButton::FDesc Desc;
        Desc.SetText(Labels[Index]).SetFont(Fonts.Body);
        Desc.bHasBorder = (Index == 2);
        Desc.OnClicked  = FOnClicked::CreateLambda([Readout, ClickCount, Label = String(Labels[Index])]()
        {
            (*ClickCount)++;
            Readout->SetText(String::Printf("%s clicked, %d in total", Label.Data(), *ClickCount));
        });

        TSharedPtr<FButton> Button = FButton::Create(Desc);
        if (Index == 3)
        {
            Button->SetEnabled(false);
        }

        Row->AddSlot(Button).SetPadding(FMargin(0, 0, 8, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    }

    FButton::FDesc WideDesc;
    WideDesc.SetText("Left aligned in a fill slot").SetFont(Fonts.Body);
    WideDesc.HorizontalContentAlignment = EHorizontalAlignment::Left;

    Row->AddSlot(FButton::Create(WideDesc)).SetFillCoefficient(1.0f).SetVerticalAlignment(EVerticalAlignment::Center);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(Row);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeCheckBoxRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock>   Readout = MakeReadout(Fonts, "nothing toggled yet");
    TSharedPtr<FVerticalBox> Column  = FVerticalBox::Create();

    struct FSample
    {
        const CHAR*    Label;
        ECheckBoxState Initial;
        bool           bIsTriState;
        bool           bIsEnabled;
    };

    const FSample Samples[] =
    {
        { "Two-state, off",             ECheckBoxState::Unchecked,    false, true  },
        { "Two-state, on",              ECheckBoxState::Checked,      false, true  },
        { "Tri-state, mixed selection", ECheckBoxState::Undetermined, true,  true  },
        { "Disabled",                   ECheckBoxState::Checked,      false, false },
    };

    for (const FSample& Sample : Samples)
    {
        FCheckBox::FDesc Desc;
        Desc.SetText(Sample.Label).SetFont(Fonts.Body);
        Desc.InitialState   = Sample.Initial;
        Desc.bIsTriState    = Sample.bIsTriState;
        Desc.OnStateChanged = FOnCheckStateChanged::CreateLambda([Readout, Label = String(Sample.Label)](ECheckBoxState NewState)
        {
            const CHAR* StateName = NewState == ECheckBoxState::Checked ? "checked" : (NewState == ECheckBoxState::Unchecked ? "unchecked" : "undetermined");
            Readout->SetText(String::Printf("%s is now %s", Label.Data(), StateName));
        });

        TSharedPtr<FCheckBox> CheckBox = FCheckBox::Create(Desc);
        CheckBox->SetEnabled(Sample.bIsEnabled);

        Column->AddSlot(CheckBox).SetPadding(FMargin(0, 3, 0, 3)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    }

    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeSliderRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "drag a handle");

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();

    struct FSample
    {
        const CHAR* Label;
        float       Min;
        float       Max;
        float       Value;
        float       Step;
        bool        bShowValueText;
    };

    const FSample Samples[] =
    {
        { "Continuous 0 to 1",    0.0f,   1.0f, 0.35f, 0.0f,  false },
        { "Stepped by 10",        0.0f, 100.0f, 40.0f, 10.0f, true  },
        { "Signed, -180 to 180", -180.0f, 180.0f, 0.0f, 1.0f, true  },
    };

    for (const FSample& Sample : Samples)
    {
        FTextBlock::FDesc LabelDesc;
        LabelDesc.Text            = Sample.Label;
        LabelDesc.Font            = Fonts.Body;
        LabelDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.Text;

        FSlider::FDesc SliderDesc;
        SliderDesc.SetRange(Sample.Min, Sample.Max).SetValue(Sample.Value);
        SliderDesc.StepSize       = Sample.Step;
        SliderDesc.bShowValueText = Sample.bShowValueText;
        SliderDesc.Font           = Fonts.Monospace;
        SliderDesc.Precision      = Sample.Step >= 1.0f ? 0 : 2;
        SliderDesc.OnValueChanged = FOnSliderValueChanged::CreateLambda([Readout, Label = String(Sample.Label)](float NewValue)
        {
            Readout->SetText(String::Printf("%s dragging: %.3f", Label.Data(), NewValue));
        });
        SliderDesc.OnValueCommitted = FOnSliderValueCommitted::CreateLambda([Readout, Label = String(Sample.Label)](float FinalValue)
        {
            Readout->SetText(String::Printf("%s committed: %.3f", Label.Data(), FinalValue));
        });

        TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
        Row->AddSlot(FTextBlock::Create(LabelDesc)).SetVerticalAlignment(EVerticalAlignment::Center).SetPadding(FMargin(0, 0, 12, 0));
        Row->AddSlot(FSlider::Create(SliderDesc)).SetFillCoefficient(1.0f).SetVerticalAlignment(EVerticalAlignment::Center);

        Column->AddSlot(Row).SetPadding(FMargin(0, 4, 0, 4));
    }

    FSlider::FDesc VerticalDesc;
    VerticalDesc.SetRange(0.0f, 1.0f).SetValue(0.6f);
    VerticalDesc.Orientation = EOrientation::Vertical;
    VerticalDesc.MinLength   = 120;

    TSharedPtr<FHorizontalBox> VerticalRow = FHorizontalBox::Create();
    VerticalRow->AddSlot(FSlider::Create(VerticalDesc)).SetPadding(FMargin(0, 0, 16, 0));

    FTextBlock::FDesc NoteDesc;
    NoteDesc.Text            = "Vertical sliders count up from the bottom.";
    NoteDesc.Font            = Fonts.Body;
    NoteDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    VerticalRow->AddSlot(FTextBlock::Create(NoteDesc)).SetVerticalAlignment(EVerticalAlignment::Center);

    Column->AddSlot(VerticalRow).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeSpinBoxRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock>     Readout = MakeReadout(Fonts, "drag across a field, or click one to type");
    TSharedPtr<FHorizontalBox> Row     = FHorizontalBox::Create();

    const CHAR* Axes[] = { "X: ", "Y: ", "Z: " };
    for (int32 Index = 0; Index < static_cast<int32>(ARRAY_COUNT(Axes)); ++Index)
    {
        FSpinBox::FDesc Desc;
        Desc.SetRange(-100.0f, 100.0f).SetValue(static_cast<float>(Index) * 5.0f).SetFont(Fonts.Monospace);
        Desc.Prefix         = Axes[Index];
        Desc.Precision      = 2;
        Desc.ScrubSpeed     = 0.25f;
        Desc.OnValueChanged = FOnSpinBoxValueChanged::CreateLambda([Readout, Axis = String(Axes[Index])](float NewValue)
        {
            Readout->SetText(String::Printf("%sscrubbing to %.2f", Axis.Data(), NewValue));
        });
        Desc.OnValueCommitted = FOnSpinBoxValueCommitted::CreateLambda([Readout, Axis = String(Axes[Index])](float FinalValue)
        {
            Readout->SetText(String::Printf("%scommitted %.2f", Axis.Data(), FinalValue));
        });

        Row->AddSlot(FSpinBox::Create(Desc)).SetFillCoefficient(1.0f).SetPadding(FMargin(0, 0, 8, 0));
    }

    // An unbounded field, which draws no fill because there is no range to show a position in
    FSpinBox::FDesc FreeDesc;
    FreeDesc.SetValue(1.0f).SetFont(Fonts.Monospace);
    FreeDesc.Prefix     = "Scale: ";
    FreeDesc.Precision  = 3;
    FreeDesc.ScrubSpeed = 0.01f;

    Row->AddSlot(FSpinBox::Create(FreeDesc)).SetFillCoefficient(1.0f);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(Row);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeScrollBarRow(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FVerticalBox> LongColumn = FVerticalBox::Create();
    for (int32 Index = 0; Index < 40; ++Index)
    {
        FTextBlock::FDesc LineDesc;
        LineDesc.Text            = String::Printf("Row %02d, scroll with the wheel or drag the bar on the right", Index);
        LineDesc.Font            = Fonts.Monospace;
        LineDesc.ColorAndOpacity = (Index % 5) == 0 ? Style.Colors.Accent : Style.Colors.Text;

        LongColumn->AddSlot(FTextBlock::Create(LineDesc)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    }

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    ScrollBox->SetContent(LongColumn);

    FBorder::FDesc FrameDesc;
    FrameDesc.BackgroundColor = Style.Colors.WindowBackground;
    FrameDesc.BorderColor     = Style.Colors.Border;
    FrameDesc.BorderThickness = Style.Metrics.BorderThickness;
    FrameDesc.CornerRadius    = Style.Metrics.CornerRadius;
    FrameDesc.Padding         = FMargin(4);
    FrameDesc.MinHeight       = 160;
    FrameDesc.Content         = ScrollBox;

    return FBorder::Create(FrameDesc);
}

static TSharedPtr<FVisualElement> MakeOverlayRow(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FTextBlock::FDesc BaseDesc;
    BaseDesc.Text            = "A panel with a badge pinned to its top-right corner and a caption at the bottom.";
    BaseDesc.Font            = Fonts.Body;
    BaseDesc.ColorAndOpacity = Style.Colors.Text;

    FBorder::FDesc BaseFrame;
    BaseFrame.BackgroundColor = Style.Colors.PanelBackground;
    BaseFrame.CornerRadius    = Style.Metrics.CornerRadius;
    BaseFrame.Padding         = FMargin(12);
    BaseFrame.MinHeight       = 88;
    BaseFrame.Content         = FTextBlock::Create(BaseDesc);

    FTextBlock::FDesc BadgeText;
    BadgeText.Text            = "12";
    BadgeText.Font            = Fonts.Body;
    BadgeText.ColorAndOpacity = FFloatColor(0.06f, 0.06f, 0.06f, 1.0f);

    FBorder::FDesc BadgeFrame;
    BadgeFrame.BackgroundColor = Style.Colors.Accent;
    BadgeFrame.CornerRadius    = 9.0f;
    BadgeFrame.Padding         = FMargin(7, 2, 7, 2);
    BadgeFrame.Content         = FTextBlock::Create(BadgeText);

    FTextBlock::FDesc CaptionText;
    CaptionText.Text            = "Bottom-centred caption";
    CaptionText.Font            = Fonts.Monospace;
    CaptionText.ColorAndOpacity = Style.Colors.TextDisabled;

    TSharedPtr<FOverlay> Overlay = FOverlay::Create();
    Overlay->AddSlot(FBorder::Create(BaseFrame));
    Overlay->AddSlot(FBorder::Create(BadgeFrame))
        .SetHorizontalAlignment(EHorizontalAlignment::Right)
        .SetVerticalAlignment(EVerticalAlignment::Top)
        .SetPadding(FMargin(6));
    Overlay->AddSlot(FTextBlock::Create(CaptionText))
        .SetHorizontalAlignment(EHorizontalAlignment::Center)
        .SetVerticalAlignment(EVerticalAlignment::Bottom)
        .SetPadding(FMargin(8));

    return Overlay;
}

static TSharedPtr<FVisualElement> MakeSpacingRow(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FTextBlock::FDesc LeftDesc;
    LeftDesc.Text            = "Pinned left";
    LeftDesc.Font            = Fonts.Body;
    LeftDesc.ColorAndOpacity = Style.Colors.Text;

    FTextBlock::FDesc RightDesc;
    RightDesc.Text            = "Pushed right by a fill spacer";
    RightDesc.Font            = Fonts.Body;
    RightDesc.ColorAndOpacity = Style.Colors.Text;

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(FTextBlock::Create(LeftDesc)).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FSpacer::CreateHorizontal(0)).SetFillCoefficient(1.0f);
    Row->AddSlot(FSeparator::CreateVertical()).SetPadding(FMargin(12, 0, 12, 0));
    Row->AddSlot(FTextBlock::Create(RightDesc)).SetVerticalAlignment(EVerticalAlignment::Center);

    FTextBlock::FDesc AboveDesc;
    AboveDesc.Text            = "Above a horizontal rule";
    AboveDesc.Font            = Fonts.Body;
    AboveDesc.ColorAndOpacity = Style.Colors.TextDisabled;

    FTextBlock::FDesc BelowDesc;
    BelowDesc.Text            = "Below it, with a fixed spacer between the rule and this line";
    BelowDesc.Font            = Fonts.Body;
    BelowDesc.ColorAndOpacity = Style.Colors.TextDisabled;

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(Row);
    Column->AddSlot(FSpacer::CreateVertical(12));
    Column->AddSlot(FTextBlock::Create(AboveDesc)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    Column->AddSlot(FSeparator::CreateHorizontal()).SetPadding(FMargin(0, 6, 0, 6));
    Column->AddSlot(FSpacer::CreateVertical(12));
    Column->AddSlot(FTextBlock::Create(BelowDesc)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

FPlaygroundScene CreateControlsScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Buttons";
        Desc.Description = "Hover, press, disabled and keyboard activation, with the label following the state.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeButtonRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Check boxes";
        Desc.Description = "Two-state and tri-state, the third state being what a mixed multi-selection shows.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeCheckBoxRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Sliders";
        Desc.Description = "Click the track to jump, drag to scrub, arrows to nudge. Continuous, stepped and vertical.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeSliderRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Spin boxes";
        Desc.Description = "Drag across a field to scrub it, click without moving to type into it. Enter or clicking away commits.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeSpinBoxRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Scroll bar";
        Desc.Description = "The bar appears only once there is something to scroll, and the wheel and the thumb agree.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeScrollBarRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Overlay";
        Desc.Description = "Layers sharing one rectangle, each aligned in it. The badge is hit before the panel under it.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeOverlayRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Spacers and separators";
        Desc.Description = "A fill spacer pushes what follows to the far end, and a rule spans whatever it is given.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeSpacingRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Controls", MakeSceneColumn("Controls", Fonts, Panels));
}
