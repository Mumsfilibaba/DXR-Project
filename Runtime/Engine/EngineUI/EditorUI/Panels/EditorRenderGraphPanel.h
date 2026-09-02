#pragma once
#include "Engine/EngineUI/EditorUI/EditorPanel.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Math/Vector2.h"

#if EDITOR_BUILD
    #include "RendererCore/Debug/RenderGraphDebug.h"
#endif

class FGraphCanvas;
class FGraphModel;
class FTextBlock;

class ENGINE_API FEditorRenderGraphPanel final : public FEditorPanel
{
public:
    FEditorRenderGraphPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorRenderGraphPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;

private:
    struct FBuiltPass
    {
        FBuiltPass()
            : Name()
            , AccessPinIds()
            , LoadPinIds()
            , NodeId(-1)
        {
        }

        String        Name;
        TArray<int32> AccessPinIds;
        TArray<int32> LoadPinIds;
        int32         NodeId;
    };

#if EDITOR_BUILD
    NODISCARD static bool IsPassInactive(const FRenderGraphDebugPass& Pass);
    NODISCARD static String BuildNodeTitle(const FRenderGraphDebugPass& Pass);
    NODISCARD static bool HasIncomingLink(const FRenderGraphDebugSnapshot& Snapshot, int32 PassIndex, int32 AccessIndex);

    NODISCARD String BuildSignature(const FRenderGraphDebugSnapshot& Snapshot) const;
    bool RebuildModel(const FRenderGraphDebugSnapshot& Snapshot);
    void RefreshStatistics(const FRenderGraphDebugSnapshot& Snapshot);
#endif

    NODISCARD TSharedPtr<FVisualElement> BuildHeaderRow();

    void SetCaptureEnabled(bool bEnabled);
    void RunAutoLayout();

    TSharedPtr<FGraphCanvas> Canvas;
    TSharedPtr<FGraphModel>  Model;
    TSharedPtr<FTextBlock>   GraphNameText;
    TSharedPtr<FTextBlock>   StatisticsText;
    TMap<String, Vector2>    PositionsByPassName;
    TArray<FBuiltPass>       BuiltPasses;
    String                   BuiltSignature;
    bool                     bIsCapturing;
    bool                     bShowCulled;
    bool                     bNeedsFitView;
};
