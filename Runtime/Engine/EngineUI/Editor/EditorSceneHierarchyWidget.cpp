#include "Core/Misc/OutputDeviceLogger.h"
#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorActorFactory.h"
#include "Engine/World/ActorFilter.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/World.h"
#include "ImGuiPlugin/ImGuiCore.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

// Payload identifiers used when dragging a row onto another row in the outliner
static const CHAR* ActorDragDropPayloadId  = "SCENE_HIERARCHY_ACTOR";
static const CHAR* FilterDragDropPayloadId = "SCENE_HIERARCHY_FILTER";

// Shared by the actor and filter rows
static const ImVec4 RowHoverBg          = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
static const ImVec4 RowNameTextColor    = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
static const ImVec4 RowTypeTextColor    = ImVec4(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);
static const ImU32  RowSelectedActive   = IM_COL32(0, 112, 224, 255);
static const ImU32  RowSelectedInactive = IM_COL32(64, 87, 111, 255);
static const ImU32  RowDropTargetColor  = IM_COL32(0, 112, 224, 255);

// Shared by the actor and filter rename inputs
static const ImU32  RenameRowTint       = IM_COL32(0x3f, 0x7b, 0xb6, 160);
static const ImVec4 RenameInputBg       = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
static const ImU32  RenameBorderNormal  = IM_COL32(51, 51, 51, 255);
static const ImU32  RenameBorderHovered = IM_COL32(74, 74, 74, 255);
static const ImU32  RenameBorderActive  = IM_COL32(9, 92, 176, 255);

static const CHAR* GetActorTypeLabel(FActor* Actor)
{
    return Actor ? Actor->GetTypeLabel() : "Actor";
}

static bool DoesActorMatchQuery(FActor* Actor, const CHAR* Query)
{
    const String& Name = Actor->GetName();
    if (!Name.IsEmpty() && CString::Stristr(*Name, Query))
    {
        return true;
    }

    return CString::Stristr(GetActorTypeLabel(Actor), Query) != nullptr;
}

static bool DoesActorOrDescendantMatchQuery(FActor* Actor, const CHAR* Query)
{
    if (DoesActorMatchQuery(Actor, Query))
    {
        return true;
    }

    for (FActor* Child : Actor->GetChildActors())
    {
        if (Child && DoesActorOrDescendantMatchQuery(Child, Query))
        {
            return true;
        }
    }

    return false;
}

static bool CanAttachActorTo(FActor* DraggedActor, FActor* TargetActor)
{
    if (!DraggedActor || !TargetActor || DraggedActor == TargetActor)
    {
        return false;
    }

    // Attaching an actor to one of its own descendants would create a cycle
    if (TargetActor->IsAttachedTo(DraggedActor))
    {
        return false;
    }

    // Nothing to do when the actor already is a child of the drop-target
    return DraggedActor->GetParentActor() != TargetActor;
}

static bool CanReparentFilter(FActorFilter* DraggedFilter, FActorFilter* TargetFilter)
{
    if (!DraggedFilter || DraggedFilter == TargetFilter)
    {
        return false;
    }

    if (TargetFilter && TargetFilter->IsDescendantOf(DraggedFilter))
    {
        return false;
    }

    return DraggedFilter->GetParentFilter() != TargetFilter;
}

static FActor* GetDraggedActor(const ImGuiPayload* Payload)
{
    if (!Payload || !Payload->IsDataType(ActorDragDropPayloadId) || Payload->DataSize != static_cast<int32>(sizeof(FActor*)))
    {
        return nullptr;
    }

    return *reinterpret_cast<FActor* const*>(Payload->Data);
}

static FActorFilter* GetDraggedFilter(const ImGuiPayload* Payload)
{
    if (!Payload || !Payload->IsDataType(FilterDragDropPayloadId) || Payload->DataSize != static_cast<int32>(sizeof(FActorFilter*)))
    {
        return nullptr;
    }

    return *reinterpret_cast<FActorFilter* const*>(Payload->Data);
}

FEditorSceneHierarchyWidget::FEditorSceneHierarchyWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , RenamingActor(nullptr)
    , PendingAttachParent(nullptr)
    , SelectionAnchor(nullptr)
    , PendingRangeTarget(nullptr)
    , SelectedFilter(nullptr)
    , RenamingFilter(nullptr)
    , PendingDestroyFilter(nullptr)
    , PendingAssignFilter(nullptr)
    , PendingCreateParent(nullptr)
    , PendingReparentFilter(nullptr)
    , PendingReparentParent(nullptr)
    , PendingAttachChildren()
    , PendingAssignActors()
    , CollapsedActors()
    , VisibleActorOrder()
    , CollapsedFilters()
    , FilterContents()
    , bVisible(true)
    , bRequestRenameFocus(false)
    , bSelectionActiveInTable(false)
    , bPendingAttachment(false)
    , bPendingFilterCreate(false)
    , bPendingFilterAssign(false)
    , bPendingFilterReparent(false)
    , bPendingRangeSelect(false)
    , bPendingRangeAdditive(false)
    , bDragHoveringSourceRow(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorSceneHierarchyWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }

    ActorSearchFilterBuffer.Fill(0);
    RenameBuffer.Fill(0);
    RenameBufferOriginal.Fill(0);
}

