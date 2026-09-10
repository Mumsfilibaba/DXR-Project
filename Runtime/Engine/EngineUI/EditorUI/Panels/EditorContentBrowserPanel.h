#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

struct FTreeItem;

class FEditorContentBrowserView;
class FHorizontalBox;
class FMenu;
class FSearchBox;
class FSplitter;
class FTextBlock;
class FTileView;
class FTreeView;

class ENGINE_API FEditorContentBrowserPanel final : public FEditorPanel
{
public:
    FEditorContentBrowserPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorContentBrowserPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;

private:
    struct FEntry
    {
        String         Name;
        bool           bIsFolder = false;
        TArray<FEntry> Children;
    };

    using FEntryPath = TArray<int32>;

    NODISCARD static String CreateUniqueName(const TArray<FEntry>& Siblings, const String& Name);

    void BuildPlaceholderTree();
    void RebuildTree();
    void RefreshTiles();
    void RefreshBreadcrumbs();
    void NavigateTo(const FEntryPath& NewPath, bool bAddToHistory);
    void NavigateBack();
    void NavigateForward();

    NODISCARD FEntry* FindEntry(const FEntryPath& Path);
    NODISCARD const FEntry* FindEntry(const FEntryPath& Path) const;
    NODISCARD TArray<FEntry>* FindChildArray(const FEntryPath& ParentPath);
    NODISCARD FEntryPath FindTreeItemPath(const TSharedPtr<FTreeItem>& Item) const;
    NODISCARD TSharedPtr<FTreeItem> FindTreeItem(const FEntryPath& Path) const;
    NODISCARD FEntryPath ResolveTileTarget(int32 TileIndex) const;

    void NewFolder();
    void RenameEntry(const FEntryPath& Path, const String& NewName);
    void DeleteEntries(const FEntryPath& ParentPath, const TArray<int32>& ChildIndices);
    void CopyEntries(const FEntryPath& ParentPath, const TArray<int32>& ChildIndices);
    void PasteEntries();
    void MoveEntries(const FEntryPath& SourceParentPath, const TArray<int32>& ChildIndices, const FEntryPath& TargetPath);
    void RequestDelete(const FEntryPath& ParentPath, const TArray<int32>& ChildIndices, bool bIsFolderPanel);

    NODISCARD bool HasClipboardContent() const;
    NODISCARD bool CanMoveInto(const FEntryPath& SourceParentPath, const TArray<int32>& ChildIndices, const FEntryPath& TargetPath) const;
    NODISCARD TArray<String> CollectEntryNames(const FEntryPath& ParentPath, const TArray<int32>& ChildIndices);

    void ReportFailure(const String& Message, const TArray<String>& FailedNames);

    NODISCARD TSharedPtr<FVisualElement> BuildFolderColumn();
    NODISCARD TSharedPtr<FVisualElement> BuildContentColumn();
    NODISCARD TSharedPtr<FVisualElement> BuildBreadcrumbBar();
    NODISCARD TSharedPtr<FMenu> BuildContextMenu(bool bIsFolderPanel);

    void OnFolderSelectionChanged(const TArray<TSharedPtr<FTreeItem>>& Selection);
    void OnFolderDragDetected(const TSharedPtr<FTreeItem>& Item, const FCursorEvent& CursorEvent);
    void OnTileActivated(int32 Index);
    void OnTileDragDetected(int32 Index, const FCursorEvent& CursorEvent);
    void OnFolderSearchTextChanged(const String& SearchText);
    void OnContentSearchTextChanged(const String& SearchText);
    void OnTreeContextMenu(int32 Target, const IntVector2& ScreenPosition);
    void OnGridContextMenu(int32 Target, const IntVector2& ScreenPosition);
    void OnTreeRenameCommitted(int32 Target, const String& NewName);
    void OnGridRenameCommitted(int32 Target, const String& NewName);
    void OnTreeRenameRequested();
    void OnGridRenameRequested();
    void OnTreeDeleteRequested();
    void OnGridDeleteRequested();
    void OnTreeDropped(const String& PayloadId, int32 Target);
    void OnGridDropped(const String& PayloadId, int32 Target);

    NODISCARD FRectangle GetTreeRowBounds(int32 Target) const;
    NODISCARD FRectangle GetTreeLabelBounds(int32 Target) const;
    NODISCARD FRectangle GetTileBounds(int32 Target) const;
    NODISCARD FRectangle GetTileLabelBounds(int32 Target) const;
    NODISCARD int32 HitTestTree(const IntVector2& ClientPosition) const;
    NODISCARD int32 HitTestGrid(const IntVector2& ClientPosition) const;
    NODISCARD bool CanDropOnTreeRow(int32 Target) const;
    NODISCARD bool CanDropOnTile(int32 Target) const;
    NODISCARD String GetTreeLabel(int32 Target) const;
    NODISCARD String GetTileLabel(int32 Target) const;

    TSharedPtr<FSplitter>                 Splitter;
    TSharedPtr<FEditorContentBrowserView> TreeWrapper;
    TSharedPtr<FEditorContentBrowserView> GridWrapper;
    TSharedPtr<FTreeView>                 FolderTree;
    TSharedPtr<FTileView>                 TileView;
    TSharedPtr<FSearchBox>                FolderSearchBox;
    TSharedPtr<FSearchBox>                ContentSearchBox;
    TSharedPtr<FHorizontalBox>            BreadcrumbBar;
    TSharedPtr<FTextBlock>                EmptyStateText;
    TArray<FEntry>                        Roots;
    FEntryPath                            CurrentPath;
    TArray<FEntryPath>                    BackHistory;
    TArray<FEntryPath>                    ForwardHistory;
    TMap<const FTreeItem*, FEntryPath>    TreeItemPaths;
    TArray<int32>                         VisibleChildIndices;
    FEntryPath                            DragParentPath;
    TArray<int32>                         DragChildIndices;
    FEntryPath                            ClipboardParentPath;
    TArray<int32>                         ClipboardChildIndices;
    String                                FolderFilterText;
    String                                ContentFilterText;
};
