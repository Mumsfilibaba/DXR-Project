#pragma once
#include "Core/Containers/Array.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

class FEditorEngine;
class FActor;
class FActorFilter;

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

    bool DrawFilterRow(FActorFilter* Filter, float Indent);
    void DrawFilterSubtree(FActorFilter* Filter, float Indent);
    void DrawMoveToFilterMenu(FActorFilter* Filter, const TArray<FActor*>& TargetActors, FActorFilter* TargetFilter);

    void RebuildFilterContents();
    const TArray<FActor*>* GetFilterContents(FActorFilter* Filter) const;
    bool FilterSubtreeHasActors(FActorFilter* Filter) const;

    bool PassesSearchFilter(FActor* Actor) const;

    bool IsActorExpanded(FActor* Actor) const;
    void SetActorExpanded(FActor* Actor, bool bExpanded);

    bool IsFilterExpanded(FActorFilter* Filter) const;
    void SetFilterExpanded(FActorFilter* Filter, bool bExpanded);

    void RequestAttachment(FActor* ChildActor, FActor* ParentActor);
    void RequestAttachment(const TArray<FActor*>& ChildActors, FActor* ParentActor);
    void ApplyPendingAttachment();

    void BeginFilterRename(FActorFilter* Filter);
    void RequestFilterAssignment(FActor* Actor, FActorFilter* Filter);
    void RequestFilterAssignment(const TArray<FActor*>& Actors, FActorFilter* Filter);
    void RequestFilterReparent(FActorFilter* Filter, FActorFilter* ParentFilter);
    void ApplyPendingFilterOperations();

    void RequestRangeSelection(FActor* TargetActor, bool bAdditive);
    void ApplyPendingRangeSelection();

    TArray<FActor*> GetActorsForOperation(FActor* Actor) const;

    FEditorEngine*          EditorEngine;
    FActor*                 RenamingActor;
    FActor*                 PendingAttachParent;
    FActor*                 SelectionAnchor;
    FActor*                 PendingRangeTarget;
    FActorFilter*           SelectedFilter;
    FActorFilter*           RenamingFilter;
    FActorFilter*           PendingDestroyFilter;
    FActorFilter*           PendingAssignFilter;
    FActorFilter*           PendingCreateParent;
    FActorFilter*           PendingReparentFilter;
    FActorFilter*           PendingReparentParent;
    FDelegateHandle         ImGuiDelegateHandle;
    TArray<FActor*>         PendingAttachChildren;
    TArray<FActor*>         PendingAssignActors;
    TArray<FActor*>         CollapsedActors;
    TArray<FActor*>         VisibleActorOrder;
    TArray<FActorFilter*>   CollapsedFilters;
    TArray<TArray<FActor*>> FilterContents;
    TStaticArray<CHAR, 256> ActorSearchFilterBuffer;
    TStaticArray<CHAR, 256> RenameBuffer;
    TStaticArray<CHAR, 256> RenameBufferOriginal;
    bool                    bVisible;
    bool                    bRequestRenameFocus;
    bool                    bSelectionActiveInTable;
    bool                    bPendingAttachment;
    bool                    bPendingFilterCreate;
    bool                    bPendingFilterAssign;
    bool                    bPendingFilterReparent;
    bool                    bPendingRangeSelect;
    bool                    bPendingRangeAdditive;
    bool                    bDragHoveringSourceRow;
};
