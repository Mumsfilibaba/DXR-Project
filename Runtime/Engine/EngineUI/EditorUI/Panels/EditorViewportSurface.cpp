#include "Engine/EngineUI/EditorUI/Panels/EditorViewportSurface.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorViewportImage.h"
#include "Engine/EngineUI/Editor/EditorCameraController.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/ElementPath.h"
#include "Application/Elements/Viewport.h"
#include "Application/Gizmo/Gizmo.h"
#include "Application/Input/Keys.h"
#include "Application/Style/UIStyle.h"

// The ring that says the world is running, in the amber the editor uses nowhere else
static const FFloatColor PLAY_BORDER_COLOR = FFloatColor(0.95f, 0.62f, 0.11f, 1.0f);

static constexpr float PLAY_BORDER_THICKNESS = 2.0f;

TSharedPtr<FEditorViewportSurface> FEditorViewportSurface::Create()
{
    return MakeSharedPtr<FEditorViewportSurface>();
}

FEditorViewportSurface::FEditorViewportSurface()
    : FVisualElement()
    , Image(nullptr)
    , HostViewport(nullptr)
    , Gizmo(nullptr)
    , CameraInput(MakeUniquePtr<FEditorCameraInputState>())
    , LastCursorPosition()
    , MarqueeStartPosition()
    , MarqueeEndPosition()
    , RightMouseDragDistance(0.0f)
    , bMouseLookActive(false)
    , bLeftPressedInside(false)
    , bMarqueeActive(false)
    , bShowPlayBorder(false)
{
}

FEditorViewportSurface::~FEditorViewportSurface()
{
}

void FEditorViewportSurface::SetLayers(const TSharedPtr<FEditorViewportImage>& InImage, const TSharedPtr<FViewport>& InViewport,
    const TSharedPtr<FGizmo>& InGizmo)
{
    Image        = InImage;
    HostViewport = InViewport;
    Gizmo        = InGizmo;

    const TWeakPtr<FVisualElement> Self = AsWeakPtr();

    if (Image)
    {
        Image->SetParentElement(Self);
    }

    if (HostViewport)
    {
        HostViewport->SetParentElement(Self);
    }

    if (Gizmo)
    {
        Gizmo->SetParentElement(Self);
    }
}

void FEditorViewportSurface::Tick(const FRectangle& AssignedBounds)
{
    SetContentRectangle(AssignedBounds);
    OnArrange(AssignedBounds);
}

IntVector2 FEditorViewportSurface::ComputeDesiredSize() const
{
    return IntVector2(0, 0);
}

void FEditorViewportSurface::OnArrange(const FRectangle& AllottedBounds)
{
    if (Image)
    {
        Image->Tick(AllottedBounds);
    }

    if (HostViewport)
    {
        HostViewport->SetSize(IntVector2(AllottedBounds.Width, AllottedBounds.Height));
        HostViewport->Tick(AllottedBounds);
    }

    if (Gizmo)
    {
        Gizmo->Tick(AllottedBounds);
    }
}

void FEditorViewportSurface::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    if (Image)
    {
        OutChildren.Add(Image);
    }

    if (HostViewport)
    {
        OutChildren.Add(HostViewport);
    }

    if (Gizmo)
    {
        OutChildren.Add(Gizmo);
    }
}

void FEditorViewportSurface::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (HostViewport && HostViewport->GetContentRectangle().EncapsulatesPoint(ClientPosition))
    {
        HostViewport->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }

    if (Gizmo && Gizmo->IsVisible() && Gizmo->IsProjected() && Gizmo->GetContentRectangle().EncapsulatesPoint(ClientPosition))
    {
        Gizmo->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}

