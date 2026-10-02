#include "StubPlatformApplication.h"
#include "WindowTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Button.h>
#include <Application/Elements/FloatingWindow.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Elements/TitleBar.h>
#include <Application/Input/Keys.h>
#include <Application/Text/FixedWidthFontFace.h>

static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

static FStubPlatformWindow& GetStubWindow(const TSharedPtr<FWindow>& Window)
{
    return static_cast<FStubPlatformWindow&>(*Window->GetPlatformWindow());
}

static FWindowTitleBarMetrics MakeLeadingInsetMetrics()
{
    FWindowTitleBarMetrics Metrics;
    Metrics.Height       = 28.0f;
    Metrics.LeadingInset = 78.0f;
    return Metrics;
}

static FWindowTitleBarMetrics MakeTrailingInsetMetrics()
{
    FWindowTitleBarMetrics Metrics;
    Metrics.Height             = 32.0f;
    Metrics.TrailingInset      = 138.0f;
    Metrics.CaptionButtonWidth = 46.0f;
    return Metrics;
}

static FTextBlock::FDesc MakeTextDesc(const String& Text, const TSharedPtr<IFontFace>& Font)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = Font;
    return Desc;
}

static void ClickElement(const TSharedPtr<FVisualElement>& Element)
{
    const IntVector2 Center = Element->GetContentRectangle().GetCenter();

    const FCursorEvent DownEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, Center, IntVector2(0, 0), FModifierKeyState(), true);
    const FCursorEvent UpEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, Center, IntVector2(0, 0), FModifierKeyState(), false);

    Element->OnMouseButtonDown(DownEvent);
    Element->OnMouseButtonUp(UpEvent);
}

static bool IsPointInteractive(const FWindowTitleBarRegions& Regions, const IntVector2& Point)
{
    for (const FWindowRect& Rect : Regions.InteractiveRects)
    {
        if (Rect.Contains(Point))
        {
            return true;
        }
    }

    return false;
}

bool TitleBarMetrics_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0), EWindowStyleFlags::Default | EWindowStyleFlags::CustomTitleBar);

    GetStubWindow(Window).SetTitleBarMetrics(MakeLeadingInsetMetrics());

    TSharedPtr<FButton> CaptionButton = FButton::Create(FButton::FDesc().SetText("File").SetFont(Font));

    FTitleBar::FDesc Desc;
    Desc.SetTitle("Sandbox").SetFont(Font).SetContent(CaptionButton);
    Desc.bShowCaptionButtons = true;

    TSharedPtr<FTitleBar> TitleBar = FTitleBar::Create(Desc);
    Window->SetContent(TitleBar);
    FApplication::LayoutWindow(Window);

    TEST_SECTION("The bar reads the platform metrics rather than assuming a height");
    TEST_EXPECT_EQ(TitleBar->GetMetrics().Height, 28.0f);
    TEST_EXPECT_EQ(TitleBar->GetMetrics().LeadingInset, 78.0f);

    TEST_SECTION("Content starts past the leading inset, so it clears the buttons the OS drew");
    TEST_EXPECT(CaptionButton->GetContentRectangle().Position.X >= 78);

    TEST_SECTION("A platform drawing its own buttons collapses ours rather than stacking them on top");
    for (const TSharedPtr<FCaptionButton>& Button : TitleBar->GetCaptionButtons())
    {
        TEST_EXPECT(Button->GetContentRectangle().IsEmpty());
    }

    TEST_SECTION("The bar is at least as tall as the chrome it is drawn over");
    TEST_EXPECT(TitleBar->GetContentRectangle().Height >= 28);

    TEST_SECTION("On a platform that hands its buttons over, ours take the reported width");
    GetStubWindow(Window).SetTitleBarMetrics(MakeTrailingInsetMetrics());
    FApplication::LayoutWindow(Window);

    TEST_EXPECT_EQ(TitleBar->GetMetrics().CaptionButtonWidth, 46.0f);
    TEST_EXPECT_EQ(TitleBar->GetCaptionButtons().Size(), 3);

    for (const TSharedPtr<FCaptionButton>& Button : TitleBar->GetCaptionButtons())
    {
        TEST_EXPECT_EQ(Button->GetContentRectangle().Width, 46);
        TEST_EXPECT_EQ(Button->GetContentRectangle().Height, 32);
    }

    TEST_SECTION("Our buttons fill the trailing inset, so the last of them reaches the window edge");
    const FRectangle CloseBounds = TitleBar->GetCaptionButtons().Last()->GetContentRectangle();
    TEST_EXPECT_EQ(CloseBounds.GetRight(), 800);

    const FRectangle MinimizeBounds = TitleBar->GetCaptionButtons().First()->GetContentRectangle();
    TEST_EXPECT_EQ(MinimizeBounds.Position.X, 800 - 138);

    TEST_SECTION("Nothing is inset on a window without a custom title bar");
    TSharedPtr<FWindow>   PlainWindow   = Application.CreateWindow(IntVector2(400, 300));
    TSharedPtr<FTitleBar> PlainTitleBar = FTitleBar::Create(FTitleBar::FDesc().SetTitle("Plain").SetFont(Font));

    PlainWindow->SetContent(PlainTitleBar);
    FApplication::LayoutWindow(PlainWindow);

    TEST_EXPECT_EQ(PlainTitleBar->GetMetrics().Height, 0.0f);
    TEST_EXPECT_EQ(PlainTitleBar->GetMetrics().LeadingInset, 0.0f);

    TEST_END();
}