FEditorSceneHierarchyWidget::~FEditorSceneHierarchyWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorSceneHierarchyWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::SceneHierarchyItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, EditorStyleVars::SceneHierarchyWindowPadding);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f));

    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("Scene Hierarchy", &bVisible, Flags))
    {
        DrawSceneInfo();
    }

    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void FEditorSceneHierarchyWidget::DrawSceneInfo()
{
    // -----------------------------------------------------------------------------------------
    // Engine / world checks
    // -----------------------------------------------------------------------------------------

    if (!EditorEngine)
    {
        ImGui::TextDisabled("No EditorEngine");
        return;
    }

    FWorld* World = EditorEngine->GetWorld();
    if (!World)
    {
        ImGui::TextDisabled("No World");
        return;
    }

    const bool bHasAnySelection = !EditorEngine->GetSelectedActors().IsEmpty();

    bDragHoveringSourceRow = false;

    // Rebuilt as the rows are drawn, so that a range-select can be resolved against the exact visual order
    VisibleActorOrder.Clear();

    const TArray<FActor*>& Actors = World->GetActors();

    // Drop the collapsed state of actors that have been removed from the world, their addresses could otherwise be
    // reused by a later actor that would then show up collapsed
    if (!CollapsedActors.IsEmpty())
    {
        CollapsedActors.RemoveAllSwap([&Actors](FActor* CollapsedActor)
        {
            return !Actors.Contains(CollapsedActor);
        });
    }

    // Same reasoning, an actor destroyed since the last frame must not be left dangling in the widget's own state
    if (RenamingActor && !Actors.Contains(RenamingActor))
    {
        RenamingActor       = nullptr;
        bRequestRenameFocus = false;
    }

    if (SelectionAnchor && !Actors.Contains(SelectionAnchor))
    {
        SelectionAnchor = nullptr;
    }

    const TArrayView<FActorFilter* const> Filters = World->GetActorFilters();

    // A filter destroyed since the last frame must not be left dangling in the selection or rename state
    if (SelectedFilter && !Filters.Contains(SelectedFilter))
    {
        SelectedFilter = nullptr;
    }

    if (RenamingFilter && !Filters.Contains(RenamingFilter))
    {
        RenamingFilter      = nullptr;
        bRequestRenameFocus = false;
    }

    // Same reasoning as the CollapsedActors scrub above, a destroyed filter's address could be reused
    if (!CollapsedFilters.IsEmpty())
    {
        CollapsedFilters.RemoveAllSwap([&Filters](FActorFilter* CollapsedFilter)
        {
            return !Filters.Contains(CollapsedFilter);
        });
    }

    RebuildFilterContents();

    TArray<FActor*> RootLevelActors;

    // Only root-actors are placed in a filter, every other actor is nested underneath its parent
    for (FActor* Actor : Actors)
    {
        if (!Actor || Actor->GetParentActor() || !PassesSearchFilter(Actor))
        {
            continue;
        }

        if (Actor->GetFilter() && Filters.Contains(Actor->GetFilter()))
        {
            continue;
        }

        RootLevelActors.Emplace(Actor);
    }

    // -----------------------------------------------------------------------------------------
    // Search Field
    // -----------------------------------------------------------------------------------------

    {
        const ImVec2 OldWindowPadding = ImGui::GetStyle().WindowPadding;
        const ImVec2 OldItemSpacing   = ImGui::GetStyle().ItemSpacing;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(OldWindowPadding.x, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(OldItemSpacing.x, 0.0f));

        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        EditorWidgets::DrawSearchField("##SceneHierarchySearch", "Search Actors", ActorSearchFilterBuffer.Data(), ActorSearchFilterBuffer.Size());
        ImGui::Dummy(ImVec2(0.0f, 12.0f));

        ImGui::PopStyleVar(2);
    }

    // -----------------------------------------------------------------------------------------
    // Actor Table
    // -----------------------------------------------------------------------------------------

    const ImGuiTableFlags TableFlags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_NoPadOuterX |
        ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp;

    ImGuiStyle& Style = ImGui::GetStyle();

    ImGui::PushStyleColor(ImGuiCol_TableRowBg,    ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f));

    const ImVec4 BorderDarkGray = ImVec4(37.0f / 255.0f, 37.0f / 255.0f, 37.0f / 255.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight,  BorderDarkGray);
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, BorderDarkGray);

    const float SavedCursorX = ImGui::GetCursorPosX();
    ImGui::SetCursorPosX(0.0f);

    const float TableHeight = ImGui::GetContentRegionAvail().y;
    const float FullWidth   = ImGui::GetContentRegionAvail().x + Style.WindowPadding.x;

    const ImVec2 TableSize    = ImVec2(FullWidth, TableHeight);
    const ImVec2 TableRectMin = ImGui::GetCursorScreenPos();
    const ImVec2 TableRectMax = ImVec2(TableRectMin.x + TableSize.x, TableRectMin.y + TableSize.y);

    if (!ImGui::BeginTable("##SceneOutliner", 3, TableFlags, TableSize))
    {
        ImGui::SetCursorPosX(SavedCursorX);
        ImGui::PopStyleColor(4);
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);

    {
        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRectFilled(TableRectMin, TableRectMax, IM_COL32(26, 26, 26, 255));
    }

    const float TypeColWidth = ImGui::CalcTextSize("DirectionalLight").x + Style.CellPadding.x * 2.0f + 12.0f;

    ImGui::TableSetupColumn("##Gutter", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_NoResize, 8.0f);
    ImGui::TableSetupColumn("Item Label", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, TypeColWidth);

    const ImVec2 SavedItemSpacing = Style.ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SavedItemSpacing.x, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 0.0f));

    // -----------------------------------------------------------------------------------------
    // Header row
    // -----------------------------------------------------------------------------------------

    {
        const float CellPaddingY  = Math::Max(0.0f, EditorStyleVars::SceneHierarchyTableRowHeight - ImGui::GetTextLineHeight()) * 0.5f;
        const float HeaderIndentX = 8.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(32.0f, CellPaddingY));

        const ImVec4 HeaderBg      = ImVec4(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);
        const ImVec4 HeaderBgHover = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, HeaderBg);
        ImGui::PushStyleColor(ImGuiCol_Header, HeaderBg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HeaderBgHover);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, HeaderBgHover);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

        float HeaderMinY = 0.0f;
        float HeaderMaxY = 0.0f;
        float SepPosX01  = 0.0f; // between column 0 and 1
        float SepPos12X  = 0.0f; // between column 1 and 2

        for (int32 Column = 0; Column < 3; Column++)
        {
            ImGui::TableSetColumnIndex(Column);

            if (Column == 1)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + HeaderIndentX);
            }
            else if (Column == 2)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + HeaderIndentX);
            } 

            ImGui::TableHeader(ImGui::TableGetColumnName(Column));

            const ImVec2 CellMin = ImGui::GetItemRectMin();
            const ImVec2 CellMax = ImGui::GetItemRectMax();

            if (Column == 1)
            {
                HeaderMinY = CellMin.y;
                HeaderMaxY = CellMax.y;
                SepPosX01  = CellMin.x;
            }
            else if (Column == 2)
            {
                SepPos12X = CellMin.x;
            }
        }

        {
            const ImU32 BorderCol = IM_COL32(26, 26, 26, 255);

            const float Thickness  = 2.0f;
            const float PositionX0 = Math::Floor(SepPosX01) + 0.5f;
            const float PositionX1 = Math::Floor(SepPos12X) + 0.5f;

            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            DrawList->AddLine(ImVec2(PositionX0, HeaderMinY), ImVec2(PositionX0, HeaderMaxY), BorderCol, Thickness);
            DrawList->AddLine(ImVec2(PositionX1, HeaderMinY), ImVec2(PositionX1, HeaderMaxY), BorderCol, Thickness);
        }

        ImGui::PopStyleColor(4); // Header colors
        ImGui::PopStyleVar();
    }

    // -----------------------------------------------------------------------------------------
    // Build tree
    // -----------------------------------------------------------------------------------------

    // Only root filters are walked here, DrawFilterSubtree recurses into the rest
    for (FActorFilter* Filter : Filters)
    {
        if (Filter && !Filter->GetParentFilter())
        {
            DrawFilterSubtree(Filter, 0.0f);
        }
    }

    // Actors that were never placed in a filter sit at the root, a project that creates no filters gets a flat list
    for (FActor* Actor : RootLevelActors)
    {
        DrawActorRow(Actor, 0.0f);
    }

    ImGui::PopStyleVar(2); // CellPadding + ItemSpacing

    ImGui::EndTable();
    ImGui::SetCursorPosX(SavedCursorX);

    ImGui::PopStyleColor(4);

    // VisibleActorOrder is complete now that every row has been drawn, so a shift-click can be turned into a span
    ApplyPendingRangeSelection();

    // Dropping an actor on the empty area below the rows returns it to the root, for both parenting and filtering
    if (ImGui::BeginDragDropTargetCustom(ImRect(TableRectMin, TableRectMax), ImGui::GetID("##SceneOutlinerDetachTarget")))
    {
        const ImGuiDragDropFlags DragDropFlags =
            ImGuiDragDropFlags_AcceptBeforeDelivery |
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect;

        if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ActorDragDropPayloadId, DragDropFlags))
        {
            FActor* DraggedActor = GetDraggedActor(Payload);

            const bool bIsPlaced = DraggedActor && (DraggedActor->GetParentActor() || DraggedActor->GetFilter());
            if (Payload->IsDelivery() && !bDragHoveringSourceRow && bIsPlaced)
            {
                RequestFilterAssignment(GetActorsForOperation(DraggedActor), nullptr);
            }
        }
        else if (const ImGuiPayload* FilterPayload = ImGui::AcceptDragDropPayload(FilterDragDropPayloadId, DragDropFlags))
        {
            FActorFilter* DraggedFilter = GetDraggedFilter(FilterPayload);

            const bool bIsNested = DraggedFilter && DraggedFilter->GetParentFilter();
            if (FilterPayload->IsDelivery() && !bDragHoveringSourceRow && bIsNested)
            {
                RequestFilterReparent(DraggedFilter, nullptr);
            }
        }

        ImGui::EndDragDropTarget();
    }

    // -----------------------------------------------------------------------------------------
    // Context menu
    // -----------------------------------------------------------------------------------------

    if (EditorWidgets::BeginPopupContextItem("SceneHierarchyContextMenu"))
    {
        const TArray<FActor*> SelectedActorsForMenu = EditorEngine->GetSelectedActors();

        FActor* PrimaryActorForMenu = EditorEngine->GetSelectedActor();

        const int32 SelectedActorCount = SelectedActorsForMenu.Size();
        const bool  bHasActorSelected  = SelectedActorCount > 0;
        const bool  bSingleActor       = SelectedActorCount == 1;
        const bool  bHasFilterSelected = (SelectedFilter != nullptr);

        EditorWidgets::MenuLabeledSeparator("Create");

        bool bRequestClosePopup = false;

        // Spawned at the origin, since a hierarchy row has no cursor ray to place against.
        {
            FSubMenuState AddActorSubMenu;
            if (EditorWidgets::BeginSubMenu(AddActorSubMenu, "##SceneHierarchyAddActorMenu", "Add Actor"))
            {
                if (FActor* NewActor = EditorActorFactory::DrawPlaceActorMenu(World, Vector3(0.0f, 0.0f, 0.0f)))
                {
                    EditorEngine->SetSelectedActor(NewActor);
                    bRequestClosePopup = true;
                }

                EditorWidgets::EndSubMenu(AddActorSubMenu);
            }
        }

        if (EditorWidgets::MenuItem("Add Filter"))
        {
            PendingCreateParent  = SelectedFilter;
            bPendingFilterCreate = true;
        }

        EditorWidgets::MenuLabeledSeparator("Common");

        {
            // Deleting a whole multi-selection is destructive, so the count goes in the label rather than staying implicit
            TStaticArray<CHAR, 64> DeleteLabel;
            if (SelectedActorCount > 1)
            {
                CString::Snprintf(DeleteLabel.Data(), static_cast<int32>(DeleteLabel.Size()), "Delete %d Actors", SelectedActorCount);
            }
            else
            {
                CString::Snprintf(DeleteLabel.Data(), static_cast<int32>(DeleteLabel.Size()), "Delete");
            }

            if (EditorWidgets::MenuItem(DeleteLabel.Data(), "Delete", false, bHasActorSelected || bHasFilterSelected))
            {
                if (bHasActorSelected)
                {
                    EditorEngine->RequestDeleteActors(SelectedActorsForMenu);
                }
                else
                {
                    PendingDestroyFilter = SelectedFilter;
                }
            }
        }

        // Renaming targets exactly one row, so a multi-selection has to be narrowed down first
        if (EditorWidgets::MenuItem("Rename", "F2", false, bSingleActor || (!bHasActorSelected && bHasFilterSelected)))
        {
            if (PrimaryActorForMenu)
            {
                bRequestRenameFocus = true;
                RenamingActor       = PrimaryActorForMenu;
                
                RenameBuffer.Fill(0);
                RenameBufferOriginal.Fill(0);
        
                const String& Name = PrimaryActorForMenu->GetName();
                if (!Name.IsEmpty())
                {
                    CString::Strncpy(RenameBuffer.Data(), *Name, RenameBuffer.Size());
                    CString::Strncpy(RenameBufferOriginal.Data(), *Name, RenameBufferOriginal.Size());
                }
            }
            else if (SelectedFilter)
            {
                BeginFilterRename(SelectedFilter);
            }
        }

        FSubMenuState MoveToFilterSubMenu;
        if (EditorWidgets::BeginSubMenu(MoveToFilterSubMenu, "##MoveToFilterMenu", "Move To Filter", bHasActorSelected || bHasFilterSelected))
        {
            FActorFilter* MovedFilter = bHasActorSelected ? nullptr : SelectedFilter;

            // With several actors selected the tick only shows when they all already share the destination
            const bool bAllAtRoot = bHasActorSelected
                ? !SelectedActorsForMenu.ContainsWithPredicate([](FActor* Actor) { return Actor && Actor->GetFilter() != nullptr; })
                : (SelectedFilter->GetParentFilter() == nullptr);

            const bool bRootAllowed = bHasActorSelected || CanReparentFilter(MovedFilter, nullptr);
            if (EditorWidgets::MenuItem("None", nullptr, bAllAtRoot, bRootAllowed))
            {
                if (bHasActorSelected)
                {
                    RequestFilterAssignment(SelectedActorsForMenu, nullptr);
                }
                else
                {
                    RequestFilterReparent(MovedFilter, nullptr);
                }
            }

            EditorWidgets::MenuSeparator();

            bool bHasRootFilter = false;
            for (FActorFilter* Filter : Filters)
            {
                if (Filter && !Filter->GetParentFilter())
                {
                    DrawMoveToFilterMenu(Filter, SelectedActorsForMenu, MovedFilter);
                    bHasRootFilter = true;
                }
            }

            if (!bHasRootFilter)
            {
                EditorWidgets::MenuItem("No Filters", nullptr, false, false);
            }

            EditorWidgets::EndSubMenu(MoveToFilterSubMenu);
        }

        // Enabled as soon as any one of the selected actors is nested, and detaches every one that is
        const bool bHasParent = SelectedActorsForMenu.ContainsWithPredicate([](FActor* Actor)
        {
            return Actor && Actor->GetParentActor() != nullptr;
        });

        if (EditorWidgets::MenuItem("Detach from Parent", nullptr, false, bHasParent))
        {
            RequestAttachment(SelectedActorsForMenu, nullptr);
        }

        if (EditorWidgets::MenuItem("Copy", "Ctrl+C", false, false))
        {
            // TODO
        }

        if (EditorWidgets::MenuItem("Copy", "Ctrl+V", false, false))
        {
            // TODO
        }

        if (bRequestClosePopup)
        {
            ImGui::CloseCurrentPopup();
        }

        EditorWidgets::EndPopupContext();
    }

    // -----------------------------------------------------------------------------------------
    // Click rules
    // -----------------------------------------------------------------------------------------

    if (EditorEngine && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const bool bHierarchyHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        if (!bHierarchyHovered)
        {
            if (bHasAnySelection)
            {
                bSelectionActiveInTable = false;
            }
        }
        else
        {
            const ImVec2 MousePos = ImGui::GetIO().MousePos;

            const bool bInTableRect = (MousePos.x >= TableRectMin.x && MousePos.x < TableRectMax.x) && (MousePos.y >= TableRectMin.y && MousePos.y < TableRectMax.y);
            if (!bInTableRect)
            {
                if (bHasAnySelection)
                {
                    bSelectionActiveInTable = false;
                }
            }
            else
            {
                const ImGuiIO& IO = ImGui::GetIO();

                // Missing a row while building a multi-selection must not throw the whole selection away
                const bool bModifyingSelection = IO.KeyCtrl || IO.KeyShift;

                if (!ImGui::IsAnyItemHovered() && !bModifyingSelection)
                {
                    EditorEngine->ClearSelection();

                    RenamingActor       = nullptr;
                    RenamingFilter      = nullptr;
                    SelectedFilter      = nullptr;
                    SelectionAnchor     = nullptr;
                    bRequestRenameFocus = false;
                }

                bSelectionActiveInTable = true;
            }
        }
    }

    // The shortcuts the context menu advertises, actor rows handle their own F2 inline
    if (bSelectionActiveInTable && !RenamingFilter && !RenamingActor)
    {
        const ImGuiIO& IO = ImGui::GetIO();

        const bool bWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (bWindowFocused && !IO.WantTextInput)
        {
            if (SelectedFilter && ImGui::IsKeyPressed(ImGuiKey_F2))
            {
                BeginFilterRename(SelectedFilter);
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                // An actor and a filter are never selected at the same time, so at most one of these applies
                const TArray<FActor*>& SelectedForDelete = EditorEngine->GetSelectedActors();
                if (!SelectedForDelete.IsEmpty())
                {
                    EditorEngine->RequestDeleteActors(SelectedForDelete);
                }
                else if (SelectedFilter)
                {
                    PendingDestroyFilter = SelectedFilter;
                }
            }
        }
    }

    ApplyPendingAttachment();
    ApplyPendingFilterOperations();
}

