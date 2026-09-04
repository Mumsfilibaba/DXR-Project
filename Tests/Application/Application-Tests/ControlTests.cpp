#include "ControlTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Button.h>
#include <Application/Elements/CheckBox.h>
#include <Application/Elements/EditableText.h>
#include <Application/Elements/Expander.h>
#include <Application/Elements/Histogram.h>
#include <Application/Elements/NumericEntry.h>
#include <Application/Elements/Overlay.h>
#include <Application/Elements/ProgressBar.h>
#include <Application/Elements/ScrollBar.h>
#include <Application/Elements/ScrollBox.h>
#include <Application/Elements/SearchBox.h>
#include <Application/Elements/Separator.h>
#include <Application/Elements/Slider.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/SpinBox.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Input/Keys.h>
#include <Application/Style/UIStyle.h>
#include <Application/Text/FixedWidthFontFace.h>

/** @brief An eight by sixteen face, so every measurement in these tests is exact. */
static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

/** @brief Measures and arranges an element on its own, the way a window would. */
static void LayoutElement(const TSharedPtr<FVisualElement>& Element, const FRectangle& Bounds)
{
    Element->PrepareDesiredSize();
    Element->Tick(Bounds);
}

static FCursorEvent MakeMoveEvent(const IntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, const IntVector2& ClientPosition, FKey Key = Keys::MouseButtonLeft)
{
    return FCursorEvent(Type, Key, ClientPosition, IntVector2(0, 0), FModifierKeyState(), Type == EInputEventType::MouseButtonDown);
}

/** @brief Presses, drags through every waypoint and releases, which is one gesture end to end. */
static void DragThrough(const TSharedPtr<FVisualElement>& Element, const IntVector2& From, const TArray<IntVector2>& Waypoints)
{
    Element->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, From));

    IntVector2 Last = From;
    for (const IntVector2& Waypoint : Waypoints)
    {
        Element->OnMouseMove(MakeMoveEvent(Waypoint));
        Last = Waypoint;
    }

    Element->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Last));
}

/** @brief The first box command drawn on or above a layer, which is a control's own fill. */
static const FDrawCommand* FindFirstBox(const FDrawCommandList& CommandList)
{
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Box)
        {
            return &Command;
        }
    }

    return nullptr;
}

static FUIBrush MakeIcon()
{
    return FUIBrush(reinterpret_cast<FRHITexture*>(0x10));
}

static FFloatColor FindFirstOutlineColor(const FDrawCommandList& CommandList)
{
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::BoxOutline)
        {
            return Command.Tint;
        }
    }

    return FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
}

/** @brief Counts how many commands of a type the list holds. */
static int32 CountCommands(const FDrawCommandList& CommandList, EDrawCommandType Type)
{
    int32 Count = 0;
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == Type)
        {
            Count++;
        }
    }

    return Count;
}

/** @brief Draws an element that has already been arranged. */
static void DrawElement(const TSharedPtr<FVisualElement>& Element, FDrawCommandList& OutCommandList)
{
    Element->OnDraw(FDrawGeometry(Element->GetContentRectangle(), 1.0f), OutCommandList, 0);
}

bool ButtonControl_Test()
{
    TEST_BEGIN();

    const FUIStyle& Style = FUIStyle::GetDefault();

    int32 ClickCount = 0;

    FButton::FDesc Desc;
    Desc.SetText("Save").SetFont(CreateFont());
    Desc.OnClicked = FOnClicked::CreateLambda([&ClickCount]() { ClickCount++; });

    TSharedPtr<FButton> Button = FButton::Create(Desc);
    Button->PrepareDesiredSize();

    TEST_SECTION("The label and the button padding decide the width, and the button height the height");
    const IntVector2 DesiredSize = Button->GetCachedDesiredSize();
    TEST_EXPECT_EQ(DesiredSize.X, (4 * 8) + Style.Metrics.ButtonPadding.GetTotalHorizontal());
    TEST_EXPECT_EQ(DesiredSize.Y, Math::Max(16 + Style.Metrics.ButtonPadding.GetTotalVertical(), Style.Metrics.ButtonHeight));

    TEST_SECTION("Clicking it fires the delegate once");
    Button->Tick(FRectangle(IntVector2(10, 10), 120, 32));

    Button->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(50, 20)));
    Button->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(50, 20)));
    TEST_EXPECT_EQ(ClickCount, 1);

    TEST_SECTION("Releasing away from it does not");
    Button->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(50, 20)));
    Button->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(900, 900)));
    TEST_EXPECT_EQ(ClickCount, 1);

    TEST_SECTION("A disabled button ignores the click entirely");
    Button->SetEnabled(false);
    Button->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(50, 20)));
    Button->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(50, 20)));
    TEST_EXPECT_EQ(ClickCount, 1);
    Button->SetEnabled(true);

    TEST_SECTION("The fill follows the interaction state");
    FDrawCommandList NormalCommands;
    DrawElement(Button, NormalCommands);

    const FDrawCommand* NormalBox = FindFirstBox(NormalCommands);
    TEST_EXPECT(NormalBox != nullptr);
    TEST_EXPECT(NormalBox->Tint == Style.Colors.ButtonNormal);

    Button->OnMouseEntered(MakeMoveEvent(IntVector2(50, 20)));

    FDrawCommandList HoveredCommands;
    DrawElement(Button, HoveredCommands);

    const FDrawCommand* HoveredBox = FindFirstBox(HoveredCommands);
    TEST_EXPECT(HoveredBox != nullptr);
    TEST_EXPECT(HoveredBox->Tint == Style.Colors.ButtonHovered);

    TEST_SECTION("The label is drawn inside the fill, centred by default");
    TEST_EXPECT_EQ(CountCommands(NormalCommands, EDrawCommandType::Text), 1);

    TEST_SECTION("Content given outright replaces the label rather than sitting beside it");
    FTextBlock::FDesc IconDesc;
    IconDesc.Text = "@@";
    IconDesc.Font = CreateFont();

    FButton::FDesc ContentDesc;
    ContentDesc.SetText("ignored").SetFont(CreateFont());
    ContentDesc.Content = FTextBlock::Create(IconDesc);

    TSharedPtr<FButton> IconButton = FButton::Create(ContentDesc);
    IconButton->PrepareDesiredSize();
    TEST_EXPECT_EQ(IconButton->GetCachedDesiredSize().X, (2 * 8) + Style.Metrics.ButtonPadding.GetTotalHorizontal());
    TEST_EXPECT(IconButton->GetText().IsEmpty());

    TEST_SECTION("The keyboard activates it the way a click does");
    Button->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Space, FModifierKeyState(), false, true));
    TEST_EXPECT_EQ(ClickCount, 2);

    TEST_END();
}