bool TitleBarRegions_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0), EWindowStyleFlags::Default | EWindowStyleFlags::CustomTitleBar);

    GetStubWindow(Window).SetTitleBarMetrics(MakeTrailingInsetMetrics());

    TSharedPtr<FHorizontalBox> MenuRow    = MakeSharedPtr<FHorizontalBox>();
    TSharedPtr<FButton>        FileButton = FButton::Create(FButton::FDesc().SetText("File").SetFont(Font));
    TSharedPtr<FButton>        EditButton = FButton::Create(FButton::FDesc().SetText("Edit").SetFont(Font));

    MenuRow->AddSlot(FileButton);
    MenuRow->AddSlot(EditButton);

    FTitleBar::FDesc Desc;
    Desc.SetTitle("Sandbox").SetFont(Font).SetContent(MenuRow);
    Desc.bShowCaptionButtons = true;

    TSharedPtr<FTitleBar> TitleBar = FTitleBar::Create(Desc);
    Window->SetContent(TitleBar);
    FApplication::LayoutWindow(Window);

    TEST_SECTION("The caption rectangle is published to the platform, not just kept");
    const FWindowTitleBarRegions& Published = GetStubWindow(Window).GetTitleBarRegions();
    TEST_EXPECT_EQ(Published.CaptionRect.Left, TitleBar->GetRegions().CaptionRect.Left);
    TEST_EXPECT_EQ(Published.CaptionRect.Right, TitleBar->GetRegions().CaptionRect.Right);

    TEST_SECTION("The caption spans the width of the bar");
    TEST_EXPECT_EQ(Published.CaptionRect.Left, 0);
    TEST_EXPECT_EQ(Published.CaptionRect.Right, 800);
    TEST_EXPECT_EQ(Published.CaptionRect.Top, 0);

    TEST_SECTION("Every button in the caption is collected, and nothing else is");
    TEST_EXPECT_EQ(Published.InteractiveRects.Size(), 5);

    TEST_EXPECT(IsPointInteractive(Published, FileButton->GetContentRectangle().GetCenter()));
    TEST_EXPECT(IsPointInteractive(Published, EditButton->GetContentRectangle().GetCenter()));

    for (const TSharedPtr<FCaptionButton>& Button : TitleBar->GetCaptionButtons())
    {
        TEST_EXPECT(IsPointInteractive(Published, Button->GetContentRectangle().GetCenter()));
    }

    TEST_SECTION("The gap between the menu row and the buttons drags the window");
    const int32 GapX = (EditButton->GetContentRectangle().GetRight() + TitleBar->GetCaptionButtons()[0]->GetContentRectangle().Position.X) / 2;
    TEST_EXPECT(Published.CaptionRect.Contains(IntVector2(GapX, 8)));
    TEST_EXPECT(!IsPointInteractive(Published, IntVector2(GapX, 8)));

    TEST_SECTION("Windows is told where the maximize button is, so it can offer Snap Layouts");
    const FRectangle MaximizeBounds = TitleBar->GetCaptionButtons()[1]->GetContentRectangle();
    TEST_EXPECT_EQ(Published.MaximizeButtonRect.Left, MaximizeBounds.Position.X);
    TEST_EXPECT_EQ(Published.MaximizeButtonRect.Right, MaximizeBounds.GetRight());

    TEST_SECTION("The regions follow the window as it is resized");
    Window->Resize(IntVector2(1200, 600));
    FApplication::LayoutWindow(Window);

    TEST_EXPECT_EQ(GetStubWindow(Window).GetTitleBarRegions().CaptionRect.Right, 1200);
    TEST_EXPECT_EQ(GetStubWindow(Window).GetTitleBarRegions().MaximizeButtonRect.Right, MaximizeBounds.GetRight() + 400);

    TEST_END();
}