void FEditorSceneHierarchyWidget::DrawActorRow(FActor* Actor, float Indent)
{
    if (!Actor)
    {
        return;
    }

    constexpr float DefaultRowHeight = 30.0f;

    // Recorded in draw order so that a shift-click can resolve its span against the exact rows the user sees
    VisibleActorOrder.Add(Actor);

    const CHAR* Type      = GetActorTypeLabel(Actor);
    const bool  bSelected = EditorEngine->IsActorSelected(Actor);

    // Renaming is a single-actor operation, so it only stays available while this row is the entire selection
    const bool  bSoleSelection = bSelected && EditorEngine->GetSelectedActors().Size() == 1;

    const auto BeginActorRename = [this](FActor* InActor)
    {
        bRequestRenameFocus = true;
        RenamingActor       = InActor;

        RenameBuffer.Fill(0);
        RenameBufferOriginal.Fill(0);

        const String& Name = InActor->GetName();
        if (!Name.IsEmpty())
        {
            CString::Strncpy(RenameBuffer.Data(), *Name, RenameBuffer.Size());
            CString::Strncpy(RenameBufferOriginal.Data(), *Name, RenameBufferOriginal.Size());
        }
    };

    const auto CancelActorRename = [this]()
    {
        RenamingActor       = nullptr;
        bRequestRenameFocus = false;
    };

    const auto CommitActorRename = [this]()
    {
        if (RenamingActor)
        {
            RenamingActor->SetName(String(RenameBuffer.Data()));
            RenamingActor = nullptr;
        }

        bRequestRenameFocus = false;
    };

    // -------------------------------------------------------------------------------------------
    // Colors
    // -------------------------------------------------------------------------------------------

    const ImVec4 ActorNameTextColor = RowNameTextColor;
    const ImVec4 ActorTypeTextColor = ImVec4(124.0f / 255.0f, 124.0f / 255.0f, 124.0f / 255.0f, 1.0f);
    const ImU32  SelectedColor      = bSelectionActiveInTable ? RowSelectedActive : RowSelectedInactive;

    ImGuiStyle& Style = ImGui::GetStyle();
    ImGui::PushID(Actor);

    bool bIsRenamingThis = (RenamingActor == Actor);
    if (bIsRenamingThis && !bSoleSelection)
    {
        CommitActorRename();
        bIsRenamingThis = false;
    }

    {
        const ImGuiIO& IO = ImGui::GetIO();

        const bool bWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (bSoleSelection && bSelectionActiveInTable && bWindowFocused && !bIsRenamingThis && !IO.WantTextInput)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_F2))
            {
                BeginActorRename(Actor);
                bIsRenamingThis = true;
            }
        }
    }

    // -------------------------------------------------------------------------------------------
    // Column 0: Empty
    // -------------------------------------------------------------------------------------------

    const float RowHeight = Math::Max(DefaultRowHeight, ImGui::GetFontSize());
    ImGui::TableNextRow(ImGuiTableRowFlags_None, RowHeight);
    ImGui::TableSetColumnIndex(0);

    const ImGuiSelectableFlags SelectableFlags =
        ImGuiSelectableFlags_SpanAllColumns |
        ImGuiSelectableFlags_AllowOverlap;

    const ImU32  RowSelectedColor = bIsRenamingThis ? RenameRowTint : SelectedColor;

    if (bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, RowSelectedColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, RowSelectedColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, RowSelectedColor);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, RowHoverBg);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, RowHoverBg);
    }

    const bool bRowPressed = ImGui::Selectable("##Row", bSelected, SelectableFlags, ImVec2(0.0f, RowHeight));

    if (bSelected)
    {
        ImGui::PopStyleColor(3);
    }
    else
    {
        ImGui::PopStyleColor(2);
    }

    const ImVec2 RowMin = ImGui::GetItemRectMin();
    const ImVec2 RowMax = ImGui::GetItemRectMax();

    // -------------------------------------------------------------------------------------------
    // Drag and drop, dropping an actor on another actor re-parents it
    //
    // The drop-target is registered before the drag-source, since the tooltip drawn by the drag-source replaces
    // the last submitted item that BeginDragDropTarget resolves the row rectangle from
    // -------------------------------------------------------------------------------------------

    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiDragDropFlags DragDropFlags =
            ImGuiDragDropFlags_AcceptBeforeDelivery |
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect;

        if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ActorDragDropPayloadId, DragDropFlags))
        {
            FActor*    DraggedActor = GetDraggedActor(Payload);
            const bool bCanAttach   = CanAttachActorTo(DraggedActor, Actor);

            const ImU32 DropBorderColor = bCanAttach ? IM_COL32(0, 112, 224, 255) : IM_COL32(120, 120, 120, 160);
            ImGui::GetWindowDrawList()->AddRect(RowMin, RowMax, DropBorderColor, 0.0f, ImDrawFlags_None, 2.0f);

            if (bCanAttach && Payload->IsDelivery())
            {
                // Dragging one row of a multi-selection brings the rest along, minus any that cannot take this parent
                TArray<FActor*> AttachActors = GetActorsForOperation(DraggedActor);
                AttachActors.RemoveAllSwap([Actor](FActor* Candidate)
                {
                    return !CanAttachActorTo(Candidate, Actor);
                });

                RequestAttachment(AttachActors, Actor);
            }
        }

        ImGui::EndDragDropTarget();
    }
    else if (ImGui::IsDragDropActive() && GetDraggedActor(ImGui::GetDragDropPayload()) == Actor)
    {
        // The row of the dragged actor never becomes a drop-target, remember it so that releasing the drag on top
        // of it is treated as a cancelled drag instead of a detach
        bDragHoveringSourceRow |= ImGui::IsMouseHoveringRect(RowMin, RowMax);
    }

    if (!bIsRenamingThis && ImGui::BeginDragDropSource())
    {
        FActor* PayloadActor = Actor;
        ImGui::SetDragDropPayload(ActorDragDropPayloadId, &PayloadActor, sizeof(FActor*));

        const String& DraggedName = Actor->GetName();
        ImGui::TextUnformatted(DraggedName.IsEmpty() ? "Actor" : *DraggedName);

        // Dragging one row of a multi-selection carries the rest along, so the preview has to say so
        const int32 OtherSelectedCount = bSelected ? (EditorEngine->GetSelectedActors().Size() - 1) : 0;
        if (OtherSelectedCount > 0)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("+%d", OtherSelectedCount);
        }

        ImGui::EndDragDropSource();
    }

    ImFont* Font = ImGui::GetFont();

    const float FontSize      = ImGui::GetFontSize();
    const float Scale         = (Font->FontSize > 0.0f) ? (FontSize / Font->FontSize) : 1.0f;
    const float Ascent        = Font->Ascent * Scale;
    const float Descent       = Font->Descent * Scale;
    const float GlyphHeight   = Ascent - Descent;
    const float CenteredTextY = RowMin.y + (RowHeight - GlyphHeight) * 0.5f;
    const float TypeTextY     = CenteredTextY;
    const float NameTextY     = CenteredTextY;
    const float ArrowAdv      = ImGui::GetFontSize();
    const float ArrowGap      = 6.0f;
   
    // -------------------------------------------------------------------------------------------
    // Column 1: Label
    // -------------------------------------------------------------------------------------------

    ImGui::TableSetColumnIndex(1);

    const ImVec2 ColPos      = ImGui::GetCursorScreenPos();
    const float  ColWidth    = ImGui::GetContentRegionAvail().x;
    const float  ColMaxX     = ColPos.x + ColWidth;
    const float  ArrowStartX = ColPos.x + Indent;
    const float  LabelStartX = ArrowStartX + ArrowAdv + ArrowGap;
    const ImVec2 MousePos    = ImGui::GetIO().MousePos;
    const bool   bInLabelCol = (MousePos.x >= LabelStartX && MousePos.x <= ColMaxX);
    const bool   bInArrowCol = (MousePos.x >= ArrowStartX && MousePos.x < LabelStartX);

    // -------------------------------------------------------------------------------------------
    // Expand arrow, only actors that have children can be expanded
    // -------------------------------------------------------------------------------------------

    const bool bHasChildren = !Actor->GetChildActors().IsEmpty();

    // While searching, the whole hierarchy is shown expanded so that matching descendants are always reachable
    const bool bForceExpanded = ActorSearchFilterBuffer[0] != '\0';

    if (bRowPressed)
    {
        bSelectionActiveInTable = true;

        const ImGuiIO& ClickIO = ImGui::GetIO();

        // An actor and a filter are never selected at the same time, so any row click drops the filter selection
        const auto TakeSelectionFromFilter = [this, &bIsRenamingThis, &CancelActorRename]()
        {
            CancelActorRename();
            bIsRenamingThis = false;

            SelectedFilter = nullptr;
            RenamingFilter = nullptr;
        };

        if (bHasChildren && bInArrowCol)
        {
            SetActorExpanded(Actor, !IsActorExpanded(Actor));
        }
        else if (ClickIO.KeyShift && SelectionAnchor)
        {
            // The visible order is only complete once every row is drawn, so the span is resolved after the table closes
            RequestRangeSelection(Actor, ClickIO.KeyCtrl);
            TakeSelectionFromFilter();
        }
        else if (ClickIO.KeyCtrl)
        {
            EditorEngine->ToggleSelectedActor(Actor);
            SelectionAnchor = Actor;

            TakeSelectionFromFilter();
        }
        else if (!bSoleSelection)
        {
            // Clicking a row that is part of a wider selection narrows down to it rather than starting a rename
            EditorEngine->SetSelectedActor(Actor);
            SelectionAnchor = Actor;

            TakeSelectionFromFilter();
        }
        else
        {
            if (bInLabelCol && !bIsRenamingThis)
            {
                BeginActorRename(Actor);
                bIsRenamingThis = true;
            }
        }
    }

    const bool bExpanded = bHasChildren && (bForceExpanded || IsActorExpanded(Actor));

    if (bHasChildren)
    {
        const float  IconSize = 16.0f;
        const ImVec2 IconPos  = ImVec2(ArrowStartX, RowMin.y + (RowHeight - IconSize) * 0.5f);
        const ImU32  IconTint = IM_COL32(101, 101, 101, 255);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();

        ImTextureID ArrowIcon = bExpanded ? EditorIcons::CollapseArrowDown : EditorIcons::CollapseArrowRight;
        if (ArrowIcon)
        {
            DrawList->AddImage(ArrowIcon, IconPos, ImVec2(IconPos.x + IconSize, IconPos.y + IconSize), ImVec2(0, 0), ImVec2(1, 1), IconTint);
        }
        else
        {
            ImGui::RenderArrow(DrawList, IconPos, IconTint, bExpanded ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
        }
    }

    if (bIsRenamingThis)
    {
        const float DesiredFramePadY = Math::Max(0.0f, (RowHeight - FontSize) * 0.5f);
        const float InputX           = LabelStartX - Style.FramePadding.x;
        const float InputY           = RowMin.y;
        const float InputWidth       = (ColMaxX - InputX) - 8.0f;
        const float BorderRounding   = 4.0f;
        const float BorderThickness  = 2.0f;

        ImGui::SetCursorScreenPos(ImVec2(InputX, InputY));
        ImGui::SetNextItemWidth(InputWidth > 0.0f ? InputWidth : 0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, BorderRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Style.FramePadding.x, DesiredFramePadY));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, RenameInputBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, RenameInputBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, RenameInputBg);
        ImGui::PushStyleColor(ImGuiCol_Text, ActorNameTextColor);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, EditorStyleVars::InputFieldSelectionColor);

        if (bRequestRenameFocus)
        {
            ImGui::SetKeyboardFocusHere();
            bRequestRenameFocus = false;
        }

        const ImGuiInputTextFlags InputFlags =
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_AutoSelectAll;

        const bool bEnter = ImGui::InputText("##RenameActor", RenameBuffer.Data(), RenameBuffer.Size(), InputFlags);

        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(2);

        {
            ImVec2 ItemMin = ImGui::GetItemRectMin();
            ItemMin.x -= 1.0f;
            ItemMin.y += 1.0f;

            ImVec2 ItemMax = ImGui::GetItemRectMax();
            ItemMax.x += 1.0f;
            ItemMax.y -= 1.0f;

            const bool bActive  = ImGui::IsItemActive();
            const bool bHovered = ImGui::IsItemHovered();

            const ImU32 BorderColor = bActive ? RenameBorderActive : (bHovered ? RenameBorderHovered : RenameBorderNormal);

            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            DrawList->AddRect(ItemMin, ItemMax, BorderColor, BorderRounding, ImDrawFlags_None, BorderThickness);
        }

        if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            CString::Strncpy(RenameBuffer.Data(), RenameBufferOriginal.Data(), RenameBuffer.Size());
            CancelActorRename();
        }
        else if (bEnter || ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemDeactivated())
        {
            CommitActorRename();
        }
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ActorNameTextColor);

        TStaticArray<CHAR, 256> SearchBuf{};
        
        const CHAR* SearchText  = EditorHelpers::GetTrimmedQuery(ActorSearchFilterBuffer.Data(), SearchBuf.Data(), static_cast<int32>(SearchBuf.Size()));
        const ImU32 BaseTextU32 = ImGui::GetColorU32(ImGuiCol_Text);

        const String& Name = Actor->GetName();
        const CHAR* NameText = Name.IsEmpty() ? "Actor" : *Name;
        EditorWidgets::DrawTextWithSearchHighlight(ImGui::GetWindowDrawList(), ImVec2(LabelStartX, NameTextY), NameText, SearchText, BaseTextU32, 1.0f, 1.0f, &RowMin, &RowMax);

        ImGui::PopStyleColor();
    }

    // -------------------------------------------------------------------------------------------
    // Column 2: Type
    // -------------------------------------------------------------------------------------------

    ImGui::TableSetColumnIndex(2);

    const float BaselineCompensationY = bIsRenamingThis ? 6.0f : 0.0f;
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TypeTextY - BaselineCompensationY));

    ImGui::PushStyleColor(ImGuiCol_Text, ActorTypeTextColor);
    ImGui::TextUnformatted(Type);
    ImGui::PopStyleColor();

    ImGui::PopID();

    // Children are drawn outside of this row's ID scope, so that their ids do not change when they are re-parented
    if (bExpanded)
    {
        DrawChildActorRows(Actor, Indent + ImGui::GetStyle().IndentSpacing);
    }
}

