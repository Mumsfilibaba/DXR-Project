#pragma once
#include "Core/Delegates/Delegate.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorStatsWidget
{
public:
    FEditorStatsWidget();
    ~FEditorStatsWidget();

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
    FDelegateHandle ImGuiDelegateHandle;
    bool            bVisible;
};
