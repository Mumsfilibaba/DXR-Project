#include "MenuTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/ElementPath.h>
#include <Application/Elements/MenuHost.h>
#include <Application/Elements/Overlay.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/TitleBar.h>
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

/** @brief True when any published rectangle covers a point, which is what the platform asks at hit-test time. */
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
    const TArray<FMenuHandle>& OpenMenus = FMenuStack::Get().GetOpenMenus();
    if (OpenMenus.IsEmpty())
    {
        return nullptr;
    }

    return StaticCastSharedPtr<FMenu>(OpenMenus.Last()->Content);
}

static FRectangle GetTopMenuBounds()
{
    const TArray<FMenuHandle>& OpenMenus = FMenuStack::Get().GetOpenMenus();
    return OpenMenus.IsEmpty() ? FRectangle() : OpenMenus.Last()->ScreenBounds;
}

static FRectangle GetClientArea(const TSharedPtr<FWindow>& Window)
{
    const IntVector2 Size = Window->GetSize();
    return FRectangle(Window->GetPosition(), Size.X, Size.Y);
}

bool MenuStackPlacement_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(100, 100));

    FMenuStack& Stack = FMenuStack::Get();

    const FRectangle ClientArea = GetClientArea(Window);

    TEST_SECTION("A drop-down sits under the anchor with their left edges flush");
    TSharedPtr<FMenu> Menu     = CreateMenu(Font, { "New", "Open", "Save" });
    FMenuHandle       DropDown = Stack.PushMenu(Window, FRectangle(IntVector2(140, 130), 60, 24), EMenuPlacement::BelowLeftAligned, Menu);

    TEST_EXPECT(DropDown != nullptr);
    const IntVector2 MenuSize = Menu->GetCachedDesiredSize();
    TEST_EXPECT_EQ(DropDown->ScreenBounds.Position, IntVector2(140, 154));
    TEST_EXPECT_EQ(DropDown->ScreenBounds.GetSize(), MenuSize);

    TEST_SECTION("One that fits is drawn inside its host, so it needs no window and no transparent surface");
    TEST_EXPECT(DropDown->bIsInline);
    TEST_EXPECT(DropDown->MenuWindow == nullptr);
    TEST_EXPECT_EQ(DropDown->HostWindow, Window);
    TEST_EXPECT(Window->GetMenuHost() != nullptr);
    TEST_EXPECT(!Window->GetMenuHost()->IsEmpty());

    Stack.DismissAll();
    TEST_EXPECT(Window->GetMenuHost()->IsEmpty());

    TEST_SECTION("With no room below, the drop-down goes above the anchor instead");
    DropDown = Stack.PushMenu(Window, FRectangle(IntVector2(140, 690), 60, 24), EMenuPlacement::BelowLeftAligned, Menu);
    TEST_EXPECT_EQ(DropDown->ScreenBounds.Position, IntVector2(140, 690 - MenuSize.Y));
    Stack.DismissAll();

    TEST_SECTION("A submenu opens beside its anchor, lifted by its padding so its first row rather than its outline is flush");
    const int32 MenuTopInset = Menu->GetContentTopInset();

    DropDown = Stack.PushMenu(Window, FRectangle(IntVector2(140, 130), 60, 24), EMenuPlacement::RightOfTopAligned, Menu);
    TEST_EXPECT_EQ(DropDown->ScreenBounds.Position, IntVector2(200, 130 - MenuTopInset));
    Stack.DismissAll();

    TEST_SECTION("With no room to the right, the submenu goes to the left of its anchor and keeps that lift");
    DropDown = Stack.PushMenu(Window, FRectangle(IntVector2(860, 130), 20, 24), EMenuPlacement::RightOfTopAligned, Menu);
    TEST_EXPECT_EQ(DropDown->ScreenBounds.Position, IntVector2(860 - MenuSize.X, 130 - MenuTopInset));
    Stack.DismissAll();

    TEST_SECTION("A context menu opens at the cursor, and in the window's corner it opens back towards it");
    DropDown = Stack.PushMenu(Window, FRectangle(IntVector2(400, 300), 0, 0), EMenuPlacement::AtCursor, Menu);
    TEST_EXPECT_EQ(DropDown->ScreenBounds.Position, IntVector2(400, 300));
    Stack.DismissAll();

    DropDown = Stack.PushMenu(Window, FRectangle(IntVector2(880, 680), 0, 0), EMenuPlacement::AtCursor, Menu);
    TEST_EXPECT_EQ(DropDown->ScreenBounds.Position, IntVector2(880 - MenuSize.X, 680 - MenuSize.Y));
    Stack.DismissAll();

    TEST_SECTION("One with nowhere left to flip to is pulled back inside the window rather than past its edge");

    TSharedPtr<FWindow> ShortWindow = Application.CreateWindow(IntVector2(400, MenuSize.Y + 20), IntVector2(1000, 100));
    const FRectangle    ShortArea   = GetClientArea(ShortWindow);

    DropDown = Stack.PushMenu(ShortWindow, FRectangle(ShortArea.GetCenter(), 0, 0), EMenuPlacement::AtCursor, Menu);

    TEST_EXPECT(DropDown->bIsInline);
    TEST_EXPECT_EQ(DropDown->ScreenBounds.Position.Y, ShortArea.GetBottom() - MenuSize.Y);
    Stack.DismissAll();

    TEST_SECTION("A menu too tall for its host falls back to a popup window, clamped to the monitor instead");
    TArray<String> ManyLabels;
    for (int32 Index = 0; Index < 80; ++Index)
    {
        ManyLabels.Add("Row");
    }

    TSharedPtr<FMenu> LongMenu = CreateMenu(Font, ManyLabels);
    FMenuHandle       LongDrop = Stack.PushMenu(Window, FRectangle(IntVector2(140, 400), 60, 24), EMenuPlacement::BelowLeftAligned, LongMenu);

    TEST_EXPECT(LongMenu->GetCachedDesiredSize().Y > ClientArea.Height);
    TEST_EXPECT(!LongDrop->bIsInline);
    TEST_EXPECT(LongDrop->MenuWindow != nullptr);
    TEST_EXPECT_EQ(LongDrop->ScreenBounds.Position.Y, 0);
    TEST_EXPECT_EQ(LongDrop->MenuWindow->GetPosition(), LongDrop->ScreenBounds.Position);

    TEST_SECTION("That popup is borderless, on top and out of the task bar");
    const EWindowStyleFlags StyleFlags = LongDrop->MenuWindow->GetPlatformWindow()->GetStyle();
    TEST_EXPECT((StyleFlags & EWindowStyleFlags::TopMost) == EWindowStyleFlags::TopMost);
    TEST_EXPECT((StyleFlags & EWindowStyleFlags::NoTaskBarIcon) == EWindowStyleFlags::NoTaskBarIcon);
    TEST_EXPECT((StyleFlags & EWindowStyleFlags::Titled) == EWindowStyleFlags::None);
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
    FMenuHandle FileHandle = Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, FileMenu);
    TEST_EXPECT(Stack.IsOpen());
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);
    TEST_EXPECT_EQ(Stack.GetMenuDepth(FileHandle), 1);
    TEST_EXPECT_EQ(Stack.GetOwningMenuDepth(Window), 0);

    TEST_SECTION("Opening from a row makes the new one that row's child, even though both share a host window");
    const TSharedPtr<FMenuItem>& FileRow = FileMenu->GetItems()[0];
    TEST_EXPECT_EQ(Stack.GetOwningMenuDepth(FileRow), 1);

    FMenuHandle RecentHandle = Stack.PushMenu(FileRow, FRectangle(IntVector2(60, 40), 40, 24), EMenuPlacement::RightOfTopAligned, Recent);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);
    TEST_EXPECT_EQ(Stack.GetMenuDepth(RecentHandle), 2);
    TEST_EXPECT(Stack.IsMenuOpen(FileHandle));
    TEST_EXPECT_EQ(RecentHandle->HostWindow, FileHandle->HostWindow);

    TEST_SECTION("Opening from the same row again replaces the sibling rather than stacking on it");
    FMenuHandle DeeperHandle = Stack.PushMenu(FileRow, FRectangle(IntVector2(60, 70), 40, 24), EMenuPlacement::RightOfTopAligned, Deeper);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);
    TEST_EXPECT(!Stack.IsMenuOpen(RecentHandle));
    TEST_EXPECT(Stack.IsMenuOpen(DeeperHandle));

    TEST_SECTION("A click inside an open menu leaves the stack alone, whether or not it has a window of its own");
    TEST_EXPECT(!Stack.DismissOnClickOutside(DeeperHandle->ScreenBounds.Position + IntVector2(2, 2)));
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);

    TEST_EXPECT(!Stack.DismissOnClickOutside(FileHandle->ScreenBounds.Position + IntVector2(2, 2)));
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);

    TEST_SECTION("A click outside every menu closes the lot");
    TEST_EXPECT(Stack.DismissOnClickOutside(IntVector2(700, 500)));
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);
    TEST_EXPECT(!Stack.IsOpen());

    TEST_SECTION("Opening from the window again starts over rather than nesting");
    FileHandle   = Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, FileMenu);
    RecentHandle = Stack.PushMenu(FileRow, FRectangle(IntVector2(60, 40), 40, 24), EMenuPlacement::RightOfTopAligned, Recent);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);

    Stack.PushMenu(Window, FRectangle(IntVector2(60, 10), 40, 24), EMenuPlacement::BelowLeftAligned, Deeper);
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    TEST_SECTION("Dismissing the top leaves its parent open");
    Stack.PushMenu(Deeper->GetItems()[0], FRectangle(IntVector2(60, 40), 40, 24), EMenuPlacement::RightOfTopAligned, Recent);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);

    Stack.DismissTop();
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    Stack.DismissAll();
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);
    TEST_EXPECT(Window->GetMenuHost()->IsEmpty());

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
    TEST_EXPECT_EQ(Plain->GetCachedDesiredSize().X, 28 + FMenuItem::GutterWidth + LabelWidth);
    TEST_EXPECT_EQ(Plain->GetCachedDesiredSize().Y, FUIStyle::GetDefault().Menu.RowHeight);

    TEST_SECTION("A shortcut hint widens the row by the gap and its own width");
    FMenuItem::FDesc ShortcutDesc;
    ShortcutDesc.SetLabel("Open").SetFont(Font);
    ShortcutDesc.ShortcutText = "Ctrl+O";

    TSharedPtr<FMenuItem> WithShortcut = FMenuItem::Create(ShortcutDesc);
    WithShortcut->PrepareDesiredSize();
    TEST_EXPECT_EQ(WithShortcut->GetCachedDesiredSize().X, Plain->GetCachedDesiredSize().X + FMenuItem::ShortcutGap + (6 * 8));

    TEST_SECTION("The hint is drawn upper case, whatever case the accelerator was named in");
    WithShortcut->Tick(FRectangle(IntVector2(0, 0), 200, 26));

    FDrawCommandList ShortcutCommands;
    DrawElement(WithShortcut, ShortcutCommands);
    TEST_EXPECT_EQ(CountCommands(ShortcutCommands, EDrawCommandType::Text), 2);
    TEST_EXPECT_EQ(ShortcutCommands.GetCommands()[1].Text, String("CTRL+O"));

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
    TEST_EXPECT_EQ(Separator->GetCachedDesiredSize().Y, Style.Metrics.MenuSeparatorThickness + 8);

    Separator->Tick(FRectangle(IntVector2(0, 0), 160, Separator->GetCachedDesiredSize().Y));

    FDrawCommandList SeparatorCommands;
    DrawElement(Separator, SeparatorCommands);
    TEST_EXPECT_EQ(CountCommands(SeparatorCommands, EDrawCommandType::Box), 1);
    TEST_EXPECT_EQ(SeparatorCommands.GetCommands()[0].Bounds.Height, Style.Metrics.MenuSeparatorThickness);
    TEST_EXPECT_EQ(SeparatorCommands.GetCommands()[0].Bounds.Width, 160 - (FMenuSeparator::InsetX * 2));

    TEST_SECTION("A section header takes the taller of its caption and the rule, plus the same padding");
    TSharedPtr<FMenuSectionHeader> Header = FMenuSectionHeader::Create("Open", Font);
    Header->PrepareDesiredSize();
    TEST_EXPECT_EQ(Header->GetCachedDesiredSize().Y, Math::Max(Font->GetLineHeight(), Style.Metrics.MenuSeparatorThickness) + 8);

    TEST_SECTION("It draws its caption upper case, whatever case it was given");
    TEST_EXPECT_EQ(Header->GetLabel(), String("OPEN"));

    const int32 HeaderHeight = Header->GetCachedDesiredSize().Y;
    Header->Tick(FRectangle(IntVector2(0, 0), 160, HeaderHeight));

    FDrawCommandList HeaderCommands;
    DrawElement(Header, HeaderCommands);
    TEST_EXPECT_EQ(CountCommands(HeaderCommands, EDrawCommandType::Text), 1);
    TEST_EXPECT_EQ(CountCommands(HeaderCommands, EDrawCommandType::Box), 1);

    TEST_SECTION("The caption sits at the same inset the rule is held back by");
    const FDrawCommand& CaptionCommand = HeaderCommands.GetCommands()[0];
    TEST_EXPECT_EQ(CaptionCommand.Type, EDrawCommandType::Text);
    TEST_EXPECT_EQ(CaptionCommand.Bounds.Position.X, FMenuSectionHeader::InsetX);

    TEST_SECTION("The rule runs to the right of the caption only, and stops at the far inset");
    const int32         CaptionWidth = Font->MeasureWidth(StringView("OPEN", 4));
    const FDrawCommand& RuleCommand  = HeaderCommands.GetCommands()[1];
    TEST_EXPECT_EQ(RuleCommand.Type, EDrawCommandType::Box);
    TEST_EXPECT_EQ(RuleCommand.Bounds.Position.X, FMenuSectionHeader::InsetX + CaptionWidth + FMenuSectionHeader::LabelGap);
    TEST_EXPECT_EQ(RuleCommand.Bounds.GetRight(), 160 - FMenuSectionHeader::InsetX);
    TEST_EXPECT_EQ(RuleCommand.Bounds.Height, Style.Metrics.MenuSeparatorThickness);

    TEST_SECTION("A menu is a fill behind a single ring, with no bevel inside it");
    TSharedPtr<FMenu> Chrome = CreateMenu(Font, { "Open" });
    Chrome->PrepareDesiredSize();
    Chrome->Tick(FRectangle(IntVector2(0, 0), 160, 80));

    FDrawCommandList ChromeCommands;
    DrawElement(Chrome, ChromeCommands);
    TEST_EXPECT_EQ(CountCommands(ChromeCommands, EDrawCommandType::BoxOutline), 1);

    const FDrawCommand& Ring = ChromeCommands.GetCommands()[1];
    TEST_EXPECT_EQ(Ring.Type, EDrawCommandType::BoxOutline);
    TEST_EXPECT_EQ(Ring.Bounds, FRectangle(IntVector2(0, 0), 160, 80));
    TEST_EXPECT(Ring.Tint == Style.Menu.Border);

    TEST_SECTION("The fill behind it comes from the menu style");
    const FDrawCommand& Fill = ChromeCommands.GetCommands()[0];
    TEST_EXPECT_EQ(Fill.Type, EDrawCommandType::Box);
    TEST_EXPECT(Fill.Tint == Style.Menu.Background);

    TEST_END();
}

