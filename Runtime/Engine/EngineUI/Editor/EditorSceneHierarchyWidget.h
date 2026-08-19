#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/Set.h"
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
    struct FHierarchyRow
    {
        FActor*       Actor  = nullptr;
        FActorFilter* Filter = nullptr;
        float         Indent = 0.0f;
    };

    void RebuildVisibleRows();
    void AppendFilterRows(FActorFilter* Filter, float Indent);
    void AppendActorRows(FActor* Actor, float Indent);

    void DrawActorRow(FActor* Actor, float Indent);
    void DrawFilterRow(FActorFilter* Filter, float Indent);
    void DrawMoveToFilterMenu(FActorFilter* Filter, const TArray<FActor*>& TargetActors, FActorFilter* TargetFilter);

    void RebuildFilterBuckets();
    const TArray<FActor*>* GetFilterContents(FActorFilter* Filter) const;
    bool FilterSubtreeHasActors(FActorFilter* Filter) const;

    void RebuildSearchMatches(const TArray<FActor*>& Actors);
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

    FEditorEngine*             EditorEngine;
    FActor*                    RenamingActor;
    FActor*                    PendingAttachParent;
    FActor*                    SelectionAnchor;
    FActor*                    PendingRangeTarget;
    FActorFilter*              SelectedFilter;
    FActorFilter*              RenamingFilter;
    FActorFilter*              PendingDestroyFilter;
    FActorFilter*              PendingAssignFilter;
    FActorFilter*              PendingCreateParent;
    FActorFilter*              PendingReparentFilter;
    FActorFilter*              PendingReparentParent;
    FDelegateHandle            ImGuiDelegateHandle;
    TArray<FActor*>            PendingAttachChildren;
    TArray<FActor*>            PendingAssignActors;
    TArray<FActor*>            VisibleActorOrder;
    TArray<FActor*>            RootLevelActors;
    TArray<FHierarchyRow>      VisibleRows;
    TArray<TArray<FActor*>>    FilterContents;
    TSet<FActor*>              CollapsedActors;
    TSet<FActorFilter*>        CollapsedFilters;
    TSet<FActor*>              LiveActors;
    TSet<FActor*>              MatchingActors;
    TMap<FActorFilter*, int32> FilterIndices;
    TStaticArray<CHAR, 256>    CachedQuery;
    TStaticArray<CHAR, 256>    ActorSearchFilterBuffer;
    TStaticArray<CHAR, 256>    RenameBuffer;
    TStaticArray<CHAR, 256>    RenameBufferOriginal;
    bool                       bVisible;
    bool                       bRequestRenameFocus     : 1;
    bool                       bSelectionActiveInTable : 1;
    bool                       bPendingAttachment      : 1;
    bool                       bPendingFilterCreate    : 1;
    bool                       bPendingFilterAssign    : 1;
    bool                       bPendingFilterReparent  : 1;
    bool                       bPendingRangeSelect     : 1;
    bool                       bPendingRangeAdditive   : 1;
    bool                       bDragHoveringSourceRow  : 1;
};
