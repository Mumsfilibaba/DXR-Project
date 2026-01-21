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
    void DrawSearchField(const char* InId, const char* InHint, TStaticArray<CHAR, 256>& InOutBuffer, float InWidth = -1.0f);
    void DrawCenteredMessage(const char* InText, const ImVec4& InMutedTextColor);
    void DrawContentHeaderBar();

    void DrawFolderTreeRecursive(FileInfo& InFolder, TArray<int32>& InPath, int32 InDepth, ImGuiStorage* InStorage, const ImVec4& InNameTextColor, const ImU32 InFolderActiveColor,
        const ImU32 InFolderInactiveColor, const ImU32 InFolderHoverColor, const ImU32 InFolderPathColor, bool bFolderSearchActive);
    bool DrawFolderRow(FileInfo& InFolder, const TArray<int32>& InPath, int32 InDepth, ImGuiStorage* InStorage, const ImVec4& InNameTextColor, const ImU32 InFolderActiveColor, 
        const ImU32 InFolderInactiveColor, const ImU32 InFolderHoverColor, const ImU32 InFolderPathColor, bool bFolderSearchActive);

    const CHAR* GetTrimmedQuery(const TStaticArray<CHAR, 256>& InBuf) const;
    bool MatchesSearch(const CHAR* InName, const TStaticArray<CHAR, 256>& InBuf) const;
    bool FolderTreeMatches(const FileInfo& InFolder) const;
    bool IsPathPrefixOfSelected(const TArray<int32>& InPath) const;
    bool HasChildFolders(const FileInfo& InFolder) const;

    FileInfo* GetFolderFromPath(const TArray<int32>& InPath);
    void BuildFolderPathString(const TArray<int32>& InPath, char* OutBuf, int32 OutBufSize) const;
    void NavigateToFolderPath(const TArray<int32>& InNewPath, bool bAddToHistory);
    void NavigateBack();
    void NavigateForward();
    bool ArePathsEqual(const TArray<int32>& A, const TArray<int32>& B) const;

private:
    FDelegateHandle         ImGuiDelegateHandle;
    TStaticArray<CHAR, 256> FolderSearchBuffer;
    TStaticArray<CHAR, 256> AssetSearchBuffer;
    int32                   SelectedFolderIndex;
    int32                   SelectedItemIndex;
    bool                    bSelectionActiveInBrowser;
    bool                    bVisible;

    // Folder navigation path (indices into RootFolders/FolderContents).
    // Example: [0]        -> RootFolders[0]
    //          [0, 2]     -> RootFolders[0].FolderContents[2]
    //          [0, 2, 1]  -> RootFolders[0].FolderContents[2].FolderContents[1]
    TArray<int32>         SelectedFolderPath;
    TArray<FileInfo>      RootFolders;
    TArray<TArray<int32>> BackHistory;
    TArray<TArray<int32>> ForwardHistory;
};
