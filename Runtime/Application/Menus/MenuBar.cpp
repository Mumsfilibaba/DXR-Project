#include "Application/Menus/MenuBar.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

TSharedPtr<FMenuBarButton> FMenuBarButton::Create(const String& InLabel, const TSharedPtr<IFontFace>& InFont)
{
    TSharedPtr<FMenuBarButton> NewButton = MakeSharedPtr<FMenuBarButton>();
    NewButton->Label = InLabel;
    NewButton->Font  = InFont;
    NewButton->SetPadding(FMargin(8, 3, 8, 3));
    return NewButton;
}

FMenuBarButton::FMenuBarButton()
    : FInteractiveElement()
    , Label()
    , Font(nullptr)
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

    return DesiredSize;
}

int32 FMenuBarButton::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&         Style   = FUIStyle::GetDefault();
    const EInteractionState State   = GetInteractionState();
    const bool              bIsOpen = Anchor && Anchor->IsOpen();

    if (bIsOpen)
    {
        OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.Accent, FCornerRadii(Style.Metrics.CornerRadius));
    }
    else if (State == EInteractionState::Hovered || State == EInteractionState::Pressed)
    {
        OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.ControlHovered, FCornerRadii(Style.Metrics.CornerRadius));
    }

    if (Font && !Label.IsEmpty())
    {
        OutCommandList.AddText(LayerId, AllottedGeometry.Bounds.Deflate(GetPadding()), Label, Font.Get(), Style.GetTextColor(State));
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
{
}

FMenuBar::~FMenuBar() = default;

void FMenuBar::Initialize()
{
    SetContent(Panel);
}

TSharedPtr<FMenuAnchor> FMenuBar::AddMenu(const String& Label, const TSharedPtr<IFontFace>& Font, const TSharedPtr<FVisualElement>& MenuContent)
{
    TSharedPtr<FMenuBarButton> Button = FMenuBarButton::Create(Label, Font);

    FMenuAnchor::FDesc Desc;
    Desc.Content     = Button;
    Desc.MenuContent = MenuContent;
    Desc.Placement   = EMenuPlacement::BelowLeftAligned;

    TSharedPtr<FMenuAnchor> Anchor = FMenuAnchor::Create(Desc);
    Button->SetOwner(this, Anchor.Get());

    Anchors.Add(Anchor);
    Panel->AddSlot(Anchor).SetVerticalAlignment(EVerticalAlignment::Fill);

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