bool MenuStyle_Test()
{
    TEST_BEGIN();

    FUIStyle::ResetDefault();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FUIMenuStyle MenuStyle;
    MenuStyle.Background         = FFloatColor(0.90f, 0.10f, 0.10f, 1.0f);
    MenuStyle.Border             = FFloatColor(0.10f, 0.90f, 0.10f, 1.0f);
    MenuStyle.ItemHovered        = FFloatColor(0.10f, 0.10f, 0.90f, 1.0f);
    MenuStyle.Separator          = FFloatColor(0.90f, 0.90f, 0.10f, 1.0f);
    MenuStyle.SectionText        = FFloatColor(0.10f, 0.90f, 0.90f, 1.0f);
    MenuStyle.ItemHighlightInset = 11;
    MenuStyle.ItemCornerRadius   = 7.0f;
    MenuStyle.CornerRadius       = 13.0f;

    TSharedPtr<FMenu> Menu = CreateMenu(Font, { "Open" });
    Menu->AddSeparator();
    Menu->AddSection("Recent", Font);
    Menu->SetStyle(MenuStyle);

    Menu->PrepareDesiredSize();
    Menu->Tick(FRectangle(IntVector2(0, 0), 160, 110));

    TEST_SECTION("The menu's own fill and ring come from the style it was given, both rounded to its radius");
    FDrawCommandList Commands;
    DrawElement(Menu, Commands);

    TEST_EXPECT(Commands.GetCommands()[0].Tint == MenuStyle.Background);
    TEST_EXPECT_EQ(Commands.GetCommands()[0].CornerRadius.TopLeft, MenuStyle.CornerRadius);
    TEST_EXPECT_EQ(Commands.GetCommands()[0].CornerRadius.BottomRight, MenuStyle.CornerRadius);

    TEST_EXPECT(Commands.GetCommands()[1].Tint == MenuStyle.Border);
    TEST_EXPECT_EQ(Commands.GetCommands()[1].CornerRadius.TopLeft, MenuStyle.CornerRadius);

    TEST_SECTION("The rule and the heading it already held take it too, not only the rows");
    TArray<TSharedPtr<FVisualElement>> Panels;
    Menu->GetChildren(Panels);
    TEST_EXPECT_EQ(Panels.Size(), 1);

    TArray<TSharedPtr<FVisualElement>> Entries;
    Panels[0]->GetChildren(Entries);

    bool bFoundSeparatorFill = false;
    bool bFoundSectionText   = false;

    for (const TSharedPtr<FVisualElement>& Entry : Entries)
    {
        FDrawCommandList EntryCommands;
        DrawElement(Entry, EntryCommands);

        for (const FDrawCommand& Command : EntryCommands.GetCommands())
        {
            bFoundSeparatorFill |= Command.Type == EDrawCommandType::Box && Command.Tint == MenuStyle.Separator;
            bFoundSectionText |= Command.Type == EDrawCommandType::Text && Command.Tint == MenuStyle.SectionText;
        }
    }

    TEST_EXPECT(bFoundSeparatorFill);
    TEST_EXPECT(bFoundSectionText);

    TEST_SECTION("The style reaches the rows the menu already held, rather than only the ones added after");
    TSharedPtr<FMenuItem> Item = Menu->GetItems()[0];
    Item->SetHighlighted(true);

    FDrawCommandList ItemCommands;
    DrawElement(Item, ItemCommands);

    const FDrawCommand& Highlight = ItemCommands.GetCommands()[0];
    TEST_EXPECT_EQ(Highlight.Type, EDrawCommandType::Box);
    TEST_EXPECT(Highlight.Tint == MenuStyle.ItemHovered);
    TEST_EXPECT(Highlight.CornerRadius == FCornerRadii(MenuStyle.ItemCornerRadius));
    TEST_EXPECT_EQ(Highlight.Bounds.Position.X, Item->GetContentRectangle().Position.X + MenuStyle.ItemHighlightInset);
    TEST_EXPECT_EQ(Highlight.Bounds.GetRight(), Item->GetContentRectangle().GetRight() - MenuStyle.ItemHighlightInset);

    Item->SetHighlighted(false);

    TEST_SECTION("A row added afterwards is styled the same way without a second call");
    FMenuItem::FDesc LateDesc;
    LateDesc.SetLabel("Close").SetFont(Font);

    TSharedPtr<FMenuItem> Late = FMenuItem::Create(LateDesc);
    Menu->AddItem(Late);
    Menu->PrepareDesiredSize();
    Menu->Tick(FRectangle(IntVector2(0, 0), 160, 110));

    Late->SetHighlighted(true);

    FDrawCommandList LateCommands;
    DrawElement(Late, LateCommands);
    TEST_EXPECT(LateCommands.GetCommands()[0].Tint == MenuStyle.ItemHovered);

    TEST_SECTION("A menu left alone still draws in the shipped colours, so the override stayed with the one menu");
    TSharedPtr<FMenu> Plain = CreateMenu(Font, { "Open" });
    Plain->PrepareDesiredSize();
    Plain->Tick(FRectangle(IntVector2(0, 0), 160, 80));

    FDrawCommandList PlainCommands;
    DrawElement(Plain, PlainCommands);

    TEST_EXPECT(PlainCommands.GetCommands()[0].Tint == FUIStyle::GetDefault().Menu.Background);
    TEST_EXPECT(PlainCommands.GetCommands()[1].Tint == FUIStyle::GetDefault().Menu.Border);
    TEST_EXPECT(!(FUIStyle::GetDefault().Menu.Background == MenuStyle.Background));

    TEST_END();
}

