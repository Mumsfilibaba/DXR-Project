#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Separator.h"
#include "Application/Elements/ToolBar.h"
#include "Application/Menus/ToolTipService.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

// The space either side of an entry's contents, and the gap between its icon and its label
constexpr int32 TOOLBAR_ITEM_PADDING = 6;
constexpr int32 TOOLBAR_ICON_SPACING = 4;

// The room a dropdown arrow takes after the label, and half the width of the arrow itself
constexpr int32 TOOLBAR_ARROW_WIDTH  = 12;
constexpr float TOOLBAR_ARROW_EXTENT = 3.5f;

// How far short of the strip's full height a rule between groups stops
constexpr int32 TOOLBAR_RULE_INSET = 3;

TSharedPtr<FToolBarButton> FToolBarButton::Create(const FToolBarItemDesc& Item, EToolBarItemType InType, const TSharedPtr<IFontFace>& InFont, int32 InIconSize)
{
    TSharedPtr<FToolBarButton> NewButton = MakeSharedPtr<FToolBarButton>();
    NewButton->Icon        = Item.Icon;
    NewButton->Label       = Item.Label;
    NewButton->ToolTipText = Item.ToolTipText;
    NewButton->Font        = InFont;
    NewButton->ItemType    = InType;
    NewButton->IconSize    = InIconSize;
    NewButton->SetPadding(FMargin(TOOLBAR_ITEM_PADDING, 2, TOOLBAR_ITEM_PADDING, 2));
    return NewButton;
}

FToolBarButton::FToolBarButton()
    : FInteractiveElement()
    , Icon()
    , Label()
    , ToolTipText()
    , Font(nullptr)
    , ItemType(EToolBarItemType::Button)
    , CheckState(ECheckBoxState::Unchecked)
    , IconSize(0)
    , OnClickedDelegate()
    , OnStateChangedDelegate()
    , OwnerBar(nullptr)
    , Anchor(nullptr)
{
}

FToolBarButton::~FToolBarButton() = default;

IntVector2 FToolBarButton::ComputeDesiredSize() const
{
    const FMargin& Inset = GetPadding();

    IntVector2 ContentSize(0, 0);
    if (Icon.IsValid())
    {
        ContentSize.X += IconSize;
        ContentSize.Y = Math::Max(ContentSize.Y, IconSize);
    }

    if (Font && !Label.IsEmpty())
    {
        ContentSize.X += (Icon.IsValid() ? TOOLBAR_ICON_SPACING : 0) + Font->MeasureWidth(StringView(Label.Data(), Label.Length()));
        ContentSize.Y = Math::Max(ContentSize.Y, Font->GetLineHeight());
    }

    if (ItemType == EToolBarItemType::DropDown)
    {
        ContentSize.X += TOOLBAR_ARROW_WIDTH;
    }

    IntVector2 DesiredSize(ContentSize.X + Inset.GetTotalHorizontal(), ContentSize.Y + Inset.GetTotalVertical());
    DesiredSize.Y = Math::Max(DesiredSize.Y, FUIStyle::GetDefault().Metrics.RowHeight);
    return DesiredSize;
}

int32 FToolBarButton::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&         Style  = FUIStyle::GetDefault();
    const EInteractionState State  = GetInteractionState();
    const FRectangle&       Bounds = AllottedGeometry.Bounds;

    if (IsHighlighted())
    {
        OutCommandList.AddBox(LayerId, Bounds, Style.Colors.Accent, FCornerRadii(Style.Metrics.CornerRadius));
    }
    else if (State == EInteractionState::Hovered || State == EInteractionState::Pressed)
    {
        OutCommandList.AddBox(LayerId, Bounds, Style.GetControlColor(State), FCornerRadii(Style.Metrics.CornerRadius));
    }

    const FFloatColor TextColor  = Style.GetTextColor(State);
    const FRectangle  Inner      = Bounds.Deflate(GetPadding());
    const FRectangle  IconBounds = GetIconBounds(Bounds);
    const int32       ArrowRoom  = ItemType == EToolBarItemType::DropDown ? TOOLBAR_ARROW_WIDTH : 0;

    if (Icon.IsValid())
    {
        OutCommandList.AddImage(LayerId, IconBounds, Icon, TextColor);
    }

    if (Font && !Label.IsEmpty())
    {
        const int32      LabelLeft   = Icon.IsValid() ? IconBounds.GetRight() + TOOLBAR_ICON_SPACING : Inner.Position.X;
        const FRectangle LabelBounds = FRectangle(IntVector2(LabelLeft, Inner.Position.Y), Math::Max(Inner.GetRight() - ArrowRoom - LabelLeft, 0), Inner.Height);

        OutCommandList.AddText(LayerId, LabelBounds, Label, Font.Get(), TextColor);
    }

    if (ItemType == EToolBarItemType::DropDown)
    {
        DrawArrow(Inner, OutCommandList, LayerId, TextColor);
    }

    return LayerId;
}

