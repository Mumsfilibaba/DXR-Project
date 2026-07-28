#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "Engine/EngineUI/Editor/EditorGuizmo.h"

class FEditorEngine;

class FEditorGuizmoWidget
{
public:
    FEditorGuizmoWidget(FEditorEngine* InEditorEngine);
    ~FEditorGuizmoWidget();

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
    void UpdateShortcuts(bool bViewportHovered);

    FEditorEngine*  EditorEngine;
    FDelegateHandle ImGuiEndFrameDelegateHandle;
    bool            bVisible;
};