bool MenuMinimumWidth_Test()
{
    TEST_BEGIN();

    FUIStyle::ResetDefault();

    const TSharedPtr<IFontFace> Font     = CreateFont();
    const int32                 MinWidth = FUIStyle::GetDefault().Menu.MinWidth;

    TEST_SECTION("A menu of narrow rows is still held out to the style's floor");
    TSharedPtr<FMenu> Narrow = CreateMenu(Font, { "On", "Off" });
    Narrow->PrepareDesiredSize();

    TEST_EXPECT_EQ(Narrow->GetCachedDesiredSize().X, MinWidth);

    TEST_SECTION("A row wider than the floor takes the menu out past it");
    TSharedPtr<FMenu> Wide = CreateMenu(Font, { "A menu entry whose label runs well past the floor" });
    Wide->PrepareDesiredSize();

    TEST_EXPECT(Wide->GetCachedDesiredSize().X > MinWidth);

    TEST_SECTION("An explicit width wins even asking for less, which is how a combo box matches its button");
    TSharedPtr<FMenu> Combo = CreateMenu(Font, { "On", "Off" });
    Combo->SetMinDesiredWidth(0);
    Combo->PrepareDesiredSize();

    TEST_EXPECT(Combo->GetCachedDesiredSize().X < MinWidth);

    TEST_SECTION("A menu dressed in its own style takes that style's floor rather than the shipped one");
    FUIMenuStyle WideStyle;
    WideStyle.MinWidth = MinWidth + 94;

    TSharedPtr<FMenu> Styled = CreateMenu(Font, { "On", "Off" });
    Styled->SetStyle(WideStyle);
    Styled->PrepareDesiredSize();

    TEST_EXPECT_EQ(Styled->GetCachedDesiredSize().X, WideStyle.MinWidth);

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

    Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, FileMenu);
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    TEST_SECTION("Choosing a row closes every menu before it runs what it was bound to");
    Save->Activate();
    TEST_EXPECT_EQ(ActivationCount, 1);
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);

    TEST_SECTION("A row with a submenu opens it on hover, once the cursor has rested");
    Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, FileMenu);

    RecentRow->OnMouseEntered(MakeMoveEvent(IntVector2(4, 4)));
    TEST_EXPECT(Stack.HasScheduledSubMenu());
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    Stack.Tick(0.1f);
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    Stack.Tick(0.2f);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);
    TEST_EXPECT(!Stack.HasScheduledSubMenu());

    TEST_SECTION("The submenu is placed beside the row that owns it, far enough up that its own first row lines up with that one");
    const FRectangle RowBounds = FMenuStack::GetScreenBounds(RecentRow);
    TEST_EXPECT_EQ(GetTopMenuBounds().Position,
        IntVector2(RowBounds.GetRight(), RowBounds.Position.Y - Recent->GetContentTopInset()));

    TEST_EXPECT(Recent->GetContentTopInset() > 0);
    TEST_EXPECT_EQ(FMenuStack::GetScreenBounds(Recent->GetItems()[0]).Position.Y, RowBounds.Position.Y);

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

    TEST_SECTION("A window activating mid-click does not swallow the click, because the press is still held");
    int32 ExitCount = 0;

    FMenuItem::FDesc ExitDesc;
    ExitDesc.SetLabel("Exit").SetFont(Font);
    ExitDesc.OnActivated = FOnMenuItemActivated::CreateLambda([&ExitCount]() { ExitCount++; });

    TSharedPtr<FMenuItem> ExitRow  = FMenuItem::Create(ExitDesc);
    TSharedPtr<FMenu>     ExitMenu = FMenu::Create();
    ExitMenu->AddItem(ExitRow);

    Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, ExitMenu);

    const IntVector2 ExitCenter = ExitRow->GetContentRectangle().GetCenter();
    ExitRow->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, ExitCenter));
    TEST_EXPECT(ExitRow->IsPressed());
    TEST_EXPECT(FApplication::Get().GetMouseCaptor().Get() == ExitRow.Get());

    ExitRow->OnFocusLost();
    TEST_EXPECT(ExitRow->IsPressed());

    ExitRow->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, ExitCenter));
    TEST_EXPECT_EQ(ExitCount, 1);
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);

    TEST_SECTION("A hovered row puts the hand under the cursor, and a greyed one leaves the arrow");
    ECursor Cursor = ECursor::Arrow;

    ExitRow->OnMouseEntered(MakeMoveEvent(ExitCenter));
    TEST_EXPECT(ExitRow->GetCursor(Cursor));
    TEST_EXPECT(Cursor == ECursor::Hand);

    ExitRow->SetEnabled(false);
    TEST_EXPECT(!ExitRow->GetCursor(Cursor));

    TEST_SECTION("The menu behind the rows has no opinion, so the padding around them keeps the arrow");
    TEST_EXPECT(!ExitMenu->GetCursor(Cursor));

    TEST_SECTION("A press the capture has moved off is still cancelled, which is what focus loss is there for");
    TSharedPtr<FMenu> PairMenu = CreateMenu(Font, { "First", "Second" });
    Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, PairMenu);

    const TSharedPtr<FMenuItem>& First  = PairMenu->GetItems()[0];
    const TSharedPtr<FMenuItem>& Second = PairMenu->GetItems()[1];

    First->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, First->GetContentRectangle().GetCenter()));
    Second->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Second->GetContentRectangle().GetCenter()));

    First->OnFocusLost();
    TEST_EXPECT(!First->IsPressed());
    TEST_EXPECT(Second->IsPressed());

    Stack.DismissAll();

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
    Stack.PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, Menu);
    Menu->SetHighlightedIndex(-1);

    TEST_EXPECT(Stack.HandleKeyDown(MakeKeyEvent(Keys::Down)));
    TEST_EXPECT_EQ(Menu->GetHighlightedIndex(), 0);

    TEST_SECTION("Escape closes one level at a time");
    TSharedPtr<FMenu> SubMenu = CreateMenu(Font, { "One", "Two" });
    Stack.PushMenu(Menu->GetItems()[0], FRectangle(IntVector2(60, 40), 40, 24), EMenuPlacement::RightOfTopAligned, SubMenu);
    TEST_EXPECT_EQ(Stack.GetDepth(), 2);

    TEST_EXPECT(Stack.HandleKeyDown(MakeKeyEvent(Keys::Escape)));
    TEST_EXPECT_EQ(Stack.GetDepth(), 1);

    TEST_EXPECT(Stack.HandleKeyDown(MakeKeyEvent(Keys::Escape)));
    TEST_EXPECT_EQ(Stack.GetDepth(), 0);

    TEST_SECTION("A section header is chrome, so it is neither a row nor somewhere the highlight can land");
    TSharedPtr<FMenu> Sectioned = FMenu::Create();
    Sectioned->AddSection("Open", Font);

    FMenuItem::FDesc FirstRow;
    FirstRow.SetLabel("New Level").SetFont(Font);
    Sectioned->AddItem(FMenuItem::Create(FirstRow));

    Sectioned->AddSection("Exit", Font);

    FMenuItem::FDesc SecondRow;
    SecondRow.SetLabel("Exit").SetFont(Font);
    Sectioned->AddItem(FMenuItem::Create(SecondRow));

    TEST_EXPECT_EQ(Sectioned->GetItems().Size(), 2);

    Sectioned->OnKeyDown(MakeKeyEvent(Keys::Down));
    TEST_EXPECT_EQ(Sectioned->GetHighlightedIndex(), 0);
    TEST_EXPECT_EQ(Sectioned->GetItems()[0]->GetLabel(), String("New Level"));

    Sectioned->OnKeyDown(MakeKeyEvent(Keys::Down));
    TEST_EXPECT_EQ(Sectioned->GetHighlightedIndex(), 1);
    TEST_EXPECT_EQ(Sectioned->GetItems()[1]->GetLabel(), String("Exit"));

    Sectioned->OnKeyDown(MakeKeyEvent(Keys::Down));
    TEST_EXPECT_EQ(Sectioned->GetHighlightedIndex(), 0);

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

    TEST_SECTION("The buttons sit side by side across the top, held apart by the style's item spacing");
    TEST_EXPECT_EQ(FileButton->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(EditButton->GetContentRectangle().Position.X, FileButton->GetContentRectangle().Width + FUIStyle::GetDefault().MenuBar.ItemSpacing);
    TEST_EXPECT(!MenuBar->IsAnyMenuOpen());

    TEST_SECTION("Hovering does nothing until the bar is in menu mode");
    EditButton->OnMouseEntered(MakeMoveEvent(IntVector2(50, 8)));
    TEST_EXPECT(!MenuBar->IsAnyMenuOpen());
    EditButton->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));

    TEST_SECTION("Clicking a button opens its drop-down flush against the highlight rather than the strip's edge");
    ClickElement(FileButton);
    TEST_EXPECT(FileAnchor->IsOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 1);
    TEST_EXPECT_EQ(FileAnchor->GetMenu()->ScreenBounds.Position,
        IntVector2(0, FileButton->GetContentRectangle().GetBottom()));

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