int32 FEditorViewportSurface::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    int32 CurrentLayer = LayerId;

    OutCommandList.PushClip(CurrentLayer, AllottedGeometry.Bounds);

    if (Image)
    {
        CurrentLayer = Image->OnDraw(FDrawGeometry(Image->GetContentRectangle(), AllottedGeometry.Scale), OutCommandList, CurrentLayer) + 1;
    }

    if (Gizmo && Gizmo->IsVisible() && Gizmo->IsProjected())
    {
        CurrentLayer = Gizmo->OnDraw(FDrawGeometry(Gizmo->GetContentRectangle(), AllottedGeometry.Scale), OutCommandList, CurrentLayer) + 1;
    }

    if (bMarqueeActive)
    {
        const FUIStyle&  Style   = FUIStyle::GetDefault();
        const FRectangle Marquee = GetMarqueeRectangle().Intersect(AllottedGeometry.Bounds);

        FFloatColor Fill = Style.Colors.Accent;
        Fill.A           = 0.2f;

        OutCommandList.AddBox(CurrentLayer, Marquee, Fill);
        OutCommandList.AddBoxOutline(CurrentLayer, Marquee, Style.Colors.Accent, 1.0f);

        ++CurrentLayer;
    }

    if (bShowPlayBorder)
    {
        OutCommandList.AddBoxOutline(CurrentLayer, AllottedGeometry.Bounds, PLAY_BORDER_COLOR, PLAY_BORDER_THICKNESS);
        ++CurrentLayer;
    }

    OutCommandList.PopClip(CurrentLayer);
    return CurrentLayer;
}

void FEditorViewportSurface::SetPlayBorderVisible(bool bInShowPlayBorder)
{
    bShowPlayBorder = bInShowPlayBorder;
}

bool FEditorViewportSurface::IsGizmoBusy() const
{
    return Gizmo && Gizmo->IsVisible() && Gizmo->IsProjected() && (Gizmo->IsHovered() || Gizmo->IsDragging());
}

FRectangle FEditorViewportSurface::GetMarqueeRectangle() const
{
    const IntVector2 Min(Math::Min(MarqueeStartPosition.X, MarqueeEndPosition.X), Math::Min(MarqueeStartPosition.Y, MarqueeEndPosition.Y));
    const IntVector2 Max(Math::Max(MarqueeStartPosition.X, MarqueeEndPosition.X), Math::Max(MarqueeStartPosition.Y, MarqueeEndPosition.Y));

    return FRectangle(Min, Max.X - Min.X, Max.Y - Min.Y);
}

void FEditorViewportSurface::EndMarquee()
{
    if (!bMarqueeActive)
    {
        return;
    }

    bMarqueeActive = false;

    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }
}

FEventResponse FEditorViewportSurface::OnMouseMove(const FCursorEvent& CursorEvent)
{
    const IntVector2 Position = CursorEvent.GetClientPosition();
    const IntVector2 Delta    = Position - LastCursorPosition;

    LastCursorPosition = Position;

    if (bMouseLookActive)
    {
        CameraInput->LookDelta.X += static_cast<float>(Delta.X);
        CameraInput->LookDelta.Y += static_cast<float>(Delta.Y);

        RightMouseDragDistance += Math::Abs(static_cast<float>(Delta.X)) + Math::Abs(static_cast<float>(Delta.Y));
        return FEventResponse::Handled();
    }

    if (CameraInput->bMiddleMouseDown)
    {
        CameraInput->PanDelta.X += static_cast<float>(Delta.X);
        CameraInput->PanDelta.Y += static_cast<float>(Delta.Y);
        return FEventResponse::Handled();
    }

    if (CameraInput->bAltDown && CameraInput->bLeftMouseDown)
    {
        CameraInput->LookDelta.X += static_cast<float>(Delta.X);
        CameraInput->LookDelta.Y += static_cast<float>(Delta.Y);
        return FEventResponse::Handled();
    }

    if (bLeftPressedInside)
    {
        MarqueeEndPosition = Position;

        if (!bMarqueeActive)
        {
            const IntVector2 Travel = Position - MarqueeStartPosition;
            bMarqueeActive = ((Travel.X * Travel.X) + (Travel.Y * Travel.Y)) > (MarqueeDragThreshold * MarqueeDragThreshold);

            if (bMarqueeActive && FApplication::IsInitialized())
            {
                FApplication::Get().CaptureMouse(AsSharedPtr());
            }
        }

        if (bMarqueeActive)
        {
            return FEventResponse::Handled();
        }
    }

    return FEventResponse::Unhandled();
}

