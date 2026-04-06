#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorEngine;
class FActor;

class FEditorSceneHierarchyWidget
{
public:
    FEditorSceneHierarchyWidget(FEditorEngine* InEditorEngine);
    ~FEditorSceneHierarchyWidget();

    void Draw();
    void DrawSceneInfo();

    bool IsVisible() const
    {
        return bVisible;
    }
    
    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

private:
    void DrawActorRow(FActor* Actor, const CHAR* Type, const bool bSelected, float Indent);

    FEditorEngine*          EditorEngine;
    FActor*                 RenamingActor;
    FDelegateHandle         ImGuiDelegateHandle;
    TStaticArray<CHAR, 256> ActorSearchFilterBuffer;
    TStaticArray<CHAR, 256> ActorRenameBuffer;
    TStaticArray<CHAR, 256> ActorRenameBufferOriginal;
    bool                    bVisible;
    bool                    bRequestRenameFocus;
    bool                    bSelectionActiveInTable;
};