bool CheckBoxControl_Test()
{
    TEST_BEGIN();

    int32          ChangeCount = 0;
    ECheckBoxState LastState   = ECheckBoxState::Unchecked;

    FCheckBox::FDesc Desc;
    Desc.SetText("Enabled").SetFont(CreateFont());
    Desc.OnStateChanged = FOnCheckStateChanged::CreateLambda([&](ECheckBoxState NewState)
    {
        ChangeCount++;
        LastState = NewState;
    });

    TSharedPtr<FCheckBox> CheckBox = FCheckBox::Create(Desc);
    LayoutElement(CheckBox, FRectangle(IntVector2(0, 0), 200, 20));

    TEST_SECTION("The box, the gap and the label together decide the width");
    TEST_EXPECT_EQ(CheckBox->GetCachedDesiredSize().X, 16 + 8 + (7 * 8));
    TEST_EXPECT_EQ(CheckBox->GetCachedDesiredSize().Y, 16);

    TEST_SECTION("A two-state box flips between checked and unchecked");
    TEST_EXPECT(CheckBox->GetCheckState() == ECheckBoxState::Unchecked);

    CheckBox->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(8, 10)));
    CheckBox->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(8, 10)));

    TEST_EXPECT(CheckBox->IsChecked());
    TEST_EXPECT_EQ(ChangeCount, 1);
    TEST_EXPECT(LastState == ECheckBoxState::Checked);

    CheckBox->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(8, 10)));
    CheckBox->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(8, 10)));

    TEST_EXPECT(!CheckBox->IsChecked());
    TEST_EXPECT_EQ(ChangeCount, 2);

    TEST_SECTION("Setting the state from outside does not fire the delegate");
    CheckBox->SetCheckState(ECheckBoxState::Checked);
    TEST_EXPECT(CheckBox->IsChecked());
    TEST_EXPECT_EQ(ChangeCount, 2);

    TEST_SECTION("A tri-state box walks unchecked, checked, undetermined and round again");
    FCheckBox::FDesc TriDesc;
    TriDesc.bIsTriState = true;

    TSharedPtr<FCheckBox> TriState = FCheckBox::Create(TriDesc);
    LayoutElement(TriState, FRectangle(IntVector2(0, 0), 20, 20));

    const ECheckBoxState Expected[] = { ECheckBoxState::Checked, ECheckBoxState::Undetermined, ECheckBoxState::Unchecked };
    for (const ECheckBoxState State : Expected)
    {
        TriState->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(8, 10)));
        TriState->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(8, 10)));
        TEST_EXPECT(TriState->GetCheckState() == State);
    }

    TEST_SECTION("The mark is only drawn once the box is set, and differs between the two set states");
    FDrawCommandList UncheckedCommands;
    DrawElement(TriState, UncheckedCommands);
    TEST_EXPECT_EQ(CountCommands(UncheckedCommands, EDrawCommandType::Polyline), 0);

    TriState->SetCheckState(ECheckBoxState::Checked);

    FDrawCommandList CheckedCommands;
    DrawElement(TriState, CheckedCommands);
    TEST_EXPECT_EQ(CountCommands(CheckedCommands, EDrawCommandType::Polyline), 1);

    TriState->SetCheckState(ECheckBoxState::Undetermined);

    FDrawCommandList MixedCommands;
    DrawElement(TriState, MixedCommands);
    TEST_EXPECT_EQ(CountCommands(MixedCommands, EDrawCommandType::Polyline), 0);
    TEST_EXPECT(CountCommands(MixedCommands, EDrawCommandType::Box) > CountCommands(UncheckedCommands, EDrawCommandType::Box));

    TEST_SECTION("The box keeps its square at the left edge whatever the label does");
    const FRectangle BoxBounds = CheckBox->GetBoxBounds();
    TEST_EXPECT_EQ(BoxBounds.Width, 16);
    TEST_EXPECT_EQ(BoxBounds.Height, 16);
    TEST_EXPECT_EQ(BoxBounds.Position.X, 0);

    TEST_END();
}

bool SliderControl_Test()
{
    TEST_BEGIN();

    TArray<float> Changes;
    TArray<float> Commits;

    FSlider::FDesc Desc;
    Desc.SetRange(0.0f, 100.0f).SetValue(0.0f);

    Desc.HandleSize       = 10;
    Desc.OnValueChanged   = FOnSliderValueChanged::CreateLambda([&Changes](float NewValue) { Changes.Add(NewValue); });
    Desc.OnValueCommitted = FOnSliderValueCommitted::CreateLambda([&Commits](float FinalValue) { Commits.Add(FinalValue); });

    TSharedPtr<FSlider> Slider = FSlider::Create(Desc);

    // A hundred pixels of travel between the two half-handle insets, so one pixel is one unit
    LayoutElement(Slider, FRectangle(IntVector2(0, 0), 110, 20));

    TEST_SECTION("The track is inset by half a handle at each end, so the handle never leaves the element");
    const FRectangle Track = Slider->GetTrackBounds();
    TEST_EXPECT_EQ(Track.Position.X, 5);
    TEST_EXPECT_EQ(Track.Width, 100);

    TEST_EXPECT_EQ(Slider->GetHandleBounds().Position.X, 0);
    Slider->SetValue(100.0f);
    TEST_EXPECT_EQ(Slider->GetHandleBounds().GetRight(), 110);
    Slider->SetValue(0.0f);

    TEST_SECTION("Clicking the track jumps the value there without waiting for a drag");
    Slider->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(30, 10)));
    TEST_EXPECT(Math::Abs(Slider->GetValue() - 25.0f) < 0.01f);
    TEST_EXPECT_EQ(Changes.Size(), 1);

    TEST_SECTION("Dragging reports every step and the release reports once");
    Slider->OnMouseMove(MakeMoveEvent(IntVector2(55, 10)));
    Slider->OnMouseMove(MakeMoveEvent(IntVector2(80, 10)));
    Slider->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(80, 10)));

    TEST_EXPECT_EQ(Changes.Size(), 3);
    TEST_EXPECT_EQ(Commits.Size(), 1);
    TEST_EXPECT(Math::Abs(Commits[0] - 75.0f) < 0.01f);

    TEST_SECTION("Dragging past either end clamps rather than running off");
    DragThrough(Slider, IntVector2(50, 10), { IntVector2(-900, 10) });
    TEST_EXPECT_EQ(Slider->GetValue(), 0.0f);

    DragThrough(Slider, IntVector2(50, 10), { IntVector2(900, 10) });
    TEST_EXPECT_EQ(Slider->GetValue(), 100.0f);

    TEST_SECTION("A step size snaps the value to its multiples");
    FSlider::FDesc SteppedDesc;
    SteppedDesc.SetRange(0.0f, 100.0f);
    SteppedDesc.StepSize   = 25.0f;
    SteppedDesc.HandleSize = 10;

    TSharedPtr<FSlider> Stepped = FSlider::Create(SteppedDesc);
    LayoutElement(Stepped, FRectangle(IntVector2(0, 0), 110, 20));

    Stepped->SetValue(30.0f);
    TEST_EXPECT_EQ(Stepped->GetValue(), 25.0f);

    Stepped->SetValue(63.0f);
    TEST_EXPECT_EQ(Stepped->GetValue(), 75.0f);

    TEST_SECTION("The arrows nudge by one step and Home and End go to the ends");
    Stepped->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Left, FModifierKeyState(), false, true));
    TEST_EXPECT_EQ(Stepped->GetValue(), 50.0f);

    Stepped->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Right, FModifierKeyState(), false, true));
    TEST_EXPECT_EQ(Stepped->GetValue(), 75.0f);

    Stepped->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Home, FModifierKeyState(), false, true));
    TEST_EXPECT_EQ(Stepped->GetValue(), 0.0f);

    Stepped->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::End, FModifierKeyState(), false, true));
    TEST_EXPECT_EQ(Stepped->GetValue(), 100.0f);

    TEST_SECTION("A vertical slider counts up from the bottom");
    FSlider::FDesc VerticalDesc;
    VerticalDesc.SetRange(0.0f, 100.0f);
    VerticalDesc.Orientation = EOrientation::Vertical;
    VerticalDesc.HandleSize  = 10;

    TSharedPtr<FSlider> Vertical = FSlider::Create(VerticalDesc);
    LayoutElement(Vertical, FRectangle(IntVector2(0, 0), 20, 110));

    Vertical->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(10, 105)));
    TEST_EXPECT(Vertical->GetValue() < 1.0f);

    Vertical->OnMouseMove(MakeMoveEvent(IntVector2(10, 5)));
    TEST_EXPECT(Vertical->GetValue() > 99.0f);

    Vertical->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(10, 5)));

    TEST_SECTION("A disabled slider does not move");
    Slider->SetValue(50.0f);
    Slider->SetEnabled(false);
    Slider->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(10, 10)));
    TEST_EXPECT_EQ(Slider->GetValue(), 50.0f);

    TEST_END();
}

