#pragma once
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

#if EDITOR_BUILD
    #include "RendererCore/Debug/RenderGraphDebug.h"
#endif

class FGraphCanvas;
class FGraphModel;
class FTextBlock;
class FToolBar;

class ENGINE_API FEditorRenderGraphPanel final : public FEditorPanel
{
public:
    FEditorRenderGraphPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorRenderGraphPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;
    virtual void OnVisibilityChanged(bool bInIsVisible) override final;

private:
    void SetCaptureEnabled(bool bEnabled);

#if EDITOR_BUILD
    void RebuildModel(const FRenderGraphDebugSnapshot& Snapshot);
    void RefreshStatistics(const FRenderGraphDebugSnapshot& Snapshot);

    NODISCARD static FFloatColor GetPassTint(const FRenderGraphDebugPass& Pass);
    NODISCARD static FFloatColor GetResourceTint(const FRenderGraphDebugResource& Resource);
#endif

    NODISCARD TSharedPtr<FToolBar> BuildToolBar();

    TSharedPtr<FGraphCanvas> Canvas;
    TSharedPtr<FGraphModel>  Model;
    TSharedPtr<FTextBlock>   StatisticsText;
    TSharedPtr<FToolBar>     ToolBar;
    TArray<TArray<int32>>    PassAccessPinIds;
    bool                     bIsCapturing;
    bool                     bAutoLayoutPending;
};
