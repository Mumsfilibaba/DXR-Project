#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/UniquePtr.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

struct FTreeItem;
class FActor;
class FActorFilter;
class FEditorSceneHierarchyView;
class FMenu;
class FSearchBox;
class FTreeView;

struct FHierarchyNode
{
    /** @brief The actor the row shows, or null on a filter row. */
    FActor* Actor = nullptr;

    /** @brief The filter the row shows, or null on an actor row. */
    FActorFilter* Filter = nullptr;
};

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
    void AddFilterItem(FActorFilter* Filter, const TSharedPtr<FTreeItem>& ParentItem, TArray<TSharedPtr<FTreeItem>>& OutRoots);
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
    void AddFilter(FActorFilter* ParentFilter);
    void AddMoveToFilterItems(const TSharedPtr<FMenu>& Menu, FActorFilter* Filter);
    void MoveSelectionToFilter(FActorFilter* Filter);

    NODISCARD TSharedPtr<FMenu> BuildRowContextMenu();
    NODISCARD TSharedPtr<FMenu> BuildMoveToFilterMenu();
    NODISCARD uint64 ComputeWorldRevision() const;
    NODISCARD TSharedPtr<FTreeItem> CreateItem(const String& ItemLabel, FActor* Actor, FActorFilter* Filter);
    NODISCARD FActorFilter* GetContextFilter() const;

    TSharedPtr<FEditorSceneHierarchyView>      HierarchyView;
    TSharedPtr<FTreeView>                      TreeView;
    TSharedPtr<FSearchBox>                     SearchBox;
    TArray<TSharedPtr<FHierarchyNode>>         Nodes;
    TMap<FActor*, TSharedPtr<FTreeItem>>       ItemsByActor;
    TMap<FActorFilter*, TSharedPtr<FTreeItem>> ItemsByFilter;
    FActorFilter*                              SelectedFilter;
    uint64                                     WorldRevision;
    bool                                       bIsSyncingSelection;
};