bool MenuBarHighlight_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    FUIStyle::ResetDefault();

    const FUIMenuBarStyle&      BarStyle = FUIStyle::GetDefault().MenuBar;
    const TSharedPtr<IFontFace> Font     = CreateFont();

    TSharedPtr<FWindow> Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    TSharedPtr<FMenuBar>    MenuBar    = FMenuBar::Create();
    TSharedPtr<FMenuAnchor> FileAnchor = MenuBar->AddMenu("File", Font, CreateMenu(Font, { "New" }));

    Window->SetContent(MenuBar);
    FApplication::LayoutWindow(Window);

    TSharedPtr<FMenuBarButton> FileButton = StaticCastSharedPtr<FMenuBarButton>(FileAnchor->GetContent());

    TEST_SECTION("The strip is never shorter than the style asks for, however small its labels measure");
    TEST_EXPECT_EQ(MenuBar->GetCachedDesiredSize().Y, BarStyle.Height);
    TEST_EXPECT(BarStyle.Height > Font->GetLineHeight() + BarStyle.ItemPadding.GetTotalVertical());

    TEST_SECTION("An untouched entry paints nothing behind its label");
    FDrawCommandList IdleCommands;
    DrawElement(FileButton, IdleCommands);
    TEST_EXPECT_EQ(CountCommands(IdleCommands, EDrawCommandType::Box), 0);

    TEST_SECTION("Hovering it lays a rounded pill, which is the entry itself rather than a shape inside it");
    FileButton->OnMouseEntered(MakeMoveEvent(FileButton->GetContentRectangle().GetCenter()));

    FDrawCommandList HoveredCommands;
    DrawElement(FileButton, HoveredCommands);
    TEST_EXPECT_EQ(CountCommands(HoveredCommands, EDrawCommandType::Box), 1);
    TEST_EXPECT_EQ(CountCommands(HoveredCommands, EDrawCommandType::BoxOutline), 0);

    const FRectangle&   ButtonBounds = FileButton->GetContentRectangle();
    const FDrawCommand& Pill         = HoveredCommands.GetCommands()[0];

    TEST_EXPECT(Pill.Tint == BarStyle.ItemHovered);
    TEST_EXPECT(Pill.CornerRadius == FCornerRadii(BarStyle.ItemCornerRadius));
    TEST_EXPECT_EQ(Pill.Bounds.Position.Y, ButtonBounds.Position.Y);
    TEST_EXPECT_EQ(Pill.Bounds.GetBottom(), ButtonBounds.GetBottom());
    TEST_EXPECT_EQ(ButtonBounds.Height, BarStyle.Height - (2 * BarStyle.ItemInset));

    TEST_SECTION("It covers the entry exactly, so what responds to the cursor is what is painted");
    TEST_EXPECT_EQ(Pill.Bounds.Position.X, ButtonBounds.Position.X);
    TEST_EXPECT_EQ(Pill.Bounds.Width, ButtonBounds.Width);

    TEST_SECTION("Opening its menu swaps the pill for the active fill without changing its shape");
    FileButton->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));
    FileAnchor->Open();

    FDrawCommandList OpenCommands;
    DrawElement(FileButton, OpenCommands);
    TEST_EXPECT_EQ(CountCommands(OpenCommands, EDrawCommandType::Box), 1);
    TEST_EXPECT(OpenCommands.GetCommands()[0].Tint == BarStyle.ItemActive);
    TEST_EXPECT_EQ(OpenCommands.GetCommands()[0].Bounds, Pill.Bounds);

    TEST_SECTION("The anchor holds no inset of its own now, and the menu still meets the pill with no gap under it");
    TEST_EXPECT_EQ(FileAnchor->GetAnchorInset(), FMargin(0));
    TEST_EXPECT_EQ(FileAnchor->GetMenu()->ScreenBounds.Position.Y, Pill.Bounds.GetBottom());

    FMenuStack::Get().DismissAll();

    TEST_SECTION("A bar given its own style hands it to the entries it already holds");
    FUIMenuBarStyle CustomStyle;
    CustomStyle.ItemHovered      = FFloatColor(0.90f, 0.10f, 0.10f, 1.0f);
    CustomStyle.Height           = 64;
    CustomStyle.ItemInset        = 9;
    CustomStyle.ItemCornerRadius = 2.0f;

    MenuBar->SetStyle(CustomStyle);
    FApplication::LayoutWindow(Window);

    FileButton->OnMouseEntered(MakeMoveEvent(FileButton->GetContentRectangle().GetCenter()));

    FDrawCommandList CustomCommands;
    DrawElement(FileButton, CustomCommands);

    const FDrawCommand& CustomPill = CustomCommands.GetCommands()[0];
    TEST_EXPECT(CustomPill.Tint == CustomStyle.ItemHovered);
    TEST_EXPECT(CustomPill.CornerRadius == FCornerRadii(2.0f));
    TEST_EXPECT_EQ(CustomPill.Bounds.Position.Y, FileButton->GetContentRectangle().Position.Y);

    TEST_SECTION("Its entries take the new height, so the pill follows the style rather than the strip");
    TEST_EXPECT(CustomStyle.Height - (2 * CustomStyle.ItemInset) > Font->GetLineHeight() + CustomStyle.ItemPadding.GetTotalVertical());
    TEST_EXPECT_EQ(FileButton->GetContentRectangle().Height, CustomStyle.Height - (2 * CustomStyle.ItemInset));

    TEST_SECTION("Wherever the pill is painted the hand follows it, and the strip around it keeps the arrow");
    ECursor Cursor = ECursor::Arrow;
    TEST_EXPECT(FileButton->GetCursor(Cursor));
    TEST_EXPECT(Cursor == ECursor::Hand);
    TEST_EXPECT(!MenuBar->GetCursor(Cursor));

    FileButton->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));
    TEST_EXPECT(!FileButton->GetCursor(Cursor));

    TEST_END();
}

