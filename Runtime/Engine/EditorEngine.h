#pragma once
#include "Engine/Engine.h"

class FEditorDockspaceWidget;
class FEditorFooterWidget;
class FEditorLogOutputWidget;
class FEditorViewportWidget;
class FEditorSceneHierarchyWidget;
class FEditorPropertiesWidget;
class FEditorContentBrowserWidget;

class ENGINE_API FEditorEngine : public FEngine
{
public:
    FEditorEngine();
    virtual ~FEditorEngine();

    // FEngine Interface
    virtual bool Init() override final;
    virtual bool InitPostRenderer() override final;

    virtual void Release() override final;

    virtual void Tick(float DeltaTime) override final;
    virtual void RenderFrame() override final;

    // Editor Widgets
    const TSharedPtr<FEditorDockspaceWidget>&          GetDockspaceWidget() const { return DockspaceWidget; }
    const TSharedPtr<FEditorFooterWidget>&             GetFooterWidget() const { return FooterWidget; }
    const TSharedPtr<FEditorLogOutputWidget>&          GetLogOutputWidget() const { return LogOutputWidget; }
    const TSharedPtr<FEditorViewportWidget>&           GetEditorViewportWidget() const { return ViewportWidget; }
    const TSharedPtr<FEditorSceneHierarchyWidget>&     GetSceneHierarchyWidget() const { return SceneHierarchyWidget; }
    const TSharedPtr<FEditorPropertiesWidget>&         GetPropertiesWidget() const { return PropertiesWidget; }
    const TSharedPtr<FEditorContentBrowserWidget>&     GetContentBrowserWidget() const { return ContentBrowserWidget; }

    void SetSelectedActor(FActor* InActor);
    void SetSelectedLight(FLight* InLight);
    void SetSelectedCamera(FCamera* InCamera);
    void SetSelectedLightProbe(FLightProbe* InProbe);
    void ClearSelection();

    FActor*      GetSelectedActor() const { return SelectedActor; }
    FLight*      GetSelectedLight() const { return SelectedLight; }
    FCamera*     GetSelectedCamera() const { return SelectedCamera; }
    FLightProbe* GetSelectedLightProbe() const { return SelectedLightProbe; }

private:
    bool CreateViewportRenderTarget();

    FActor*      SelectedActor;
    FLight*      SelectedLight;
    FCamera*     SelectedCamera;
    FLightProbe* SelectedLightProbe;

    // Editor Interface
    TSharedPtr<FEditorDockspaceWidget>         DockspaceWidget;
    TSharedPtr<FEditorFooterWidget>            FooterWidget;
    TSharedPtr<FEditorLogOutputWidget>         LogOutputWidget;
    TSharedPtr<FEditorViewportWidget>          ViewportWidget;
    TSharedPtr<FEditorSceneHierarchyWidget>    SceneHierarchyWidget;
    TSharedPtr<FEditorPropertiesWidget>        PropertiesWidget;
    TSharedPtr<FEditorContentBrowserWidget>    ContentBrowserWidget;
    FRHITextureRef                             ViewportImage;
    FIntVector2                                ViewportImageSize;
};