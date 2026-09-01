#include "Application/Menus/PopupWindow.h"
#include "Application/Application.h"
#include "Core/Math/Math.h"

TSharedPtr<FWindow> Popups::Open(const TSharedPtr<FWindow>& ParentWindow, const FRectangle& Bounds,
    const TSharedPtr<FVisualElement>& Content, bool bAcceptsInput)
{
    if (!Content || !ParentWindow || !FApplication::IsInitialized())
    {
        return nullptr;
    }

    FWindow::FDesc Desc;
    Desc.Title           = "Popup";
    Desc.ParentWindow    = ParentWindow;
    Desc.Size            = IntVector2(Bounds.Width, Bounds.Height);
    Desc.Position        = Bounds.Position;
    Desc.StyleFlags      = EWindowStyleFlags::TopMost | EWindowStyleFlags::NoTaskBarIcon | EWindowStyleFlags::Opaque;
    Desc.bActivateOnShow = false;
    Desc.bAcceptsInput   = bAcceptsInput;
    Desc.bShowOnCreate   = false;

    TSharedPtr<FWindow> PopupWindow = FWindow::Create(Desc);
    PopupWindow->SetContent(Content);

    FApplication::Get().CreateWindow(PopupWindow);
    FApplication::LayoutWindow(PopupWindow);

    return PopupWindow;
}

void Popups::Close(const TSharedPtr<FWindow>& PopupWindow)
{
    if (PopupWindow && FApplication::IsInitialized())
    {
        FApplication::Get().DestroyWindow(PopupWindow);
    }
}

FRectangle Popups::FindWorkArea(const IntVector2& Point)
{
    TArray<FMonitorInfo> Monitors;
    if (FApplication::IsInitialized())
    {
        FApplication::Get().GetDisplayInfo(Monitors);
    }

    FRectangle PrimaryArea;
    for (const FMonitorInfo& Monitor : Monitors)
    {
        const FRectangle WorkArea(Monitor.WorkPosition, Monitor.WorkSize.X, Monitor.WorkSize.Y);
        if (WorkArea.EncapsulatesPoint(Point))
        {
            return WorkArea;
        }

        if (Monitor.bIsPrimary || PrimaryArea.IsEmpty())
        {
            PrimaryArea = WorkArea;
        }
    }

    return PrimaryArea;
}

FRectangle Popups::ClampToWorkArea(const FRectangle& Bounds)
{
    const FRectangle WorkArea = FindWorkArea(Bounds.Position);
    if (WorkArea.IsEmpty())
    {
        return Bounds;
    }

    FRectangle Result = Bounds;
    Result.Position.X = Math::Clamp(Result.Position.X, WorkArea.Position.X,
        Math::Max(WorkArea.Position.X, WorkArea.GetRight() - Bounds.Width));
    Result.Position.Y = Math::Clamp(Result.Position.Y, WorkArea.Position.Y,
        Math::Max(WorkArea.Position.Y, WorkArea.GetBottom() - Bounds.Height));

    return Result;
}
