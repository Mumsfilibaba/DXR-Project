#pragma once
#include "Core/Containers/Array.h"
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
    void DrawActorRow(FActor* Actor, float Indent);
    void DrawChildActorRows(FActor* Actor, float Indent);
    
    bool PassesSearchFilter(FActor* Actor) const;

    bool IsActorExpanded(FActor* Actor) const;
    void SetActorExpanded(FActor* Actor, bool bExpanded);

    void RequestAttachment(FActor* ChildActor, FActor* ParentActor);
    void ApplyPendingAttachment();

    FEditorEngine*          EditorEngine;
    FActor*                 RenamingActor;
    FActor*                 PendingAttachChild;
    FActor*                 PendingAttachParent;
    FDelegateHandle         ImGuiDelegateHandle;
    TArray<FActor*>         CollapsedActors;
    TStaticArray<CHAR, 256> ActorSearchFilterBuffer;
    TStaticArray<CHAR, 256> ActorRenameBuffer;
    TStaticArray<CHAR, 256> ActorRenameBufferOriginal;
    bool                    bVisible;
    bool                    bRequestRenameFocus;
    bool                    bSelectionActiveInTable;
    bool                    bPendingAttachment;
    bool                    bDragHoveringSourceRow;
};