void FEditorSceneHierarchyWidget::DrawChildActorRows(FActor* Actor, float Indent)
{
    for (FActor* Child : Actor->GetChildActors())
    {
        if (Child && PassesSearchFilter(Child))
        {
            DrawActorRow(Child, Indent);
        }
    }
}

bool FEditorSceneHierarchyWidget::PassesSearchFilter(FActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    TStaticArray<CHAR, 256> QueryBuffer{};

    const CHAR* Query = EditorHelpers::GetTrimmedQuery(ActorSearchFilterBuffer.Data(), QueryBuffer.Data(), static_cast<int32>(QueryBuffer.Size()));
    if (!Query || Query[0] == '\0')
    {
        return true;
    }

    return DoesActorOrDescendantMatchQuery(Actor, Query);
}

bool FEditorSceneHierarchyWidget::IsActorExpanded(FActor* Actor) const
{
    return !CollapsedActors.Contains(Actor);
}

void FEditorSceneHierarchyWidget::SetActorExpanded(FActor* Actor, bool bExpanded)
{
    if (bExpanded)
    {
        CollapsedActors.Remove(Actor);
    }
    else if (!CollapsedActors.Contains(Actor))
    {
        CollapsedActors.Emplace(Actor);
    }
}

bool FEditorSceneHierarchyWidget::IsFilterExpanded(FActorFilter* Filter) const
{
    return !CollapsedFilters.Contains(Filter);
}

