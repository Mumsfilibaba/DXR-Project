#pragma once
#include "Core/Delegates/Delegate.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorRHIInfoWidget
{
public:
    FEditorRHIInfoWidget();
    ~FEditorRHIInfoWidget();

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
    void DrawAdapterInfo();
    void DrawBudgetSection();
    void DrawCommandSubmission();
    void DrawResourceMemory();
    void DrawAllocatorDetails();

    FDelegateHandle ImGuiDelegateHandle;
    bool            bVisible;
};