bool SpinBoxControl_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Harness;

    TArray<float> Changes;
    TArray<float> Commits;

    FSpinBox::FDesc Desc;
    Desc.SetRange(-10.0f, 10.0f).SetValue(0.0f).SetFont(CreateFont());
    Desc.ScrubSpeed       = 0.1f;
    Desc.Precision        = 1;
    Desc.Prefix           = "X: ";
    Desc.OnValueChanged   = FOnSpinBoxValueChanged::CreateLambda([&Changes](float NewValue) { Changes.Add(NewValue); });
    Desc.OnValueCommitted = FOnSpinBoxValueCommitted::CreateLambda([&Commits](float FinalValue) { Commits.Add(FinalValue); });

    TSharedPtr<FSpinBox> SpinBox = FSpinBox::Create(Desc);
    LayoutElement(SpinBox, FRectangle(IntVector2(0, 0), 120, 24));

    TEST_SECTION("The value is written at the requested precision behind its prefix");
    TEST_EXPECT(SpinBox->GetFormattedValue() == String("0.0"));

    TEST_SECTION("A press that never moves opens the text box rather than scrubbing");
    SpinBox->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(60, 12)));
    SpinBox->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(60, 12)));

    TEST_EXPECT(SpinBox->IsTyping());
    TEST_EXPECT_EQ(Changes.Size(), 0);

    TEST_SECTION("Committing what was typed parses it and closes the box");
    SpinBox->EndTyping(false);
    TEST_EXPECT(!SpinBox->IsTyping());

    Commits.Clear();

    TEST_SECTION("A press that moves past the threshold scrubs and never opens the box");
    SpinBox->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(60, 12)));
    SpinBox->OnMouseMove(MakeMoveEvent(IntVector2(61, 12)));
    TEST_EXPECT_EQ(Changes.Size(), 0);

    SpinBox->OnMouseMove(MakeMoveEvent(IntVector2(80, 12)));
    TEST_EXPECT_EQ(Changes.Size(), 1);
    TEST_EXPECT(Math::Abs(SpinBox->GetValue() - 2.0f) < 0.01f);

    SpinBox->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(80, 12)));
    TEST_EXPECT(!SpinBox->IsTyping());
    TEST_EXPECT_EQ(Commits.Size(), 1);

    TEST_SECTION("Scrubbing past the range clamps rather than running off");
    SpinBox->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(60, 12)));
    SpinBox->OnMouseMove(MakeMoveEvent(IntVector2(6000, 12)));
    TEST_EXPECT_EQ(SpinBox->GetValue(), 10.0f);

    SpinBox->OnMouseMove(MakeMoveEvent(IntVector2(-6000, 12)));
    TEST_EXPECT_EQ(SpinBox->GetValue(), -10.0f);
    SpinBox->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(-6000, 12)));

    TEST_SECTION("Typing mode takes focus, and losing it commits on the next arrange");
    SpinBox->SetValue(1.0f);
    SpinBox->BeginTyping();
    TEST_EXPECT(SpinBox->IsTyping());
    TEST_EXPECT(SpinBox->GetFocusTarget() != StaticCastSharedPtr<FVisualElement>(SpinBox));

    Harness.GetApplication().SetFocusElement(nullptr);
    LayoutElement(SpinBox, FRectangle(IntVector2(0, 0), 120, 24));
    TEST_EXPECT(!SpinBox->IsTyping());

    TEST_SECTION("A drag cursor is offered while scrubbing is available and withheld while typing");
    ECursor Cursor = ECursor::Arrow;
    TEST_EXPECT(SpinBox->GetCursor(Cursor));
    TEST_EXPECT(Cursor == ECursor::ResizeEW);

    SpinBox->BeginTyping();
    TEST_EXPECT(!SpinBox->GetCursor(Cursor));
    SpinBox->EndTyping(false);

    TEST_END();
}

bool ScrollBarControl_Test()
{
    TEST_BEGIN();

    TArray<int32> Offsets;

    FScrollBar::FDesc Desc;
    Desc.Orientation     = EOrientation::Vertical;
    Desc.Thickness       = 12;
    Desc.MinThumbLength  = 20;
    Desc.TrackPadding    = FMargin();
    Desc.OnOffsetChanged = FOnScrollBarOffsetChanged::CreateLambda([&Offsets](int32 NewOffset) { Offsets.Add(NewOffset); });

    TSharedPtr<FScrollBar> ScrollBar = FScrollBar::Create(Desc);
    LayoutElement(ScrollBar, FRectangle(IntVector2(0, 0), 12, 200));

    TEST_SECTION("A bar with nothing to scroll has no travel and draws only its track");
    TEST_EXPECT(!ScrollBar->IsScrollable());
    TEST_EXPECT_EQ(ScrollBar->GetMaxOffset(), 0);

    FDrawCommandList EmptyCommands;
    DrawElement(ScrollBar, EmptyCommands);
    TEST_EXPECT_EQ(CountCommands(EmptyCommands, EDrawCommandType::Box), 1);

    TEST_SECTION("The thumb takes the fraction of the track that is in view");
    ScrollBar->SetScrollState(800, 200, 0);
    TEST_EXPECT(ScrollBar->IsScrollable());
    TEST_EXPECT_EQ(ScrollBar->GetMaxOffset(), 600);
    TEST_EXPECT(Math::Abs(ScrollBar->GetVisibleFraction() - 0.25f) < 0.001f);
    TEST_EXPECT_EQ(ScrollBar->GetThumbBounds().Height, 50);
    TEST_EXPECT_EQ(ScrollBar->GetThumbBounds().Position.Y, 0);

    TEST_SECTION("The thumb sits at the far end once the content is scrolled all the way");
    ScrollBar->SetScrollState(800, 200, 600);
    TEST_EXPECT_EQ(ScrollBar->GetThumbBounds().GetBottom(), 200);

    TEST_SECTION("A very long content still leaves a thumb big enough to grab");
    ScrollBar->SetScrollState(100000, 200, 0);
    TEST_EXPECT_EQ(ScrollBar->GetThumbBounds().Height, 20);

    TEST_SECTION("Grabbing the thumb anywhere holds that point under the cursor rather than snapping");
    ScrollBar->SetScrollState(800, 200, 0);
    Offsets.Clear();

    ScrollBar->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(6, 5)));
    ScrollBar->OnMouseMove(MakeMoveEvent(IntVector2(6, 5)));

    TEST_EXPECT_EQ(ScrollBar->GetOffset(), 0);
    TEST_EXPECT_EQ(Offsets.Size(), 0);

    TEST_SECTION("Dragging it reports the offset it lands on, and the far end is reachable");
    ScrollBar->OnMouseMove(MakeMoveEvent(IntVector2(6, 80)));
    TEST_EXPECT_EQ(ScrollBar->GetOffset(), 300);

    ScrollBar->OnMouseMove(MakeMoveEvent(IntVector2(6, 900)));
    ScrollBar->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(6, 900)));

    TEST_EXPECT_EQ(ScrollBar->GetOffset(), 600);
    TEST_EXPECT_EQ(Offsets.Size(), 2);

    TEST_SECTION("Clicking the empty track centres the thumb under the cursor");
    ScrollBar->SetScrollState(800, 200, 0);
    Offsets.Clear();

    ScrollBar->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(6, 100)));
    TEST_EXPECT_EQ(Offsets.Size(), 1);
    TEST_EXPECT_EQ(ScrollBar->GetOffset(), 300);

    ScrollBar->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(6, 100)));

    TEST_SECTION("An offset pushed in from the host is clamped and fires nothing");
    Offsets.Clear();
    ScrollBar->SetOffset(9999);
    TEST_EXPECT_EQ(ScrollBar->GetOffset(), 600);
    TEST_EXPECT_EQ(Offsets.Size(), 0);

    TEST_SECTION("A horizontal bar is the same thing on the other axis");
    FScrollBar::FDesc HorizontalDesc;
    HorizontalDesc.Orientation  = EOrientation::Horizontal;
    HorizontalDesc.TrackPadding = FMargin();

    TSharedPtr<FScrollBar> Horizontal = FScrollBar::Create(HorizontalDesc);
    LayoutElement(Horizontal, FRectangle(IntVector2(0, 0), 200, 12));

    Horizontal->SetScrollState(400, 200, 200);
    TEST_EXPECT_EQ(Horizontal->GetThumbBounds().Width, 100);
    TEST_EXPECT_EQ(Horizontal->GetThumbBounds().GetRight(), 200);

    TEST_END();
}