bool MenuBarTallStrip_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    FUIStyle::ResetDefault();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FUIMenuBarStyle TallStyle;
    TallStyle.Height = 64;

    TSharedPtr<FWindow> Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    TSharedPtr<FMenuBar>    MenuBar    = FMenuBar::Create();
    TSharedPtr<FMenuAnchor> FileAnchor = MenuBar->AddMenu("File", Font, CreateMenu(Font, { "New" }));

    MenuBar->SetStyle(TallStyle);
    Window->SetContent(MenuBar);
    FApplication::LayoutWindow(Window);

    TSharedPtr<FMenuBarButton> FileButton = StaticCastSharedPtr<FMenuBarButton>(FileAnchor->GetContent());

    TEST_SECTION("A strip made taller than its entries need leaves the pill at the height the style asks for");
    TEST_EXPECT_EQ(MenuBar->GetCachedDesiredSize().Y, TallStyle.Height);
    TEST_EXPECT_EQ(FileButton->GetContentRectangle().Height, TallStyle.Height - (2 * TallStyle.ItemInset));

    TEST_SECTION("And centres it, so the same gap is left above and below");
    const FRectangle& BarBounds    = MenuBar->GetContentRectangle();
    const FRectangle& ButtonBounds = FileButton->GetContentRectangle();
    TEST_EXPECT_EQ(ButtonBounds.Position.Y - BarBounds.Position.Y, BarBounds.GetBottom() - ButtonBounds.GetBottom());

    TEST_SECTION("What it paints is the rectangle it responds to, whatever the strip does around it");
    FileButton->OnMouseEntered(MakeMoveEvent(ButtonBounds.GetCenter()));

    FDrawCommandList HoveredCommands;
    DrawElement(FileButton, HoveredCommands);
    TEST_EXPECT_EQ(CountCommands(HoveredCommands, EDrawCommandType::Box), 1);
    TEST_EXPECT_EQ(HoveredCommands.GetCommands()[0].Bounds, ButtonBounds);

    TEST_END();
}