void FEditorSceneHierarchyWidget::SetFilterExpanded(FActorFilter* Filter, bool bExpanded)
{
    if (bExpanded)
    {
        CollapsedFilters.Remove(Filter);
    }
    else if (!CollapsedFilters.Contains(Filter))
    {
        CollapsedFilters.Emplace(Filter);
    }
}

void FEditorSceneHierarchyWidget::RebuildFilterContents()
{
    FWorld* World = EditorEngine ? EditorEngine->GetWorld() : nullptr;
    if (!World)
    {
        FilterContents.Clear();
        return;
    }

    const TArrayView<FActorFilter* const> Filters = World->GetActorFilters();

    FilterContents.Clear();
    FilterContents.Resize(Filters.Size());

    // Only root-actors are bucketed, every other actor is drawn nested underneath its parent actor instead
    for (FActor* Actor : World->GetActors())
    {
        if (!Actor || Actor->GetParentActor() || !PassesSearchFilter(Actor))
        {
            continue;
        }

        FActorFilter* ActorFilter = Actor->GetFilter();
        if (!ActorFilter)
        {
            continue;
        }

        const int32 FilterIndex = Filters.Find(ActorFilter);
        if (FilterIndex != Filters.InvalidIndex)
        {
            FilterContents[FilterIndex].Emplace(Actor);
        }
    }
}