bool ScrollBoxScrollBar_Test()
{
    TEST_BEGIN();

    FTextBlock::FDesc TallDesc;
    TallDesc.Text = "Line";
    TallDesc.Font = CreateFont();

    TSharedPtr<FTextBlock> Tall = FTextBlock::Create(TallDesc);
    Tall->SetMargin(FMargin(0, 0, 0, 400));

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    ScrollBox->SetContent(Tall);

    TEST_SECTION("Nothing to scroll leaves the bar out and the content the full width");
    TSharedPtr<FTextBlock> Short = FTextBlock::Create(TallDesc);

    TSharedPtr<FScrollBox> ShortBox = FScrollBox::Create();
    ShortBox->SetContent(Short);
    LayoutElement(ShortBox, FRectangle(IntVector2(0, 0), 200, 300));

    TEST_EXPECT(!ShortBox->IsScrollBarVisible());
    TEST_EXPECT_EQ(Short->GetContentRectangle().Width, 200);

    TEST_SECTION("Content taller than the view brings the bar in beside it");
    LayoutElement(ScrollBox, FRectangle(IntVector2(0, 0), 200, 100));
    LayoutElement(ScrollBox, FRectangle(IntVector2(0, 0), 200, 100));

    TEST_EXPECT(ScrollBox->IsScrollBarVisible());

    const TSharedPtr<FScrollBar>& Bar = ScrollBox->GetScrollBar();
    TEST_EXPECT(Bar != nullptr);
    TEST_EXPECT_EQ(Bar->GetContentRectangle().GetRight(), 200);
    TEST_EXPECT_EQ(Tall->GetContentRectangle().Width, 200 - Bar->GetCachedDesiredSize().X);

    TEST_SECTION("The bar knows what the box knows, without either being told twice");
    TEST_EXPECT_EQ(Bar->GetMaxOffset(), ScrollBox->GetMaxScrollOffset());
    TEST_EXPECT_EQ(Bar->GetOffset(), ScrollBox->GetScrollOffset());

    TEST_SECTION("Dragging the bar scrolls the box");
    DragThrough(Bar, Bar->GetThumbBounds().GetCenter(), { IntVector2(Bar->GetThumbBounds().GetCenter().X, 90) });

    TEST_EXPECT(ScrollBox->GetScrollOffset() > 0);
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), Bar->GetOffset());

    TEST_SECTION("Always keeps the strip reserved even when everything fits");
    ShortBox->SetScrollBarVisibility(EScrollBarVisibility::Always);
    LayoutElement(ShortBox, FRectangle(IntVector2(0, 0), 200, 300));

    TEST_EXPECT(ShortBox->IsScrollBarVisible());
    TEST_EXPECT(Short->GetContentRectangle().Width < 200);

    TEST_SECTION("Never leaves the wheel as the only way to scroll");
    ScrollBox->SetScrollBarVisibility(EScrollBarVisibility::Never);
    LayoutElement(ScrollBox, FRectangle(IntVector2(0, 0), 200, 100));

    TEST_EXPECT(!ScrollBox->IsScrollBarVisible());
    TEST_EXPECT_EQ(Tall->GetContentRectangle().Width, 200);

    TEST_END();
}

bool OverlayControl_Test()
{
    TEST_BEGIN();

    FTextBlock::FDesc WideDesc;
    WideDesc.Text = "0123456789";
    WideDesc.Font = CreateFont();

    FTextBlock::FDesc BadgeDesc;
    BadgeDesc.Text = "9";
    BadgeDesc.Font = CreateFont();

    TSharedPtr<FTextBlock> Base  = FTextBlock::Create(WideDesc);
    TSharedPtr<FTextBlock> Badge = FTextBlock::Create(BadgeDesc);

    TSharedPtr<FOverlay> Overlay = FOverlay::Create();
    Overlay->AddSlot(Base);
    Overlay->AddSlot(Badge).SetHorizontalAlignment(EHorizontalAlignment::Right).SetVerticalAlignment(EVerticalAlignment::Top);

    LayoutElement(Overlay, FRectangle(IntVector2(0, 0), 200, 100));

    TEST_SECTION("The overlay is as big as its largest layer rather than their sum");
    TEST_EXPECT_EQ(Overlay->GetCachedDesiredSize().X, 80);
    TEST_EXPECT_EQ(Overlay->GetCachedDesiredSize().Y, 16);

    TEST_SECTION("A filling layer takes the whole rectangle and an aligned one only its corner");
    TEST_EXPECT_EQ(Base->GetContentRectangle().Width, 200);
    TEST_EXPECT_EQ(Base->GetContentRectangle().Height, 100);

    TEST_EXPECT_EQ(Badge->GetContentRectangle().Width, 8);
    TEST_EXPECT_EQ(Badge->GetContentRectangle().GetRight(), 200);
    TEST_EXPECT_EQ(Badge->GetContentRectangle().Position.Y, 0);

    TEST_SECTION("Later layers draw over earlier ones");
    FDrawCommandList Commands;
    DrawElement(Overlay, Commands);

    TEST_EXPECT_EQ(CountCommands(Commands, EDrawCommandType::Text), 2);

    int32 BaseLayer  = -1;
    int32 BadgeLayer = -1;
    for (const FDrawCommand& Command : Commands.GetCommands())
    {
        if (Command.Type != EDrawCommandType::Text)
        {
            continue;
        }

        if (Command.Text == String("0123456789"))
        {
            BaseLayer = Command.LayerId;
        }
        else if (Command.Text == String("9"))
        {
            BadgeLayer = Command.LayerId;
        }
    }

    TEST_EXPECT(BaseLayer >= 0);
    TEST_EXPECT(BadgeLayer > BaseLayer);

    TEST_SECTION("A click lands on the topmost layer under it");
    FElementPath Path;
    Overlay->FindChildrenContainingPoint(IntVector2(196, 4), Path);

    TEST_EXPECT(Path.GetElements().Size() >= 2);
    TEST_EXPECT(Path.GetElements().Last() == StaticCastSharedPtr<FVisualElement>(Badge));

    FElementPath BasePath;
    Overlay->FindChildrenContainingPoint(IntVector2(20, 60), BasePath);
    TEST_EXPECT(BasePath.GetElements().Last() == StaticCastSharedPtr<FVisualElement>(Base));

    TEST_SECTION("Removing a layer takes it out of measurement and hit testing alike");
    TEST_EXPECT(Overlay->RemoveSlot(Badge));
    TEST_EXPECT_EQ(Overlay->GetSlots().Size(), 1);
    TEST_EXPECT(!Overlay->RemoveSlot(Badge));

    TEST_END();
}

