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

	int32 SelectedFolderIndex       = 0;
	int32 SelectedItemIndex         = -1;
	bool  bSelectionActiveInBrowser = false;
	bool  bVisible;
};
