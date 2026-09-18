#include "Application/Menus/PopupWindow.h"
#include "Application/Application.h"
#include "Application/IApplicationRenderer.h"
#include "Core/Math/Math.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "RHI/RHI.h"
#include "RHI/RHISwapChain.h"

EPopupCornerRounding Popups::ResolveCornerRounding()
{
    if (FPlatformApplicationMisc::SupportsRoundedWindowCorners())
    {
        return EPopupCornerRounding::System;
    }

    return RHI::bSupportsTransparentSwapChain ? EPopupCornerRounding::Content : EPopupCornerRounding::None;
}

TSharedPtr<FWindow> Popups::Open(const TSharedPtr<FWindow>& ParentWindow, const FRectangle& Bounds,
    const TSharedPtr<FVisualElement>& Content, bool bAcceptsInput)
{
    if (!Content || !ParentWindow || !FApplication::IsInitialized())
    {
        return nullptr;
    }

    const EPopupCornerRounding Rounding = ResolveCornerRounding();

    FWindow::FDesc Desc;
    Desc.Title           = "Popup";
    Desc.ParentWindow    = ParentWindow;
    Desc.Size            = IntVector2(Bounds.Width, Bounds.Height);
    Desc.Position        = Bounds.Position;
    Desc.StyleFlags      = EWindowStyleFlags::TopMost | EWindowStyleFlags::NoTaskBarIcon;
    Desc.bActivateOnShow = false;
    Desc.bAcceptsInput   = bAcceptsInput;
    Desc.bShowOnCreate   = false;

    if (Rounding != EPopupCornerRounding::Content)
    {
        Desc.StyleFlags |= EWindowStyleFlags::Opaque;
    }

    TSharedPtr<FWindow> PopupWindow = FWindow::Create(Desc);
    PopupWindow->SetContent(Content);

    FApplication::Get().CreateWindow(PopupWindow);

    bool bContentDrawsCorners = Rounding == EPopupCornerRounding::Content;

    if (TSharedPtr<IApplicationRenderer> Renderer = FApplication::Get().GetRenderer())
    {
        Renderer->EnsureWindowSurface(PopupWindow);

        FRHISwapChainRef SwapChain = Renderer->GetWindowSwapChain(PopupWindow);
        if (SwapChain && !SwapChain->GetDesc().IsTransparent())
        {
            bContentDrawsCorners = false;
        }
    }

    if (!bContentDrawsCorners)
    {
        Content->SetOuterCornerRadius(0.0f);
    }

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