bool MenuBarInTitleBar_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0), EWindowStyleFlags::Default | EWindowStyleFlags::CustomTitleBar);

    // macOS draws its own buttons at the leading edge and leaves the rest of the strip to the application
    FWindowTitleBarMetrics Metrics;
    Metrics.Height       = 28.0f;
    Metrics.LeadingInset = 78.0f;
    static_cast<FStubPlatformWindow&>(*Window->GetPlatformWindow()).SetTitleBarMetrics(Metrics);

    TSharedPtr<FMenuBar>    MenuBar    = FMenuBar::Create();
    TSharedPtr<FMenuAnchor> FileAnchor = MenuBar->AddMenu("File", Font, CreateMenu(Font, { "Exit" }));
    TSharedPtr<FMenuAnchor> EditAnchor = MenuBar->AddMenu("Edit", Font, CreateMenu(Font, { "Undo", "Redo" }));

    FTitleBar::FDesc Desc;
    Desc.SetTitle("DXR Engine").SetFont(Font).SetContent(MenuBar);

    TSharedPtr<FTitleBar> TitleBar = FTitleBar::Create(Desc);
    Window->SetContent(TitleBar);
    FApplication::LayoutWindow(Window);

    TSharedPtr<FMenuBarButton> FileButton = StaticCastSharedPtr<FMenuBarButton>(FileAnchor->GetContent());
    TSharedPtr<FMenuBarButton> EditButton = StaticCastSharedPtr<FMenuBarButton>(EditAnchor->GetContent());

    TEST_SECTION("The row starts after the inset the platform reserved for its own buttons");
    TEST_EXPECT_EQ(FileButton->GetContentRectangle().Position.X, 78);

    const FWindowTitleBarRegions& Published = static_cast<FStubPlatformWindow&>(*Window->GetPlatformWindow()).GetTitleBarRegions();

    TEST_SECTION("Both entries are published as clickable, or the platform drags the window instead of pressing them");
    TEST_EXPECT_EQ(Published.InteractiveRects.Size(), 2);
    TEST_EXPECT(IsPointInteractive(Published, FileButton->GetContentRectangle().GetCenter()));
    TEST_EXPECT(IsPointInteractive(Published, EditButton->GetContentRectangle().GetCenter()));

    TEST_SECTION("Every point an entry covers is clickable, not just its middle");
    const FRectangle FileBounds = FileButton->GetContentRectangle();
    TEST_EXPECT(IsPointInteractive(Published, FileBounds.Position));
    TEST_EXPECT(IsPointInteractive(Published, IntVector2(FileBounds.GetRight() - 1, FileBounds.GetBottom() - 1)));

    TEST_SECTION("The inset the platform owns is left to it, and the gap after the row drags the window");
    TEST_EXPECT(!IsPointInteractive(Published, IntVector2(40, 14)));
    TEST_EXPECT(Published.CaptionRect.Contains(IntVector2(600, 14)));
    TEST_EXPECT(!IsPointInteractive(Published, IntVector2(600, 14)));

    TEST_END();
}

