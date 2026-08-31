#include "MenuTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Spacer.h>
#include <Application/Input/Keys.h>
#include <Application/Menus/ComboBox.h>
#include <Application/Menus/DragDropService.h>
#include <Application/Menus/Menu.h>
#include <Application/Menus/MenuAnchor.h>
#include <Application/Menus/MenuBar.h>
#include <Application/Menus/MenuItem.h>
#include <Application/Menus/MenuStack.h>
#include <Application/Menus/ToolTipService.h>
#include <Application/Style/UIStyle.h>
#include <Application/Text/FixedWidthFontFace.h>

static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

class FScopedMenuTestServices
{
public:
    FScopedMenuTestServices() = default;

    ~FScopedMenuTestServices()
    {
        FMenuStack::Shutdown();
        FToolTipService::Shutdown();
    }

    FScopedMenuTestServices(const FScopedMenuTestServices&) = delete;
    FScopedMenuTestServices& operator=(const FScopedMenuTestServices&) = delete;
};

static TSharedPtr<FMenu> CreateMenu(const TSharedPtr<IFontFace>& Font, const TArray<String>& Labels)
{
    TSharedPtr<FMenu> Menu = FMenu::Create();
    for (const String& Label : Labels)
    {
        FMenuItem::FDesc Desc;
        Desc.SetLabel(Label).SetFont(Font);

        Menu->AddItem(FMenuItem::Create(Desc));
    }

    return Menu;
}

static TSharedPtr<FVisualElement> CreateDropTarget(const FRectangle& ScreenBounds)
{
    TSharedPtr<FSpacer> Target = FSpacer::Create(ScreenBounds.GetSize());
    Target->PrepareDesiredSize();
    Target->Tick(ScreenBounds);

    return Target;
}

static FCursorEvent MakeMoveEvent(const IntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, const IntVector2& ClientPosition)
{
    return FCursorEvent(Type, Keys::MouseButtonLeft, ClientPosition, IntVector2(0, 0), FModifierKeyState(), Type == EInputEventType::MouseButtonDown);
}

static FKeyEvent MakeKeyEvent(FKey Key)
{
    return FKeyEvent(EInputEventType::KeyDown, Key, FModifierKeyState(), false, true);
}

static void ClickElement(const TSharedPtr<FVisualElement>& Element)
{
    const IntVector2 Center = Element->GetContentRectangle().GetCenter();

    Element->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Center));
    Element->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Center));
}

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

static void DrawElement(const TSharedPtr<FVisualElement>& Element, FDrawCommandList& OutCommandList)
{
    Element->OnDraw(FDrawGeometry(Element->GetContentRectangle(), 1.0f), OutCommandList, 0);
}

static TSharedPtr<FMenu> GetTopMenu()
{
    const TArray<TSharedPtr<FWindow>>& OpenMenus = FMenuStack::Get().GetOpenMenus();
    if (OpenMenus.IsEmpty())
    {
        return nullptr;
    }

    return StaticCastSharedPtr<FMenu>(OpenMenus.Last()->GetContent());
}

