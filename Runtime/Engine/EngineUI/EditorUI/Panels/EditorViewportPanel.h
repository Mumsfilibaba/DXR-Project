#pragma once
#include "Core/Containers/UniquePtr.h"
#include "Core/Math/Matrix4.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"
#include "Engine/EngineUI/EditorUI/IEditorViewportHost.h"
#include "Application/Gizmo/GizmoTypes.h"

class FComboBox;
class FEditorCameraController;
class FEditorViewportImage;
class FEditorViewportSurface;
class FGizmo;
class FMenu;
class FToolBar;
class FToolBarButton;

enum class EEditorGizmoPlacement
{
    Center,
    Pivot
};

class ENGINE_API FEditorViewportPanel final : public FEditorPanel, public IEditorViewportHost
{
public:
    FEditorViewportPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorViewportPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;

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

    /**
     * @brief Sets which handles the gizmo shows.
     *
     * @param InOperation Translate, Rotate or Scale.
     */
    void SetGizmoOperation(EGizmoOperation InOperation);

    /** @return The handle set the gizmo is showing. */
    NODISCARD EGizmoOperation GetGizmoOperation() const;

    /**
     * @brief Sets whether the handles align to the world axes or to the selection.
     *
     * @param InMode World or Local.
     */
    void SetGizmoMode(EGizmoMode InMode);

    /** @return Whether the handles align to the world axes or to the selection. */
    NODISCARD EGizmoMode GetGizmoMode() const;

    /**
     * @brief Sets whether the gizmo sits at the selection's bounds centre or at its pivot.
     *
     * @param InPlacement Center or Pivot.
     */
    void SetGizmoPlacement(EEditorGizmoPlacement InPlacement);

    /** @return Where the gizmo sits on the selection. */
    NODISCARD EEditorGizmoPlacement GetGizmoPlacement() const;

    /**
     * @brief Sets the debug view the scene is drawn with.
     *
     * @param InDebugView The view to draw, Nothing for the lit image.
     */
    void SetDebugView(FSceneRenderView::EDebugView InDebugView);

    /** @brief Starts play when the world is being authored, and stops it when it is running. */
    void TogglePlay();

private:
    void OnViewportClicked(const IntVector2& ImagePosition, bool bAdditive);
    void OnViewportContextMenu(const IntVector2& ImagePosition, const IntVector2& ScreenPosition);
    void OnViewportMarqueeSelect(const IntVector2& ImageMin, const IntVector2& ImageMax, bool bAdditive);
    void OnGizmoDragStarted(EGizmoHandle Handle);
    void OnGizmoTransformChanged(const Matrix4& NewTransform, const Matrix4& Delta);
    void OnGizmoDragFinished(const Matrix4& TransformAtDragStart, const Matrix4& Transform);
    NODISCARD bool OnViewportShortcut(const struct FKeyEvent& KeyEvent);

    void UpdateGizmoFromSelection();
    void UpdateGizmoCamera();
    void RefreshToolBarState();
    void ShowContextMenu();
    void SelectSpawnedActor(FActor* SpawnedActor);
    NODISCARD bool ComputeFallbackPlacement(const Vector2& Ndc, Vector3& OutLocation) const;

    NODISCARD EGizmoOperation ResolveGizmoOperation(const TArray<FActor*>& Selection) const;
    NODISCARD Vector3 ComputeGizmoLocation(const TArray<FActor*>& Selection) const;
    NODISCARD Vector3 GetActorGizmoPoint(FActor* Actor) const;

    NODISCARD TSharedPtr<FToolBar> BuildToolBar();
    NODISCARD int32 ComputeViewButtonWidth() const;
    NODISCARD TSharedPtr<FComboBox> BuildSecondaryDebugViewCombo();
    NODISCARD TSharedPtr<FVisualElement> BuildCameraMenu();
    NODISCARD TSharedPtr<FVisualElement> BuildViewOptionsMenu();
    NODISCARD TSharedPtr<FMenu> BuildContextMenu();
    NODISCARD TSharedPtr<FMenu> BuildPlaceActorMenu();

    TUniquePtr<FEditorCameraController> CameraController;
    TSharedPtr<FEditorViewportSurface>  Surface;
    TSharedPtr<FEditorViewportImage>    Image;
    TSharedPtr<FGizmo>                  Gizmo;
    TSharedPtr<FToolBar>                DebugViewBar;
    TSharedPtr<FComboBox>               SecondaryDebugViewCombo;
    TSharedPtr<FToolBarButton>          TranslateItem;
    TSharedPtr<FToolBarButton>          RotateItem;
    TSharedPtr<FToolBarButton>          ScaleItem;
    TSharedPtr<FToolBarButton>          CenterItem;
    TSharedPtr<FToolBarButton>          PivotItem;
    TSharedPtr<FToolBarButton>          LocalItem;
    TSharedPtr<FToolBarButton>          WorldItem;
    TSharedPtr<FToolBarButton>          PlayItem;
    TSharedPtr<FToolBarButton>          PauseItem;
    TSharedPtr<FToolBarButton>          ViewItem;
    FRHITextureRef                      ViewportImage;
    IntVector2                          CachedViewportSize;
    Matrix4                             ContextMenuViewProjectionInverse;
    Vector3                             ContextMenuLocation;
    Vector2                             ContextMenuNdc;
    IntVector2                          ContextMenuScreenPosition;
    uint64                              ContextMenuPickRequestId;
    FSceneRenderView::EDebugView        DebugView;
    FSceneRenderView::EDebugView        SecondaryDebugView;
    FSceneRenderView::EDebugViewChannel DebugViewChannelMask;
    EEditorGizmoPlacement               GizmoPlacement;
    EGizmoOperation                     RequestedGizmoOperation;
    TArray<FActor*>                     DragActors;
    TArray<Matrix4>                     DragStartTransforms;
};
