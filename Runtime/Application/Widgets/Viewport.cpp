#include "Viewport.h"
#include "Window.h"
#include "Application/Application.h"
#include "Project/ProjectManager.h"
#include "RHI/RHIInterface.h"

FViewportWidget::FViewportWidget()
    : FWidget()
    , ViewportRHI(nullptr)
    , ViewportInterface(nullptr)
{
}

void FViewportWidget::Initialize(const FInitializer& Initializer)
{
    ViewportInterface = Initializer.ViewportInterface;
    Width  = Initializer.Width;
    Height = Initializer.Height;
}

bool FViewportWidget::CreateRHI()
{
    TWeakPtr<FWidget> ParentElement;// = GetParentElement();
    if (!ParentElement.IsValid())
    {
        DEBUG_BREAK();
        return false;
    }

    if (ViewportRHI)
    {
        DEBUG_BREAK();
        return true;
    }

    TSharedRef<FGenericWindow> NativeWindow;// = ParentWindow->GetNativeWindow();
    if (!NativeWindow)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIViewportDesc ViewportDesc(
        NativeWindow->GetPlatformHandle(),
        EFormat::R8G8B8A8_Unorm,
        EFormat::Unknown,
        static_cast<uint16>(NativeWindow->GetWidth()),
        static_cast<uint16>(NativeWindow->GetHeight()));

    ViewportRHI = RHICreateViewport(ViewportDesc);
    if (!ViewportRHI)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FViewportWidget::DestroyRHI()
{
    ViewportRHI.Reset();
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

void FViewportWidget::Paint(const FRectangle& AssignedBounds)
{
}

FResponse FViewportWidget::OnControllerButtonUp(const FControllerEvent& ControllerEvent)
{
    return ViewportInterface ? ViewportInterface->OnControllerButtonUp(ControllerEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnControllerButtonDown(const FControllerEvent& ControllerEvent)
{
    return ViewportInterface ? ViewportInterface->OnControllerButtonDown(ControllerEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnControllerButtonAnalog(const FControllerEvent& ControllerEvent)
{
    return ViewportInterface ? ViewportInterface->OnControllerButtonAnalog(ControllerEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyDown(KeyEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyUp(KeyEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnKeyChar(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyChar(KeyEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnMouseMove(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseMove(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnMouseButtonDown(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonDown(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnMouseButtonUp(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonUp(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnMouseEntered(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseEntered(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnMouseLeft(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseLeft(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewportWidget::OnMouseDoubleClick(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseDoubleClick(MouseEvent) : FResponse::Unhandled();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

FIntVector2 FViewportWidget::GetSize() const
{
    TWeakPtr<FWindow> ParentWindow;
    if (ParentWindow.IsValid())
    {
        return FIntVector2
        { 
            static_cast<int32>(ParentWindow->GetWidth()),
            static_cast<int32>(ParentWindow->GetHeight())
        };
    }

    return FIntVector2{ 0, 0 };
}