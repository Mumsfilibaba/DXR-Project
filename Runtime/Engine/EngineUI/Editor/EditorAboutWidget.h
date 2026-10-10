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
    void DrawProperty(const CHAR_T* Label, const CHAR_T* ValueText);

    FDelegateHandle ImGuiDelegateHandle;
    bool            bVisible;
    FString         ContextRowLabel;
    FString         ContextRowValue;
};
