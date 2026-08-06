#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorAboutWidget
{
public:
    FEditorAboutWidget();
    ~FEditorAboutWidget();

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
    void DrawTitle();
    void DrawSourceInfo();
    void DrawBuildInfo();
    void DrawGraphicsInfo();
    void DrawContextMenu();
    void DrawProperty(const CHAR* Label, const CHAR* ValueText);

    FDelegateHandle ImGuiDelegateHandle;
    bool            bVisible;
    String          ContextRowLabel;
    String          ContextRowValue;
};
