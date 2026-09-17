#include "Application/Menus/MenuStack.h"
#include "Application/Menus/PopupWindow.h"
#include "Application/Application.h"
#include "Application/ElementPath.h"
#include "Application/Elements/MenuHost.h"
#include "Application/Input/Keys.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Math/Math.h"

TUniquePtr<FMenuStack> FMenuStack::MenuStack = nullptr;

FMenuStack& FMenuStack::Get()
{
    if (!MenuStack)
    {
        MenuStack = MakeUniquePtr<FMenuStack>();
    }

    return *MenuStack;
}

void FMenuStack::Shutdown()
{
    if (MenuStack)
    {
        MenuStack->DismissAll();
        MenuStack.Reset();
    }
}

FRectangle FMenuStack::GetScreenBounds(const TSharedPtr<FVisualElement>& Element)
{
    if (!Element || !FApplication::IsInitialized())
    {
        return FRectangle();
    }

    FRectangle Bounds = Element->GetContentRectangle();
    if (TSharedPtr<FWindow> OwningWindow = FApplication::Get().FindWindow(Element))
    {
        Bounds.Position += OwningWindow->GetPosition();
    }

    return Bounds;
}

FMenuStack::FMenuStack()
    : OpenMenus()
    , PendingSubMenu()
{
}

FMenuStack::~FMenuStack()
{
    DismissAll();
}

FMenuHandle FMenuStack::PushMenu(
    const TSharedPtr<FVisualElement>& AnchorElement,
    const FRectangle&                 AnchorBounds,
    EMenuPlacement                    Placement,
    const TSharedPtr<FVisualElement>& MenuContent)
{
    if (!MenuContent || !AnchorElement || !FApplication::IsInitialized())
    {
        return nullptr;
    }

    PendingSubMenu = FPendingSubMenu();

    const int32 ParentIndex = FindParentIndex(AnchorElement);
    DismissToDepth(ParentIndex + 1);

    TSharedPtr<FWindow> HostWindow;
    bool                bParentIsWindowed = false;

    if (ParentIndex >= 0)
    {
        HostWindow        = OpenMenus[ParentIndex]->HostWindow;
        bParentIsWindowed = !OpenMenus[ParentIndex]->bIsInline;
    }
    else
    {
        HostWindow = FApplication::Get().FindWindow(AnchorElement);
    }

    if (!HostWindow)
    {
        return nullptr;
    }

    MenuContent->PrepareDesiredSize();

    const IntVector2 MenuSize = MenuContent->GetCachedDesiredSize();
    if (MenuSize.X <= 0 || MenuSize.Y <= 0)
    {
        return nullptr;
    }

    const IntVector2 HostSize = HostWindow->GetSize();
    const FRectangle HostArea(HostWindow->GetPosition(), HostSize.X, HostSize.Y);

    const bool bFitsInHost = MenuSize.X <= HostArea.Width && MenuSize.Y <= HostArea.Height;
    const bool bIsInline   = bFitsInHost && !bParentIsWindowed;

    const FRectangle ClampArea  = bIsInline ? HostArea : Popups::FindWorkArea(AnchorBounds.Position);
    const FRectangle MenuBounds = ResolveBounds(AnchorBounds, MenuSize, Placement, MenuContent->GetContentTopInset(), ClampArea);

    FMenuHandle Menu   = MakeSharedPtr<FMenuLayer>();
    Menu->HostWindow   = HostWindow;
    Menu->Content      = MenuContent;
    Menu->ScreenBounds = MenuBounds;
    Menu->bIsInline    = bIsInline;

    if (bIsInline)
    {
        const FRectangle ClientBounds(MenuBounds.Position - HostArea.Position, MenuBounds.Width, MenuBounds.Height);
        HostWindow->GetOrCreateMenuHost()->AddChild(MenuContent, ClientBounds);
    }
    else
    {
        Menu->MenuWindow = Popups::Open(HostWindow, MenuBounds, MenuContent);
        if (!Menu->MenuWindow)
        {
            return nullptr;
        }
    }

    OpenMenus.Add(Menu);
    return Menu;
}

void FMenuStack::DismissTop()
{
    DismissToDepth(OpenMenus.Size() - 1);
}

void FMenuStack::DismissAll()
{
    DismissToDepth(0);
}

void FMenuStack::DismissToDepth(int32 Depth)
{
    const int32 Target = Math::Max(Depth, 0);
    if (OpenMenus.Size() <= Target)
    {
        return;
    }

    PendingSubMenu = FPendingSubMenu();

    while (OpenMenus.Size() > Target)
    {
        FMenuHandle Menu = OpenMenus.Last();
        OpenMenus.Pop();

        CloseLayer(Menu);
    }
}

bool FMenuStack::DismissOnClickOutside(const IntVector2& ScreenPosition)
{
    if (OpenMenus.IsEmpty())
    {
        return false;
    }

    for (const FMenuHandle& Menu : OpenMenus)
    {
        if (Menu->ScreenBounds.EncapsulatesPoint(ScreenPosition))
        {
            return false;
        }
    }

    DismissAll();
    return true;
}

bool FMenuStack::IsOpen() const
{
    return !OpenMenus.IsEmpty();
}

int32 FMenuStack::GetDepth() const
{
    return OpenMenus.Size();
}

bool FMenuStack::IsMenuOpen(const FMenuHandle& Menu) const
{
    return FindMenuIndex(Menu) >= 0;
}

int32 FMenuStack::GetMenuDepth(const FMenuHandle& Menu) const
{
    return FindMenuIndex(Menu) + 1;
}

int32 FMenuStack::GetOwningMenuDepth(const TSharedPtr<FVisualElement>& Element) const
{
    return FindParentIndex(Element) + 1;
}

