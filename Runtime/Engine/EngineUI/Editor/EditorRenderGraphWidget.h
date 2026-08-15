#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Engine/EngineUI/Editor/EditorNodeGraph.h"
#include "ImGuiPlugin/ImGuiCore.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "RendererCore/Debug/RenderGraphDebug.h"

class FEditorRenderGraphWidget
{
public:
    FEditorRenderGraphWidget();
    ~FEditorRenderGraphWidget();

    void Draw();
    void DrawWindow();

    bool IsVisible() const
    {
        return bVisible;
    }

    void SetVisible(bool bInVisible);

private:
    void SyncCaptureEnabled() const;
    void EnsureNodeLayout(bool bForceAutoLayout);
    void BuildVisiblePassIndices(TArray<int32>& OutVisiblePassIndices) const;
    bool HasIncomingLink(int32 PassIndex, int32 AccessIndex) const;
    void EstimateNodeSizes(TArrayView<const int32> VisiblePassIndices, TArray<EditorNodeGraph::FNodeLayoutInput>& OutMeasured) const;
    bool TryRequestFitViewToNodes();

    FRenderGraphDebugSnapshot Snapshot;
    TMap<String, ImVec2>      PositionsByPassKey;
    TArray<ImVec2>            NodePositions;
    FDelegateHandle           ImGuiDelegateHandle;
    bool                      bVisible;
    bool                      bShowCulled;
    bool                      bForceAutoLayout;
    bool                      bNeedsFitView;
};