bool MenuStackPlacement_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(100, 100));

    FMenuStack& Stack = FMenuStack::Get();

    TEST_SECTION("A drop-down sits under the anchor with their left edges flush");
    TSharedPtr<FMenu>   Menu       = CreateMenu(Font, { "New", "Open", "Save" });
    TSharedPtr<FWindow> MenuWindow = Stack.PushMenu(Window, FRectangle(IntVector2(140, 130), 60, 24), EMenuPlacement::BelowLeftAligned, Menu);

    TEST_EXPECT(MenuWindow != nullptr);
    const IntVector2 MenuSize = Menu->GetCachedDesiredSize();
    TEST_EXPECT_EQ(MenuWindow->GetPosition(), IntVector2(140, 154));
    TEST_EXPECT_EQ(MenuWindow->GetSize(), MenuSize);

    TEST_SECTION("The popup is borderless, on top, out of the task bar and does not take activation");
    const EWindowStyleFlags StyleFlags = MenuWindow->GetPlatformWindow()->GetStyle();
    TEST_EXPECT((StyleFlags & EWindowStyleFlags::TopMost) == EWindowStyleFlags::TopMost);
    TEST_EXPECT((StyleFlags & EWindowStyleFlags::NoTaskBarIcon) == EWindowStyleFlags::NoTaskBarIcon);
    TEST_EXPECT((StyleFlags & EWindowStyleFlags::Titled) == EWindowStyleFlags::None);
    Stack.DismissAll();

    TEST_SECTION("With no room below, the drop-down goes above the anchor instead");
    MenuWindow = Stack.PushMenu(Window, FRectangle(IntVector2(140, 1050), 60, 24), EMenuPlacement::BelowLeftAligned, Menu);
    TEST_EXPECT_EQ(MenuWindow->GetPosition(), IntVector2(140, 1050 - MenuSize.Y));
    Stack.DismissAll();

    TEST_SECTION("A submenu opens beside its anchor with their top edges flush");
    MenuWindow = Stack.PushMenu(Window, FRectangle(IntVector2(140, 130), 60, 24), EMenuPlacement::RightOfTopAligned, Menu);
    TEST_EXPECT_EQ(MenuWindow->GetPosition(), IntVector2(200, 130));
    Stack.DismissAll();

    TEST_SECTION("With no room to the right, the submenu goes to the left of its anchor");
    MenuWindow = Stack.PushMenu(Window, FRectangle(IntVector2(1890, 130), 20, 24), EMenuPlacement::RightOfTopAligned, Menu);
    TEST_EXPECT_EQ(MenuWindow->GetPosition(), IntVector2(1890 - MenuSize.X, 130));
    Stack.DismissAll();

    TEST_SECTION("A context menu opens at the cursor, and in the corner it opens back towards it");
    MenuWindow = Stack.PushMenu(Window, FRectangle(IntVector2(400, 300), 0, 0), EMenuPlacement::AtCursor, Menu);
    TEST_EXPECT_EQ(MenuWindow->GetPosition(), IntVector2(400, 300));
    Stack.DismissAll();

    MenuWindow = Stack.PushMenu(Window, FRectangle(IntVector2(1915, 1075), 0, 0), EMenuPlacement::AtCursor, Menu);
    TEST_EXPECT_EQ(MenuWindow->GetPosition(), IntVector2(1915 - MenuSize.X, 1075 - MenuSize.Y));
    Stack.DismissAll();

    TEST_SECTION("A menu with nowhere left to flip to is pulled onto the monitor rather than off it");
    TArray<String> ManyLabels;
    for (int32 Index = 0; Index < 80; ++Index)
    {
        ManyLabels.Add("Row");
    }

    TSharedPtr<FMenu> LongMenu = CreateMenu(Font, ManyLabels);
    MenuWindow                 = Stack.PushMenu(Window, FRectangle(IntVector2(140, 900), 60, 24), EMenuPlacement::BelowLeftAligned, LongMenu);

    TEST_EXPECT(LongMenu->GetCachedDesiredSize().Y > 1080);
    TEST_EXPECT_EQ(MenuWindow->GetPosition().Y, 0);
    Stack.DismissAll();

    TEST_END();
}

bool MenuStackDepth_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    FMenuStack& Stack = FMenuStack::Get();

    TSharedPtr<FMenu> FileMenu = CreateMenu(Font, { "New", "Open" });
    TSharedPtr<FMenu> Recent   = CreateMenu(Font, { "One", "Two" });
    TSharedPtr<FMenu> Deeper   = CreateMenu(Font, { "Left", "Right" });

    TEST_SECTION("Opening from a window is the outermost level");
    TSharedPtr<FWindow> FileWindow = Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, FileMenu);
    TEST_EXPECT(Stack.IsOpen());
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);
    TEST_EXPECT_EQ(Stack.GetMenuDepth(FileWindow), 1);
    TEST_EXPECT_EQ(Stack.GetMenuDepth(Window), 0);

    TEST_SECTION("Opening from a menu makes the new one its child");
    TSharedPtr<FWindow> RecentWindow = Stack.PushMenu(FileWindow, FRectangle(IntVector2(60, 40), 40, 24), EMenuPlacement::RightOfTopAligned, Recent);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);
    TEST_EXPECT_EQ(Stack.GetMenuDepth(RecentWindow), 2);

    TEST_SECTION("Opening from the same parent again replaces the sibling rather than stacking on it");
    TSharedPtr<FWindow> DeeperWindow = Stack.PushMenu(FileWindow, FRectangle(IntVector2(60, 70), 40, 24), EMenuPlacement::RightOfTopAligned, Deeper);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);
    TEST_EXPECT(!Stack.IsMenuOpen(RecentWindow));
    TEST_EXPECT(Stack.IsMenuOpen(DeeperWindow));

    TEST_SECTION("A click inside an open menu leaves the stack alone");
    TEST_EXPECT(!Stack.DismissOnClickOutside(DeeperWindow->GetPosition() + IntVector2(2, 2)));
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);

    TEST_SECTION("A click outside every menu closes the lot");
    TEST_EXPECT(Stack.DismissOnClickOutside(IntVector2(700, 500)));
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);
    TEST_EXPECT(!Stack.IsOpen());

    TEST_SECTION("Opening from a window again starts over rather than nesting");
    FileWindow   = Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, FileMenu);
    RecentWindow = Stack.PushMenu(FileWindow, FRectangle(IntVector2(60, 40), 40, 24), EMenuPlacement::RightOfTopAligned, Recent);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);

    Stack.PushMenu(Window, FRectangle(IntVector2(60, 10), 40, 24), EMenuPlacement::BelowLeftAligned, Deeper);
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    TEST_SECTION("Dismissing the top leaves its parent open");
    Stack.PushMenu(Stack.GetOpenMenus().Last(), FRectangle(IntVector2(60, 40), 40, 24), EMenuPlacement::RightOfTopAligned, Recent);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);

    Stack.DismissTop();
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    Stack.DismissAll();
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);

    TEST_END();
}