bool SpacerSeparatorControl_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A spacer asks for its size and draws nothing");
    TSharedPtr<FSpacer> Spacer = FSpacer::Create(IntVector2(24, 8));
    LayoutElement(Spacer, FRectangle(IntVector2(0, 0), 24, 8));

    TEST_EXPECT_EQ(Spacer->GetCachedDesiredSize().X, 24);
    TEST_EXPECT_EQ(Spacer->GetCachedDesiredSize().Y, 8);

    FDrawCommandList SpacerCommands;
    DrawElement(Spacer, SpacerCommands);
    TEST_EXPECT_EQ(SpacerCommands.GetCommands().Size(), 0);

    TEST_SECTION("A one-axis spacer leaves the other axis alone");
    TEST_EXPECT_EQ(FSpacer::CreateHorizontal(16)->ComputeDesiredSize().Y, 0);
    TEST_EXPECT_EQ(FSpacer::CreateVertical(16)->ComputeDesiredSize().X, 0);

    TEST_SECTION("A horizontal rule is thick on one axis and asks for nothing on the other");
    TSharedPtr<FSeparator> Horizontal = FSeparator::CreateHorizontal();
    LayoutElement(Horizontal, FRectangle(IntVector2(0, 0), 200, 20));

    TEST_EXPECT_EQ(Horizontal->GetCachedDesiredSize().X, 0);
    TEST_EXPECT_EQ(Horizontal->GetCachedDesiredSize().Y, FUIStyle::GetDefault().Metrics.SeparatorThickness);

    TEST_SECTION("The line is centred in whatever space it is given");
    FDrawCommandList Commands;
    DrawElement(Horizontal, Commands);

    const FDrawCommand* Line = FindFirstBox(Commands);
    TEST_EXPECT(Line != nullptr);
    TEST_EXPECT_EQ(Line->Bounds.Width, 200);
    TEST_EXPECT_EQ(Line->Bounds.Height, FUIStyle::GetDefault().Metrics.SeparatorThickness);
    TEST_EXPECT_EQ(Line->Bounds.Position.Y, (20 - FUIStyle::GetDefault().Metrics.SeparatorThickness) / 2);

    TEST_SECTION("A vertical rule is the same thing turned ninety degrees");
    TSharedPtr<FSeparator> Vertical = FSeparator::CreateVertical();
    LayoutElement(Vertical, FRectangle(IntVector2(0, 0), 20, 200));

    TEST_EXPECT_EQ(Vertical->GetCachedDesiredSize().Y, 0);

    FDrawCommandList VerticalCommands;
    DrawElement(Vertical, VerticalCommands);

    const FDrawCommand* VerticalLine = FindFirstBox(VerticalCommands);
    TEST_EXPECT(VerticalLine != nullptr);
    TEST_EXPECT_EQ(VerticalLine->Bounds.Height, 200);
    TEST_EXPECT_EQ(VerticalLine->Bounds.Width, FUIStyle::GetDefault().Metrics.SeparatorThickness);

    TEST_SECTION("Padding insets the ends of the line without changing its thickness");
    FSeparator::FDesc InsetDesc;
    InsetDesc.Padding = FMargin(10, 0, 10, 0);

    TSharedPtr<FSeparator> Inset = FSeparator::Create(InsetDesc);
    LayoutElement(Inset, FRectangle(IntVector2(0, 0), 200, 20));

    FDrawCommandList InsetCommands;
    DrawElement(Inset, InsetCommands);

    const FDrawCommand* InsetLine = FindFirstBox(InsetCommands);
    TEST_EXPECT(InsetLine != nullptr);
    TEST_EXPECT_EQ(InsetLine->Bounds.Position.X, 10);
    TEST_EXPECT_EQ(InsetLine->Bounds.Width, 180);

    TEST_END();
}

bool ExpanderControl_Test()
{
    TEST_BEGIN();

    constexpr int32 HeaderHeight = 20;

    const FMargin ContentPadding(12, 4, 4, 4);

    TArray<bool> StateChanges;

    FTextBlock::FDesc ContentDesc;
    ContentDesc.Text = "Body";
    ContentDesc.Font = CreateFont();

    TSharedPtr<FTextBlock> Content = FTextBlock::Create(ContentDesc);

    FExpander::FDesc Desc;
    Desc.Label          = "Section";
    Desc.Font           = CreateFont();
    Desc.Content        = Content;
    Desc.bIsExpanded    = false;
    Desc.HeaderHeight   = HeaderHeight;
    Desc.ContentPadding = ContentPadding;
    Desc.OnStateChanged = FOnExpanderStateChanged::CreateLambda([&StateChanges](bool bNewState) { StateChanges.Add(bNewState); });

    TSharedPtr<FExpander> Expander = FExpander::Create(Desc);
    LayoutElement(Expander, FRectangle(IntVector2(0, 0), 200, 100));

    TEST_SECTION("A closed section asks only for the height of its header");
    const IntVector2 ClosedSize = Expander->GetCachedDesiredSize();
    TEST_EXPECT_EQ(ClosedSize.Y, HeaderHeight);

    TEST_SECTION("The content is measured even while the section is closed, so opening it needs no further pass");
    TEST_EXPECT_EQ(Content->GetCachedDesiredSize().X, 4 * 8);
    TEST_EXPECT_EQ(Content->GetCachedDesiredSize().Y, 16);

    TEST_SECTION("Clicking the header opens the section and reports the state it moved to, once");
    Expander->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(20, 10)));

    TEST_EXPECT(Expander->IsExpanded());
    TEST_EXPECT_EQ(StateChanges.Size(), 1);
    TEST_EXPECT(StateChanges[0]);

    TEST_SECTION("An open one asks for the header, the content and the content padding together");
    Expander->PrepareDesiredSize();

    const IntVector2 OpenSize = Expander->GetCachedDesiredSize();
    TEST_EXPECT_EQ(OpenSize.Y, HeaderHeight + Content->GetCachedDesiredSize().Y + ContentPadding.GetTotalVertical());
    TEST_EXPECT_EQ(OpenSize.X, Math::Max(ClosedSize.X, Content->GetCachedDesiredSize().X + ContentPadding.GetTotalHorizontal()));

    TEST_SECTION("Clicking it again closes the section");
    Expander->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(20, 10)));

    TEST_EXPECT(!Expander->IsExpanded());
    TEST_EXPECT_EQ(StateChanges.Size(), 2);
    TEST_EXPECT(!StateChanges[1]);

    TEST_SECTION("A press below the header leaves the section as it was");
    Expander->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(20, 60)));

    TEST_EXPECT(!Expander->IsExpanded());
    TEST_EXPECT_EQ(StateChanges.Size(), 2);

    TEST_SECTION("Setting the state it already holds fires nothing, and setting the other one fires once");
    Expander->SetExpanded(false);
    TEST_EXPECT_EQ(StateChanges.Size(), 2);

    Expander->SetExpanded(true);
    TEST_EXPECT(Expander->IsExpanded());
    TEST_EXPECT_EQ(StateChanges.Size(), 3);

    TEST_SECTION("Setting the content takes its parent over, and the children hand it back");
    TSharedPtr<FTextBlock> Replacement = FTextBlock::Create(ContentDesc);
    Expander->SetContent(Replacement);

    TArray<TSharedPtr<FVisualElement>> Children;
    Expander->GetChildren(Children);

    TEST_EXPECT_EQ(Children.Size(), 1);
    TEST_EXPECT(Children[0] == StaticCastSharedPtr<FVisualElement>(Replacement));
    TEST_EXPECT(Expander->GetContent() == StaticCastSharedPtr<FVisualElement>(Replacement));
    TEST_EXPECT(Replacement->GetParentElement().Get() == Expander.Get());

    TEST_SECTION("The label the header shows can be replaced afterwards");
    Expander->SetLabel("Renamed");
    TEST_EXPECT(Expander->GetLabel() == String("Renamed"));

    TEST_END();
}