bool CaptionButtons_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0), EWindowStyleFlags::Default | EWindowStyleFlags::CustomTitleBar);

    GetStubWindow(Window).SetTitleBarMetrics(MakeTrailingInsetMetrics());

    FTitleBar::FDesc Desc;
    Desc.SetTitle("Sandbox").SetFont(Font);
    Desc.bShowCaptionButtons = true;

    TSharedPtr<FTitleBar> TitleBar = FTitleBar::Create(Desc);
    Window->SetContent(TitleBar);
    FApplication::LayoutWindow(Window);

    const TArray<TSharedPtr<FCaptionButton>>& Buttons = TitleBar->GetCaptionButtons();
    TEST_EXPECT_EQ(Buttons.Size(), 3);

    TEST_SECTION("They are laid out in the order the desktop draws them");
    TEST_EXPECT_EQ(Buttons[0]->GetKind(), ECaptionButtonKind::Minimize);
    TEST_EXPECT_EQ(Buttons[1]->GetKind(), ECaptionButtonKind::Maximize);
    TEST_EXPECT_EQ(Buttons[2]->GetKind(), ECaptionButtonKind::Close);
    TEST_EXPECT(Buttons[0]->GetContentRectangle().Position.X < Buttons[2]->GetContentRectangle().Position.X);

    TEST_SECTION("Minimize takes the window down");
    ClickElement(Buttons[0]);
    TEST_EXPECT(Window->IsMinimized());

    TEST_SECTION("Maximize is a toggle, so the same button restores");
    ClickElement(Buttons[1]);
    TEST_EXPECT(Window->IsMaximized());
    TEST_EXPECT(!Window->IsMinimized());

    ClickElement(Buttons[1]);
    TEST_EXPECT(!Window->IsMaximized());

    TEST_SECTION("A button only draws a fill once the cursor is on it");
    FDrawCommandList NormalCommands;
    Buttons[2]->OnDraw(FDrawGeometry(Buttons[2]->GetContentRectangle(), 1.0f), NormalCommands, 0);
    const int32 NormalCount = NormalCommands.GetCommands().Size();

    const FCursorEvent EnterEvent(EInputEventType::MouseMoved, Buttons[2]->GetContentRectangle().GetCenter(), IntVector2(0, 0), FModifierKeyState());
    Buttons[2]->OnMouseEntered(EnterEvent);

    FDrawCommandList HoveredCommands;
    Buttons[2]->OnDraw(FDrawGeometry(Buttons[2]->GetContentRectangle(), 1.0f), HoveredCommands, 0);
    TEST_EXPECT(HoveredCommands.GetCommands().Size() > NormalCount);

    TEST_SECTION("Close destroys the window it is in");
    TEST_EXPECT_EQ(FApplication::Get().GetWindows().Size(), 1);
    ClickElement(Buttons[2]);
    TEST_EXPECT_EQ(FApplication::Get().GetWindows().Size(), 0);

    TEST_END();
}

