#pragma once
#include "Core/Containers/Map.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

struct FTreeItem;

class FActor;
class FEditorSceneHierarchyView;
class FMenu;
class FSearchBox;
class FToolBar;
class FTreeView;

class ENGINE_API FEditorSceneHierarchyPanel final : public FEditorPanel
{
public:
    FEditorSceneHierarchyPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorSceneHierarchyPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;
    virtual void OnActorRemoved(FActor* Actor) override final;

private:
    void RebuildTree();
    void AddActorItem(FActor* Actor, const TSharedPtr<FTreeItem>& ParentItem, TArray<TSharedPtr<FTreeItem>>& OutRoots);
    void SyncSelectionFromEngine();
    void OnTreeSelectionChanged(const TArray<TSharedPtr<FTreeItem>>& Selection);
    void OnTreeItemActivated(const TSharedPtr<FTreeItem>& Item);
    void OnTreeDragDetected(const TSharedPtr<FTreeItem>& Item, const FCursorEvent& CursorEvent);
    void OnSearchTextChanged(const String& SearchText);
    void OnRowContextMenu(const TSharedPtr<FTreeItem>& Item, const IntVector2& ScreenPosition);
    void OnDeleteRequested();
    void OnRenameCommitted(const TSharedPtr<FTreeItem>& Item, const String& NewName);
    void OnItemsDropped(const TArray<TSharedPtr<FTreeItem>>& Items, const TSharedPtr<FTreeItem>& TargetItem);

    NODISCARD TSharedPtr<FToolBar> BuildToolBar();
    NODISCARD TSharedPtr<FMenu> BuildRowContextMenu();
    NODISCARD uint64 ComputeWorldRevision() const;

    TSharedPtr<FEditorSceneHierarchyView> HierarchyView;
    TSharedPtr<FTreeView>                 TreeView;
    TSharedPtr<FSearchBox>                SearchBox;
    TSharedPtr<FToolBar>                  ToolBar;
    TMap<FActor*, TSharedPtr<FTreeItem>>  ItemsByActor;
    uint64                                WorldRevision;
    bool                                  bIsSyncingSelection;
};
