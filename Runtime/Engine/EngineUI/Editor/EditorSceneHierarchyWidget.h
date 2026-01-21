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

    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

    bool IsVisible() const
    {
        return bVisible;
    }

private:
    void DrawActorRow(FActor* Actor, const char* Type, const bool bSelected, float IndentPx);

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