bool FloatingWindow_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    const TSharedPtr<IFontFace> Font       = CreateFont();
    TSharedPtr<FWindow>         MainWindow = Application.CreateWindow(IntVector2(1280, 720));
    TSharedPtr<FTextBlock>      Body       = FTextBlock::Create(MakeTextDesc("Details", Font));

    FFloatingWindow::FDesc Desc;
    Desc.SetTitle("Inspector").SetBounds(IntVector2(200, 150), IntVector2(320, 240)).SetContent(Body);
    Desc.ParentWindow = MainWindow;
    Desc.Font         = Font;

    TSharedPtr<FFloatingWindow> Floating = FFloatingWindow::Create(Desc);
    TEST_EXPECT(Floating != nullptr);
    TEST_EXPECT(Floating->IsOpen());

    TEST_SECTION("The window is created where it was asked for, over its parent");
    TSharedPtr<FWindow> Window = Floating->GetWindow();
    TEST_EXPECT_EQ(Window->GetPosition(), IntVector2(200, 150));
    TEST_EXPECT_EQ(Window->GetSize(), IntVector2(320, 240));
    TEST_EXPECT(Window->GetParentWindow() == MainWindow);

    TEST_SECTION("It asks the platform for a caption of its own to draw");
    const EWindowStyleFlags StyleFlags = Window->GetPlatformWindow()->GetStyle();
    TEST_EXPECT((StyleFlags & EWindowStyleFlags::CustomTitleBar) == EWindowStyleFlags::CustomTitleBar);

    TEST_SECTION("The caption sits over the body rather than beside it");
    GetStubWindow(Window).SetTitleBarMetrics(MakeTrailingInsetMetrics());
    FApplication::LayoutWindow(Window);

    TSharedPtr<FTitleBar> TitleBar = Floating->GetTitleBar();
    TEST_EXPECT(TitleBar != nullptr);
    TEST_EXPECT_EQ(TitleBar->GetContentRectangle().Position.Y, 0);
    TEST_EXPECT_EQ(TitleBar->GetContentRectangle().Width, 320);
    TEST_EXPECT(Body->GetContentRectangle().Position.Y >= TitleBar->GetContentRectangle().GetBottom());

    TEST_SECTION("The caption carries the title it was created with");
    TEST_EXPECT_EQ(TitleBar->GetTitle(), String("Inspector"));

    TitleBar->SetTitle("Inspector - Sponza");
    TEST_EXPECT_EQ(TitleBar->GetTitle(), String("Inspector - Sponza"));

    TEST_SECTION("Replacing the body keeps the caption on top of it");
    TSharedPtr<FTextBlock> NewBody = FTextBlock::Create(MakeTextDesc("Materials", Font));
    Floating->SetContent(NewBody);
    FApplication::LayoutWindow(Window);

    TEST_EXPECT(NewBody->GetContentRectangle().Position.Y >= TitleBar->GetContentRectangle().GetBottom());

    TEST_SECTION("Closing takes the window with it and leaves the helper holding nothing");
    const int32 NumWindowsBefore = FApplication::Get().GetWindows().Size();
    Floating->Close();

    TEST_EXPECT(!Floating->IsOpen());
    TEST_EXPECT(Floating->GetWindow() == nullptr);
    TEST_EXPECT_EQ(FApplication::Get().GetWindows().Size(), NumWindowsBefore - 1);

    TEST_END();
}
