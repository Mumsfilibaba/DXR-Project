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
class FEditorAboutWidget;
class FCameraComponent;

class ENGINE_API FEditorEngine : public FEngine
{
public:
    FEditorEngine();
    virtual ~FEditorEngine();

    // FEngine Interface
    virtual bool Init()                override final;
    virtual bool InitPostRenderer()    override final;
    virtual void Tick(float DeltaTime) override final;
    virtual void Release()             override final;

    virtual FSceneRenderPacket BuildRenderPacket() override final;

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
    const TSharedPtr<FEditorAboutWidget>&            GetAboutWidget()            const { return AboutWidget; }

    void SetSelectedActor(FActor* InActor);
    void ClearSelection();

    FCameraComponent* GetActiveViewportCamera() const;

    FActor* GetSelectedActor() const
    {
        return SelectedActor;
    }

private:
    static constexpr EFormat ViewportImageFormat = EFormat::R8G8B8A8_Unorm;

    bool CreateViewportRenderTarget();
    void OnActorRemoved(FActor* RemovedActor);

    FActor*                                   SelectedActor;
    FCameraComponent*                         LastViewportCamera;
    FDelegateHandle                           ActorRemovedDelegateHandle;
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
    TSharedPtr<FEditorAboutWidget>            AboutWidget;
    FRHITextureRef                            ViewportImage;
    IntVector2                                ViewportImageSize;
};
