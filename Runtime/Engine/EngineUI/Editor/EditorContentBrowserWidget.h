#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include <imgui.h>

struct ImGuiStorage;

class FEditorContentBrowserWidget
{
public:
    FEditorContentBrowserWidget();
    ~FEditorContentBrowserWidget();

    void Draw();

    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

    bool IsVisible() const
    {
        return bVisible;
    }

private:

    // NOTE: Remove this when we actually search the file tree
    struct FileInfo
    {
        const CHAR*      Name;
        bool             bIsFolder;
        TArray<FileInfo> FolderContents;
    };

private:
    void DrawLayoutTable();
    void DrawFolderPanel();
    void DrawContentPanel();
    void DrawItemTooltip(const FileInfo& InItem);
    void DrawContentGrid();
    void DrawSearchField(const CHAR* InId, const CHAR* InHint, TStaticArray<CHAR, 256>& InOutBuffer, float InWidth = -1.0f);
    void DrawCenteredMessage(const CHAR* InText, const ImVec4& InMutedTextColor);
    void DrawContentHeaderBar();
    void DrawContentHeaderArea(const ImVec4& InBackGround, float InSidePadding, float InSearchRowHeight);
    void DrawFolderTreeRecursive(FileInfo& InFolder, TArray<int32>& InPath, int32 InDepth, ImGuiStorage* InStorage, const ImVec4& InNameTextColor, const ImU32 InFolderActiveColor,
        const ImU32 InFolderInactiveColor, const ImU32 InFolderHoverColor, const ImU32 InFolderPathColor, bool bFolderSearchActive);
    bool DrawFolderRow(FileInfo& InFolder, const TArray<int32>& InPath, int32 InDepth, ImGuiStorage* InStorage, const ImVec4& InNameTextColor, const ImU32 InFolderActiveColor, 
        const ImU32 InFolderInactiveColor, const ImU32 InFolderHoverColor, const ImU32 InFolderPathColor, bool bFolderSearchActive);

    void ResetDragPreviewState();
    void ClearItemSelection();
    void SelectSingleItem(int32 InIndex);
    void ToggleItemSelection(int32 InIndex);
    void SelectItemRange(int32 InStartIndex, int32 InEndIndex, bool bAddToExisting);
    bool IsItemSelected(int32 InIndex) const;
    bool MoveItemToFolder(const TArray<int32>& InSourceParentPath, int32 InSourceIndex, const TArray<int32>& InTargetFolderPath);
    bool MoveItemsToFolder(const TArray<int32>& InSourceParentPath, const TArray<int32>& InSourceIndices, const TArray<int32>& InTargetFolderPath);

    const CHAR* GetTrimmedQuery(const TStaticArray<CHAR, 256>& InBuf) const;
    bool MatchesSearch(const CHAR* InName, const TStaticArray<CHAR, 256>& InBuf) const;
    bool FolderTreeMatches(const FileInfo& InFolder) const;
    bool IsPathPrefixOfSelected(const TArray<int32>& InPath) const;
    bool HasChildFolders(const FileInfo& InFolder) const;

    FileInfo* GetFolderFromPath(const TArray<int32>& InPath);
    void BuildFolderPathString(const TArray<int32>& InPath, CHAR* OutBuf, int32 OutBufSize) const;
    void NavigateToFolderPath(const TArray<int32>& InNewPath, bool bAddToHistory);
    void NavigateBack();
    void NavigateForward();
    bool ArePathsEqual(const TArray<int32>& PathA, const TArray<int32>& PathB) const;

private:
    FDelegateHandle         ImGuiDelegateHandle;
    TStaticArray<CHAR, 256> FolderSearchBuffer;
    TStaticArray<CHAR, 256> AssetSearchBuffer;
    int32                   SelectedFolderIndex;
    TArray<int32>           SelectedItemIndices;
    int32                   LastSelectedItemIndex;
    bool                    bSelectionActiveInBrowser;
    bool                    bVisible;

    bool                    bPendingMove;
    TArray<int32>           PendingMoveSourceParentPath;
    TArray<int32>           PendingMoveSourceIndices;
    TArray<int32>           PendingMoveTargetFolderPath;

    bool                    bDragPreviewActive;
    bool                    bDragPreviewInvalidSelfMove;
    ImTextureID             DragPreviewIcon;
    bool                    bDragPreviewIsFolder;
    int32                   DragPreviewSelectionCount;
    CHAR                    DragPreviewSourceName[256];
    CHAR                    DragPreviewTargetName[256];

    // Folder navigation path (indices into RootFolders/FolderContents).
    // Example: [0]        -> RootFolders[0]
    //          [0, 2]     -> RootFolders[0].FolderContents[2]
    //          [0, 2, 1]  -> RootFolders[0].FolderContents[2].FolderContents[1]
    TArray<int32>           SelectedFolderPath;
    TArray<FileInfo>        RootFolders;
    TArray<TArray<int32>>   BackHistory;
    TArray<TArray<int32>>   ForwardHistory;
};
