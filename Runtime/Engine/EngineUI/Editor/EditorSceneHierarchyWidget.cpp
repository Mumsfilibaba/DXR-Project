#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/OutputDeviceManager.h"
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

static float GetHierarchyRowHeight()
{
    constexpr float DefaultRowHeight = 30.0f;
    return Math::Max(DefaultRowHeight, ImGui::GetFontSize());
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
    , VisibleActorOrder()
    , RootLevelActors()
    , VisibleRows()
    , FilterContents()
    , CollapsedActors()
    , CollapsedFilters()
    , LiveActors()
    , MatchingActors()
    , FilterIndices()
    , CachedQuery()
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
    CachedQuery.Fill(0);
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

    TRACE_SCOPE("Scene Hierarchy");

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::SceneHierarchyItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, EditorStyleVars::SceneHierarchyWindowPadding);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f));

    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    if (EditorWidgets::BeginEditorWindow("Scene Hierarchy", &bVisible, Flags))
    {
        DrawSceneInfo();
    }

    EditorWidgets::EndEditorWindow();

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

    const TArray<FActor*>& Actors = World->GetActors();

    LiveActors.Clear();
    LiveActors.Reserve(Actors.Size());

    for (FActor* Actor : Actors)
    {
        LiveActors.Add(Actor);
    }

    if (!CollapsedActors.IsEmpty())
    {
        TArray<FActor*> StaleActors;

        for (FActor* CollapsedActor : CollapsedActors)
        {
            if (!LiveActors.Contains(CollapsedActor))
            {
                StaleActors.Emplace(CollapsedActor);
            }
        }

        for (FActor* StaleActor : StaleActors)
        {
            CollapsedActors.Remove(StaleActor);
        }
    }

    if (RenamingActor && !LiveActors.Contains(RenamingActor))
    {
        RenamingActor       = nullptr;
        bRequestRenameFocus = false;
    }

    if (SelectionAnchor && !LiveActors.Contains(SelectionAnchor))
    {
        SelectionAnchor = nullptr;
    }

    const TArrayView<FActorFilter* const> Filters = World->GetActorFilters();
    if (SelectedFilter && !Filters.Contains(SelectedFilter))
    {
        SelectedFilter = nullptr;
    }

    if (RenamingFilter && !Filters.Contains(RenamingFilter))
    {
        RenamingFilter      = nullptr;
        bRequestRenameFocus = false;
    }

    if (!CollapsedFilters.IsEmpty())
    {
        TArray<FActorFilter*> StaleFilters;

        for (FActorFilter* CollapsedFilter : CollapsedFilters)
        {
            if (!Filters.Contains(CollapsedFilter))
            {
                StaleFilters.Emplace(CollapsedFilter);
            }
        }

        for (FActorFilter* StaleFilter : StaleFilters)
        {
            CollapsedFilters.Remove(StaleFilter);
        }
    }

    RebuildSearchMatches(Actors);
    RebuildFilterBuckets();
    RebuildVisibleRows();

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
    const float FullWidth   = Math::Max(1.0f, ImGui::GetContentRegionAvail().x + Style.WindowPadding.x);

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

    ImGuiListClipper Clipper;
    Clipper.Begin(VisibleRows.Size(), GetHierarchyRowHeight());

    if (RenamingActor || RenamingFilter)
    {
        for (int32 Index = 0; Index < VisibleRows.Size(); ++Index)
        {
            const FHierarchyRow& Row = VisibleRows[Index];
            if ((RenamingActor && Row.Actor == RenamingActor) || (RenamingFilter && Row.Filter == RenamingFilter))
            {
                Clipper.IncludeItemByIndex(Index);
                break;
            }
        }
    }

    while (Clipper.Step())
    {
        for (int32 Index = Clipper.DisplayStart; Index < Clipper.DisplayEnd; ++Index)
        {
            const FHierarchyRow& Row = VisibleRows[Index];

            if (Row.Filter)
            {
                DrawFilterRow(Row.Filter, Row.Indent);
            }
            else
            {
                DrawActorRow(Row.Actor, Row.Indent);
            }
        }
    }

    ImGui::PopStyleVar(2); // CellPadding + ItemSpacing

    ImGui::EndTable();
    ImGui::SetCursorPosX(SavedCursorX);

    ImGui::PopStyleColor(4);

    ApplyPendingRangeSelection();

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

    const CHAR* Type           = GetActorTypeLabel(Actor);
    const bool  bSelected      = EditorEngine->IsActorSelected(Actor);
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

    const float RowHeight = GetHierarchyRowHeight();
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
        bDragHoveringSourceRow |= ImGui::IsMouseHoveringRect(RowMin, RowMax);
    }

    if (!bIsRenamingThis && ImGui::BeginDragDropSource())
    {
        FActor* PayloadActor = Actor;
        ImGui::SetDragDropPayload(ActorDragDropPayloadId, &PayloadActor, sizeof(FActor*));

        const String& DraggedName = Actor->GetName();
        ImGui::TextUnformatted(DraggedName.IsEmpty() ? "Actor" : *DraggedName);

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

    const bool bHasChildren   = !Actor->GetChildActors().IsEmpty();
    const bool bForceExpanded = CachedQuery[0] != '\0';

    if (bRowPressed)
    {
        bSelectionActiveInTable = true;

        const ImGuiIO& ClickIO = ImGui::GetIO();
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

        const FEditorIcon& ArrowIcon = bExpanded ? EditorIcons::CollapseArrowDown : EditorIcons::CollapseArrowRight;
        if (ArrowIcon)
        {
            EditorWidgets::DrawIcon(DrawList, ArrowIcon, IconPos, ImVec2(IconPos.x + IconSize, IconPos.y + IconSize), IconTint);
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

        const CHAR* SearchText  = CachedQuery.Data();
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
}

void FEditorSceneHierarchyWidget::RebuildSearchMatches(const TArray<FActor*>& Actors)
{
    TStaticArray<CHAR, 256> QueryBuffer{};

    const CHAR* Query = EditorHelpers::GetTrimmedQuery(ActorSearchFilterBuffer.Data(), QueryBuffer.Data(), static_cast<int32>(QueryBuffer.Size()));
    if (Query)
    {
        CString::Strncpy(CachedQuery.Data(), Query, CachedQuery.Size());
        CachedQuery[CachedQuery.Size() - 1] = '\0';
    }
    else
    {
        CachedQuery[0] = '\0';
    }

    MatchingActors.Clear();

    if (CachedQuery[0] == '\0')
    {
        return;
    }

    for (FActor* Actor : Actors)
    {
        if (!Actor || !DoesActorMatchQuery(Actor, CachedQuery.Data()))
        {
            continue;
        }

        for (FActor* Ancestor = Actor; Ancestor; Ancestor = Ancestor->GetParentActor())
        {
            bool bAlreadyPresent = false;
            MatchingActors.Add(Ancestor, &bAlreadyPresent);

            if (bAlreadyPresent)
            {
                break;
            }
        }
    }
}

bool FEditorSceneHierarchyWidget::PassesSearchFilter(FActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    return CachedQuery[0] == '\0' || MatchingActors.Contains(Actor);
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
    else
    {
        CollapsedActors.Add(Actor);
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
    else
    {
        CollapsedFilters.Add(Filter);
    }
}

void FEditorSceneHierarchyWidget::RebuildFilterBuckets()
{
    RootLevelActors.Clear();
    FilterIndices.Clear();

    FWorld* World = EditorEngine ? EditorEngine->GetWorld() : nullptr;
    if (!World)
    {
        FilterContents.Clear();
        return;
    }

    const TArrayView<FActorFilter* const> Filters = World->GetActorFilters();

    for (int32 Index = 0; Index < Filters.Size(); ++Index)
    {
        if (Filters[Index])
        {
            FilterIndices.Add(Filters[Index], Index);
        }
    }

    FilterContents.Resize(Filters.Size());

    for (TArray<FActor*>& Bucket : FilterContents)
    {
        Bucket.Clear();
    }

    for (FActor* Actor : World->GetActors())
    {
        if (!Actor || Actor->GetParentActor() || !PassesSearchFilter(Actor))
        {
            continue;
        }

        const int32* FilterIndex = Actor->GetFilter() ? FilterIndices.Find(Actor->GetFilter()) : nullptr;
        if (FilterIndex)
        {
            FilterContents[*FilterIndex].Emplace(Actor);
        }
        else
        {
            RootLevelActors.Emplace(Actor);
        }
    }
}

const TArray<FActor*>* FEditorSceneHierarchyWidget::GetFilterContents(FActorFilter* Filter) const
{
    const int32* FilterIndex = FilterIndices.Find(Filter);
    if (!FilterIndex || *FilterIndex >= FilterContents.Size())
    {
        return nullptr;
    }

    return &FilterContents[*FilterIndex];
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

    ImGui::PushID(Filter);

    const bool bMovingActors = !TargetActors.IsEmpty();
    const bool bEnabled      = bMovingActors || CanReparentFilter(TargetFilter, Filter);
    const bool bIsMember     = bMovingActors
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

void FEditorSceneHierarchyWidget::DrawFilterRow(FActorFilter* Filter, float Indent)
{
    const float DefaultRowHeight = GetHierarchyRowHeight();

    ImGui::PushID(Filter);

    bool bOpen = IsFilterExpanded(Filter);

    const ImU32 SelectedColor  = bSelectionActiveInTable ? RowSelectedActive : RowSelectedInactive;
    const bool  bFilterSelected = (SelectedFilter == Filter);

    bool bIsRenamingThis = (RenamingFilter == Filter);

    const auto CommitFilterRename = [this, Filter]()
    {
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

    // ----------------------------------------------------------------------------------------------------
    // A filter row takes both payloads, an actor is placed in the filter and a filter is nested inside it
    // ----------------------------------------------------------------------------------------------------

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

    const ImVec2 ColPos   = ImGui::GetCursorScreenPos();
    const float  ColMaxX  = ColPos.x + ImGui::GetContentRegionAvail().x;
    const ImVec2 IconPos  = ImVec2(ColPos.x + Indent, RowMin.y + (DefaultRowHeight - IconSz) * 0.5f);
    const ImU32  Tint     = IM_COL32(101, 101, 101, 255);
    ImDrawList*  DrawList = ImGui::GetWindowDrawList();

    const FEditorIcon& ArrowIcon = bOpen ? EditorIcons::CollapseArrowDown : EditorIcons::CollapseArrowRight;
    if (ArrowIcon)
    {
        EditorWidgets::DrawIcon(DrawList, ArrowIcon, IconPos, ImVec2(IconPos.x + IconSz, IconPos.y + IconSz), Tint);
    }
    else
    {
        ImGui::RenderArrow(DrawList, IconPos, Tint, bOpen ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
    }

    float X = IconPos.x + IconSz + 6.0f;

    const FEditorIcon& FolderIcon = bOpen ? EditorIcons::FolderOpenSmallIcon : EditorIcons::FolderSmallIcon;
    if (FolderIcon)
    {
        const float FolderSz = 16.0f;
        const float FolderY  = RowMin.y + (DefaultRowHeight - FolderSz) * 0.5f;
    
        EditorWidgets::DrawIcon(DrawList, FolderIcon, ImVec2(X, FolderY), ImVec2(X + FolderSz, FolderY + FolderSz));
        X += FolderSz + 6.0f;
    }

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
            const CHAR* SearchText  = CachedQuery.Data();
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
}

void FEditorSceneHierarchyWidget::RebuildVisibleRows()
{
    VisibleRows.Clear();
    VisibleActorOrder.Clear();

    FWorld* World = EditorEngine ? EditorEngine->GetWorld() : nullptr;
    if (!World)
    {
        return;
    }

    for (FActorFilter* Filter : World->GetActorFilters())
    {
        if (Filter && !Filter->GetParentFilter())
        {
            AppendFilterRows(Filter, 0.0f);
        }
    }

    for (FActor* Actor : RootLevelActors)
    {
        AppendActorRows(Actor, 0.0f);
    }
}

void FEditorSceneHierarchyWidget::AppendFilterRows(FActorFilter* Filter, float Indent)
{
    if (!Filter)
    {
        return;
    }

    const bool bIsSearching = CachedQuery[0] != '\0';
    if (bIsSearching && !FilterSubtreeHasActors(Filter))
    {
        return;
    }

    FHierarchyRow& Row = VisibleRows.Emplace();
    Row.Filter = Filter;
    Row.Indent = Indent;

    if (!IsFilterExpanded(Filter))
    {
        return;
    }

    const float ChildIndent = Indent + ImGui::GetStyle().IndentSpacing;

    for (FActorFilter* Child : Filter->GetChildFilters())
    {
        AppendFilterRows(Child, ChildIndent);
    }

    if (const TArray<FActor*>* Contents = GetFilterContents(Filter))
    {
        for (FActor* Actor : *Contents)
        {
            AppendActorRows(Actor, ChildIndent);
        }
    }
}

void FEditorSceneHierarchyWidget::AppendActorRows(FActor* Actor, float Indent)
{
    if (!Actor)
    {
        return;
    }

    FHierarchyRow& Row = VisibleRows.Emplace();
    Row.Actor  = Actor;
    Row.Indent = Indent;

    VisibleActorOrder.Add(Actor);

    const bool bForceExpanded = CachedQuery[0] != '\0';
    if (Actor->GetChildActors().IsEmpty() || !(bForceExpanded || IsActorExpanded(Actor)))
    {
        return;
    }

    const float ChildIndent = Indent + ImGui::GetStyle().IndentSpacing;
    for (FActor* Child : Actor->GetChildActors())
    {
        if (Child && PassesSearchFilter(Child))
        {
            AppendActorRows(Child, ChildIndent);
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
            if (ChildActor->AttachToActor(PendingAttachParent, EAttachmentRule::KeepWorld))
            {
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

    Range.Remove(Target);
    Range.Add(Target);

    EditorEngine->SetSelectedActors(Range);
}

TArray<FActor*> FEditorSceneHierarchyWidget::GetActorsForOperation(FActor* Actor) const
{
    TArray<FActor*> Result;
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