bool SearchBoxControl_Test()
{
    TEST_BEGIN();

    const FRectangle Bounds(IntVector2(0, 0), 200, 24);

    TArray<String> Changes;

    FInputFrameStyle FrameStyle;
    FrameStyle.Fill          = FFloatColor(0.05f, 0.05f, 0.05f, 1.0f);
    FrameStyle.BorderNormal  = FFloatColor(0.20f, 0.20f, 0.20f, 1.0f);
    FrameStyle.BorderHovered = FFloatColor(0.30f, 0.30f, 0.30f, 1.0f);
    FrameStyle.BorderFocused = FFloatColor(0.40f, 0.40f, 0.40f, 1.0f);

    FSearchBox::FDesc Desc;
    Desc.HintText      = "Search";
    Desc.Font          = CreateFont();
    Desc.SearchIcon    = MakeIcon();
    Desc.ClearIcon     = MakeIcon();
    Desc.Style         = FrameStyle;
    Desc.OnTextChanged = FOnSearchTextChanged::CreateLambda([&Changes](const String& NewText) { Changes.Add(NewText); });

    TSharedPtr<FSearchBox> SearchBox = FSearchBox::Create(Desc);
    LayoutElement(SearchBox, Bounds);

    TEST_SECTION("A new box holds nothing, so there is no clear button to press");
    TEST_EXPECT(SearchBox->IsEmpty());
    TEST_EXPECT(SearchBox->GetText().IsEmpty());
    TEST_EXPECT(SearchBox->GetClearButtonRectangle(Bounds).IsEmpty());

    TEST_SECTION("The magnifier holds the left square of an empty field");
    const FRectangle IconBounds = SearchBox->GetSearchIconRectangle(Bounds);
    TEST_EXPECT(!IconBounds.IsEmpty());
    TEST_EXPECT_EQ(IconBounds.Position.X, Bounds.Position.X + Desc.Padding.Left);

    TEST_SECTION("The line the text is typed into is a child of the box, and starts past that square");
    TArray<TSharedPtr<FVisualElement>> Children;
    SearchBox->GetChildren(Children);

    TEST_EXPECT(SearchBox->GetEditor() != nullptr);
    TEST_EXPECT_EQ(Children.Size(), 1);
    TEST_EXPECT(Children[0] == StaticCastSharedPtr<FVisualElement>(SearchBox->GetEditor()));

    const FRectangle EmptyEditorBounds = SearchBox->GetEditor()->GetContentRectangle();
    TEST_EXPECT(EmptyEditorBounds.Position.X > IconBounds.GetRight());

    TEST_SECTION("Nothing is reserved at the right, so the text runs to the far padding");
    TEST_EXPECT_EQ(EmptyEditorBounds.GetRight(), Bounds.GetRight() - Desc.Padding.Right);

    TEST_SECTION("Setting the text fills the field and reports what it now holds");
    SearchBox->SetText("Mesh");

    TEST_EXPECT(!SearchBox->IsEmpty());
    TEST_EXPECT(SearchBox->GetText() == String("Mesh"));
    TEST_EXPECT_EQ(Changes.Size(), 1);
    TEST_EXPECT(Changes[0] == String("Mesh"));

    TEST_SECTION("The clear button takes over the very square the magnifier held, which goes away");
    const FRectangle ClearBounds = SearchBox->GetClearButtonRectangle(Bounds);
    TEST_EXPECT(!ClearBounds.IsEmpty());
    TEST_EXPECT(ClearBounds == IconBounds);
    TEST_EXPECT(SearchBox->GetSearchIconRectangle(Bounds).IsEmpty());

    TEST_SECTION("Swapping the two leaves the line of text where it was");
    LayoutElement(SearchBox, Bounds);
    TEST_EXPECT(SearchBox->GetEditor()->GetContentRectangle() == EmptyEditorBounds);

    TEST_SECTION("Clicking it empties the field and reports that too");
    SearchBox->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, ClearBounds.GetCenter()));

    TEST_EXPECT(SearchBox->IsEmpty());
    TEST_EXPECT_EQ(Changes.Size(), 2);
    TEST_EXPECT(Changes[1].IsEmpty());
    TEST_EXPECT(SearchBox->GetClearButtonRectangle(Bounds).IsEmpty());

    TEST_SECTION("Clearing it outright does the same without the click");
    SearchBox->SetText("Material");
    TEST_EXPECT(!SearchBox->IsEmpty());
    TEST_EXPECT_EQ(Changes.Size(), 3);

    SearchBox->ClearText();
    TEST_EXPECT(SearchBox->IsEmpty());
    TEST_EXPECT_EQ(Changes.Size(), 4);
    TEST_EXPECT(Changes[3].IsEmpty());

    TEST_SECTION("The frame is filled from the description rather than the process-wide style");
    FDrawCommandList NormalList;
    DrawElement(SearchBox, NormalList);

    const FDrawCommand* FrameFill = FindFirstBox(NormalList);
    TEST_EXPECT(FrameFill != nullptr);
    TEST_EXPECT(FrameFill->Tint == FrameStyle.Fill);

    TEST_SECTION("An untouched field carries the normal stroke");
    TEST_EXPECT(FindFirstOutlineColor(NormalList) == FrameStyle.BorderNormal);

    TEST_SECTION("The cursor resting over it replaces that with the hovered one");
    SearchBox->OnMouseEntered(MakeMoveEvent(Bounds.GetCenter()));

    FDrawCommandList HoveredList;
    DrawElement(SearchBox, HoveredList);
    TEST_EXPECT(FindFirstOutlineColor(HoveredList) == FrameStyle.BorderHovered);

    TEST_SECTION("Focus outranks hover, so the focused stroke wins while typing lands here");
    SearchBox->GetEditor()->OnFocusGained();

    FDrawCommandList FocusedList;
    DrawElement(SearchBox, FocusedList);
    TEST_EXPECT(FindFirstOutlineColor(FocusedList) == FrameStyle.BorderFocused);

    TEST_SECTION("Losing both puts the normal stroke back");
    SearchBox->GetEditor()->OnFocusLost();
    SearchBox->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));

    FDrawCommandList IdleList;
    DrawElement(SearchBox, IdleList);
    TEST_EXPECT(FindFirstOutlineColor(IdleList) == FrameStyle.BorderNormal);

    TEST_END();
}

