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
    , OnGetMenuContentDelegate()
    , OnOpenChangedDelegate()
    , MenuWindow(nullptr)
{
}

FMenuAnchor::~FMenuAnchor() = default;

void FMenuAnchor::Initialize(const FDesc& Desc)
{
    MenuContent              = Desc.MenuContent;
    Placement                = Desc.Placement;
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

    TSharedPtr<FWindow> OwningWindow = FApplication::Get().FindWindow(AsSharedPtr());
    if (!OwningWindow)
    {
        return;
    }

    MenuWindow = FMenuStack::Get().PushMenu(OwningWindow, FMenuStack::GetScreenBounds(AsSharedPtr()), Placement, Content);
    if (MenuWindow)
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
    Stack.DismissToDepth(Stack.GetMenuDepth(MenuWindow) - 1);

    MenuWindow = nullptr;
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
    return MenuWindow != nullptr;
}

void FMenuAnchor::SetPlacement(EMenuPlacement InPlacement)
{
    Placement = InPlacement;
}

void FMenuAnchor::SetMenuContent(const TSharedPtr<FVisualElement>& InMenuContent)
{
    MenuContent = InMenuContent;
}

TSharedPtr<FWindow> FMenuAnchor::GetMenuWindow() const
{
    SyncOpenState();
    return MenuWindow;
}

void FMenuAnchor::SyncOpenState() const
{
    if (MenuWindow && !FMenuStack::Get().IsMenuOpen(MenuWindow))
    {
        MenuWindow = nullptr;
        OnOpenChangedDelegate.ExecuteIfBound(false);
    }
}
