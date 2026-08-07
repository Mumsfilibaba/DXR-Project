#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "RHI/RHIResources.h"
#include "Application/Widgets/ViewportWidget.h"
#include "Engine/EngineUI/Editor/EditorGuizmo.h"
#include "Engine/EngineUI/Editor/EditorCameraController.h"
#include "RendererCore/Interfaces/IRendererModule.h"

class FActor;
class FCameraComponent;
class FEditorEngine;

class FEditorViewportWidget
{
public:
    enum class EGizmoPlacement
    {
        Center,
        Pivot
    };

    FEditorViewportWidget(FEditorEngine* InEditorEngine);
    ~FEditorViewportWidget();
    
    void Draw();
    void Tick(float DeltaTime);
    
    void SetViewportWidget(const TSharedPtr<FViewportWidget>& ViewportWidget);
    void SetViewportImage(FRHITextureRef InViewportImage);
    
    IntVector2 GetViewportSize() const;
    
    FSceneRenderView::EDebugView GetDebugView() const;
    FSceneRenderView::EDebugView GetSecondaryDebugView() const;
    FSceneRenderView::EDebugViewChannel GetDebugViewChannelMask() const;
    FCameraComponent* GetViewCamera() const;

    void OnActorRemoved(FActor* Actor);
    void OnContextMenuPickResult(const FEditorPickResult& Result, FActor* PickedActor);

    bool ConsumeCameraCut();

    EGizmoPlacement GetGizmoPlacement() const
    {
        return GizmoPlacement;
    }

    void SetGizmoPlacement(EGizmoPlacement InPlacement)
    {
        GizmoPlacement = InPlacement;
    }

    EditorGuizmo::EMode GetGizmoOrientation() const
    {
        return GizmoOrientation;
    }

    void SetGizmoOrientation(EditorGuizmo::EMode InOrientation)
    {
        GizmoOrientation = InOrientation;
    }

    EditorGuizmo::EOperation::Type GetGizmoOperation() const
    {
        return GizmoOperation;
    }

    void SetGizmoOperation(EditorGuizmo::EOperation::Type InOperation)
    {
        GizmoOperation = InOperation;
    }

    bool IsVisible() const
    {
        return bVisible;
    }
    
    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

private:
    void EndMouseLook();
    void DrawContextMenu();

    bool ComputeViewportPixel(const ImVec2& ImageMin, const ImVec2& ImageSize, uint32& OutPixelX, uint32& OutPixelY) const;
    bool ComputeFallbackPlacement(const Vector2& Ndc, Vector3& OutLocation) const;

    FEditorEngine*                      EditorEngine;
    TUniquePtr<FEditorCameraController> CameraController;
    TSharedPtr<FViewportWidget>         ViewportWidget;
    IntVector2                          CachedViewportSize;
    FImGuiTexture                       ViewportImage;
    FDelegateHandle                     ImGuiDelegateHandle;
    bool                                bVisible;
    bool                                bViewportInputActive;
    bool                                bMouseLookActive;
    bool                                bRawLookActive;
    bool                                bCursorWasVisible;
    IntVector2                          MouseLookRestorePosition;
    FEditorCameraInputState             PendingCameraInput;
    float                               SpeedOverlayTimer;
    bool                                bRightMousePressedOnImage;
    float                               RightMouseDragDistance;
    uint64                              ContextMenuPickRequestId;
    Vector2                             ContextMenuNdc;
    Matrix4                             ContextMenuViewProjectionInverse;
    Vector3                             ContextMenuLocation;
    FActor*                             ContextMenuActor;
    FSceneRenderView::EDebugView        DebugView;
    FSceneRenderView::EDebugView        SecondaryDebugView;
    FSceneRenderView::EDebugViewChannel DebugViewChannelMask;
    EGizmoPlacement                     GizmoPlacement;
    EditorGuizmo::EMode                 GizmoOrientation;
    EditorGuizmo::EOperation::Type      GizmoOperation;
};