const TArray<FActor*>* FEditorSceneHierarchyWidget::GetFilterContents(FActorFilter* Filter) const
{
    FWorld* World = EditorEngine ? EditorEngine->GetWorld() : nullptr;
    if (!World)
    {
        return nullptr;
    }

    const TArrayView<FActorFilter* const> Filters = World->GetActorFilters();

    const int32 FilterIndex = Filters.Find(Filter);
    if (FilterIndex == Filters.InvalidIndex || FilterIndex >= FilterContents.Size())
    {
        return nullptr;
    }

    return &FilterContents[FilterIndex];
}

bool FEditorSceneHierarchyWidget::FilterSubtreeHasActors(FActorFilter* Filter) const
{
    if (!Filter)
    {
        return false;
    }

    const TArray<FActor*>* Contents = GetFilterContents(Filter);
    if (Contents && !Contents->IsEmpty())
    {
        return true;
    }

    for (FActorFilter* Child : Filter->GetChildFilters())
    {
        if (FilterSubtreeHasActors(Child))
        {
            return true;
        }
    }

    return false;
}

void FEditorSceneHierarchyWidget::DrawMoveToFilterMenu(FActorFilter* Filter, const TArray<FActor*>& TargetActors, FActorFilter* TargetFilter)
{
    if (!Filter)
    {
        return;
    }

    // MenuItem and BeginSubMenu push the label as their ID, so two filters sharing a name would otherwise collide
    ImGui::PushID(Filter);

    const bool bMovingActors = !TargetActors.IsEmpty();

    // A filter cannot be moved into itself or into its own subtree, an actor can go anywhere
    const bool bEnabled = bMovingActors || CanReparentFilter(TargetFilter, Filter);

    // With several actors selected the tick only shows when every one of them is already in this filter
    const bool bIsMember = bMovingActors
        ? !TargetActors.ContainsWithPredicate([Filter](FActor* Actor) { return !Actor || Actor->GetFilter() != Filter; })
        : (TargetFilter && TargetFilter->GetParentFilter() == Filter);

    const auto MoveHere = [this, Filter, &TargetActors, TargetFilter, bMovingActors]()
    {
        if (bMovingActors)
        {
            RequestFilterAssignment(TargetActors, Filter);
        }
        else
        {
            RequestFilterReparent(TargetFilter, Filter);
        }
    };

    const TArray<FActorFilter*>& ChildFilters = Filter->GetChildFilters();
    if (ChildFilters.IsEmpty())
    {
        if (EditorWidgets::MenuItem(*Filter->GetName(), nullptr, bIsMember, bEnabled))
        {
            MoveHere();
        }
    }
    else
    {
        FSubMenuState ChildrenSubMenu;
        if (EditorWidgets::BeginSubMenu(ChildrenSubMenu, "##MoveToFilterChildren", *Filter->GetName()))
        {
            // A filter with children is a destination as well as a branch, so it needs an entry of its own
            if (EditorWidgets::MenuItem("Move Here", nullptr, bIsMember, bEnabled))
            {
                MoveHere();
            }

            EditorWidgets::MenuSeparator();

            for (FActorFilter* Child : ChildFilters)
            {
                DrawMoveToFilterMenu(Child, TargetActors, TargetFilter);
            }

            EditorWidgets::EndSubMenu(ChildrenSubMenu);
        }
    }

    ImGui::PopID();
}