bool MenuItemLayout_Test()
{
    TEST_BEGIN();

    const FUIStyle&             Style = FUIStyle::GetDefault();
    const TSharedPtr<IFontFace> Font  = CreateFont();

    TEST_SECTION("A plain row reserves the check gutter beside its label");
    FMenuItem::FDesc PlainDesc;
    PlainDesc.SetLabel("Open").SetFont(Font);

    TSharedPtr<FMenuItem> Plain = FMenuItem::Create(PlainDesc);
    Plain->PrepareDesiredSize();

    const int32 LabelWidth = 4 * 8;
    TEST_EXPECT_EQ(Plain->GetCachedDesiredSize().X, 12 + FMenuItem::GutterWidth + LabelWidth);
    TEST_EXPECT_EQ(Plain->GetCachedDesiredSize().Y, Style.Metrics.RowHeight);

    TEST_SECTION("A shortcut hint widens the row by the gap and its own width");
    FMenuItem::FDesc ShortcutDesc;
    ShortcutDesc.SetLabel("Open").SetFont(Font);
    ShortcutDesc.ShortcutText = "Ctrl+O";

    TSharedPtr<FMenuItem> WithShortcut = FMenuItem::Create(ShortcutDesc);
    WithShortcut->PrepareDesiredSize();
    TEST_EXPECT_EQ(WithShortcut->GetCachedDesiredSize().X, Plain->GetCachedDesiredSize().X + FMenuItem::ShortcutGap + (6 * 8));

    TEST_SECTION("A submenu widens the row by the arrow it has to draw");
    FMenuItem::FDesc SubMenuDesc;
    SubMenuDesc.SetLabel("Open").SetFont(Font);
    SubMenuDesc.SubMenu = CreateMenu(Font, { "One" });

    TSharedPtr<FMenuItem> WithSubMenu = FMenuItem::Create(SubMenuDesc);
    WithSubMenu->PrepareDesiredSize();
    TEST_EXPECT(WithSubMenu->HasSubMenu());
    TEST_EXPECT_EQ(WithSubMenu->GetCachedDesiredSize().X, Plain->GetCachedDesiredSize().X + FMenuItem::ShortcutGap + FMenuItem::ArrowWidth);

    TEST_SECTION("A row draws its label and nothing behind it until it is highlighted");
    Plain->Tick(FRectangle(IntVector2(0, 0), 160, 24));

    FDrawCommandList Commands;
    DrawElement(Plain, Commands);
    TEST_EXPECT_EQ(CountCommands(Commands, EDrawCommandType::Box), 0);
    TEST_EXPECT_EQ(CountCommands(Commands, EDrawCommandType::Text), 1);

    TEST_SECTION("Keyboard navigation lights a row the same way hovering does");
    Plain->SetHighlighted(true);

    FDrawCommandList HighlightedCommands;
    DrawElement(Plain, HighlightedCommands);
    TEST_EXPECT_EQ(CountCommands(HighlightedCommands, EDrawCommandType::Box), 1);
    Plain->SetHighlighted(false);

    TEST_SECTION("A checked row draws a tick, and an undetermined one a dash");
    FMenuItem::FDesc CheckDesc;
    CheckDesc.SetLabel("Grid").SetFont(Font);
    CheckDesc.bIsCheckable = true;
    CheckDesc.CheckState   = ECheckBoxState::Checked;

    TSharedPtr<FMenuItem> Checkable = FMenuItem::Create(CheckDesc);
    Checkable->PrepareDesiredSize();
    Checkable->Tick(FRectangle(IntVector2(0, 0), 160, 24));

    FDrawCommandList CheckedCommands;
    DrawElement(Checkable, CheckedCommands);
    TEST_EXPECT_EQ(CountCommands(CheckedCommands, EDrawCommandType::Polyline), 1);

    Checkable->SetCheckState(ECheckBoxState::Undetermined);

    FDrawCommandList DashCommands;
    DrawElement(Checkable, DashCommands);
    TEST_EXPECT_EQ(CountCommands(DashCommands, EDrawCommandType::Polyline), 0);
    TEST_EXPECT_EQ(CountCommands(DashCommands, EDrawCommandType::Box), 1);

    Checkable->SetCheckState(ECheckBoxState::Unchecked);

    FDrawCommandList UncheckedCommands;
    DrawElement(Checkable, UncheckedCommands);
    TEST_EXPECT_EQ(CountCommands(UncheckedCommands, EDrawCommandType::Box), 0);

    TEST_SECTION("A row with a submenu draws the arrow that says so");
    WithSubMenu->Tick(FRectangle(IntVector2(0, 0), 200, 24));

    FDrawCommandList ArrowCommands;
    DrawElement(WithSubMenu, ArrowCommands);
    TEST_EXPECT_EQ(CountCommands(ArrowCommands, EDrawCommandType::Polyline), 1);

    TEST_SECTION("A separator is a rule across the middle of its own row");
    TSharedPtr<FMenuSeparator> Separator = FMenuSeparator::Create();
    Separator->PrepareDesiredSize();
    TEST_EXPECT_EQ(Separator->GetCachedDesiredSize().Y, Style.Metrics.SeparatorThickness + 6);

    Separator->Tick(FRectangle(IntVector2(0, 0), 160, 7));

    FDrawCommandList SeparatorCommands;
    DrawElement(Separator, SeparatorCommands);
    TEST_EXPECT_EQ(CountCommands(SeparatorCommands, EDrawCommandType::Box), 1);
    TEST_EXPECT_EQ(SeparatorCommands.GetCommands()[0].Bounds.Height, Style.Metrics.SeparatorThickness);
    TEST_EXPECT_EQ(SeparatorCommands.GetCommands()[0].Bounds.Width, 160 - 12);

    TEST_END();
}

