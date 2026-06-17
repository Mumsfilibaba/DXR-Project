#pragma once
#include "Core/Containers/String.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiCore.h"

struct ImGuiStorage;

class FEditorContentBrowserWidget
{
    // NOTE: Remove this when we actually search the file tree
    struct FileInfo
    {
        String           Name;
        bool             bIsFolder;
        TArray<FileInfo> FolderContents;
    };

    struct FFolderMoveRequest
    {
        TArray<int32> SourceParentPath;
        TArray<int32> SourceIndices;
        TArray<int32> TargetFolderPath;
    };

    struct FCBDndPayload
    {
        int32                   Depth;
        TStaticArray<int32, 32> Indices;
        int32                   SourceIndex;
        bool                    bIsFolder;
    };

    struct FCBFolderDndPayload
    {
        int32                   Depth;
        TStaticArray<int32, 32> Indices;
    };

public:
    FEditorContentBrowserWidget();
    ~FEditorContentBrowserWidget();

    void Draw();

    bool IsVisible() const
    {
        return bVisible;
    }
    
    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

private:
    static FileInfo DeepCopyFileInfo(const FileInfo& In);

    void DrawLayoutTable();
    void DrawFolderPanel();
    void DrawContentPanel();
    void DrawItemTooltip(const FileInfo& InItem);
    void DrawContentGrid();
    void DrawCenteredMessage(const CHAR* InText, const ImVec4& InMutedTextColor);
    void DrawContentHeaderBar();
    void DrawContentHeaderArea(const ImVec4& InBackGround, float InSidePadding, float InSearchRowHeight);
    void DrawFolderTreeRecursive(FileInfo& InFolder, TArray<int32>& InPath, int32 InDepth, ImGuiStorage* InStorage, const ImVec4& InNameTextColor, const ImU32 InFolderActiveColor,
        const ImU32 InFolderInactiveColor, const ImU32 InFolderHoverColor, const ImU32 InFolderPathColor, const CHAR* InFolderSearchQuery);
    bool DrawFolderRow(FileInfo& InFolder, const TArray<int32>& InPath, int32 InDepth, ImGuiStorage* InStorage, const ImVec4& InNameTextColor, const ImU32 InFolderActiveColor, 
        const ImU32 InFolderInactiveColor, const ImU32 InFolderHoverColor, const ImU32 InFolderPathColor, const CHAR* InFolderSearchQuery);

    void BeginFolderRename(const TArray<int32>& InPath, const FileInfo& InFolder);
    void CommitFolderRename();
    void CancelFolderRename();
    void BeginItemRename(const TArray<int32>& InParentPath, int32 InIndex, const FileInfo& InItem);
    void CommitItemRename();
    void CancelItemRename();
    bool IsRenamingFolderPath(const TArray<int32>& InPath) const;
    bool IsRenamingItem(const TArray<int32>& InParentPath, int32 InIndex) const;

    void ResetDragPreviewState();
    void ClearItemSelection();
    void SelectSingleItem(int32 InIndex);
    void ToggleItemSelection(int32 InIndex);
    void SelectItemRange(int32 InStartIndex, int32 InEndIndex, bool bAddToExisting);
    bool IsItemSelected(int32 InIndex) const;
    bool MoveItemsToFolder(const TArray<int32>& InSourceParentPath, const TArray<int32>& InSourceIndices, const TArray<int32>& InTargetFolderPath);
    void ReportFailedMove(const String& InFullPath);

    bool MatchesSearch(const CHAR* InName, const CHAR* InQuery) const;
    bool FolderTreeMatches(const FileInfo& InFolder, const CHAR* InQuery) const;
    bool IsPathPrefixOfSelected(const TArray<int32>& InPath) const;
    bool HasChildFolders(const FileInfo& InFolder) const;

