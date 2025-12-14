#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorEngine;

class FEditorPropertiesWidget
{
public:
    FEditorPropertiesWidget(FEditorEngine* InEditorEngine);
    ~FEditorPropertiesWidget();

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
    FEditorEngine*  EditorEngine;
    FDelegateHandle ImGuiDelegateHandle;
    bool            bVisible;
};