bool MenuItemActivation_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    FMenuStack& Stack = FMenuStack::Get();

    int32 ActivationCount = 0;

    FMenuItem::FDesc SaveDesc;
    SaveDesc.SetLabel("Save").SetFont(Font);
    SaveDesc.OnActivated = FOnMenuItemActivated::CreateLambda([&ActivationCount]() { ActivationCount++; });

    TSharedPtr<FMenuItem> Save   = FMenuItem::Create(SaveDesc);
    TSharedPtr<FMenu>     Recent = CreateMenu(Font, { "One", "Two" });

    FMenuItem::FDesc RecentDesc;
    RecentDesc.SetLabel("Recent").SetFont(Font);
    RecentDesc.SubMenu = Recent;

    TSharedPtr<FMenuItem> RecentRow = FMenuItem::Create(RecentDesc);

    TSharedPtr<FMenu> FileMenu = FMenu::Create();
    FileMenu->AddItem(Save);
    FileMenu->AddSeparator();
    FileMenu->AddItem(RecentRow);

    TSharedPtr<FWindow> FileWindow = Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, FileMenu);
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    TEST_SECTION("Choosing a row closes every menu before it runs what it was bound to");
    Save->Activate();
    TEST_EXPECT_EQ(ActivationCount, 1);
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);

    TEST_SECTION("A row with a submenu opens it on hover, once the cursor has rested");
    FileWindow = Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, FileMenu);

    RecentRow->OnMouseEntered(MakeMoveEvent(IntVector2(4, 4)));
    TEST_EXPECT(Stack.HasScheduledSubMenu());
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    Stack.Tick(0.1f);
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    Stack.Tick(0.2f);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);
    TEST_EXPECT(!Stack.HasScheduledSubMenu());

    TEST_SECTION("The submenu is placed beside the row that owns it");
    const FRectangle RowBounds = FMenuStack::GetScreenBounds(RecentRow);
    TEST_EXPECT_EQ(Stack.GetOpenMenus().Last()->GetPosition(), IntVector2(RowBounds.GetRight(), RowBounds.Position.Y));

    TEST_SECTION("Moving onto a plain row in the parent closes the branch the neighbour opened");
    Save->OnMouseEntered(MakeMoveEvent(IntVector2(4, 4)));
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    TEST_SECTION("Leaving a row before it opens drops what it had scheduled");
    RecentRow->OnMouseEntered(MakeMoveEvent(IntVector2(4, 4)));
    TEST_EXPECT(Stack.HasScheduledSubMenu());

    RecentRow->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));
    TEST_EXPECT(!Stack.HasScheduledSubMenu());

    Stack.Tick(1.0f);
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    TEST_SECTION("Clicking a row with a submenu opens it at once rather than waiting");
    ClickElement(RecentRow);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);

    TEST_SECTION("Choosing a row in the submenu closes the whole branch");
    GetTopMenu()->SetHighlightedIndex(0);
    GetTopMenu()->ActivateHighlighted();
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);

    TEST_END();
}