FEventResponse FEditorViewportSurface::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    const FKey Key = CursorEvent.GetKey();

    LastCursorPosition = CursorEvent.GetClientPosition();

    CameraInput->bAltDown = CursorEvent.GetModifierKeys().IsAltDown();
    CameraInput->bCmdDown = CursorEvent.GetModifierKeys().IsShortcutChordDown();

    if (Key == Keys::MouseButtonRight)
    {
        CameraInput->bRightMouseDown = true;
        CameraInput->bFlyActive      = true;

        bMouseLookActive       = true;
        RightMouseDragDistance = 0.0f;

        return FEventResponse::Handled();
    }

    if (Key == Keys::MouseButtonMiddle)
    {
        CameraInput->bMiddleMouseDown = true;
        return FEventResponse::Handled();
    }

    if (Key == Keys::MouseButtonLeft)
    {
        CameraInput->bLeftMouseDown = true;

        bLeftPressedInside = !IsGizmoBusy();

        EndMarquee();

        MarqueeStartPosition = LastCursorPosition;
        MarqueeEndPosition   = LastCursorPosition;

        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FEditorViewportSurface::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    const FKey Key = CursorEvent.GetKey();
    if (Key == Keys::MouseButtonRight)
    {
        const bool bWasClick = RightMouseDragDistance <= ContextMenuDragLimit;

        CameraInput->bRightMouseDown = false;
        CameraInput->bFlyActive      = false;
        bMouseLookActive             = false;

        if (bWasClick)
        {
            OnContextMenuDelegate.ExecuteIfBound(CursorEvent.GetClientPosition() - GetContentRectangle().Position, CursorEvent.GetScreenPosition());
        }

        return FEventResponse::Handled();
    }

    if (Key == Keys::MouseButtonMiddle)
    {
        CameraInput->bMiddleMouseDown = false;
        return FEventResponse::Handled();
    }

    if (Key == Keys::MouseButtonLeft)
    {
        CameraInput->bLeftMouseDown = false;

        const IntVector2 Origin = GetContentRectangle().Position;

        if (bMarqueeActive)
        {
            const FRectangle Marquee   = GetMarqueeRectangle();
            const bool       bAdditive = CursorEvent.GetModifierKeys().IsShortcutChordDown() || CursorEvent.GetModifierKeys().IsShiftDown();

            EndMarquee();

            OnMarqueeSelectDelegate.ExecuteIfBound(Marquee.Position - Origin, IntVector2(Marquee.GetRight(), Marquee.GetBottom()) - Origin, bAdditive);
        }
        else if (bLeftPressedInside && !IsGizmoBusy())
        {
            const bool bAdditive = CursorEvent.GetModifierKeys().IsShortcutChordDown();
            OnClickedDelegate.ExecuteIfBound(CursorEvent.GetClientPosition() - Origin, bAdditive);
        }

        bLeftPressedInside = false;
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FEditorViewportSurface::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    CameraInput->WheelDelta += CursorEvent.GetScrollDelta();
    return FEventResponse::Handled();
}

FEventResponse FEditorViewportSurface::OnKeyDown(const FKeyEvent& KeyEvent)
{
    const FKey Key = KeyEvent.GetKey();

    CameraInput->bBoost   = KeyEvent.GetModifierKeys().IsShiftDown();
    CameraInput->bAltDown = KeyEvent.GetModifierKeys().IsAltDown();

    if (bMouseLookActive)
    {
        if (Key == Keys::W)
        {
            CameraInput->MoveAxis.Z =  1.0f;
            return FEventResponse::Handled();
        }

        if (Key == Keys::S)
        {
            CameraInput->MoveAxis.Z = -1.0f;
            return FEventResponse::Handled();
        }

        if (Key == Keys::D)
        {
            CameraInput->MoveAxis.X = -1.0f;
            return FEventResponse::Handled();
        }

        if (Key == Keys::A)
        {
            CameraInput->MoveAxis.X = 1.0f;
            return FEventResponse::Handled();
        }

        if (Key == Keys::E)
        {
            CameraInput->MoveAxis.Y = 1.0f;
            return FEventResponse::Handled();
        }

        if (Key == Keys::Q)
        {
            CameraInput->MoveAxis.Y = -1.0f;
            return FEventResponse::Handled();
        }

        if (Key == Keys::R)
        {
            CameraInput->bResetPressed = true;
            return FEventResponse::Handled();
        }
    }

    if (Key == Keys::F)
    {
        CameraInput->bFocusPressed = true;
        return FEventResponse::Handled();
    }

    if (Key == Keys::Right)
    {
        CameraInput->RotationAxis.X =  1.0f;
        return FEventResponse::Handled();
    }

    if (Key == Keys::Left)
    {
        CameraInput->RotationAxis.X = -1.0f;
        return FEventResponse::Handled();
    }

    if (Key == Keys::Down)
    {
        CameraInput->RotationAxis.Y =  1.0f;
        return FEventResponse::Handled();
    }

    if (Key == Keys::Up)
    {
        CameraInput->RotationAxis.Y = -1.0f;
        return FEventResponse::Handled();
    }

    if (!bMouseLookActive && OnShortcutDelegate.IsBound() && OnShortcutDelegate.Execute(KeyEvent))
    {
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FEditorViewportSurface::OnKeyUp(const FKeyEvent& KeyEvent)
{
    const FKey Key = KeyEvent.GetKey();

    CameraInput->bBoost   = KeyEvent.GetModifierKeys().IsShiftDown();
    CameraInput->bAltDown = KeyEvent.GetModifierKeys().IsAltDown();

    if (Key == Keys::W || Key == Keys::S)
    {
        CameraInput->MoveAxis.Z = 0.0f;
        return FEventResponse::Handled();
    }

    if (Key == Keys::A || Key == Keys::D)
    {
        CameraInput->MoveAxis.X = 0.0f;
        return FEventResponse::Handled();
    }

    if (Key == Keys::Q || Key == Keys::E)
    {
        CameraInput->MoveAxis.Y = 0.0f;
        return FEventResponse::Handled();
    }

    if (Key == Keys::Left || Key == Keys::Right)
    {
        CameraInput->RotationAxis.X = 0.0f;
        return FEventResponse::Handled();
    }

    if (Key == Keys::Up   || Key == Keys::Down) 
    {
        CameraInput->RotationAxis.Y = 0.0f;
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FEditorViewportSurface::OnFocusLost()
{
    CameraInput->MoveAxis     = Vector3(0.0f);
    CameraInput->RotationAxis = Vector2(0.0f, 0.0f);

    return FEventResponse::Unhandled();
}

bool FEditorViewportSurface::SupportsKeyboardFocus() const
{
    return true;
}

void FEditorViewportSurface::ResetInputState()
{
    EndMarquee();

    *CameraInput = FEditorCameraInputState();

    MarqueeStartPosition   = IntVector2();
    MarqueeEndPosition     = IntVector2();
    RightMouseDragDistance = 0.0f;
    bMouseLookActive       = false;
    bLeftPressedInside     = false;
}

const FEditorCameraInputState& FEditorViewportSurface::GetCameraInput() const
{
    return *CameraInput;
}

void FEditorViewportSurface::ConsumeCameraInputDeltas()
{
    CameraInput->LookDelta     = Vector2(0.0f, 0.0f);
    CameraInput->PanDelta      = Vector2(0.0f, 0.0f);
    CameraInput->WheelDelta    = 0.0f;
    CameraInput->bFocusPressed = false;
    CameraInput->bResetPressed = false;
}