FEventResponse FToolBarButton::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    const FEventResponse Response = FInteractiveElement::OnMouseEntered(CursorEvent);
    if (!IsEnabled())
    {
        return Response;
    }

    if (!ToolTipText.IsEmpty())
    {
        FToolTipService& ToolTips = FToolTipService::Get();
        ToolTips.NotifyCursorMoved(CursorEvent.GetScreenPosition());
        ToolTips.RequestTextToolTip(AsSharedPtr(), ToolTipText, Font, EToolTipPlacement::BelowAnchor);
    }

    if (OwnerBar)
    {
        OwnerBar->OnButtonHovered(Anchor);
    }

    return Response;
}

FEventResponse FToolBarButton::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    if (!ToolTipText.IsEmpty())
    {
        FToolTipService::Get().CancelToolTip(AsSharedPtr());
    }

    return FInteractiveElement::OnMouseLeft(CursorEvent);
}

void FToolBarButton::SetOwner(FToolBar* InOwnerBar, FMenuAnchor* InAnchor)
{
    OwnerBar = InOwnerBar;
    Anchor   = InAnchor;
}

void FToolBarButton::SetCheckState(ECheckBoxState InState)
{
    CheckState = InState;
}

void FToolBarButton::SetLabel(const String& InLabel)
{
    Label = InLabel;
}

void FToolBarButton::SetOnClicked(const FOnClicked& InOnClicked)
{
    OnClickedDelegate = InOnClicked;
}

void FToolBarButton::SetOnStateChanged(const FOnCheckStateChanged& InOnStateChanged)
{
    OnStateChangedDelegate = InOnStateChanged;
}

bool FToolBarButton::IsHighlighted() const
{
    if (ItemType == EToolBarItemType::DropDown)
    {
        return Anchor && Anchor->IsOpen();
    }

    return ItemType == EToolBarItemType::Toggle && CheckState != ECheckBoxState::Unchecked;
}

void FToolBarButton::OnClicked()
{
    switch (ItemType)
    {
        case EToolBarItemType::Toggle:
        {
            CheckState = IsChecked() ? ECheckBoxState::Unchecked : ECheckBoxState::Checked;
            OnStateChangedDelegate.ExecuteIfBound(CheckState);
            break;
        }

        case EToolBarItemType::DropDown:
        {
            if (Anchor)
            {
                Anchor->Toggle();
            }

            break;
        }

        default:
        {
            break;
        }
    }

    OnClickedDelegate.ExecuteIfBound();
}

FRectangle FToolBarButton::GetIconBounds(const FRectangle& Bounds) const
{
    if (!Icon.IsValid())
    {
        return FRectangle();
    }

    const FRectangle Inner = Bounds.Deflate(GetPadding());
    return FRectangle(IntVector2(Inner.Position.X, Inner.Position.Y + ((Inner.Height - IconSize) / 2)), IconSize, IconSize);
}

void FToolBarButton::DrawArrow(const FRectangle& Bounds, FDrawCommandList& OutCommandList, int32 LayerId, const FFloatColor& Tint) const
{
    const float CenterX = static_cast<float>(Bounds.GetRight()) - (TOOLBAR_ARROW_WIDTH * 0.5f);
    const float CenterY = static_cast<float>(Bounds.Position.Y) + (Bounds.Height * 0.5f);

    const Vector2 Left (CenterX - TOOLBAR_ARROW_EXTENT, CenterY - (TOOLBAR_ARROW_EXTENT * 0.5f));
    const Vector2 Right(CenterX + TOOLBAR_ARROW_EXTENT, CenterY - (TOOLBAR_ARROW_EXTENT * 0.5f));
    const Vector2 Tip  (CenterX, CenterY + (TOOLBAR_ARROW_EXTENT * 0.5f));

    OutCommandList.AddTriangle(LayerId, Left, Right, Tip, Tint);
}

TSharedPtr<FToolBar> FToolBar::Create(const FDesc& Desc)
{
    TSharedPtr<FToolBar> NewToolBar = MakeSharedPtr<FToolBar>();
    NewToolBar->Initialize(Desc);
    return NewToolBar;
}

FToolBar::FToolBar()
    : FCompoundElement()
    , Panel(nullptr)
    , Items()
    , Anchors()
    , Font(nullptr)
    , Orientation(EOrientation::Horizontal)
    , IconSize(0)
    , ItemSpacing(0)
    , bHasBackground(true)
{
}

FToolBar::~FToolBar() = default;

void FToolBar::Initialize(const FDesc& Desc)
{
    Font           = Desc.Font;
    Orientation    = Desc.Orientation;
    IconSize       = Desc.IconSize;
    ItemSpacing    = Desc.ItemSpacing;
    bHasBackground = Desc.bHasBackground;

    if (Orientation == EOrientation::Horizontal)
    {
        Panel = FHorizontalBox::Create();
    }
    else
    {
        Panel = FVerticalBox::Create();
    }

    SetPadding(Desc.Padding);
    SetContent(Panel);
}

int32 FToolBar::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (bHasBackground)
    {
        OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, FUIStyle::GetDefault().Colors.PanelBackground);
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
}