bool MenuKeyboard_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    FMenuStack& Stack = FMenuStack::Get();

    int32 ActivationCount = 0;

    // The third row is disabled, so the navigation has something it has to step over
    TSharedPtr<FMenu> Menu = CreateMenu(Font, { "New", "Open", "Import", "Quit" });
    Menu->GetItems()[2]->SetEnabled(false);

    TSharedPtr<FMenu> WithDelegate = FMenu::Create();
    for (int32 Index = 0; Index < 3; ++Index)
    {
        FMenuItem::FDesc Desc;
        Desc.SetLabel("Row").SetFont(Font);
        Desc.OnActivated = FOnMenuItemActivated::CreateLambda([&ActivationCount]() { ActivationCount++; });

        WithDelegate->AddItem(FMenuItem::Create(Desc));
    }

    TEST_SECTION("Nothing is highlighted until a key arrives");
    TEST_EXPECT_EQ(Menu->GetHighlightedIndex(), -1);

    TEST_SECTION("Down walks the rows and steps over the ones that cannot be chosen");
    Menu->OnKeyDown(MakeKeyEvent(Keys::Down));
    TEST_EXPECT_EQ(Menu->GetHighlightedIndex(), 0);

    Menu->OnKeyDown(MakeKeyEvent(Keys::Down));
    TEST_EXPECT_EQ(Menu->GetHighlightedIndex(), 1);

    Menu->OnKeyDown(MakeKeyEvent(Keys::Down));
    TEST_EXPECT_EQ(Menu->GetHighlightedIndex(), 3);

    TEST_SECTION("It wraps at either end");
    Menu->OnKeyDown(MakeKeyEvent(Keys::Down));
    TEST_EXPECT_EQ(Menu->GetHighlightedIndex(), 0);

    Menu->OnKeyDown(MakeKeyEvent(Keys::Up));
    TEST_EXPECT_EQ(Menu->GetHighlightedIndex(), 3);

    TEST_SECTION("Home and End go to the first and last rows that can be chosen");
    Menu->OnKeyDown(MakeKeyEvent(Keys::Home));
    TEST_EXPECT_EQ(Menu->GetHighlightedIndex(), 0);

    Menu->OnKeyDown(MakeKeyEvent(Keys::End));
    TEST_EXPECT_EQ(Menu->GetHighlightedIndex(), 3);

    TEST_SECTION("Only the highlighted row draws lit");
    TEST_EXPECT(Menu->GetItems()[3]->IsHighlighted());
    TEST_EXPECT(!Menu->GetItems()[0]->IsHighlighted());

    TEST_SECTION("Enter chooses the highlighted row");
    Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, WithDelegate);
    WithDelegate->OnKeyDown(MakeKeyEvent(Keys::Down));
    WithDelegate->OnKeyDown(MakeKeyEvent(Keys::Enter));
    TEST_EXPECT_EQ(ActivationCount, 1);
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);

    TEST_SECTION("The stack routes keys to the deepest menu, because a menu never holds focus");
    TSharedPtr<FWindow> MenuWindow = Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, Menu);
    Menu->SetHighlightedIndex(-1);

    TEST_EXPECT(Stack.HandleKeyDown(MakeKeyEvent(Keys::Down)));
    TEST_EXPECT_EQ(Menu->GetHighlightedIndex(), 0);

    TEST_SECTION("Escape closes one level at a time");
    TSharedPtr<FMenu> SubMenu = CreateMenu(Font, { "One", "Two" });
    Stack.PushMenu(MenuWindow, FRectangle(IntVector2(60, 40), 40, 24), EMenuPlacement::RightOfTopAligned, SubMenu);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);

    TEST_EXPECT(Stack.HandleKeyDown(MakeKeyEvent(Keys::Escape)));
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    TEST_EXPECT(Stack.HandleKeyDown(MakeKeyEvent(Keys::Escape)));
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);

    TEST_SECTION("With nothing open the stack leaves the key for whoever else wants it");
    TEST_EXPECT(!Stack.HandleKeyDown(MakeKeyEvent(Keys::Escape)));

    TEST_END();
}

bool MenuBarSwitching_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    TSharedPtr<FMenuBar> MenuBar = FMenuBar::Create();
    TSharedPtr<FMenuAnchor> FileAnchor = MenuBar->AddMenu("File", Font, CreateMenu(Font, { "New", "Open" }));
    TSharedPtr<FMenuAnchor> EditAnchor = MenuBar->AddMenu("Edit", Font, CreateMenu(Font, { "Undo", "Redo" }));

    Window->SetContent(MenuBar);
    FApplication::LayoutWindow(Window);

    TSharedPtr<FMenuBarButton> FileButton = StaticCastSharedPtr<FMenuBarButton>(FileAnchor->GetContent());
    TSharedPtr<FMenuBarButton> EditButton = StaticCastSharedPtr<FMenuBarButton>(EditAnchor->GetContent());

    TEST_SECTION("The buttons sit side by side across the top");
    TEST_EXPECT_EQ(FileButton->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(EditButton->GetContentRectangle().Position.X, FileButton->GetContentRectangle().Width);
    TEST_EXPECT(!MenuBar->IsAnyMenuOpen());

    TEST_SECTION("Hovering does nothing until the bar is in menu mode");
    EditButton->OnMouseEntered(MakeMoveEvent(IntVector2(50, 8)));
    TEST_EXPECT(!MenuBar->IsAnyMenuOpen());
    EditButton->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));

    TEST_SECTION("Clicking a button opens its drop-down under it");
    ClickElement(FileButton);
    TEST_EXPECT(FileAnchor->IsOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 1);
    TEST_EXPECT_EQ(FileAnchor->GetMenuWindow()->GetPosition(), IntVector2(0, FileButton->GetContentRectangle().GetBottom()));

    TEST_SECTION("With one open, hovering a sibling switches to it");
    EditButton->OnMouseEntered(MakeMoveEvent(IntVector2(50, 8)));
    TEST_EXPECT(!FileAnchor->IsOpen());
    TEST_EXPECT(EditAnchor->IsOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 1);

    TEST_SECTION("Clicking the open button closes it again");
    ClickElement(EditButton);
    TEST_EXPECT(!EditAnchor->IsOpen());
    TEST_EXPECT(!MenuBar->IsAnyMenuOpen());

    TEST_SECTION("An anchor notices its menu was closed from underneath it");
    FileAnchor->Open();
    TEST_EXPECT(FileAnchor->IsOpen());

    FMenuStack::Get().DismissAll();
    TEST_EXPECT(!FileAnchor->IsOpen());

    TEST_END();
}

