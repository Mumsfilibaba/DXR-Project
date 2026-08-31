#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "RHI/RHIResources.h"
#include "Application/Elements/Viewport.h"
#include "Engine/EngineUI/Editor/EditorGuizmo.h"
#include "Engine/EngineUI/Editor/EditorCameraController.h"
#include "Engine/EngineUI/EditorUI/IEditorViewportHost.h"
#include "RendererCore/Interfaces/IRendererModule.h"

class FActor;
class FCameraComponent;
class FEditorEngine;

class FEditorViewportWidget final : public IEditorViewportHost
{
public:
    enum class EGizmoPlacement
    {
        Center,
        Pivot
    };

    FEditorViewportWidget(FEditorEngine* InEditorEngine);
    virtual ~FEditorViewportWidget();

    void Draw();

    // IEditorViewportHost Interface
    virtual void OnActorRemoved(FActor* Actor) override final;
    virtual void OnContextMenuPickResult(const FEditorPickResult& Result, FActor* PickedActor) override final;

    virtual bool ConsumeCameraCut() override final;
    virtual void ResetInputState() override final;
    virtual void FocusOnActor(FActor* Actor) override final;

    virtual void SetViewportImage(FRHITextureRef InViewportImage) override final;

    virtual IntVector2                          GetViewportSize() const override final;
    virtual FSceneRenderView::EDebugView        GetDebugView() const override final;
    virtual FSceneRenderView::EDebugView        GetSecondaryDebugView() const override final;
    virtual FSceneRenderView::EDebugViewChannel GetDebugViewChannelMask() const override final;
    virtual FCameraComponent*                   GetViewCamera() const override final;

    void SetViewport(const TSharedPtr<FViewport>& InViewport);

    ImVec2 GetViewportImageMin() const
    {
        return CachedImageMin;
    }

    ImVec2 GetViewportImageSize() const
    {
        return CachedImageSize;
    }

    ImDrawList* GetViewportImageDrawList() const
    {
        return CachedImageDrawList;
    }

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
    void DrawViewportWindow();
    void UpdateCamera(float DeltaTime);
    void EndMouseLook();
    void DrawContextMenu();
    void HandlePlayShortcuts();
    void TogglePlay();

    bool ComputeViewportPixel(const ImVec2& ImageMin, const ImVec2& ImageSize, uint32& OutPixelX, uint32& OutPixelY) const;
    bool ComputeFallbackPlacement(const Vector2& Ndc, Vector3& OutLocation) const;

    FEditorEngine*                      EditorEngine;
    TUniquePtr<FEditorCameraController> CameraController;
    TSharedPtr<FViewport>               HostViewport;
    IntVector2                          CachedViewportSize;
    ImVec2                              CachedImageMin;
    ImVec2                              CachedImageSize;
    ImDrawList*                         CachedImageDrawList;
    FImGuiTexture                       ViewportImage;
    FDelegateHandle                     ImGuiDelegateHandle;
    bool                                bVisible;
    bool                                bViewportInputActive;
    bool                                bMouseLookActive;
    bool                                bRawLookActive;
    bool                                bCursorWasVisible;
    IntVector2                          MouseLookRestorePosition;
    Vector2                             MarqueeStartPos;
    bool                                bPickArmed;
    bool                                bMarqueeActive;
    bool                                bMarqueeAdditive;
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
    float                               CachedMaxDebugLabelWidth;
    ImFont*                             CachedDebugLabelFont;
    float                               CachedDebugLabelFontSize;
};
