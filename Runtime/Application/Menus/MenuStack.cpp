#include "Application/Menus/MenuStack.h"
#include "Application/Menus/PopupWindow.h"
#include "Application/Application.h"
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

TSharedPtr<FWindow> FMenuStack::PushMenu(
    const TSharedPtr<FWindow>&        ParentWindow,
    const FRectangle&                 AnchorBounds,
    EMenuPlacement                    Placement,
    const TSharedPtr<FVisualElement>& MenuContent)
{
    if (!MenuContent || !ParentWindow || !FApplication::IsInitialized())
    {
        return nullptr;
    }

    PendingSubMenu = FPendingSubMenu();

    const int32 ParentIndex = FindMenuIndex(ParentWindow);
    DismissToDepth(ParentIndex + 1);

    MenuContent->PrepareDesiredSize();

    const IntVector2 MenuSize = MenuContent->GetCachedDesiredSize();
    if (MenuSize.X <= 0 || MenuSize.Y <= 0)
    {
        return nullptr;
    }

    const FRectangle MenuBounds = ResolveBounds(AnchorBounds, MenuSize, Placement);

    TSharedPtr<FWindow> MenuWindow = Popups::Open(ParentWindow, MenuBounds, MenuContent);
    if (!MenuWindow)
    {
        return nullptr;
    }

    OpenMenus.Add(MenuWindow);
    return MenuWindow;
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
        TSharedPtr<FWindow> MenuWindow = OpenMenus.Last();
        OpenMenus.Pop();

        Popups::Close(MenuWindow);
    }
}

bool FMenuStack::DismissOnClickOutside(const IntVector2& ScreenPosition)
{
    if (OpenMenus.IsEmpty())
    {
        return false;
    }

    for (const TSharedPtr<FWindow>& MenuWindow : OpenMenus)
    {
        const IntVector2 Position = MenuWindow->GetPosition();
        const IntVector2 Size     = MenuWindow->GetSize();

        if (FRectangle(Position, Size.X, Size.Y).EncapsulatesPoint(ScreenPosition))
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

bool FMenuStack::IsMenuOpen(const TSharedPtr<FWindow>& MenuWindow) const
{
    return FindMenuIndex(MenuWindow) >= 0;
}

int32 FMenuStack::GetMenuDepth(const TSharedPtr<FWindow>& MenuWindow) const
{
    return FindMenuIndex(MenuWindow) + 1;
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

    if (TSharedPtr<FVisualElement> TopContent = OpenMenus.Last()->GetContent())
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

    if (FApplication::IsInitialized())
    {
        if (TSharedPtr<FWindow> OwningWindow = FApplication::Get().FindWindow(Pending.Item))
        {
            PushMenu(OwningWindow, Pending.AnchorBounds, EMenuPlacement::RightOfTopAligned, Pending.Content);
        }
    }
}

FRectangle FMenuStack::ResolveBounds(const FRectangle& AnchorBounds, const IntVector2& MenuSize, EMenuPlacement Placement) const
{
    const FRectangle WorkArea = Popups::FindWorkArea(AnchorBounds.Position);

    FRectangle Result = FRectangle(IntVector2(), MenuSize.X, MenuSize.Y);
    switch (Placement)
    {
        case EMenuPlacement::BelowLeftAligned:
        {
            Result.Position = IntVector2(AnchorBounds.Position.X, AnchorBounds.GetBottom());

            if (Result.GetBottom() > WorkArea.GetBottom() && (AnchorBounds.Position.Y - MenuSize.Y) >= WorkArea.Position.Y)
            {
                Result.Position.Y = AnchorBounds.Position.Y - MenuSize.Y;
            }

            if (Result.GetRight() > WorkArea.GetRight())
            {
                Result.Position.X = AnchorBounds.GetRight() - MenuSize.X;
            }

            break;
        }
        case EMenuPlacement::RightOfTopAligned:
        {
            Result.Position = IntVector2(AnchorBounds.GetRight(), AnchorBounds.Position.Y);

            if (Result.GetRight() > WorkArea.GetRight() && (AnchorBounds.Position.X - MenuSize.X) >= WorkArea.Position.X)
            {
                Result.Position.X = AnchorBounds.Position.X - MenuSize.X;
            }

            break;
        }
        case EMenuPlacement::AtCursor:
        {
            Result.Position = AnchorBounds.Position;

            if (Result.GetBottom() > WorkArea.GetBottom() && (AnchorBounds.Position.Y - MenuSize.Y) >= WorkArea.Position.Y)
            {
                Result.Position.Y = AnchorBounds.Position.Y - MenuSize.Y;
            }

            if (Result.GetRight() > WorkArea.GetRight() && (AnchorBounds.Position.X - MenuSize.X) >= WorkArea.Position.X)
            {
                Result.Position.X = AnchorBounds.Position.X - MenuSize.X;
            }

            break;
        }
    }

    Result.Position.X = Math::Clamp(Result.Position.X, WorkArea.Position.X, Math::Max(WorkArea.Position.X, WorkArea.GetRight() - MenuSize.X));
    Result.Position.Y = Math::Clamp(Result.Position.Y, WorkArea.Position.Y, Math::Max(WorkArea.Position.Y, WorkArea.GetBottom() - MenuSize.Y));

    return Result;
}

int32 FMenuStack::FindMenuIndex(const TSharedPtr<FWindow>& MenuWindow) const
{
    if (!MenuWindow)
    {
        return -1;
    }

    for (int32 Index = 0; Index < OpenMenus.Size(); ++Index)
    {
        if (OpenMenus[Index] == MenuWindow)
        {
            return Index;
        }
    }

    return -1;
}