bool ToolTipService_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(100, 50));

    TSharedPtr<FMenu> Owner = CreateMenu(Font, { "Hover me" });
    Window->SetContent(Owner);
    FApplication::LayoutWindow(Window);

    FToolTipService& ToolTips = FToolTipService::Get();

    TEST_SECTION("A tip waits for the cursor to rest before it appears");
    ToolTips.NotifyCursorMoved(IntVector2(300, 200));
    ToolTips.RequestTextToolTip(Owner, "Opens the file", Font, EToolTipPlacement::FollowCursor, 0.5f);
    TEST_EXPECT(ToolTips.IsPending());
    TEST_EXPECT(!ToolTips.IsShowing());

    ToolTips.Tick(0.3f);
    TEST_EXPECT(!ToolTips.IsShowing());

    ToolTips.Tick(0.3f);
    TEST_EXPECT(ToolTips.IsShowing());
    TEST_EXPECT(!ToolTips.IsPending());

    TEST_SECTION("A following tip sits below and right of the cursor");
    TSharedPtr<FWindow> ToolTipWindow = ToolTips.GetToolTipWindow();
    TEST_EXPECT(ToolTipWindow != nullptr);
    TEST_EXPECT_EQ(ToolTipWindow->GetPosition(), IntVector2(300 + FToolTipService::CursorOffset, 200 + FToolTipService::CursorOffset));

    TEST_SECTION("A tip takes no input, so it cannot steal the hover keeping it up");
    TEST_EXPECT(!ToolTipWindow->GetPlatformWindow()->GetAcceptsInput());

    TEST_SECTION("It follows the cursor rather than being torn down and built again");
    ToolTips.NotifyCursorMoved(IntVector2(340, 260));
    TEST_EXPECT(ToolTips.IsShowing());
    TEST_EXPECT_EQ(ToolTips.GetToolTipWindow(), ToolTipWindow);
    TEST_EXPECT_EQ(ToolTipWindow->GetPosition(), IntVector2(340 + FToolTipService::CursorOffset, 260 + FToolTipService::CursorOffset));

    TEST_SECTION("Leaving the element it describes takes it down");
    ToolTips.CancelToolTip(Owner);
    TEST_EXPECT(!ToolTips.IsShowing());
    TEST_EXPECT(ToolTips.GetOwner() == nullptr);

    TEST_SECTION("Moving while a tip is pending starts the rest over");
    ToolTips.RequestTextToolTip(Owner, "Opens the file", Font, EToolTipPlacement::FollowCursor, 0.5f);
    ToolTips.Tick(0.4f);
    ToolTips.NotifyCursorMoved(IntVector2(500, 300));
    ToolTips.Tick(0.4f);
    TEST_EXPECT(!ToolTips.IsShowing());

    ToolTips.Tick(0.2f);
    TEST_EXPECT(ToolTips.IsShowing());
    ToolTips.DismissToolTip();

    TEST_SECTION("An anchored tip sits under the element instead of at the cursor");
    ToolTips.RequestTextToolTip(Owner, "Opens the file", Font, EToolTipPlacement::BelowAnchor, 0.0f);
    ToolTips.Tick(0.1f);
    TEST_EXPECT(ToolTips.IsShowing());

    const FRectangle OwnerBounds = FMenuStack::GetScreenBounds(Owner);
    TEST_EXPECT_EQ(ToolTips.GetToolTipWindow()->GetPosition(), IntVector2(OwnerBounds.Position.X, OwnerBounds.GetBottom() + 2));

    TEST_SECTION("In the corner it is pulled back onto the monitor");
    ToolTips.DismissToolTip();
    ToolTips.NotifyCursorMoved(IntVector2(1910, 1070));
    ToolTips.RequestTextToolTip(Owner, "Opens the file", Font, EToolTipPlacement::FollowCursor, 0.0f);
    ToolTips.Tick(0.1f);

    const IntVector2 ToolTipSize = ToolTips.GetToolTipWindow()->GetSize();
    TEST_EXPECT_EQ(ToolTips.GetToolTipWindow()->GetPosition(), IntVector2(1920 - ToolTipSize.X, 1080 - ToolTipSize.Y));

    TEST_END();
}

