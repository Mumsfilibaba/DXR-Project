#include <Core/Containers/SharedPtr.h>
#include <Application/Application.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Button.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Input/Keys.h>
#include <Application/Menus/ComboBox.h>
#include <Application/Menus/Menu.h>
#include <Application/Menus/MenuAnchor.h>
#include <Application/Menus/MenuBar.h>
#include <Application/Menus/MenuItem.h>
#include <Application/Menus/MenuStack.h>
#include <Application/Menus/ToolTipService.h>
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

static TSharedPtr<FMenuItem> MakeItem(
    const FPlaygroundFonts&       Fonts,
    const String&                 Label,
    const String&                 ShortcutText,
    const TSharedPtr<FTextBlock>& Readout)
{
    FMenuItem::FDesc Desc;
    Desc.SetLabel(Label).SetFont(Fonts.Body);
    Desc.ShortcutText = ShortcutText;
    Desc.OnActivated  = FOnMenuItemActivated::CreateLambda([Readout, Label]()
    {
        Readout->SetText(String::Printf("chose %s", Label.Data()));
    });

    return FMenuItem::Create(Desc);
}

static TSharedPtr<FMenuItem> MakeCheckableItem(
    const FPlaygroundFonts&       Fonts,
    const String&                 Label,
    bool                          bIsChecked,
    const TSharedPtr<FTextBlock>& Readout)
{
    FMenuItem::FDesc Desc;
    Desc.SetLabel(Label).SetFont(Fonts.Body);
    Desc.bIsCheckable = true;
    Desc.CheckState   = bIsChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;

    TSharedPtr<FMenuItem> Item     = FMenuItem::Create(Desc);
    TWeakPtr<FMenuItem>   WeakItem = Item;

    Item->SetOnActivated(FOnMenuItemActivated::CreateLambda([WeakItem, Readout, Label]()
    {
        TSharedPtr<FMenuItem> LiveItem(WeakItem);
        if (!LiveItem)
        {
            return;
        }

        const bool bIsNowChecked = LiveItem->GetCheckState() != ECheckBoxState::Checked;
        LiveItem->SetCheckState(bIsNowChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

        Readout->SetText(String::Printf("%s is now %s", Label.Data(), bIsNowChecked ? "on" : "off"));
    }));

    return Item;
}

static TSharedPtr<FVisualElement> MakeMenuBarRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "nothing chosen yet");

    TSharedPtr<FMenu> FileMenu = FMenu::Create();
    FileMenu->AddItem(MakeItem(Fonts, "New Scene", "Ctrl+N", Readout));
    FileMenu->AddItem(MakeItem(Fonts, "Open Scene", "Ctrl+O", Readout));

    TSharedPtr<FMenu> RecentMenu = FMenu::Create();
    RecentMenu->AddItem(MakeItem(Fonts, "Sponza.scene", "", Readout));
    RecentMenu->AddItem(MakeItem(Fonts, "Bistro.scene", "", Readout));
    RecentMenu->AddSeparator();

    TSharedPtr<FMenu> DeeperMenu = FMenu::Create();
    DeeperMenu->AddItem(MakeItem(Fonts, "Clear the list", "", Readout));

    FMenuItem::FDesc DeeperDesc;
    DeeperDesc.SetLabel("More").SetFont(Fonts.Body);
    DeeperDesc.SubMenu = DeeperMenu;
    RecentMenu->AddItem(FMenuItem::Create(DeeperDesc));

    FMenuItem::FDesc RecentDesc;
    RecentDesc.SetLabel("Open Recent").SetFont(Fonts.Body);
    RecentDesc.SubMenu = RecentMenu;
    FileMenu->AddItem(FMenuItem::Create(RecentDesc));

    FileMenu->AddSeparator();
    FileMenu->AddItem(MakeItem(Fonts, "Save", "Ctrl+S", Readout));

    TSharedPtr<FMenuItem> Disabled = MakeItem(Fonts, "Save As", "Ctrl+Shift+S", Readout);
    Disabled->SetEnabled(false);
    FileMenu->AddItem(Disabled);

    TSharedPtr<FMenu> EditMenu = FMenu::Create();
    EditMenu->AddItem(MakeItem(Fonts, "Undo", "Ctrl+Z", Readout));
    EditMenu->AddItem(MakeItem(Fonts, "Redo", "Ctrl+Y", Readout));
    EditMenu->AddSeparator();
    EditMenu->AddItem(MakeItem(Fonts, "Delete", "Del", Readout));

    TSharedPtr<FMenu> ViewMenu = FMenu::Create();
    ViewMenu->AddItem(MakeCheckableItem(Fonts, "Grid", true, Readout));
    ViewMenu->AddItem(MakeCheckableItem(Fonts, "Wireframe", false, Readout));
    ViewMenu->AddItem(MakeCheckableItem(Fonts, "Statistics", false, Readout));

    TSharedPtr<FMenuBar> MenuBar = FMenuBar::Create();
    MenuBar->AddMenu("File", Fonts.Body, FileMenu);
    MenuBar->AddMenu("Edit", Fonts.Body, EditMenu);
    MenuBar->AddMenu("View", Fonts.Body, ViewMenu);

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = FUIStyle::GetDefault().Colors.ControlNormal;
    BorderDesc.CornerRadius    = FUIStyle::GetDefault().Metrics.CornerRadius;
    BorderDesc.Content         = MenuBar;

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FBorder::Create(BorderDesc)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