bool ToolTipService_Test()
{
    TEST_BEGIN();

    FScopedStubApplication  Application;
    FScopedMenuTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(100, 50));

    TSharedPtr<FMenu>    Owner = CreateMenu(Font, { "Hover me" });
    TSharedPtr<FOverlay> Root  = FOverlay::Create();
    Root->AddSlot(Owner).SetHorizontalAlignment(EHorizontalAlignment::Left).SetVerticalAlignment(EVerticalAlignment::Top);

    Window->SetContent(Root);
    FApplication::LayoutWindow(Window);

    const FRectangle ClientArea = GetClientArea(Window);

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
    TEST_EXPECT_EQ(ToolTips.GetToolTipBounds().Position,
        IntVector2(300 + FToolTipService::CursorOffset, 200 + FToolTipService::CursorOffset));

    TEST_SECTION("One that fits is drawn in the window it describes rather than in a window of its own");
    TEST_EXPECT(ToolTips.GetToolTipWindow() == nullptr);
    TEST_EXPECT(Window->GetMenuHost() != nullptr);
    TEST_EXPECT(!Window->GetMenuHost()->IsEmpty());

    TEST_SECTION("A tip takes no input, so it cannot steal the hover keeping it up");
    FElementPath HoverPath;
    Window->FindChildrenContainingPoint(ToolTips.GetToolTipBounds().GetCenter() - Window->GetPosition(), HoverPath);
    TEST_EXPECT(!HoverPath.Contains(Window->GetMenuHost()));

    TEST_SECTION("It follows the cursor rather than being torn down and built again");
    ToolTips.NotifyCursorMoved(IntVector2(340, 260));
    TEST_EXPECT(ToolTips.IsShowing());
    TEST_EXPECT_EQ(ToolTips.GetToolTipBounds().Position,
        IntVector2(340 + FToolTipService::CursorOffset, 260 + FToolTipService::CursorOffset));

    TEST_SECTION("Leaving the element it describes takes it down");
    ToolTips.CancelToolTip(Owner);
    TEST_EXPECT(!ToolTips.IsShowing());
    TEST_EXPECT(ToolTips.GetOwner() == nullptr);
    TEST_EXPECT(Window->GetMenuHost()->IsEmpty());

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
    TEST_EXPECT_EQ(ToolTips.GetToolTipBounds().Position, IntVector2(OwnerBounds.Position.X, OwnerBounds.GetBottom() + 2));

    TEST_SECTION("A tip placed to the side sits right of the element with their top edges flush");
    ToolTips.DismissToolTip();
    ToolTips.RequestTextToolTip(Owner, "Opens the file", Font, EToolTipPlacement::RightOfAnchor, 0.0f);
    ToolTips.Tick(0.1f);
    TEST_EXPECT(ToolTips.IsShowing());

    TEST_EXPECT_EQ(ToolTips.GetToolTipBounds().Position,
        IntVector2(OwnerBounds.GetRight() + FToolTipService::AnchorGap, OwnerBounds.Position.Y));

    TEST_SECTION("With no room on that side it flips to the other, rather than sliding back over the element");
    TSharedPtr<FWindow>  EdgeWindow = Application.CreateWindow(IntVector2(400, 200), IntVector2(200, 700));
    TSharedPtr<FMenu>    EdgeOwner  = CreateMenu(Font, { "Hover me" });
    TSharedPtr<FOverlay> EdgeRoot   = FOverlay::Create();

    EdgeRoot->AddSlot(EdgeOwner).SetHorizontalAlignment(EHorizontalAlignment::Right).SetVerticalAlignment(EVerticalAlignment::Top);

    EdgeWindow->SetContent(EdgeRoot);
    FApplication::LayoutWindow(EdgeWindow);

    ToolTips.DismissToolTip();
    ToolTips.RequestTextToolTip(EdgeOwner, "Opens the file", Font, EToolTipPlacement::RightOfAnchor, 0.0f);
    ToolTips.Tick(0.1f);
    TEST_EXPECT(ToolTips.IsShowing());

    const FRectangle EdgeBounds = FMenuStack::GetScreenBounds(EdgeOwner);
    const FRectangle EdgeTip    = ToolTips.GetToolTipBounds();

    TEST_EXPECT_EQ(EdgeTip.Position,
        IntVector2(EdgeBounds.Position.X - FToolTipService::AnchorGap - EdgeTip.Width, EdgeBounds.Position.Y));

    TEST_SECTION("In the window's corner it is pulled back inside rather than hanging over the edge");
    ToolTips.DismissToolTip();
    ToolTips.NotifyCursorMoved(IntVector2(895, 645));
    ToolTips.RequestTextToolTip(Owner, "Opens the file", Font, EToolTipPlacement::FollowCursor, 0.0f);
    ToolTips.Tick(0.1f);

    const FRectangle CornerTip = ToolTips.GetToolTipBounds();
    TEST_EXPECT_EQ(CornerTip.Position,
        IntVector2(ClientArea.GetRight() - CornerTip.Width, ClientArea.GetBottom() - CornerTip.Height));

    TEST_SECTION("A tip too large for the window it describes still gets one of its own");
    ToolTips.DismissToolTip();

    TSharedPtr<FWindow>  TinyWindow = Application.CreateWindow(IntVector2(60, 40), IntVector2(400, 400));
    TSharedPtr<FMenu>    TinyOwner  = CreateMenu(Font, { "?" });
    TSharedPtr<FOverlay> TinyRoot   = FOverlay::Create();

    TinyRoot->AddSlot(TinyOwner).SetHorizontalAlignment(EHorizontalAlignment::Left).SetVerticalAlignment(EVerticalAlignment::Top);

    TinyWindow->SetContent(TinyRoot);
    FApplication::LayoutWindow(TinyWindow);

    ToolTips.RequestTextToolTip(TinyOwner, "A tip far too wide for this window", Font, EToolTipPlacement::BelowAnchor, 0.0f);
    ToolTips.Tick(0.1f);
    TEST_EXPECT(ToolTips.IsShowing());
    TEST_EXPECT(ToolTips.GetToolTipWindow() != nullptr);
    TEST_EXPECT(!ToolTips.GetToolTipWindow()->GetPlatformWindow()->GetAcceptsInput());

    ToolTips.DismissToolTip();

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
    TEST_EXPECT(GetTopMenuBounds().Width >= ComboBox->GetContentRectangle().Width);

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