TSharedPtr<FToolBarButton> FToolBar::AddButton(const FToolBarItemDesc& Item, const FOnClicked& OnClicked)
{
    TSharedPtr<FToolBarButton> Button = FToolBarButton::Create(Item, EToolBarItemType::Button, Font, IconSize);
    Button->SetOnClicked(OnClicked);

    AppendSlot(Button, Button, EToolBarItemType::Button);
    return Button;
}

TSharedPtr<FToolBarButton> FToolBar::AddToggle(const FToolBarItemDesc& Item, ECheckBoxState InitialState, const FOnCheckStateChanged& OnStateChanged)
{
    TSharedPtr<FToolBarButton> Button = FToolBarButton::Create(Item, EToolBarItemType::Toggle, Font, IconSize);
    Button->SetCheckState(InitialState);
    Button->SetOnStateChanged(OnStateChanged);

    AppendSlot(Button, Button, EToolBarItemType::Toggle);
    return Button;
}

TSharedPtr<FMenuAnchor> FToolBar::AddDropDown(const FToolBarItemDesc& Item, const TSharedPtr<FVisualElement>& MenuContent)
{
    TSharedPtr<FToolBarButton> Button = FToolBarButton::Create(Item, EToolBarItemType::DropDown, Font, IconSize);

    FMenuAnchor::FDesc Desc;
    Desc.Content     = Button;
    Desc.MenuContent = MenuContent;
    Desc.Placement   = Orientation == EOrientation::Horizontal ? EMenuPlacement::BelowLeftAligned : EMenuPlacement::RightOfTopAligned;

    TSharedPtr<FMenuAnchor> NewAnchor = FMenuAnchor::Create(Desc);
    Button->SetOwner(this, NewAnchor.Get());

    Anchors.Add(NewAnchor);
    AppendSlot(NewAnchor, Button, EToolBarItemType::DropDown);

    return NewAnchor;
}

void FToolBar::AddSeparator()
{
    TSharedPtr<FSeparator> Rule = Orientation == EOrientation::Horizontal ? FSeparator::CreateVertical() : FSeparator::CreateHorizontal();
    AppendSlot(Rule, nullptr, EToolBarItemType::Separator);
}

void FToolBar::AddWidget(const TSharedPtr<FVisualElement>& Widget)
{
    if (Widget)
    {
        AppendSlot(Widget, nullptr, EToolBarItemType::Custom);
    }
}

void FToolBar::ClearItems()
{
    CloseActiveMenu();

    Panel->ClearSlots();
    Items.Clear();
    Anchors.Clear();
}

void FToolBar::CloseActiveMenu()
{
    for (const TSharedPtr<FMenuAnchor>& OpenAnchor : Anchors)
    {
        if (OpenAnchor->IsOpen())
        {
            OpenAnchor->Close();
            return;
        }
    }
}

bool FToolBar::IsAnyMenuOpen() const
{
    for (const TSharedPtr<FMenuAnchor>& OpenAnchor : Anchors)
    {
        if (OpenAnchor->IsOpen())
        {
            return true;
        }
    }

    return false;
}

void FToolBar::OnButtonHovered(FMenuAnchor* HoveredAnchor)
{
    if (!HoveredAnchor || HoveredAnchor->IsOpen())
    {
        return;
    }

    if (!IsAnyMenuOpen())
    {
        return;
    }

    CloseActiveMenu();
    HoveredAnchor->Open();
}

TSharedPtr<FToolBarButton> FToolBar::GetButton(int32 Index) const
{
    if (Index < 0 || Index >= Items.Size())
    {
        return nullptr;
    }

    return Items[Index].Button;
}

TSharedPtr<FToolBarButton> FToolBar::FindButton(const String& InLabel) const
{
    for (const FToolBarEntry& Item : Items)
    {
        if (Item.Button && Item.Button->GetLabel() == InLabel)
        {
            return Item.Button;
        }
    }

    return nullptr;
}

void FToolBar::AppendSlot(const TSharedPtr<FVisualElement>& Element, const TSharedPtr<FToolBarButton>& Button, EToolBarItemType Type)
{
    const bool  bIsSeparator = Type == EToolBarItemType::Separator;
    const int32 LeadingGap   = Items.IsEmpty() ? 0 : ItemSpacing;

    FBoxSlot& Slot = Panel->AddSlot(Element);
    if (Orientation == EOrientation::Horizontal)
    {
        Slot.SetVerticalAlignment(bIsSeparator ? EVerticalAlignment::Fill : EVerticalAlignment::Center);
        Slot.SetPadding(FMargin(LeadingGap, bIsSeparator ? TOOLBAR_RULE_INSET : 0, 0, bIsSeparator ? TOOLBAR_RULE_INSET : 0));
    }
    else
    {
        Slot.SetHorizontalAlignment(EHorizontalAlignment::Fill);
        Slot.SetPadding(FMargin(bIsSeparator ? TOOLBAR_RULE_INSET : 0, LeadingGap, bIsSeparator ? TOOLBAR_RULE_INSET : 0, 0));
    }

    FToolBarEntry Entry;
    Entry.Element = Element;
    Entry.Button  = Button;
    Entry.Type    = Type;

    Items.Add(Entry);
}