    FileInfo* GetFolderFromPath(const TArray<int32>& InPath);
    const FileInfo* GetFolderFromPath(const TArray<int32>& InPath) const;
    void BuildFolderPathString(const TArray<int32>& InPath, CHAR* OutBuf, int32 OutBufSize) const;
    void NavigateToFolderPath(const TArray<int32>& InNewPath, bool bAddToHistory);
    void NavigateBack();
    void NavigateForward();
    bool ArePathsEqual(const TArray<int32>& PathA, const TArray<int32>& PathB) const;
    bool IsPathPrefix(const TArray<int32>& Prefix, const TArray<int32>& Full) const;
    int32 FindPathIndex(const TArray<TArray<int32>>& Paths, const TArray<int32>& Path) const;
    bool ContainsPath(const TArray<TArray<int32>>& Paths, const TArray<int32>& Path) const;
    void AddUniquePath(TArray<TArray<int32>>& Paths, const TArray<int32>& Path) const;
    void RemovePath(TArray<TArray<int32>>& Paths, const TArray<int32>& Path) const;
    TArray<TArray<int32>> BuildUniquePaths(const TArray<TArray<int32>>& InPaths) const;
    TArray<TArray<int32>> RemoveRootPaths(const TArray<TArray<int32>>& InPaths) const;
    TArray<TArray<int32>> FilterTopLevelPaths(const TArray<TArray<int32>>& InPaths) const;
    TArray<TArray<int32>> GetFilteredFolderSelectionPaths() const;
    void QueueFolderMoveRequests(const TArray<TArray<int32>>& DragPaths, const TArray<int32>& TargetPath);
    void BuildDragSourceSelection(const FCBDndPayload& Data, TArray<int32>& OutSourceParentPath, const FileInfo*& OutSourceParentFolder, TArray<int32>& OutSourceIndices, TArray<TArray<int32>>& OutSourceFolderPaths) const;
    void AppendFolderPayloadPath(const ImGuiPayload* Payload, TArray<TArray<int32>>& InOutPaths) const;
    int32 FindChildFolderIndexByName(const FileInfo& ParentFolder, const String& FolderName) const;
    int32 FindChildFileIndexByName(const FileInfo& ParentFolder, const String& FileName) const;
    void UpdateDragPreviewNameConflicts(const TArray<TArray<int32>>& SourceFolderPaths, const FileInfo* SourceParentFolder, const TArray<int32>* SourceIndices, const TArray<int32>* SourceParentPath, const TArray<int32>& TargetPath);
    void SetDragPreviewTarget(const CHAR* TargetName, const TArray<int32>& TargetPath, const TArray<TArray<int32>>& SourceFolderPaths, const FileInfo* SourceParentFolder, const TArray<int32>* SourceIndices, const TArray<int32>* SourceParentPath);
    bool MergeFolderContents(FileInfo& TargetFolder, FileInfo& SourceFolder, const String& SourceFolderPath);

    void AddNewFolderInCurrentPath();
    void DeleteSelectedContentItems();
    void DeleteSelectedFolderInTree();
    void CopySelectedContent();
    void CopySelectedFolderPaths();
    bool HasClipboardContent() const;
    void PasteInCurrentFolder();

private:
    FDelegateHandle            ImGuiDelegateHandle;
    TStaticArray<CHAR, 256>    FolderSearchBuffer;
    TStaticArray<CHAR, 256>    AssetSearchBuffer;
    TArray<int32>              SelectedItemIndices;
    int32                      LastSelectedItemIndex;
    bool                       bSelectionActiveInBrowser;
    bool                       bVisible;
    float                      FolderPanelWidth;

    bool                       bPendingMove;
    TArray<int32>              PendingMoveSourceParentPath;
    TArray<int32>              PendingMoveSourceIndices;
    TArray<int32>              PendingMoveTargetFolderPath;
    TArray<FFolderMoveRequest> PendingFolderMoves;

    TArray<TArray<int32>>      FolderSelectionPaths;
    TArray<int32>              FolderSelectionAnchor;
    bool                       bFolderSelectionAnchorValid;
    TArray<TArray<int32>>      FolderVisiblePaths;
    TArray<int32>              LastActiveFolderPath;
    bool                       bHasLastActiveFolderPath;

    bool                       bDragPreviewActive;
    ImTextureID                DragPreviewIcon;
    bool                       bDragPreviewIsFolder;
    int32                      DragPreviewSelectionCount;
    bool                       bDragPreviewHasAnyLegalMove;
    int32                      DragPreviewIllegalMoveCount;
    TStaticArray<CHAR, 256>    DragPreviewSourceName;
    TStaticArray<CHAR, 256>    DragPreviewTargetName;

    TArray<int32>              RenamingFolderPath;
    TStaticArray<CHAR, 256>    FolderRenameBuffer;
    TStaticArray<CHAR, 256>    FolderRenameBufferOriginal;
    bool                       bRequestFolderRenameFocus;

    TArray<int32>              RenamingItemParentPath;
    int32                      RenamingItemIndex;
    TStaticArray<CHAR, 256>    ItemRenameBuffer;
    TStaticArray<CHAR, 256>    ItemRenameBufferOriginal;
    TStaticArray<CHAR, 64>     ItemRenameExtension;
    bool                       bRequestItemRenameFocus;

    // Folder navigation path (indices into RootFolders/FolderContents).
    // Example: [0]        -> RootFolders[0]
    //          [0, 2]     -> RootFolders[0].FolderContents[2]
    //          [0, 2, 1]  -> RootFolders[0].FolderContents[2].FolderContents[1]
    TArray<int32>              SelectedFolderPath;
    ErrorWindowContext         FailedMoveErrorContext;

    bool                       bClipboardValid;
    bool                       bClipboardFromContentPanel;
    TArray<int32>              ClipboardContentParentPath;
    TArray<int32>              ClipboardContentIndices;
    TArray<TArray<int32>>      ClipboardFolderPaths;

    ConfirmDialogContext       DeleteConfirmContext;
    bool                       bPendingDeleteContent;
    bool                       bPendingDeleteFolder;

    TArray<FileInfo>           RootFolders;
    TArray<TArray<int32>>      BackHistory;
    TArray<TArray<int32>>      ForwardHistory;
};