bool NumericEntryControl_Test()
{
    TEST_BEGIN();

    const FRectangle Bounds(IntVector2(0, 0), 120, 24);
    const int32      Travel = FNumericEntryFloat::ScrubThreshold + 7;

    TArray<float> FloatChanges;

    FNumericEntryFloat::FDesc FloatDesc;
    FloatDesc.Label          = "X";
    FloatDesc.Font           = CreateFont();
    FloatDesc.MinValue       = -100.0f;
    FloatDesc.MaxValue       = 100.0f;
    FloatDesc.Step           = 0.5f;
    FloatDesc.OnValueChanged = FNumericEntryFloat::FOnValueChanged::CreateLambda([&FloatChanges](float NewValue) { FloatChanges.Add(NewValue); });

    TSharedPtr<FNumericEntryFloat> FloatEntry = FNumericEntryFloat::Create(FloatDesc);
    LayoutElement(FloatEntry, Bounds);

    TEST_SECTION("A value pushed in from the host is clamped to the range and reports nothing back");
    FloatEntry->SetValue(500.0f);
    TEST_EXPECT_EQ(FloatEntry->GetValue(), 100.0f);

    FloatEntry->SetValue(-500.0f);
    TEST_EXPECT_EQ(FloatEntry->GetValue(), -100.0f);

    FloatEntry->SetValue(0.0f);
    TEST_EXPECT_EQ(FloatEntry->GetValue(), 0.0f);
    TEST_EXPECT_EQ(FloatChanges.Size(), 0);

    TEST_SECTION("A drag past the scrub threshold moves the value by the travel times the step, and reports it");
    DragThrough(FloatEntry, IntVector2(40, 12), { IntVector2(40 + Travel, 12) });

    const float ScrubbedValue = static_cast<float>(Travel) * FloatDesc.Step;
    TEST_EXPECT(Math::Abs(FloatEntry->GetValue() - ScrubbedValue) < 0.001f);
    TEST_EXPECT_EQ(FloatChanges.Size(), 1);
    TEST_EXPECT(Math::Abs(FloatChanges[0] - ScrubbedValue) < 0.001f);
    TEST_EXPECT(!FloatEntry->IsScrubbing());

    TEST_SECTION("A drag shorter than the threshold is a click rather than a scrub, so the value stands still");
    DragThrough(FloatEntry, IntVector2(40, 12), { IntVector2(40 + FNumericEntryFloat::ScrubThreshold - 1, 12) });

    TEST_EXPECT(Math::Abs(FloatEntry->GetValue() - ScrubbedValue) < 0.001f);
    TEST_EXPECT_EQ(FloatChanges.Size(), 1);

    TEST_SECTION("The coloured tag takes the label width at the left, and the value the room beside it");
    const FRectangle LabelBounds = FloatEntry->GetLabelRectangle(Bounds);
    TEST_EXPECT_EQ(LabelBounds.Position.X, Bounds.Position.X);
    TEST_EXPECT_EQ(LabelBounds.Width, FloatDesc.LabelWidth);
    TEST_EXPECT_EQ(LabelBounds.Height, Bounds.Height);

    const FRectangle EditorBounds = FloatEntry->GetEditorRectangle(Bounds);
    TEST_EXPECT_EQ(EditorBounds.Position.X, LabelBounds.GetRight() + FNumericEntryFloat::TextInset);
    TEST_EXPECT_EQ(EditorBounds.GetRight(), Bounds.GetRight() - FNumericEntryFloat::TextInset);
    TEST_EXPECT_EQ(EditorBounds.Height, Bounds.Height);

    TEST_SECTION("Hiding the tag empties its rectangle and hands the value the whole field");
    FNumericEntryFloat::FDesc HiddenDesc;
    HiddenDesc.Font       = CreateFont();
    HiddenDesc.bShowLabel = false;

    TSharedPtr<FNumericEntryFloat> Hidden = FNumericEntryFloat::Create(HiddenDesc);
    LayoutElement(Hidden, Bounds);

    TEST_EXPECT(Hidden->GetLabelRectangle(Bounds).IsEmpty());
    TEST_EXPECT_EQ(Hidden->GetEditorRectangle(Bounds).Position.X, Bounds.Position.X + FNumericEntryFloat::TextInset);

    TEST_SECTION("The field never asks to be narrower than its minimum width");
    TEST_EXPECT_EQ(Hidden->GetCachedDesiredSize().X, FNumericEntryFloat::MinWidth);
    TEST_EXPECT(FloatEntry->GetCachedDesiredSize().X >= FNumericEntryFloat::MinWidth);

    TEST_SECTION("An integer field is the same thing counting in whole steps");
    TArray<int32> IntChanges;

    FNumericEntryInt::FDesc IntDesc;
    IntDesc.Label          = "Y";
    IntDesc.Font           = CreateFont();
    IntDesc.MinValue       = -100;
    IntDesc.MaxValue       = 100;
    IntDesc.Step           = 2;
    IntDesc.OnValueChanged = FNumericEntryInt::FOnValueChanged::CreateLambda([&IntChanges](int32 NewValue) { IntChanges.Add(NewValue); });

    TSharedPtr<FNumericEntryInt> IntEntry = FNumericEntryInt::Create(IntDesc);
    LayoutElement(IntEntry, Bounds);

    IntEntry->SetValue(1000);
    TEST_EXPECT_EQ(IntEntry->GetValue(), 100);

    IntEntry->SetValue(0);
    TEST_EXPECT_EQ(IntChanges.Size(), 0);

    DragThrough(IntEntry, IntVector2(40, 12), { IntVector2(40 + Travel, 12) });

    TEST_EXPECT_EQ(IntEntry->GetValue(), Travel * IntDesc.Step);
    TEST_EXPECT_EQ(IntChanges.Size(), 1);
    TEST_EXPECT_EQ(IntChanges[0], Travel * IntDesc.Step);
    TEST_EXPECT(IntEntry->GetCachedDesiredSize().X >= FNumericEntryInt::MinWidth);

    TEST_SECTION("A maximum set below the minimum is raised to it, so the range holds the one value");
    FNumericEntryInt::FDesc InvertedDesc;
    InvertedDesc.Font     = CreateFont();
    InvertedDesc.MinValue = 5;
    InvertedDesc.MaxValue = 1;
    InvertedDesc.Value    = 0;

    TSharedPtr<FNumericEntryInt> Inverted = FNumericEntryInt::Create(InvertedDesc);
    TEST_EXPECT_EQ(Inverted->GetValue(), 5);

    Inverted->SetValue(1000);
    TEST_EXPECT_EQ(Inverted->GetValue(), 5);

    Inverted->SetValue(-1000);
    TEST_EXPECT_EQ(Inverted->GetValue(), 5);

    TEST_SECTION("Enter on the line of text reads the typed value back and reports it");
    FloatEntry->SetValue(0.0f);
    const int32 ChangesBeforeCommit = FloatChanges.Size();

    FloatEntry->GetEditor()->SetText("12.5");
    FloatEntry->GetEditor()->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Enter, FModifierKeyState(), false, true));

    TEST_EXPECT(Math::Abs(FloatEntry->GetValue() - 12.5f) < 0.001f);
    TEST_EXPECT_EQ(FloatChanges.Size(), ChangesBeforeCommit + 1);

    TEST_SECTION("Committing rewrites the line in the form the field formats values in");
    TEST_EXPECT(FloatEntry->GetEditor()->GetText() == String("12.500"));

    TEST_SECTION("A typed value outside the range is clamped the same way a pushed one is");
    FloatEntry->GetEditor()->SetText("999");
    FloatEntry->GetEditor()->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Enter, FModifierKeyState(), false, true));

    TEST_EXPECT_EQ(FloatEntry->GetValue(), 100.0f);

    TEST_SECTION("Committing a value the field already holds reports nothing");
    const int32 ChangesBeforeRepeat = FloatChanges.Size();

    FloatEntry->GetEditor()->SetText("100");
    FloatEntry->GetEditor()->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Enter, FModifierKeyState(), false, true));

    TEST_EXPECT_EQ(FloatEntry->GetValue(), 100.0f);
    TEST_EXPECT_EQ(FloatChanges.Size(), ChangesBeforeRepeat);

    TEST_SECTION("Text that names no number parses as zero, which the range then clamps");
    FloatEntry->GetEditor()->SetText("not a number");
    FloatEntry->GetEditor()->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Enter, FModifierKeyState(), false, true));

    TEST_EXPECT_EQ(FloatEntry->GetValue(), 0.0f);

    TEST_SECTION("An integer line commits the same way, and the step does not quantize what was typed");
    IntEntry->SetValue(0);
    const int32 IntChangesBeforeCommit = IntChanges.Size();

    IntEntry->GetEditor()->SetText("7");
    IntEntry->GetEditor()->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Enter, FModifierKeyState(), false, true));

    TEST_EXPECT_EQ(IntEntry->GetValue(), 7);
    TEST_EXPECT_EQ(IntChanges.Size(), IntChangesBeforeCommit + 1);
    TEST_EXPECT(IntEntry->GetEditor()->GetText() == String("7"));

    TEST_END();
}