class FContextMenuArea final : public FCompoundElement
{
public:
    static TSharedPtr<FContextMenuArea> Create(const TSharedPtr<FVisualElement>& InContent, const TSharedPtr<FMenu>& InMenu)
    {
        TSharedPtr<FContextMenuArea> NewArea = MakeSharedPtr<FContextMenuArea>();
        NewArea->Menu = InMenu;
        NewArea->SetContent(InContent);
        NewArea->SetPadding(FMargin(12, 12, 12, 12));

        return NewArea;
    }

    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override
    {
        if (CursorEvent.GetKey() != Keys::MouseButtonRight || !FApplication::IsInitialized())
        {
            return FEventResponse::Unhandled();
        }

        TSharedPtr<FWindow> OwningWindow = FApplication::Get().FindWindow(AsSharedPtr());
        if (!OwningWindow)
        {
            return FEventResponse::Unhandled();
        }

        const FRectangle CursorBounds(CursorEvent.GetScreenPosition(), 0, 0);
        FMenuStack::Get().PushMenu(OwningWindow, CursorBounds, EMenuPlacement::AtCursor, Menu);

        return FEventResponse::Handled();
    }

    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override
    {
        const FUIStyle& Style = FUIStyle::GetDefault();

        OutCommandList.AddBoxOutline(LayerId, AllottedGeometry.Bounds, Style.Colors.Border, 1.0f, FCornerRadii(Style.Metrics.CornerRadius));
        return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId + 1);
    }

private:
    TSharedPtr<FMenu> Menu;
};