bool FEditorSceneHierarchyWidget::DrawFilterRow(FActorFilter* Filter, float Indent)
{
    constexpr float DefaultRowHeight = 30.0f;

    // Keyed on the filter itself rather than its name, so state survives a rename and two filters that happen to
    // share a name cannot collide
    ImGui::PushID(Filter);

    bool bOpen = IsFilterExpanded(Filter);

    const ImU32 SelectedColor  = bSelectionActiveInTable ? RowSelectedActive : RowSelectedInactive;
    const bool  bFilterSelected = (SelectedFilter == Filter);

    bool bIsRenamingThis = (RenamingFilter == Filter);

    const auto CommitFilterRename = [this, Filter]()
    {
        // An empty name is rejected so that a filter can never become unlabelled
        const String NewName(RenameBuffer.Data());
        if (!NewName.IsEmpty())
        {
            Filter->SetName(NewName);
        }

        RenamingFilter      = nullptr;
        bRequestRenameFocus = false;
    };

    if (bIsRenamingThis && !bFilterSelected)
    {
        CommitFilterRename();
        bIsRenamingThis = false;
    }

    // Read after the commit above, which can replace the string this points into
    const CHAR* Label = *Filter->GetName();

    ImGuiStyle& Style = ImGui::GetStyle();

    ImGui::TableNextRow(ImGuiTableRowFlags_None, DefaultRowHeight);

    // -------------------------------------------------------------------------------------------
    // Column 0: Empty
    // -------------------------------------------------------------------------------------------

    ImGui::TableSetColumnIndex(0);

    const ImGuiSelectableFlags SelectableFlags =
        ImGuiSelectableFlags_SpanAllColumns |
        ImGuiSelectableFlags_AllowOverlap;

    if (bFilterSelected)
    {
        const ImU32 RowSelectedColor = bIsRenamingThis ? RenameRowTint : SelectedColor;
        ImGui::PushStyleColor(ImGuiCol_Header, RowSelectedColor);
    }

    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, RowHoverBg);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, RowHoverBg);

    const bool bPressed = ImGui::Selectable("##Row", bFilterSelected, SelectableFlags, ImVec2(0.0f, DefaultRowHeight));

    ImGui::PopStyleColor(bFilterSelected ? 3 : 2);

    const ImVec2 RowMin = ImGui::GetItemRectMin();
    const ImVec2 RowMax = ImGui::GetItemRectMax();

    // -------------------------------------------------------------------------------------------
    // A filter row takes both payloads, an actor is placed in the filter and a filter is nested inside it
    // -------------------------------------------------------------------------------------------

    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiDragDropFlags DragDropFlags =
            ImGuiDragDropFlags_AcceptBeforeDelivery |
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect;

        if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ActorDragDropPayloadId, DragDropFlags))
        {
            FActor* DraggedActor = GetDraggedActor(Payload);

            ImGui::GetWindowDrawList()->AddRect(RowMin, RowMax, RowDropTargetColor, 0.0f, ImDrawFlags_None, 2.0f);

            if (DraggedActor && Payload->IsDelivery())
            {
                RequestFilterAssignment(GetActorsForOperation(DraggedActor), Filter);
            }
        }
        else if (const ImGuiPayload* FilterPayload = ImGui::AcceptDragDropPayload(FilterDragDropPayloadId, DragDropFlags))
        {
            FActorFilter* DraggedFilter = GetDraggedFilter(FilterPayload);
            if (CanReparentFilter(DraggedFilter, Filter))
            {
                ImGui::GetWindowDrawList()->AddRect(RowMin, RowMax, RowDropTargetColor, 0.0f, ImDrawFlags_None, 2.0f);

                if (FilterPayload->IsDelivery())
                {
                    RequestFilterReparent(DraggedFilter, Filter);
                }
            }
        }

        ImGui::EndDragDropTarget();
    }
    else if (ImGui::IsDragDropActive() && GetDraggedFilter(ImGui::GetDragDropPayload()) == Filter)
    {
        // The row of the dragged filter never becomes a drop-target, remember it so that releasing the drag on top
        // of it is treated as a cancelled drag instead of a move to the root
        bDragHoveringSourceRow |= ImGui::IsMouseHoveringRect(RowMin, RowMax);
    }

    if (!bIsRenamingThis && ImGui::BeginDragDropSource())
    {
        FActorFilter* PayloadFilter = Filter;
        ImGui::SetDragDropPayload(FilterDragDropPayloadId, &PayloadFilter, sizeof(FActorFilter*));

        ImGui::TextUnformatted(Label);

        ImGui::EndDragDropSource();
    }

    const float TextHeight = ImGui::GetTextLineHeight();
    const float TextY      = RowMin.y + (DefaultRowHeight - TextHeight) * 0.5f;
    const float IconSz     = 16.0f;

    // -------------------------------------------------------------------------------------------
    // Column 1: Label
    // -------------------------------------------------------------------------------------------

    ImGui::TableSetColumnIndex(1);

    const ImVec2 ColPos  = ImGui::GetCursorScreenPos();
    const float  ColMaxX = ColPos.x + ImGui::GetContentRegionAvail().x;
    const ImVec2 IconPos = ImVec2(ColPos.x + Indent, RowMin.y + (DefaultRowHeight - IconSz) * 0.5f);
    const ImU32  Tint    = IM_COL32(101, 101, 101, 255);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    ImTextureID ArrowIcon = bOpen ? EditorIcons::CollapseArrowDown : EditorIcons::CollapseArrowRight;
    if (ArrowIcon)
    {
        DrawList->AddImage(ArrowIcon, IconPos, ImVec2(IconPos.x + IconSz, IconPos.y + IconSz), ImVec2(0, 0), ImVec2(1, 1), Tint);
    }
    else
    {
        ImGui::RenderArrow(DrawList, IconPos, Tint, bOpen ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
    }

    float X = IconPos.x + IconSz + 6.0f;

    ImTextureID FolderIcon = bOpen ? EditorIcons::FolderOpenSmallIcon : EditorIcons::FolderSmallIcon;
    if (FolderIcon)
    {
        const float FolderSz = 16.0f;
        const float FolderY  = RowMin.y + (DefaultRowHeight - FolderSz) * 0.5f;
        DrawList->AddImage(FolderIcon, ImVec2(X, FolderY), ImVec2(X + FolderSz, FolderY + FolderSz));
        X += FolderSz + 6.0f;
    }

    // Hit regions match DrawActorRow: the arrow column toggles, the label column selects, and clicking the label
    // of an already-selected row starts a rename
    const float  LabelStartX = X;
    const ImVec2 MousePos    = ImGui::GetIO().MousePos;
    const bool   bInArrowCol = (MousePos.x >= IconPos.x && MousePos.x < LabelStartX);
    const bool   bInLabelCol = (MousePos.x >= LabelStartX && MousePos.x <= ColMaxX);

    if (bPressed)
    {
        bSelectionActiveInTable = true;

        if (bInArrowCol)
        {
            bOpen = !bOpen;
            SetFilterExpanded(Filter, bOpen);
        }
        else if (!bFilterSelected)
        {
            EditorEngine->ClearSelection();

            SelectedFilter      = Filter;
            RenamingFilter      = nullptr;
            SelectionAnchor     = nullptr;
            bRequestRenameFocus = false;
        }
        else if (bInLabelCol && !bIsRenamingThis)
        {
            BeginFilterRename(Filter);
            bIsRenamingThis = true;
        }
    }

    if (bIsRenamingThis)
    {
        const float FontSize         = ImGui::GetFontSize();
        const float DesiredFramePadY = Math::Max(0.0f, (DefaultRowHeight - FontSize) * 0.5f);
        const float InputX           = LabelStartX - Style.FramePadding.x;
        const float InputWidth       = (RowMax.x - InputX) - 8.0f;
        const float BorderRounding   = 4.0f;
        const float BorderThickness  = 2.0f;

        ImGui::SetCursorScreenPos(ImVec2(InputX, RowMin.y));
        ImGui::SetNextItemWidth(InputWidth > 0.0f ? InputWidth : 0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, BorderRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Style.FramePadding.x, DesiredFramePadY));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, RenameInputBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, RenameInputBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, RenameInputBg);
        ImGui::PushStyleColor(ImGuiCol_Text, RowNameTextColor);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, EditorStyleVars::InputFieldSelectionColor);

        if (bRequestRenameFocus)
        {
            ImGui::SetKeyboardFocusHere();
            bRequestRenameFocus = false;
        }

        const ImGuiInputTextFlags InputFlags =
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_AutoSelectAll;

        const bool bEnter = ImGui::InputText("##RenameFilter", RenameBuffer.Data(), RenameBuffer.Size(), InputFlags);

        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(2);

        {
            ImVec2 ItemMin = ImGui::GetItemRectMin();
            ItemMin.x -= 1.0f;
            ItemMin.y += 1.0f;

            ImVec2 ItemMax = ImGui::GetItemRectMax();
            ItemMax.x += 1.0f;
            ItemMax.y -= 1.0f;

            const bool bActive  = ImGui::IsItemActive();
            const bool bHovered = ImGui::IsItemHovered();

            const ImU32 BorderColor = bActive ? RenameBorderActive : (bHovered ? RenameBorderHovered : RenameBorderNormal);

            DrawList->AddRect(ItemMin, ItemMax, BorderColor, BorderRounding, ImDrawFlags_None, BorderThickness);
        }

        if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            CString::Strncpy(RenameBuffer.Data(), RenameBufferOriginal.Data(), RenameBuffer.Size());

            RenamingFilter      = nullptr;
            bRequestRenameFocus = false;
        }
        else if (bEnter || ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemDeactivated())
        {
            CommitFilterRename();
        }
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, RowNameTextColor);

        {
            TStaticArray<CHAR, 256> SearchBuf{};

            const CHAR* SearchText  = EditorHelpers::GetTrimmedQuery(ActorSearchFilterBuffer.Data(), SearchBuf.Data(), static_cast<int32>(SearchBuf.Size()));
            const ImU32 BaseTextU32 = ImGui::GetColorU32(ImGuiCol_Text);

            EditorWidgets::DrawTextWithSearchHighlight(DrawList, ImVec2(X, TextY), Label, SearchText, BaseTextU32, 1.0f, 1.0f, &RowMin, &RowMax);
        }

        ImGui::PopStyleColor();
    }

    // -------------------------------------------------------------------------------------------
    // Column 2: Type
    // -------------------------------------------------------------------------------------------

    ImGui::TableSetColumnIndex(2);
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));

    ImGui::PushStyleColor(ImGuiCol_Text, RowTypeTextColor);
    ImGui::TextUnformatted("Filter");
    ImGui::PopStyleColor();

    ImGui::PopID();
    return bOpen;
}

void FEditorSceneHierarchyWidget::DrawFilterSubtree(FActorFilter* Filter, float Indent)
{
    if (!Filter)
    {
        return;
    }

    const bool bIsSearching = ActorSearchFilterBuffer[0] != '\0';
    if (bIsSearching && !FilterSubtreeHasActors(Filter))
    {
        return;
    }

    if (!DrawFilterRow(Filter, Indent))
    {
        return;
    }

    const float ChildIndent = Indent + ImGui::GetStyle().IndentSpacing;

    for (FActorFilter* Child : Filter->GetChildFilters())
    {
        DrawFilterSubtree(Child, ChildIndent);
    }

    if (const TArray<FActor*>* Contents = GetFilterContents(Filter))
    {
        for (FActor* Actor : *Contents)
        {
            DrawActorRow(Actor, ChildIndent);
        }
    }
}

