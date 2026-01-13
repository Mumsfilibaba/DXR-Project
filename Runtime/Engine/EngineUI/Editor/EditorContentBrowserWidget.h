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
    bool            bVisible;
};