bool ProgressBarControl_Test()
{
    TEST_BEGIN();

    constexpr int32 PreferredHeight = 20;

    const FRectangle Bounds(IntVector2(0, 0), 200, PreferredHeight);

    FProgressBar::FDesc Desc;
    Desc.Percent         = 0.25f;
    Desc.PreferredHeight = PreferredHeight;

    TSharedPtr<FProgressBar> ProgressBar = FProgressBar::Create(Desc);
    LayoutElement(ProgressBar, Bounds);

    TEST_SECTION("The bar asks for its preferred height and leaves the width to whatever holds it");
    TEST_EXPECT_EQ(ProgressBar->GetCachedDesiredSize().Y, PreferredHeight);
    TEST_EXPECT_EQ(ProgressBar->GetCachedDesiredSize().X, 0);

    TEST_SECTION("A percent pushed in from outside is clamped between empty and full");
    ProgressBar->SetPercent(-1.0f);
    TEST_EXPECT_EQ(ProgressBar->GetPercent(), 0.0f);

    ProgressBar->SetPercent(4.0f);
    TEST_EXPECT_EQ(ProgressBar->GetPercent(), 1.0f);

    TEST_SECTION("The fill is drawn over the track and takes the share of it the percent names");
    ProgressBar->SetPercent(0.25f);

    FDrawCommandList QuarterCommands;
    DrawElement(ProgressBar, QuarterCommands);

    TEST_EXPECT_EQ(CountCommands(QuarterCommands, EDrawCommandType::Box), 2);

    const FDrawCommand* Track = FindFirstBox(QuarterCommands);
    TEST_EXPECT(Track != nullptr);
    TEST_EXPECT_EQ(Track->Bounds.Width, Bounds.Width);

    TEST_EXPECT_EQ(QuarterCommands[1].Bounds.Width, Bounds.Width / 4);
    TEST_EXPECT_EQ(QuarterCommands[1].Bounds.Position.X, Bounds.Position.X);
    TEST_EXPECT_EQ(QuarterCommands[1].Bounds.Height, Bounds.Height);

    TEST_SECTION("A bar at zero percent draws no fill at all");
    ProgressBar->SetPercent(0.0f);

    FDrawCommandList EmptyCommands;
    DrawElement(ProgressBar, EmptyCommands);
    TEST_EXPECT_EQ(CountCommands(EmptyCommands, EDrawCommandType::Box), 1);

    TEST_SECTION("A new fill color is the one that reaches the command list");
    ProgressBar->SetPercent(0.5f);
    ProgressBar->SetFillColor(FFloatColor::Red);

    FDrawCommandList RedCommands;
    DrawElement(ProgressBar, RedCommands);

    TEST_EXPECT_EQ(CountCommands(RedCommands, EDrawCommandType::Box), 2);
    TEST_EXPECT(RedCommands[1].Tint == FFloatColor::Red);

    TEST_SECTION("The overlay text waits for a font before there is anything to draw it with");
    ProgressBar->SetOverlayText("50%");
    TEST_EXPECT(ProgressBar->GetOverlayText() == String("50%"));

    FDrawCommandList FontlessCommands;
    DrawElement(ProgressBar, FontlessCommands);
    TEST_EXPECT_EQ(CountCommands(FontlessCommands, EDrawCommandType::Text), 0);

    FProgressBar::FDesc LabelledDesc;
    LabelledDesc.Percent         = 0.5f;
    LabelledDesc.OverlayText     = "50%";
    LabelledDesc.Font            = CreateFont();
    LabelledDesc.PreferredHeight = PreferredHeight;

    TSharedPtr<FProgressBar> Labelled = FProgressBar::Create(LabelledDesc);
    LayoutElement(Labelled, Bounds);

    FDrawCommandList LabelledCommands;
    DrawElement(Labelled, LabelledCommands);
    TEST_EXPECT_EQ(CountCommands(LabelledCommands, EDrawCommandType::Text), 1);

    TEST_END();
}

bool HistogramControl_Test()
{
    TEST_BEGIN();

    constexpr int32 Capacity = 4;

    const FUIStyle&  Style  = FUIStyle::GetDefault();
    const FRectangle Bounds(IntVector2(0, 0), 200, 64);

    FHistogram::FDesc Desc;
    Desc.Capacity        = Capacity;
    Desc.PreferredHeight = Bounds.Height;
    Desc.MinValue        = 0.0f;
    Desc.MaxValue        = 0.0f;
    Desc.bAutoScale      = true;

    TSharedPtr<FHistogram> Histogram = FHistogram::Create(Desc);
    LayoutElement(Histogram, Bounds);

    TEST_SECTION("A new strip holds nothing and reads back nothing");
    TEST_EXPECT_EQ(Histogram->GetNumSamples(), 0);
    TEST_EXPECT_EQ(Histogram->GetLatest(), 0.0f);
    TEST_EXPECT_EQ(Histogram->GetAverage(), 0.0f);
    TEST_EXPECT_EQ(Histogram->GetMaximum(), 0.0f);
    TEST_EXPECT_EQ(Histogram->GetHoveredSample(), FHistogram::InvalidSampleIndex);

    TEST_SECTION("Samples are appended one after another until the buffer is full");
    for (int32 Index = 0; Index < Capacity; ++Index)
    {
        Histogram->AddSample(static_cast<float>(Index + 1));
    }

    TEST_EXPECT_EQ(Histogram->GetNumSamples(), Capacity);
    TEST_EXPECT_EQ(Histogram->GetSample(0), 1.0f);
    TEST_EXPECT_EQ(Histogram->GetLatest(), static_cast<float>(Capacity));

    TEST_SECTION("Past the capacity the oldest is dropped rather than the count growing");
    Histogram->AddSample(static_cast<float>(Capacity + 1));

    TEST_EXPECT_EQ(Histogram->GetNumSamples(), Capacity);
    TEST_EXPECT_EQ(Histogram->GetSample(0), 2.0f);
    TEST_EXPECT_EQ(Histogram->GetLatest(), 5.0f);

    TEST_SECTION("The mean and the largest are read off what is still held");
    TEST_EXPECT(Math::Abs(Histogram->GetAverage() - 3.5f) < 0.001f);
    TEST_EXPECT_EQ(Histogram->GetMaximum(), 5.0f);

    TEST_SECTION("An index outside what is held reads back zero");
    TEST_EXPECT_EQ(Histogram->GetSample(-1), 0.0f);
    TEST_EXPECT_EQ(Histogram->GetSample(Capacity), 0.0f);

    TEST_SECTION("Clearing drops every sample and leaves the strip reading back nothing");
    Histogram->Clear();

    TEST_EXPECT_EQ(Histogram->GetNumSamples(), 0);
    TEST_EXPECT_EQ(Histogram->GetSample(0), 0.0f);
    TEST_EXPECT_EQ(Histogram->GetLatest(), 0.0f);

    TEST_SECTION("With auto-scaling on the bars stand against the tallest sample held rather than against the range");
    Histogram->AddSample(5.0f);
    Histogram->AddSample(10.0f);

    FDrawCommandList AutoScaledCommands;
    DrawElement(Histogram, AutoScaledCommands);

    TArray<int32> AutoScaledBars;
    for (const FDrawCommand& Command : AutoScaledCommands.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Box && Command.Tint == Style.Colors.Accent)
        {
            AutoScaledBars.Add(Command.Bounds.Height);
        }
    }

    TEST_EXPECT_EQ(AutoScaledBars.Size(), 2);
    TEST_EXPECT_EQ(AutoScaledBars[1], Bounds.Height);
    TEST_EXPECT_EQ(AutoScaledBars[0], Bounds.Height / 2);

    TEST_SECTION("A draw puts the strip down first and then one bar for every sample held");
    Histogram->AddSample(7.5f);

    FDrawCommandList BarCommands;
    DrawElement(Histogram, BarCommands);

    TEST_EXPECT_EQ(Histogram->GetNumSamples(), 3);
    TEST_EXPECT_EQ(CountCommands(BarCommands, EDrawCommandType::Box), 1 + Histogram->GetNumSamples());

    TEST_SECTION("With auto-scaling off the same samples stand against the range that was set instead");
    Histogram->SetAutoScale(false);
    Histogram->SetRange(0.0f, 20.0f);

    FDrawCommandList RangedCommands;
    DrawElement(Histogram, RangedCommands);

    int32 TallestRangedBar = 0;
    for (const FDrawCommand& Command : RangedCommands.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Box && Command.Tint == Style.Colors.Accent)
        {
            TallestRangedBar = Math::Max(TallestRangedBar, Command.Bounds.Height);
        }
    }

    TEST_EXPECT_EQ(TallestRangedBar, Bounds.Height / 2);

    TEST_SECTION("A maximum set below the minimum is raised to it, so the range collapses and no bar has any height");
    Histogram->SetRange(20.0f, 10.0f);

    FDrawCommandList CollapsedCommands;
    DrawElement(Histogram, CollapsedCommands);
    TEST_EXPECT_EQ(CountCommands(CollapsedCommands, EDrawCommandType::Box), 1);

    TEST_SECTION("Moving over a bar names it, and leaving the strip puts that back");
    Histogram->OnMouseMove(MakeMoveEvent(IntVector2(75, 30)));
    TEST_EXPECT_EQ(Histogram->GetHoveredSample(), 1);

    Histogram->OnMouseMove(MakeMoveEvent(IntVector2(900, 30)));
    TEST_EXPECT_EQ(Histogram->GetHoveredSample(), FHistogram::InvalidSampleIndex);

    Histogram->OnMouseMove(MakeMoveEvent(IntVector2(75, 30)));
    Histogram->OnMouseLeft(MakeMoveEvent(IntVector2(75, 30)));
    TEST_EXPECT_EQ(Histogram->GetHoveredSample(), FHistogram::InvalidSampleIndex);

    TEST_END();
}
