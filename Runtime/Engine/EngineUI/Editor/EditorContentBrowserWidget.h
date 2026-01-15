#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

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
    FDelegateHandle ImGuiDelegateHandle;

    TStaticArray<CHAR, 256> FolderSearchBuffer;
    TStaticArray<CHAR, 256> AssetSearchBuffer;

    int32 SelectedFolderIndex;
    int32 SelectedItemIndex;
    bool  bSelectionActiveInBrowser;
    bool  bVisible;

    // Folder navigation path (indices into RootFolders/FolderContents).
    // Example: [0]        -> RootFolders[0]
    //          [0, 2]     -> RootFolders[0].FolderContents[2]
    //          [0, 2, 1]  -> RootFolders[0].FolderContents[2].FolderContents[1]
    TArray<int32> SelectedFolderPath;

    // NOTE: Remove this when we actually search the file tree
    struct FileInfo
    {
        const CHAR* Name;
        bool             bIsFolder;
        TArray<FileInfo> FolderContents;
    };

    TArray<FileInfo> RootFolders;
};