void FEditorSceneHierarchyWidget::RequestAttachment(FActor* ChildActor, FActor* ParentActor)
{
    PendingAttachChildren.Clear();

    if (ChildActor)
    {
        PendingAttachChildren.Add(ChildActor);
    }

    PendingAttachParent = ParentActor;
    bPendingAttachment  = true;
}

void FEditorSceneHierarchyWidget::RequestAttachment(const TArray<FActor*>& ChildActors, FActor* ParentActor)
{
    PendingAttachChildren = ChildActors;
    PendingAttachParent   = ParentActor;
    bPendingAttachment    = true;
}

void FEditorSceneHierarchyWidget::ApplyPendingAttachment()
{
    if (!bPendingAttachment)
    {
        return;
    }

    for (FActor* ChildActor : PendingAttachChildren)
    {
        if (!ChildActor)
        {
            continue;
        }

        if (PendingAttachParent)
        {
            // A multi-selection can hold an ancestor of the new parent, and AttachToActor rejects those on its own
            if (ChildActor->AttachToActor(PendingAttachParent, EAttachmentRule::KeepWorld))
            {
                // Make sure the newly attached actor is not hidden inside a collapsed parent
                SetActorExpanded(PendingAttachParent, true);
            }
        }
        else
        {
            ChildActor->DetachFromParent(EAttachmentRule::KeepWorld);
        }
    }

    PendingAttachChildren.Clear();
    PendingAttachParent = nullptr;
    bPendingAttachment  = false;
}

void FEditorSceneHierarchyWidget::BeginFilterRename(FActorFilter* Filter)
{
    if (!Filter)
    {
        return;
    }

    EditorEngine->ClearSelection();

    bRequestRenameFocus = true;
    RenamingActor       = nullptr;
    RenamingFilter      = Filter;
    SelectedFilter      = Filter;

    RenameBuffer.Fill(0);
    RenameBufferOriginal.Fill(0);

    const String& Name = Filter->GetName();
    if (!Name.IsEmpty())
    {
        CString::Strncpy(RenameBuffer.Data(), *Name, RenameBuffer.Size());
        CString::Strncpy(RenameBufferOriginal.Data(), *Name, RenameBufferOriginal.Size());
    }
}

void FEditorSceneHierarchyWidget::RequestFilterAssignment(FActor* Actor, FActorFilter* Filter)
{
    PendingAssignActors.Clear();

    if (Actor)
    {
        PendingAssignActors.Add(Actor);
    }

    PendingAssignFilter  = Filter;
    bPendingFilterAssign = true;
}

void FEditorSceneHierarchyWidget::RequestFilterAssignment(const TArray<FActor*>& Actors, FActorFilter* Filter)
{
    PendingAssignActors  = Actors;
    PendingAssignFilter  = Filter;
    bPendingFilterAssign = true;
}

void FEditorSceneHierarchyWidget::RequestRangeSelection(FActor* TargetActor, bool bAdditive)
{
    PendingRangeTarget    = TargetActor;
    bPendingRangeSelect   = true;
    bPendingRangeAdditive = bAdditive;
}

void FEditorSceneHierarchyWidget::ApplyPendingRangeSelection()
{
    if (!bPendingRangeSelect)
    {
        return;
    }

    FActor* Target = PendingRangeTarget;

    PendingRangeTarget  = nullptr;
    bPendingRangeSelect = false;

    if (!EditorEngine || !Target)
    {
        return;
    }

    const int32 AnchorIndex = VisibleActorOrder.Find(SelectionAnchor);
    const int32 TargetIndex = VisibleActorOrder.Find(Target);

    // A collapsed subtree can hide the anchor, and with no span to walk the click falls back to a plain select
    if (AnchorIndex == TArray<FActor*>::InvalidIndex || TargetIndex == TArray<FActor*>::InvalidIndex)
    {
        EditorEngine->SetSelectedActor(Target);
        SelectionAnchor = Target;
        return;
    }

    const int32 FirstIndex = Math::Min(AnchorIndex, TargetIndex);
    const int32 LastIndex  = Math::Max(AnchorIndex, TargetIndex);

    TArray<FActor*> Range;
    if (bPendingRangeAdditive)
    {
        Range = EditorEngine->GetSelectedActors();
    }

    Range.Reserve(Range.Size() + (LastIndex - FirstIndex) + 1);

    for (int32 Index = FirstIndex; Index <= LastIndex; Index++)
    {
        Range.Add(VisibleActorOrder[Index]);
    }

    // SetSelectedActors makes the last entry the primary, so the clicked row is moved to the back to become it
    Range.Remove(Target);
    Range.Add(Target);

    EditorEngine->SetSelectedActors(Range);
}

TArray<FActor*> FEditorSceneHierarchyWidget::GetActorsForOperation(FActor* Actor) const
{
    TArray<FActor*> Result;

    // Acting on a row inside the selection acts on the whole selection, acting on one outside it does not
    if (EditorEngine && EditorEngine->IsActorSelected(Actor))
    {
        Result = EditorEngine->GetSelectedActors();
    }
    else if (Actor)
    {
        Result.Add(Actor);
    }

    return Result;
}

void FEditorSceneHierarchyWidget::RequestFilterReparent(FActorFilter* Filter, FActorFilter* ParentFilter)
{
    PendingReparentFilter  = Filter;
    PendingReparentParent  = ParentFilter;
    bPendingFilterReparent = true;
}

void FEditorSceneHierarchyWidget::ApplyPendingFilterOperations()
{
    FWorld* World = EditorEngine ? EditorEngine->GetWorld() : nullptr;
    if (!World)
    {
        return;
    }

    if (bPendingFilterAssign)
    {
        for (FActor* AssignActor : PendingAssignActors)
        {
            if (!AssignActor)
            {
                continue;
            }

            if (AssignActor->GetParentActor())
            {
                AssignActor->DetachFromParent(EAttachmentRule::KeepWorld);
            }

            AssignActor->SetFilter(PendingAssignFilter);
        }

        PendingAssignActors.Clear();
        PendingAssignFilter  = nullptr;
        bPendingFilterAssign = false;
    }

    if (bPendingFilterReparent)
    {
        if (PendingReparentFilter)
        {
            World->SetActorFilterParent(PendingReparentFilter, PendingReparentParent);

            // Make sure the moved filter is not hidden inside a collapsed destination
            if (PendingReparentParent)
            {
                SetFilterExpanded(PendingReparentParent, true);
            }
        }

        PendingReparentFilter  = nullptr;
        PendingReparentParent  = nullptr;
        bPendingFilterReparent = false;
    }

    if (PendingDestroyFilter)
    {
        if (SelectedFilter == PendingDestroyFilter)
        {
            SelectedFilter = nullptr;
        }

        if (RenamingFilter == PendingDestroyFilter)
        {
            RenamingFilter      = nullptr;
            bRequestRenameFocus = false;
        }

        World->DestroyActorFilter(PendingDestroyFilter);
        PendingDestroyFilter = nullptr;
    }

    if (bPendingFilterCreate)
    {
        String Name("New Filter");
        for (int32 Suffix = 1; World->FindActorFilter(Name, PendingCreateParent); Suffix++)
        {
            Name = String::Printf("New Filter %d", Suffix);
        }

        if (PendingCreateParent)
        {
            SetFilterExpanded(PendingCreateParent, true);
        }

        BeginFilterRename(World->CreateActorFilter(Name, PendingCreateParent));

        PendingCreateParent  = nullptr;
        bPendingFilterCreate = false;
    }
}
