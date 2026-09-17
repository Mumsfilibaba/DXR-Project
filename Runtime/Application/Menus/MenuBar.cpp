#include "Application/Menus/MenuBar.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

TSharedPtr<FMenuBarButton> FMenuBarButton::Create(const String& InLabel, const TSharedPtr<IFontFace>& InFont)
{
    TSharedPtr<FMenuBarButton> NewButton = MakeSharedPtr<FMenuBarButton>();
    NewButton->Label = InLabel;
    NewButton->Font  = InFont;
    NewButton->SetPadding(NewButton->Style.ItemPadding);
    return NewButton;
}

FMenuBarButton::FMenuBarButton()
    : FInteractiveElement()
    , Label()
    , Font(nullptr)
    , Style(FUIStyle::GetDefault().MenuBar)
    , OwnerBar(nullptr)
    , Anchor(nullptr)
{
}

FMenuBarButton::~FMenuBarButton() = default;

IntVector2 FMenuBarButton::ComputeDesiredSize() const
{
    const FMargin& Inset = GetPadding();

    IntVector2 DesiredSize(Inset.GetTotalHorizontal(), Inset.GetTotalVertical());
    if (Font)
    {
        DesiredSize.X += Font->MeasureWidth(StringView(Label.Data(), Label.Length()));
        DesiredSize.Y += Font->GetLineHeight();
    }

    DesiredSize.Y = Math::Max(DesiredSize.Y, Style.Height - (2 * Style.ItemInset));
    return DesiredSize;
}

int32 FMenuBarButton::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const EInteractionState State   = GetInteractionState();
    const bool              bIsOpen = Anchor && Anchor->IsOpen();

    const FRectangle&  Highlight = AllottedGeometry.Bounds;
    const FCornerRadii Radii(Style.ItemCornerRadius);

    if (bIsOpen || State == EInteractionState::Pressed)
    {
        OutCommandList.AddBox(LayerId, Highlight, Style.ItemActive, Radii);
    }
    else if (State == EInteractionState::Hovered)
    {
        OutCommandList.AddBox(LayerId, Highlight, Style.ItemHovered, Radii);
    }

    if (Font && !Label.IsEmpty())
    {
        const FFloatColor& TextColor = FUIStyle::GetDefault().GetTextColor(State);
        OutCommandList.AddText(LayerId, AllottedGeometry.Bounds.Deflate(GetPadding()), Label, Font.Get(), TextColor);
    }

    return LayerId;
}

FEventResponse FMenuBarButton::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    const FEventResponse Response = FInteractiveElement::OnMouseEntered(CursorEvent);

    if (IsEnabled() && OwnerBar)
    {
        OwnerBar->OnButtonHovered(Anchor);
    }

    return Response;
}

void FMenuBarButton::SetOwner(FMenuBar* InOwnerBar, FMenuAnchor* InAnchor)
{
    OwnerBar = InOwnerBar;
    Anchor   = InAnchor;
}

void FMenuBarButton::SetStyle(const FUIMenuBarStyle& InStyle)
{
    Style = InStyle;
    SetPadding(Style.ItemPadding);
}

void FMenuBarButton::OnClicked()
{
    if (Anchor)
    {
        Anchor->Toggle();
    }
}

TSharedPtr<FMenuBar> FMenuBar::Create()
{
    TSharedPtr<FMenuBar> NewMenuBar = MakeSharedPtr<FMenuBar>();
    NewMenuBar->Initialize();
    return NewMenuBar;
}

FMenuBar::FMenuBar()
    : FCompoundElement()
    , Panel(FHorizontalBox::Create())
    , Anchors()
    , Buttons()
    , Style(FUIStyle::GetDefault().MenuBar)
{
}

FMenuBar::~FMenuBar() = default;

void FMenuBar::Initialize()
{
    SetContent(Panel);
}

IntVector2 FMenuBar::ComputeDesiredSize() const
{
    IntVector2 DesiredSize = FCompoundElement::ComputeDesiredSize();
    DesiredSize.Y          = Math::Max(DesiredSize.Y, Style.Height);
    return DesiredSize;
}

TSharedPtr<FMenuAnchor> FMenuBar::AddMenu(const String& Label, const TSharedPtr<IFontFace>& Font, const TSharedPtr<FVisualElement>& MenuContent)
{
    TSharedPtr<FMenuBarButton> Button = FMenuBarButton::Create(Label, Font);
    Button->SetStyle(Style);

    FMenuAnchor::FDesc Desc;
    Desc.Content     = Button;
    Desc.MenuContent = MenuContent;
    Desc.Placement   = EMenuPlacement::BelowLeftAligned;

    TSharedPtr<FMenuAnchor> Anchor = FMenuAnchor::Create(Desc);
    Button->SetOwner(this, Anchor.Get());

    Anchors.Add(Anchor);
    Buttons.Add(Button);
    const int32 LeadingGap = Panel->GetNumSlots() > 0 ? Style.ItemSpacing : 0;
    Panel->AddSlot(Anchor).SetVerticalAlignment(EVerticalAlignment::Center).SetPadding(FMargin(LeadingGap, 0, 0, 0));

    return Anchor;
}

void FMenuBar::CloseActiveMenu()
{
    for (const TSharedPtr<FMenuAnchor>& Anchor : Anchors)
    {
        if (Anchor->IsOpen())
        {
            Anchor->Close();
            return;
        }
    }
}

bool FMenuBar::IsAnyMenuOpen() const
{
    for (const TSharedPtr<FMenuAnchor>& Anchor : Anchors)
    {
        if (Anchor->IsOpen())
        {
            return true;
        }
    }

    return false;
}

void FMenuBar::OnButtonHovered(FMenuAnchor* HoveredAnchor)
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

void FMenuBar::SetStyle(const FUIMenuBarStyle& InStyle)
{
    Style = InStyle;

    for (const TSharedPtr<FMenuBarButton>& Button : Buttons)
    {
        Button->SetStyle(Style);
    }

    for (int32 Index = 0; Index < Panel->GetNumSlots(); ++Index)
    {
        Panel->GetSlot(Index).SetPadding(FMargin(Index > 0 ? Style.ItemSpacing : 0, 0, 0, 0));
    }
}
