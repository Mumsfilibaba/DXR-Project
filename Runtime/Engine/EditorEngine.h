#pragma once
#include "Engine/Engine.h"

class FEditorDockspaceWidget;
class FEditorFooterWidget;
class FEditorOutputLogWidget;
class FEditorViewportWidget;
class FEditorSceneHierarchyWidget;
class FEditorPropertiesWidget;
class FEditorContentBrowserWidget;
class FEditorGuizmoWidget;
class FEditorRendererSettingsWidget;
class FEditorGPUProfilerWidget;
class FEditorFrameProfilerWidget;
class FEditorRHIInfoWidget;
class FEditorStatsWidget;

class ENGINE_API FEditorEngine : public FEngine
{
public:
    FEditorEngine();
    virtual ~FEditorEngine();

    // FEngine Interface
    virtual bool Init()             override final;
    virtual bool InitPostRenderer() override final;
    virtual void Release()          override final;

    virtual void Tick(float DeltaTime) override final;
    virtual void RenderFrame()         override final;

    // Editor Widgets
    const TSharedPtr<FEditorDockspaceWidget>&        GetDockspaceWidget()        const { return DockspaceWidget; }
    const TSharedPtr<FEditorFooterWidget>&           GetFooterWidget()           const { return FooterWidget; }
    const TSharedPtr<FEditorOutputLogWidget>&        GetOutputLogWidget()        const { return OutputLogWidget; }
    const TSharedPtr<FEditorViewportWidget>&         GetEditorViewportWidget()   const { return ViewportWidget; }
    const TSharedPtr<FEditorSceneHierarchyWidget>&   GetSceneHierarchyWidget()   const { return SceneHierarchyWidget; }
    const TSharedPtr<FEditorPropertiesWidget>&       GetPropertiesWidget()       const { return PropertiesWidget; }
    const TSharedPtr<FEditorContentBrowserWidget>&   GetContentBrowserWidget()   const { return ContentBrowserWidget; }
    const TSharedPtr<FEditorGuizmoWidget>&           GetGuizmoWidget()           const { return GuizmoWidget; }
    const TSharedPtr<FEditorRendererSettingsWidget>& GetRendererSettingsWidget() const { return RendererSettingsWidget; }
    const TSharedPtr<FEditorGPUProfilerWidget>&      GetGPUProfilerWidget()      const { return GPUProfilerWidget; }
    const TSharedPtr<FEditorFrameProfilerWidget>&    GetFrameProfilerWidget()    const { return FrameProfilerWidget; }
    const TSharedPtr<FEditorRHIInfoWidget>&          GetRHIInfoWidget()          const { return RHIInfoWidget; }
    const TSharedPtr<FEditorStatsWidget>&            GetStatsWidget()            const { return StatsWidget; }

    void SetSelectedActor(FActor* InActor);
    void SetSelectedLight(FLight* InLight);
    void SetSelectedCamera(FCamera* InCamera);
    void SetSelectedLightProbe(FLightProbe* InProbe);
    void ClearSelection();

    FActor*      GetSelectedActor()      const { return SelectedActor; }
    FLight*      GetSelectedLight()      const { return SelectedLight; }
    FCamera*     GetSelectedCamera()     const { return SelectedCamera; }
    FLightProbe* GetSelectedLightProbe() const { return SelectedLightProbe; }

private:
    static constexpr EFormat ViewportImageFormat = EFormat::R8G8B8A8_Unorm;

    bool CreateViewportRenderTarget();

    FActor*      SelectedActor;
    FLight*      SelectedLight;
    FCamera*     SelectedCamera;
    FLightProbe* SelectedLightProbe;

    // Editor Interface
    TSharedPtr<FEditorDockspaceWidget>        DockspaceWidget;
    TSharedPtr<FEditorFooterWidget>           FooterWidget;
    TSharedPtr<FEditorOutputLogWidget>        OutputLogWidget;
    TSharedPtr<FEditorViewportWidget>         ViewportWidget;
    TSharedPtr<FEditorSceneHierarchyWidget>   SceneHierarchyWidget;
    TSharedPtr<FEditorPropertiesWidget>       PropertiesWidget;
    TSharedPtr<FEditorContentBrowserWidget>   ContentBrowserWidget;
    TSharedPtr<FEditorGuizmoWidget>           GuizmoWidget;
    TSharedPtr<FEditorRendererSettingsWidget> RendererSettingsWidget;
    TSharedPtr<FEditorGPUProfilerWidget>      GPUProfilerWidget;
    TSharedPtr<FEditorFrameProfilerWidget>    FrameProfilerWidget;
    TSharedPtr<FEditorRHIInfoWidget>          RHIInfoWidget;
    TSharedPtr<FEditorStatsWidget>            StatsWidget;
    FRHITextureRef                            ViewportImage;
    FIntVector2                               ViewportImageSize;
};