void FMenuStack::ScheduleSubMenu(
    const TSharedPtr<FVisualElement>& Item,
    const FRectangle&                 AnchorBounds,
    const TSharedPtr<FVisualElement>& SubMenuContent,
    float                             DelaySeconds)
{
    if (!Item || !SubMenuContent)
    {
        return;
    }

    PendingSubMenu.Item             = Item;
    PendingSubMenu.Content          = SubMenuContent;
    PendingSubMenu.AnchorBounds     = AnchorBounds;
    PendingSubMenu.RemainingSeconds = Math::Max(DelaySeconds, 0.0f);
}

void FMenuStack::CancelScheduledSubMenu(const TSharedPtr<FVisualElement>& Item)
{
    if (PendingSubMenu.Item == Item)
    {
        PendingSubMenu = FPendingSubMenu();
    }
}

bool FMenuStack::HasScheduledSubMenu() const
{
    return PendingSubMenu.Item != nullptr;
}

bool FMenuStack::HandleKeyDown(const FKeyEvent& KeyEvent)
{
    if (OpenMenus.IsEmpty())
    {
        return false;
    }

    if (KeyEvent.GetKey() == Keys::Escape)
    {
        DismissTop();
        return true;
    }

    if (TSharedPtr<FVisualElement> TopContent = OpenMenus.Last()->Content)
    {
        return TopContent->OnKeyDown(KeyEvent).IsEventHandled();
    }

    return false;
}

void FMenuStack::Tick(float DeltaSeconds)
{
    if (!PendingSubMenu.Item)
    {
        return;
    }

    PendingSubMenu.RemainingSeconds -= DeltaSeconds;
    if (PendingSubMenu.RemainingSeconds > 0.0f)
    {
        return;
    }

    const FPendingSubMenu Pending = PendingSubMenu;
    PendingSubMenu                = FPendingSubMenu();

    PushMenu(Pending.Item, Pending.AnchorBounds, EMenuPlacement::RightOfTopAligned, Pending.Content);
}

FRectangle FMenuStack::ResolveBounds(const FRectangle& AnchorBounds, const IntVector2& MenuSize, EMenuPlacement Placement,
    int32 ContentTopInset, const FRectangle& ClampArea) const
{
    FRectangle Result = FRectangle(IntVector2(), MenuSize.X, MenuSize.Y);
    switch (Placement)
    {
        case EMenuPlacement::BelowLeftAligned:
        {
            Result.Position = IntVector2(AnchorBounds.Position.X, AnchorBounds.GetBottom());

            if (Result.GetBottom() > ClampArea.GetBottom() && (AnchorBounds.Position.Y - MenuSize.Y) >= ClampArea.Position.Y)
            {
                Result.Position.Y = AnchorBounds.Position.Y - MenuSize.Y;
            }

            if (Result.GetRight() > ClampArea.GetRight())
            {
                Result.Position.X = AnchorBounds.GetRight() - MenuSize.X;
            }

            break;
        }
        case EMenuPlacement::RightOfTopAligned:
        {
            Result.Position = IntVector2(AnchorBounds.GetRight(), AnchorBounds.Position.Y - ContentTopInset);

            if (Result.GetRight() > ClampArea.GetRight() && (AnchorBounds.Position.X - MenuSize.X) >= ClampArea.Position.X)
            {
                Result.Position.X = AnchorBounds.Position.X - MenuSize.X;
            }

            break;
        }
        case EMenuPlacement::AtCursor:
        {
            Result.Position = AnchorBounds.Position;

            if (Result.GetBottom() > ClampArea.GetBottom() && (AnchorBounds.Position.Y - MenuSize.Y) >= ClampArea.Position.Y)
            {
                Result.Position.Y = AnchorBounds.Position.Y - MenuSize.Y;
            }

            if (Result.GetRight() > ClampArea.GetRight() && (AnchorBounds.Position.X - MenuSize.X) >= ClampArea.Position.X)
            {
                Result.Position.X = AnchorBounds.Position.X - MenuSize.X;
            }

            break;
        }
    }

    Result.Position.X = Math::Clamp(Result.Position.X, ClampArea.Position.X, Math::Max(ClampArea.Position.X, ClampArea.GetRight() - MenuSize.X));
    Result.Position.Y = Math::Clamp(Result.Position.Y, ClampArea.Position.Y, Math::Max(ClampArea.Position.Y, ClampArea.GetBottom() - MenuSize.Y));

    return Result;
}

int32 FMenuStack::FindMenuIndex(const FMenuHandle& Menu) const
{
    if (!Menu)
    {
        return -1;
    }

    for (int32 Index = 0; Index < OpenMenus.Size(); ++Index)
    {
        if (OpenMenus[Index] == Menu)
        {
            return Index;
        }
    }

    return -1;
}

int32 FMenuStack::FindParentIndex(const TSharedPtr<FVisualElement>& AnchorElement) const
{
    if (!AnchorElement || OpenMenus.IsEmpty())
    {
        return -1;
    }

    FElementPath AnchorPath(EVisibility::Hidden | EVisibility::Visible);
    AnchorElement->FindParentElements(AnchorPath);

    for (int32 Index = OpenMenus.Size() - 1; Index >= 0; --Index)
    {
        if (AnchorPath.Contains(OpenMenus[Index]->Content))
        {
            return Index;
        }
    }

    return -1;
}

void FMenuStack::CloseLayer(const FMenuHandle& Menu)
{
    if (!Menu)
    {
        return;
    }

    if (Menu->bIsInline)
    {
        if (Menu->HostWindow)
        {
            if (TSharedPtr<FMenuHost> Host = Menu->HostWindow->GetMenuHost())
            {
                Host->RemoveChild(Menu->Content);
            }
        }
    }
    else
    {
        Popups::Close(Menu->MenuWindow);
    }
}
