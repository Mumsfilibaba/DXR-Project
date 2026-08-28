#include "InteractionTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Elements/InteractiveElement.h>
#include <Application/Input/Keys.h>

/** @brief A minimal interactive element that counts what the base class called it back for. */
class FCountingInteractiveElement final : public FInteractiveElement
{
public:
    FCountingInteractiveElement()
        : FInteractiveElement()
        , ClickCount(0)
        , StateChangeCount(0)
        , DragCount(0)
        , DesiredSize(100, 30)
    {
    }

    virtual IntVector2 ComputeDesiredSize() const override
    {
        return DesiredSize;
    }

    int32      ClickCount;
    int32      StateChangeCount;
    int32      DragCount;
    IntVector2 DesiredSize;

protected:
    virtual void OnClicked() override
    {
        ClickCount++;
    }

    virtual void OnInteractionStateChanged() override
    {
        StateChangeCount++;
    }

    virtual void OnDragged(const FCursorEvent& CursorEvent) override
    {
        UNREFERENCED_VARIABLE(CursorEvent);
        DragCount++;
    }
};

static TSharedPtr<FCountingInteractiveElement> CreateElement(const FRectangle& Bounds)
{
    TSharedPtr<FCountingInteractiveElement> Element = MakeSharedPtr<FCountingInteractiveElement>();
    Element->PrepareDesiredSize();
    Element->Tick(Bounds);
    return Element;
}

/** @brief A move, enter or leave, which carries a position but no button. */
static FCursorEvent MakeMoveEvent(EInputEventType Type, const IntVector2& ClientPosition)
{
    return FCursorEvent(Type, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

/** @brief A press or a release of one button at a position. */
static FCursorEvent MakeButtonEvent(EInputEventType Type, const IntVector2& ClientPosition, FKey Key)
{
    return FCursorEvent(Type, Key, ClientPosition, IntVector2(0, 0), FModifierKeyState(), Type == EInputEventType::MouseButtonDown);
}

bool InteractionState_Test()
{
    TEST_BEGIN();

    TSharedPtr<FCountingInteractiveElement> Element = CreateElement(FRectangle(IntVector2(10, 10), 100, 30));

    TEST_SECTION("A fresh element is enabled, unhovered and unpressed");
    TEST_EXPECT(Element->IsEnabled());
    TEST_EXPECT(!Element->IsHovered());
    TEST_EXPECT(!Element->IsPressed());
    TEST_EXPECT(Element->GetInteractionState() == EInteractionState::Normal);

    TEST_SECTION("Entering hovers it and leaving takes the hover away again");
    Element->OnMouseEntered(MakeMoveEvent(EInputEventType::MouseEntered, IntVector2(50, 20)));
    TEST_EXPECT(Element->IsHovered());
    TEST_EXPECT(Element->GetInteractionState() == EInteractionState::Hovered);

    Element->OnMouseLeft(MakeMoveEvent(EInputEventType::MouseLeft, IntVector2(500, 500)));
    TEST_EXPECT(!Element->IsHovered());
    TEST_EXPECT(Element->GetInteractionState() == EInteractionState::Normal);

    TEST_SECTION("Pressed outranks hovered, since that is what the eye reads first");
    Element->OnMouseEntered(MakeMoveEvent(EInputEventType::MouseEntered, IntVector2(50, 20)));
    Element->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(50, 20), Keys::MouseButtonLeft));
    TEST_EXPECT(Element->IsPressed());
    TEST_EXPECT(Element->GetInteractionState() == EInteractionState::Pressed);

    TEST_SECTION("Disabled outranks everything, and a disabled element forgets it was pressed");
    Element->SetEnabled(false);
    TEST_EXPECT(!Element->IsEnabled());
    TEST_EXPECT(!Element->IsPressed());
    TEST_EXPECT(!Element->IsHovered());
    TEST_EXPECT(Element->GetInteractionState() == EInteractionState::Disabled);

    TEST_SECTION("A disabled element ignores input rather than queueing it up");
    const int32 ClicksBefore = Element->ClickCount;
    Element->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(50, 20), Keys::MouseButtonLeft));
    Element->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(50, 20), Keys::MouseButtonLeft));
    TEST_EXPECT_EQ(Element->ClickCount, ClicksBefore);

    TEST_SECTION("Enabling it again leaves it unhovered until the cursor says otherwise");
    Element->SetEnabled(true);
    TEST_EXPECT(Element->GetInteractionState() == EInteractionState::Normal);

    TEST_SECTION("The state change hook fires on every transition and not on a repeat");
    const int32 ChangesBefore = Element->StateChangeCount;
    Element->SetEnabled(true);
    TEST_EXPECT_EQ(Element->StateChangeCount, ChangesBefore);

    Element->OnMouseEntered(MakeMoveEvent(EInputEventType::MouseEntered, IntVector2(50, 20)));
    Element->OnMouseEntered(MakeMoveEvent(EInputEventType::MouseEntered, IntVector2(51, 20)));
    TEST_EXPECT_EQ(Element->StateChangeCount, ChangesBefore + 1);

    TEST_END();
}

