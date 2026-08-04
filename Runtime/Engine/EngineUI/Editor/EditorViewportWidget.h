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
    FCameraComponent* GetViewCamera() const;

    void OnActorRemoved(FActor* Actor);
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
    FSceneRenderView::EDebugView        DebugView;
    FSceneRenderView::EDebugView        SecondaryDebugView;
    EGizmoPlacement                     GizmoPlacement;
    EditorGuizmo::EMode                 GizmoOrientation;
    EditorGuizmo::EOperation::Type      GizmoOperation;
};
