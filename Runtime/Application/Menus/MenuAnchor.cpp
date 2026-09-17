#include "Application/Menus/MenuAnchor.h"
#include "Application/Application.h"

TSharedPtr<FMenuAnchor> FMenuAnchor::Create(const FDesc& Desc)
{
    TSharedPtr<FMenuAnchor> NewAnchor = MakeSharedPtr<FMenuAnchor>();
    NewAnchor->Initialize(Desc);
    return NewAnchor;
}

FMenuAnchor::FMenuAnchor()
    : FCompoundElement()
    , MenuContent(nullptr)
    , Placement(EMenuPlacement::BelowLeftAligned)
    , AnchorInset()
    , OnGetMenuContentDelegate()
    , OnOpenChangedDelegate()
    , Menu(nullptr)
{
}

FMenuAnchor::~FMenuAnchor() = default;

void FMenuAnchor::Initialize(const FDesc& Desc)
{
    MenuContent              = Desc.MenuContent;
    Placement                = Desc.Placement;
    AnchorInset              = Desc.AnchorInset;
    OnGetMenuContentDelegate = Desc.OnGetMenuContent;
    OnOpenChangedDelegate    = Desc.OnOpenChanged;

    SetContent(Desc.Content);
}

void FMenuAnchor::Open()
{
    if (IsOpen() || !FApplication::IsInitialized())
    {
        return;
    }

    TSharedPtr<FVisualElement> Content = OnGetMenuContentDelegate.IsBound() ? OnGetMenuContentDelegate.Execute() : MenuContent;
    if (!Content)
    {
        return;
    }

    const FRectangle AnchorBounds = FMenuStack::GetScreenBounds(AsSharedPtr()).Deflate(AnchorInset);

    Menu = FMenuStack::Get().PushMenu(AsSharedPtr(), AnchorBounds, Placement, Content);
    if (Menu)
    {
        OnOpenChangedDelegate.ExecuteIfBound(true);
    }
}

void FMenuAnchor::Close()
{
    if (!IsOpen())
    {
        return;
    }

    FMenuStack& Stack = FMenuStack::Get();
    Stack.DismissToDepth(Stack.GetMenuDepth(Menu) - 1);

    Menu = nullptr;
    OnOpenChangedDelegate.ExecuteIfBound(false);
}

void FMenuAnchor::Toggle()
{
    if (IsOpen())
    {
        Close();
    }
    else
    {
        Open();
    }
}

bool FMenuAnchor::IsOpen() const
{
    SyncOpenState();
    return Menu != nullptr;
}

void FMenuAnchor::SetPlacement(EMenuPlacement InPlacement)
{
    Placement = InPlacement;
}

void FMenuAnchor::SetMenuContent(const TSharedPtr<FVisualElement>& InMenuContent)
{
    MenuContent = InMenuContent;
}

void FMenuAnchor::SetAnchorInset(const FMargin& InAnchorInset)
{
    AnchorInset = InAnchorInset;
}

FMenuHandle FMenuAnchor::GetMenu() const
{
    SyncOpenState();
    return Menu;
}

void FMenuAnchor::SyncOpenState() const
{
    if (Menu && !FMenuStack::Get().IsMenuOpen(Menu))
    {
        Menu = nullptr;
        OnOpenChangedDelegate.ExecuteIfBound(false);
    }
}