bool InteractionClick_Test()
{
    TEST_BEGIN();

    const FRectangle Bounds(IntVector2(10, 10), 100, 30);

    TEST_SECTION("A press and a release both inside count as one click");
    TSharedPtr<FCountingInteractiveElement> Element = CreateElement(Bounds);

    Element->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(50, 20), Keys::MouseButtonLeft));
    Element->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(60, 25), Keys::MouseButtonLeft));

    TEST_EXPECT_EQ(Element->ClickCount, 1);
    TEST_EXPECT(!Element->IsPressed());

    TEST_SECTION("Dragging off the element before releasing cancels the click");
    TSharedPtr<FCountingInteractiveElement> Dragged = CreateElement(Bounds);

    Dragged->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(50, 20), Keys::MouseButtonLeft));
    Dragged->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(500, 500), Keys::MouseButtonLeft));

    TEST_EXPECT_EQ(Dragged->ClickCount, 0);
    TEST_EXPECT(!Dragged->IsPressed());

    TEST_SECTION("A release with no press before it does nothing");
    TSharedPtr<FCountingInteractiveElement> Stray = CreateElement(Bounds);

    const FEventResponse StrayResponse = Stray->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(50, 20), Keys::MouseButtonLeft));
    TEST_EXPECT(!StrayResponse.IsEventHandled());
    TEST_EXPECT_EQ(Stray->ClickCount, 0);

    TEST_SECTION("The right button is left to whatever wants to open a context menu with it");
    TSharedPtr<FCountingInteractiveElement> RightClicked = CreateElement(Bounds);

    const FEventResponse RightResponse = RightClicked->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(50, 20), Keys::MouseButtonRight));
    TEST_EXPECT(!RightResponse.IsEventHandled());
    TEST_EXPECT(!RightClicked->IsPressed());

    TEST_SECTION("A move while held is reported as a drag, and one while idle is not");
    TSharedPtr<FCountingInteractiveElement> Scrubbed = CreateElement(Bounds);

    Scrubbed->OnMouseMove(MakeMoveEvent(EInputEventType::MouseMoved, IntVector2(50, 20)));
    TEST_EXPECT_EQ(Scrubbed->DragCount, 0);

    Scrubbed->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(50, 20), Keys::MouseButtonLeft));
    Scrubbed->OnMouseMove(MakeMoveEvent(EInputEventType::MouseMoved, IntVector2(70, 20)));
    Scrubbed->OnMouseMove(MakeMoveEvent(EInputEventType::MouseMoved, IntVector2(400, 20)));
    TEST_EXPECT_EQ(Scrubbed->DragCount, 2);

    TEST_SECTION("Hover follows the cursor during a drag, since the leave event is held back while held");
    TEST_EXPECT(!Scrubbed->IsHovered());
    TEST_EXPECT(Scrubbed->IsPressed());

    TEST_END();
}

bool InteractionCapture_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Harness;

    TSharedPtr<FWindow>                     Window  = Harness.CreateWindow(IntVector2(400, 300));
    TSharedPtr<FCountingInteractiveElement> Element = MakeSharedPtr<FCountingInteractiveElement>();

    Window->SetContent(Element);
    FApplication::LayoutWindow(Window);

    TEST_SECTION("Hovering runs through the real enter path rather than being set by hand");
    Harness.MoveCursor(IntVector2(50, 20));
    TEST_EXPECT(Element->IsHovered());

    TEST_SECTION("A press takes capture, so the application routes to the element wherever the cursor goes");
    Harness.PressMouseButton(Window);
    TEST_EXPECT(Element->IsPressed());
    TEST_EXPECT(Harness.GetApplication().HasMouseCapture());

    TEST_SECTION("A release outside the element still reaches it, and is not counted as a click");
    Harness.MoveCursor(IntVector2(5000, 5000));
    Harness.ReleaseMouseButton();

    TEST_EXPECT(!Element->IsPressed());
    TEST_EXPECT_EQ(Element->ClickCount, 0);

    TEST_SECTION("Capture is handed back once the press ends");
    TEST_EXPECT(!Harness.GetApplication().HasMouseCapture());

    TEST_SECTION("A press and release in place clicks");
    Harness.MoveCursor(IntVector2(50, 20));
    Harness.PressMouseButton(Window);
    Harness.ReleaseMouseButton();

    TEST_EXPECT_EQ(Element->ClickCount, 1);
    TEST_EXPECT(Element->IsHovered());

    TEST_SECTION("Losing focus mid-press abandons it rather than leaving the button stuck down");
    Harness.PressMouseButton(Window);
    TEST_EXPECT(Element->IsPressed());

    Element->OnFocusLost();
    TEST_EXPECT(!Element->IsPressed());
    TEST_EXPECT(!Harness.GetApplication().HasMouseCapture());

    Harness.ReleaseMouseButton();

    TEST_END();
}

bool InteractionKeyboard_Test()
{
    TEST_BEGIN();

    TSharedPtr<FCountingInteractiveElement> Element = CreateElement(FRectangle(IntVector2(0, 0), 100, 30));

    TEST_SECTION("An enabled element takes the keyboard and a disabled one does not");
    TEST_EXPECT(Element->SupportsKeyboardFocus());

    Element->SetEnabled(false);
    TEST_EXPECT(!Element->SupportsKeyboardFocus());
    Element->SetEnabled(true);

    TEST_SECTION("Space and Enter activate the element the way a click does");
    Element->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Space, FModifierKeyState(), false, true));
    TEST_EXPECT_EQ(Element->ClickCount, 1);

    Element->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Enter, FModifierKeyState(), false, true));
    TEST_EXPECT_EQ(Element->ClickCount, 2);

    TEST_SECTION("Anything else is left to whatever is listening above");
    const FEventResponse Response = Element->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::A, FModifierKeyState(), false, true));
    TEST_EXPECT(!Response.IsEventHandled());
    TEST_EXPECT_EQ(Element->ClickCount, 2);

    TEST_SECTION("A disabled element ignores the keyboard too");
    Element->SetEnabled(false);
    Element->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::Space, FModifierKeyState(), false, true));
    TEST_EXPECT_EQ(Element->ClickCount, 2);

    TEST_END();
}
