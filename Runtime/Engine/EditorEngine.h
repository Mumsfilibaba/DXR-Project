#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Engine/Engine.h"
#include "Engine/World/WorldSnapshot.h"

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
class FEditorRenderGraphWidget;
class FEditorRHIInfoWidget;
class FEditorStatsWidget;
class FEditorAboutWidget;
class FEditorShell;
class FCameraComponent;
struct IEditorViewportHost;

enum class EEditorPickPurpose : uint8
{
    Selection,
    ContextMenu,
};

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

    virtual bool StartPlay() override final;
    virtual void StopPlay()  override final;

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
    const TSharedPtr<FEditorRenderGraphWidget>&      GetRenderGraphWidget()      const { return RenderGraphWidget; }
    const TSharedPtr<FEditorRHIInfoWidget>&          GetRHIInfoWidget()          const { return RHIInfoWidget; }
    const TSharedPtr<FEditorStatsWidget>&            GetStatsWidget()            const { return StatsWidget; }
    const TSharedPtr<FEditorAboutWidget>&            GetAboutWidget()            const { return AboutWidget; }

    const TSharedPtr<FEditorShell>& GetEditorShell() const { return EditorShell; }
    bool IsUsingCustomEditorUI() const { return bUseCustomEditorUI; }

    void SetSelectedActor(FActor* InActor);
    void SetSelectedActors(const TArray<FActor*>& InActors);
    void AddSelectedActor(FActor* InActor);
    void RemoveSelectedActor(FActor* InActor);
    void ToggleSelectedActor(FActor* InActor);
    void ClearSelection();

    bool IsActorSelected(FActor* InActor) const;

    uint64 RequestPick(uint32 PixelX, uint32 PixelY, EEditorPickPurpose Purpose);

    void RequestDeleteActor(FActor* InActor);
    void RequestDeleteActors(const TArray<FActor*>& InActors);

    void SetPendingPickAdditive(bool bAdditive)
    {
        bPendingPickAdditive = bAdditive;
    }

    void SetPendingRectPickAdditive(bool bAdditive)
    {
        bPendingRectPickAdditive = bAdditive;
    }

    FCameraComponent* GetActiveViewportCamera() const;

    FActor* GetSelectedActor() const
    {
        return SelectedActor;
    }

    const TArray<FActor*>& GetSelectedActors() const
    {
        return SelectedActors;
    }

    /** @return The viewport of whichever UI stack was built, which is never null once Init has succeeded. */
    NODISCARD FORCEINLINE IEditorViewportHost* GetViewportHost() const
    {
        return ViewportHost;
    }

protected:
    // FEngine Interface
    virtual IntVector2 GetSceneRenderSize() const override final;
    virtual void SetSceneRenderTarget(const FRHITextureRef& InViewportImage) override final;

private:
    void OnActorRemoved(FActor* RemovedActor);
    void DrainPendingDestroyActors();

    EEditorPickPurpose ConsumePickPurpose(uint64 RequestId);

    FActor*                                   SelectedActor;
    TArray<FActor*>                           SelectedActors;
    TArray<FActor*>                           PendingDestroyActors;
    FCameraComponent*                         LastViewportCamera;
    FDelegateHandle                           ActorRemovedDelegateHandle;
    uint64                                    NextPickRequestId;
    TArray<TPair<uint64, EEditorPickPurpose>> PendingPickPurposes;
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
    TSharedPtr<FEditorRenderGraphWidget>      RenderGraphWidget;
    TSharedPtr<FEditorRHIInfoWidget>          RHIInfoWidget;
    TSharedPtr<FEditorStatsWidget>            StatsWidget;
    TSharedPtr<FEditorAboutWidget>            AboutWidget;
    TSharedPtr<FEditorShell>                  EditorShell;
    IEditorViewportHost*                      ViewportHost;
    FWorldSnapshot                            Snapshot;
    bool                                      bUseCustomEditorUI;
    bool                                      bPendingPickAdditive;
    bool                                      bPendingRectPickAdditive;
};
