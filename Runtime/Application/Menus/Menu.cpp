#include "Application/Menus/Menu.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Input/Keys.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

TSharedPtr<FMenu> FMenu::Create()
{
    TSharedPtr<FMenu> NewMenu = MakeSharedPtr<FMenu>();
    NewMenu->Initialize();
    return NewMenu;
}

FMenu::FMenu()
    : FCompoundElement()
    , Panel(FVerticalBox::Create())
    , Items()
    , Separators()
    , Sections()
    , Style(FUIStyle::GetDefault().Menu)
    , HighlightedIndex(-1)
    , MinDesiredWidth(-1)
{
}

FMenu::~FMenu() = default;

void FMenu::Initialize()
{
    SetContent(Panel);
    SetPadding(FMargin(0, 10, 0, 10));
}

IntVector2 FMenu::ComputeDesiredSize() const
{
    IntVector2  DesiredSize = FCompoundElement::ComputeDesiredSize();
    const int32 MinWidth    = (MinDesiredWidth >= 0) ? MinDesiredWidth : Style.MinWidth;

    DesiredSize.X = Math::Max(DesiredSize.X, MinWidth);
    return DesiredSize;
}

int32 FMenu::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FRectangle   Bounds = AllottedGeometry.Bounds;
    const FCornerRadii Radii(Style.CornerRadius);

    OutCommandList.AddBox(LayerId, Bounds, Style.Background, Radii);
    OutCommandList.AddBoxOutline(LayerId, Bounds, Style.Border, 1.0f, Radii);

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId + 1);
}

FEventResponse FMenu::OnKeyDown(const FKeyEvent& KeyEvent)
{
    const FKey Key = KeyEvent.GetKey();

    if (Key == Keys::Down)
    {
        MoveHighlight(1);
        return FEventResponse::Handled();
    }

    if (Key == Keys::Up)
    {
        MoveHighlight(-1);
        return FEventResponse::Handled();
    }

    if (Key == Keys::Home)
    {
        SetHighlightedIndex(-1);
        MoveHighlight(1);
        return FEventResponse::Handled();
    }

    if (Key == Keys::End)
    {
        SetHighlightedIndex(Items.Size());
        MoveHighlight(-1);
        return FEventResponse::Handled();
    }

    if (Key == Keys::Enter || Key == Keys::KeypadEnter || Key == Keys::Space)
    {
        ActivateHighlighted();
        return FEventResponse::Handled();
    }

    if (Key == Keys::Right)
    {
        if (Items.IsValidIndex(HighlightedIndex) && Items[HighlightedIndex]->HasSubMenu())
        {
            Items[HighlightedIndex]->OpenSubMenu();
            return FEventResponse::Handled();
        }

        return FEventResponse::Unhandled();
    }

    if (Key == Keys::Left)
    {
        FMenuStack& Stack = FMenuStack::Get();
        if (Stack.GetDepth() > 1)
        {
            Stack.DismissTop();
            return FEventResponse::Handled();
        }

        return FEventResponse::Unhandled();
    }

    return FCompoundElement::OnKeyDown(KeyEvent);
}

void FMenu::AddItem(const TSharedPtr<FMenuItem>& Item)
{
    if (!Item)
    {
        return;
    }

    Item->SetStyle(Style);

    Items.Add(Item);
    Panel->AddSlot(Item).SetHorizontalAlignment(EHorizontalAlignment::Fill);
}

void FMenu::AddSeparator()
{
    TSharedPtr<FMenuSeparator> Separator = FMenuSeparator::Create();
    Separator->SetStyle(Style);

    Separators.Add(Separator);
    Panel->AddSlot(Separator).SetHorizontalAlignment(EHorizontalAlignment::Fill);
}

void FMenu::AddSection(const String& Label, const TSharedPtr<IFontFace>& Font)
{
    TSharedPtr<FMenuSectionHeader> Header = FMenuSectionHeader::Create(Label, Font);
    Header->SetStyle(Style);

    Sections.Add(Header);
    Panel->AddSlot(Header).SetHorizontalAlignment(EHorizontalAlignment::Fill);
}

void FMenu::AddCustomEntry(const TSharedPtr<FVisualElement>& Element)
{
    if (Element)
    {
        Panel->AddSlot(Element).SetHorizontalAlignment(EHorizontalAlignment::Fill);
    }
}

void FMenu::ClearEntries()
{
    Panel->ClearSlots();
    Items.Clear();
    Separators.Clear();
    Sections.Clear();
    HighlightedIndex = -1;
}

void FMenu::MoveHighlight(int32 Delta)
{
    if (Items.IsEmpty() || Delta == 0)
    {
        return;
    }

    const int32 Step  = (Delta > 0) ? 1 : -1;
    const int32 Count = Items.Size();

    int32 Index = HighlightedIndex;
    for (int32 Attempt = 0; Attempt < Count; ++Attempt)
    {
        Index = Index + Step;
        if (Index < 0)
        {
            Index = Count - 1;
        }
        else if (Index >= Count)
        {
            Index = 0;
        }

        if (Items[Index]->IsEnabled())
        {
            SetHighlightedIndex(Index);
            return;
        }
    }
}

void FMenu::SetHighlightedIndex(int32 Index)
{
    for (int32 ItemIndex = 0; ItemIndex < Items.Size(); ++ItemIndex)
    {
        Items[ItemIndex]->SetHighlighted(ItemIndex == Index);
    }

    HighlightedIndex = Items.IsValidIndex(Index) ? Index : -1;
}

void FMenu::ActivateHighlighted()
{
    if (Items.IsValidIndex(HighlightedIndex))
    {
        Items[HighlightedIndex]->Activate();
    }
}

void FMenu::SetMinDesiredWidth(int32 InMinDesiredWidth)
{
    MinDesiredWidth = Math::Max(InMinDesiredWidth, 0);
}

void FMenu::SetStyle(const FUIMenuStyle& InStyle)
{
    Style = InStyle;

    for (const TSharedPtr<FMenuItem>& Item : Items)
    {
        Item->SetStyle(Style);
    }

    for (const TSharedPtr<FMenuSeparator>& Separator : Separators)
    {
        Separator->SetStyle(Style);
    }

    for (const TSharedPtr<FMenuSectionHeader>& Section : Sections)
    {
        Section->SetStyle(Style);
    }
}

void FMenu::SetOuterCornerRadius(float InCornerRadius)
{
    Style.CornerRadius = InCornerRadius;
}