bool ComboBoxControl_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    int32 SelectionChangedCount = 0;
    int32 LastSelectedIndex     = -1;

    FComboBox::FDesc Desc;
    Desc.SetOptions({ "Low", "Medium", "High" }).SetFont(Font);
    Desc.PlaceholderText    = "None";
    Desc.OnSelectionChanged = FOnComboSelectionChanged::CreateLambda([&](int32 Index)
    {
        SelectionChangedCount++;
        LastSelectedIndex = Index;
    });

    TSharedPtr<FComboBox> ComboBox = FComboBox::Create(Desc);
    Window->SetContent(ComboBox);
    FApplication::LayoutWindow(Window);

    TEST_SECTION("With nothing selected it shows the placeholder");
    TEST_EXPECT_EQ(ComboBox->GetSelectedIndex(), -1);
    TEST_EXPECT(ComboBox->GetSelectedText() == String("None"));

    TEST_SECTION("It is wide enough for the longest option, so it does not resize as the choice changes");
    ComboBox->PrepareDesiredSize();
    const int32 WidestOption = 6 * 8;
    TEST_EXPECT(ComboBox->GetCachedDesiredSize().X >= WidestOption);

    TEST_SECTION("Selecting reports the new index once");
    ComboBox->SetSelectedIndex(1);
    TEST_EXPECT_EQ(SelectionChangedCount, 1);
    TEST_EXPECT_EQ(LastSelectedIndex, 1);
    TEST_EXPECT(ComboBox->GetSelectedText() == String("Medium"));

    ComboBox->SetSelectedIndex(1);
    TEST_EXPECT_EQ(SelectionChangedCount, 1);

    TEST_SECTION("Opening lists every option with the selected one ticked");
    ComboBox->OpenMenu();
    TEST_EXPECT(ComboBox->IsMenuOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 1);

    TSharedPtr<FMenu> OptionsMenu = GetTopMenu();
    TEST_EXPECT(OptionsMenu != nullptr);
    TEST_EXPECT_EQ(OptionsMenu->GetItems().Size(), 3);
    TEST_EXPECT_EQ(OptionsMenu->GetItems()[1]->GetCheckState(), ECheckBoxState::Checked);
    TEST_EXPECT_EQ(OptionsMenu->GetItems()[0]->GetCheckState(), ECheckBoxState::Unchecked);
    TEST_EXPECT_EQ(OptionsMenu->GetHighlightedIndex(), 1);

    TEST_SECTION("The list is at least as wide as the button it dropped from");
    TEST_EXPECT(FMenuStack::Get().GetOpenMenus().Last()->GetSize().X >= ComboBox->GetContentRectangle().Width);

    TEST_SECTION("Choosing an option selects it and closes the list");
    OptionsMenu->GetItems()[2]->Activate();
    TEST_EXPECT_EQ(ComboBox->GetSelectedIndex(), 2);
    TEST_EXPECT_EQ(LastSelectedIndex, 2);
    TEST_EXPECT(!ComboBox->IsMenuOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 0);

    TEST_SECTION("Replacing the options clears the selection");
    ComboBox->SetOptions({ "One", "Two" });
    TEST_EXPECT_EQ(ComboBox->GetSelectedIndex(), -1);
    TEST_EXPECT(ComboBox->GetSelectedText() == String("None"));

    TEST_END();
}