static TSharedPtr<FVisualElement> MakeContextMenuRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "right-click the box");

    TSharedPtr<FMenu> Menu = FMenu::Create();
    Menu->AddItem(MakeItem(Fonts, "Cut", "Ctrl+X", Readout));
    Menu->AddItem(MakeItem(Fonts, "Copy", "Ctrl+C", Readout));
    Menu->AddItem(MakeItem(Fonts, "Paste", "Ctrl+V", Readout));
    Menu->AddSeparator();

    TSharedPtr<FMenu> ConvertMenu = FMenu::Create();
    ConvertMenu->AddItem(MakeItem(Fonts, "To Static Mesh", "", Readout));
    ConvertMenu->AddItem(MakeItem(Fonts, "To Instanced Mesh", "", Readout));

    FMenuItem::FDesc ConvertDesc;
    ConvertDesc.SetLabel("Convert").SetFont(Fonts.Body);
    ConvertDesc.SubMenu = ConvertMenu;
    Menu->AddItem(FMenuItem::Create(ConvertDesc));

    FTextBlock::FDesc HintDesc;
    HintDesc.Text = "Right-click anywhere in this box";
    HintDesc.Font = Fonts.Body;

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FContextMenuArea::Create(FTextBlock::Create(HintDesc), Menu)).SetHorizontalAlignment(EHorizontalAlignment::Fill);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeComboBoxRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock>     Readout = MakeReadout(Fonts, "nothing selected yet");
    TSharedPtr<FHorizontalBox> Row     = FHorizontalBox::Create();

    struct FSample
    {
        const CHAR* Label;
        int32       InitialIndex;
    };

    const FSample Samples[] =
    {
        { "Shadow quality", 2 },
        { "Anti-aliasing", -1 },
    };

    const TArray<String> Options[] =
    {
        { "Off", "Low", "Medium", "High", "Ultra" },
        { "None", "FXAA", "TAA" },
    };

    for (int32 Index = 0; Index < static_cast<int32>(ARRAY_COUNT(Samples)); ++Index)
    {
        FTextBlock::FDesc LabelDesc;
        LabelDesc.Text = Samples[Index].Label;
        LabelDesc.Font = Fonts.Body;

        FComboBox::FDesc Desc;
        Desc.SetOptions(Options[Index]).SetFont(Fonts.Body);
        Desc.SelectedIndex      = Samples[Index].InitialIndex;
        Desc.PlaceholderText    = "Select...";
        Desc.OnSelectionChanged = FOnComboSelectionChanged::CreateLambda([Readout, Label = String(Samples[Index].Label), Choices = Options[Index]](int32 SelectedIndex)
        {
            const String& Choice = Choices.IsValidIndex(SelectedIndex) ? Choices[SelectedIndex] : String("nothing");
            Readout->SetText(String::Printf("%s is now %s", Label.Data(), Choice.Data()));
        });

        Row->AddSlot(FTextBlock::Create(LabelDesc)).SetPadding(FMargin(0, 0, 8, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
        Row->AddSlot(FComboBox::Create(Desc)).SetPadding(FMargin(0, 0, 24, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    }

    Row->AddSlot(FSpacer::Create(IntVector2(0, 0))).SetFillCoefficient(1.0f);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(Row);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeToolTipRow(const FPlaygroundFonts& Fonts)
{
    FButton::FDesc FollowDesc;
    FollowDesc.SetText("Follows the cursor").SetFont(Fonts.Body);

    FButton::FDesc AnchoredDesc;
    AnchoredDesc.SetText("Sits under the button").SetFont(Fonts.Body);

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(FToolTipHost::Create(FButton::Create(FollowDesc), "Rests half a second, then follows", Fonts.Body))
        .SetPadding(FMargin(0, 0, 12, 0))
        .SetVerticalAlignment(EVerticalAlignment::Center);

    Row->AddSlot(FToolTipHost::Create(FButton::Create(AnchoredDesc), "Anchored under the thing it describes", Fonts.Body, EToolTipPlacement::BelowAnchor))
        .SetVerticalAlignment(EVerticalAlignment::Center);

    Row->AddSlot(FSpacer::Create(IntVector2(0, 0))).SetFillCoefficient(1.0f);
    return Row;
}

FPlaygroundScene CreateMenusScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Menu bar";
        Desc.Description = "Click a title to open it, then hover the siblings to switch. Arrows navigate, Escape closes a level.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeMenuBarRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Context menu";
        Desc.Description = "The same menu opened at the cursor instead of under an anchor, flipping when it meets a screen edge.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeContextMenuRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Combo boxes";
        Desc.Description = "A button that drops its options as a menu, ticking the one in force and matching the button's width.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeComboBoxRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Tool tips";
        Desc.Description = "Rest on a button and its tip appears, following the cursor or anchored under it.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeToolTipRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Menus", MakeSceneColumn("Menus", Fonts, Panels));
}
