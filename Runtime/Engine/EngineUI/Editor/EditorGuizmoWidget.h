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

    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

    bool IsVisible() const
    {
        return bVisible;
    }

private:
    void UpdateShortcuts(bool bViewportHovered);

private:
    FEditorEngine*           EditorEngine;
    FDelegateHandle          ImGuiEndFrameDelegateHandle;
    bool                     bVisible;
    EditorGuizmo::EOperation Operation;
    EditorGuizmo::EMode      Mode;
};