bool DragDropService_Test()
{
    TEST_BEGIN();

    FDragDropService& DragDrop = FDragDropService::Get();

    int32      DropCount    = 0;
    String     DroppedType;
    IntVector2 DropPosition = IntVector2(0, 0);

    TSharedPtr<FVisualElement> Accepting = CreateDropTarget(FRectangle(IntVector2(100, 100), 200, 100));
    TSharedPtr<FVisualElement> Refusing  = CreateDropTarget(FRectangle(IntVector2(400, 100), 200, 100));

    DragDrop.RegisterTarget(Accepting, FOnDragDropped::CreateLambda([&](const FDragDropPayload& Payload, const IntVector2& ScreenPosition)
    {
        DropCount++;
        DroppedType  = Payload.TypeId;
        DropPosition = ScreenPosition;
    }), FOnDragOver());

    DragDrop.RegisterTarget(Refusing, FOnDragDropped(), FOnDragOver::CreateLambda([](const FDragDropPayload&) -> bool
    {
        return false;
    }));

    FDragDropPayload AssetPayload;
    AssetPayload.TypeId      = "Asset";
    AssetPayload.DisplayText = "Rock.mesh";

    TEST_SECTION("A payload naming no type is not a drag, so beginning one is ignored");
    FDragDropPayload Anonymous;
    Anonymous.DisplayText = "Something";

    DragDrop.BeginDrag(Anonymous, IntVector2(150, 150));
    TEST_EXPECT(!DragDrop.IsDragging());
    TEST_EXPECT(!DragDrop.HasTarget());

    TEST_SECTION("A payload naming one starts the drag and is remembered with where the cursor was");
    DragDrop.BeginDrag(AssetPayload, IntVector2(150, 150));
    TEST_EXPECT(DragDrop.IsDragging());
    TEST_EXPECT(DragDrop.GetPayload().TypeId == String("Asset"));
    TEST_EXPECT(DragDrop.GetPayload().DisplayText == String("Rock.mesh"));
    TEST_EXPECT_EQ(DragDrop.GetScreenPosition(), IntVector2(150, 150));
    TEST_EXPECT(DragDrop.HasTarget());

    TEST_SECTION("Moving the drag re-resolves the target as the cursor passes over one");
    DragDrop.UpdateDrag(IntVector2(700, 400));
    TEST_EXPECT_EQ(DragDrop.GetScreenPosition(), IntVector2(700, 400));
    TEST_EXPECT(!DragDrop.HasTarget());

    DragDrop.UpdateDrag(IntVector2(200, 150));
    TEST_EXPECT(DragDrop.HasTarget());

    TEST_SECTION("A target that will not take the payload is passed over rather than picked");
    DragDrop.UpdateDrag(IntVector2(500, 150));
    TEST_EXPECT(!DragDrop.HasTarget());

    TEST_SECTION("Ending the drag hands the payload and the release point to the target under the cursor");
    DragDrop.EndDrag(IntVector2(220, 160));
    TEST_EXPECT_EQ(DropCount, 1);
    TEST_EXPECT(DroppedType == String("Asset"));
    TEST_EXPECT_EQ(DropPosition, IntVector2(220, 160));
    TEST_EXPECT(!DragDrop.IsDragging());
    TEST_EXPECT(!DragDrop.HasTarget());

    TEST_SECTION("Ending it over nothing drops nothing and still ends the drag");
    DragDrop.BeginDrag(AssetPayload, IntVector2(150, 150));
    DragDrop.EndDrag(IntVector2(900, 900));
    TEST_EXPECT_EQ(DropCount, 1);
    TEST_EXPECT(!DragDrop.IsDragging());

    TEST_SECTION("Cancelling ends the drag without firing anything");
    DragDrop.BeginDrag(AssetPayload, IntVector2(150, 150));
    TEST_EXPECT(DragDrop.IsDragging());

    DragDrop.CancelDrag();
    TEST_EXPECT(!DragDrop.IsDragging());
    TEST_EXPECT(!DragDrop.HasTarget());
    TEST_EXPECT_EQ(DropCount, 1);

    TEST_SECTION("A target that has been forgotten is no longer found under the cursor");
    DragDrop.UnregisterTarget(Accepting);
    DragDrop.BeginDrag(AssetPayload, IntVector2(150, 150));
    TEST_EXPECT(!DragDrop.HasTarget());

    DragDrop.EndDrag(IntVector2(150, 150));
    TEST_EXPECT_EQ(DropCount, 1);

    TEST_SECTION("A target that has since been destroyed is skipped rather than followed");
    int32 SurvivorDropCount = 0;

    TSharedPtr<FVisualElement> Survivor = CreateDropTarget(FRectangle(IntVector2(100, 100), 200, 100));
    TSharedPtr<FVisualElement> Doomed   = CreateDropTarget(FRectangle(IntVector2(100, 100), 200, 100));

    DragDrop.RegisterTarget(Survivor, FOnDragDropped::CreateLambda([&SurvivorDropCount](const FDragDropPayload&, const IntVector2&)
    {
        SurvivorDropCount++;
    }), FOnDragOver());

    DragDrop.RegisterTarget(Doomed, FOnDragDropped(), FOnDragOver());
    Doomed.Reset();

    DragDrop.BeginDrag(AssetPayload, IntVector2(150, 150));
    TEST_EXPECT(DragDrop.HasTarget());

    DragDrop.EndDrag(IntVector2(150, 150));
    TEST_EXPECT_EQ(SurvivorDropCount, 1);

    TEST_SECTION("The ghost is drawn only while a drag is in flight, and offset from the cursor");
    FDrawCommandList IdleCommands;
    DragDrop.DrawDragVisual(IdleCommands, 0);
    TEST_EXPECT(IdleCommands.IsEmpty());

    DragDrop.BeginDrag(AssetPayload, IntVector2(150, 150));

    FDrawCommandList DragCommands;
    DragDrop.DrawDragVisual(DragCommands, 0);
    TEST_EXPECT(!DragCommands.IsEmpty());
    TEST_EXPECT_EQ(CountCommands(DragCommands, EDrawCommandType::Box), 1);
    TEST_EXPECT_EQ(CountCommands(DragCommands, EDrawCommandType::Text), 1);

    if (!DragCommands.IsEmpty())
    {
        TEST_EXPECT_EQ(DragCommands[0].Bounds.Position, IntVector2(150, 150) + IntVector2(FDragDropService::DragVisualCursorOffset, FDragDropService::DragVisualCursorOffset));
    }

    FDragDropService::Release();

    TEST_END();
}
