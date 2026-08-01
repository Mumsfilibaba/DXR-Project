#include "Core/Misc/OutputDeviceLogger.h"
#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/Components/DirectionalLightComponent.h"
#include "Engine/World/Components/LightComponent.h"
#include "Engine/World/Components/LightProbeComponent.h"
#include "Engine/World/Components/PointLightComponent.h"
#include "Engine/World/Components/SkyLightComponent.h"
#include "Engine/World/World.h"
#include "ImGuiPlugin/ImGuiCore.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

// Payload identifier used when dragging an actor onto another actor in the outliner
static const CHAR* ActorDragDropPayloadId = "SCENE_HIERARCHY_ACTOR";

enum class EActorFolder : uint8
{
    Cameras,
    Actors,
    Lighting,
};

static const CHAR* GetActorTypeLabel(FActor* Actor)
{
    if (!Actor)
    {
        return "Actor";
    }

    if (Actor->HasComponentOfType<FCameraComponent>())
    {
        return "Camera";
    }

    if (Actor->HasComponentOfType<FPointLightComponent>())
    {
        return "PointLight";
    }

    if (Actor->HasComponentOfType<FDirectionalLightComponent>())
    {
        return "DirectionalLight";
    }

    if (Actor->HasComponentOfType<FSkyLightComponent>())
    {
        return "SkyLight";
    }

    if (Actor->HasComponentOfType<FLightProbeComponent>())
    {
        return "LightProbe";
    }
    
    if (Actor->HasComponentOfType<FLightComponent>())
    {
        return "Light";
    }

    return "Actor";
}

static EActorFolder GetActorFolder(FActor* Actor)
{
    if (Actor->HasComponentOfType<FCameraComponent>())
    {
        return EActorFolder::Cameras;
    }

    if (Actor->HasComponentOfType<FLightComponent>() || Actor->HasComponentOfType<FLightProbeComponent>())
    {
        return EActorFolder::Lighting;
    }

    return EActorFolder::Actors;
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

static FActor* GetDraggedActor(const ImGuiPayload* Payload)
{
    if (!Payload || !Payload->IsDataType(ActorDragDropPayloadId) || Payload->DataSize != static_cast<int32>(sizeof(FActor*)))
    {
        return nullptr;
    }

    return *reinterpret_cast<FActor* const*>(Payload->Data);
}

FEditorSceneHierarchyWidget::FEditorSceneHierarchyWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , RenamingActor(nullptr)
    , PendingAttachChild(nullptr)
    , PendingAttachParent(nullptr)
    , CollapsedActors()
    , bVisible(true)
    , bRequestRenameFocus(false)
    , bSelectionActiveInTable(false)
    , bPendingAttachment(false)
    , bDragHoveringSourceRow(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorSceneHierarchyWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }

    ActorSearchFilterBuffer.Fill(0);
    ActorRenameBuffer.Fill(0);
    ActorRenameBufferOriginal.Fill(0);
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
    // Shared colors
    // -----------------------------------------------------------------------------------------

    const ImVec4 RowHoverBg    = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    const ImVec4 NameTextColor = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
    const ImVec4 TypeTextColor = ImVec4(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);

    constexpr float DefaultRowHeight = 30.0f;

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

    FActor* SelectedActor = EditorEngine->GetSelectedActor();
    const bool bHasAnySelection = SelectedActor != nullptr;

    bDragHoveringSourceRow = false;

    bool bHasActors   = false;
    bool bHasCameras  = false;
    bool bHasLighting = false;

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

    // Only root-actors are placed in a folder, every other actor is nested underneath its parent
    for (FActor* Actor : Actors)
    {
        if (!Actor || Actor->GetParentActor() || !PassesSearchFilter(Actor))
        {
            continue;
        }

        switch (GetActorFolder(Actor))
        {
            case EActorFolder::Cameras:  bHasCameras  = true; break;
            case EActorFolder::Lighting: bHasLighting = true; break;
            default:                     bHasActors   = true; break;
        }
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
    const float ChildIndent = Style.IndentSpacing;

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
    // Row helper lambdas
    // -----------------------------------------------------------------------------------------

    const auto DrawFolderRow = [&](const CHAR* Label, const CHAR* Type, const CHAR* OpenKey, bool bDefaultOpen, float Indent) -> bool
    {
        ImGui::PushID(OpenKey);

        ImGuiStorage* Storage = ImGui::GetStateStorage();

        const ImGuiID OpenId  = ImGui::GetID("Open");
        bool bOpen = Storage->GetBool(OpenId, bDefaultOpen);

        ImGui::TableNextRow(ImGuiTableRowFlags_None, DefaultRowHeight);

        // -------------------------------------------------------------------------------------------
        // Column 0: Empty
        // -------------------------------------------------------------------------------------------

        ImGui::TableSetColumnIndex(0);

        const ImGuiSelectableFlags SelectableFlags =
            ImGuiSelectableFlags_SpanAllColumns |
            ImGuiSelectableFlags_AllowItemOverlap;

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, RowHoverBg);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, RowHoverBg);

        const bool bPressed = ImGui::Selectable("##Row", false, SelectableFlags, ImVec2(0.0f, DefaultRowHeight));
        if (bPressed)
        {
            bSelectionActiveInTable = true;
            bOpen = !bOpen;

            Storage->SetBool(OpenId, bOpen);
        }

        ImGui::PopStyleColor(2);

        const ImVec2 RowMin = ImGui::GetItemRectMin();
        const ImVec2 RowMax = ImGui::GetItemRectMax();

        const float TextHeight = ImGui::GetTextLineHeight();
        const float TextY      = RowMin.y + (DefaultRowHeight - TextHeight) * 0.5f;
        const float IconSz = 16.0f;

        // -------------------------------------------------------------------------------------------
        // Column 1: Label
        // -------------------------------------------------------------------------------------------

        ImGui::TableSetColumnIndex(1);

        const ImVec2 ColPos   = ImGui::GetCursorScreenPos();
        const ImVec2 IconPos  = ImVec2(ColPos.x + Indent, RowMin.y + (DefaultRowHeight - IconSz) * 0.5f);
        const ImU32  Tint     = IM_COL32(101, 101, 101, 255);

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

        ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);
        
        {
            TStaticArray<CHAR, 256> FilterBuf{};

            const CHAR* FilterText  = EditorHelpers::GetTrimmedQuery(ActorSearchFilterBuffer.Data(), FilterBuf.Data(), static_cast<int32>(FilterBuf.Size()));
            const ImU32 BaseTextU32 = ImGui::GetColorU32(ImGuiCol_Text);

            EditorWidgets::DrawTextWithSearchHighlight(DrawList, ImVec2(X, TextY), Label, FilterText, BaseTextU32, 1.0f, 1.0f, &RowMin, &RowMax);
        }

        ImGui::PopStyleColor();

        // -------------------------------------------------------------------------------------------
        // Column 2: Type
        // -------------------------------------------------------------------------------------------

        ImGui::TableSetColumnIndex(2);
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));
        
        ImGui::PushStyleColor(ImGuiCol_Text, TypeTextColor);
        ImGui::TextUnformatted(Type);
        ImGui::PopStyleColor();

        ImGui::PopID();
        return bOpen;
    };

    // -----------------------------------------------------------------------------------------
    // Build tree
    // -----------------------------------------------------------------------------------------

    const auto DrawFolderContents = [&](EActorFolder Folder)
    {
        for (FActor* Actor : Actors)
        {
            if (!Actor || Actor->GetParentActor() || GetActorFolder(Actor) != Folder || !PassesSearchFilter(Actor))
            {
                continue;
            }

            DrawActorRow(Actor, ChildIndent);
        }
    };

    if (bHasCameras && DrawFolderRow("Cameras", "Folder", "CamerasFolder", true, 0.0f))
    {
        DrawFolderContents(EActorFolder::Cameras);
    }

    if (bHasActors && DrawFolderRow("Actors", "Folder", "ActorsFolder", true, 0.0f))
    {
        DrawFolderContents(EActorFolder::Actors);
    }

    if (bHasLighting && DrawFolderRow("Lighting", "Folder", "LightingFolder", true, 0.0f))
    {
        DrawFolderContents(EActorFolder::Lighting);
    }

    ImGui::PopStyleVar(2); // CellPadding + ItemSpacing

    ImGui::EndTable();
    ImGui::SetCursorPosX(SavedCursorX);

    ImGui::PopStyleColor(4);

    // Dropping an actor on the empty area below the rows detaches it, which is what makes it a root-actor again
    if (ImGui::BeginDragDropTargetCustom(ImRect(TableRectMin, TableRectMax), ImGui::GetID("##SceneOutlinerDetachTarget")))
    {
        const ImGuiDragDropFlags DragDropFlags =
            ImGuiDragDropFlags_AcceptBeforeDelivery |
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect;

        if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ActorDragDropPayloadId, DragDropFlags))
        {
            FActor* DraggedActor = GetDraggedActor(Payload);
            if (Payload->IsDelivery() && !bDragHoveringSourceRow && DraggedActor && DraggedActor->GetParentActor())
            {
                RequestAttachment(DraggedActor, nullptr);
            }
        }

        ImGui::EndDragDropTarget();
    }

    // -----------------------------------------------------------------------------------------
    // Context menu
    // -----------------------------------------------------------------------------------------

    if (EditorWidgets::BeginPopupContextItem("SceneHierarchyContextMenu"))
    {
        EditorWidgets::MenuLabeledSeparator("Create");

        if (EditorWidgets::MenuItem("Add Actor", nullptr, false, false))
        {
            // TODO
        }

        EditorWidgets::MenuLabeledSeparator("Common");
        
        if (EditorWidgets::MenuItem("Delete", "Delete", false, false))
        {
            // TODO
        }

        FActor* SelectedActorForMenu = EditorEngine->GetSelectedActor();
        const bool bHasActorSelected = (SelectedActorForMenu != nullptr);
        if (EditorWidgets::MenuItem("Rename", "F2", false, bHasActorSelected))
        {
            if (SelectedActorForMenu)
            {
                bRequestRenameFocus = true;
                RenamingActor       = SelectedActorForMenu;
                
                ActorRenameBuffer.Fill(0);
                ActorRenameBufferOriginal.Fill(0);
        
                const String& Name = SelectedActorForMenu->GetName();
                if (!Name.IsEmpty())
                {
                    CString::Strncpy(ActorRenameBuffer.Data(), *Name, ActorRenameBuffer.Size());
                    CString::Strncpy(ActorRenameBufferOriginal.Data(), *Name, ActorRenameBufferOriginal.Size());
                }
            }
        }

        const bool bHasParent = bHasActorSelected && SelectedActorForMenu->GetParentActor() != nullptr;
        if (EditorWidgets::MenuItem("Detach from Parent", nullptr, false, bHasParent))
        {
            RequestAttachment(SelectedActorForMenu, nullptr);
        }

        if (EditorWidgets::MenuItem("Copy", "Ctrl+C", false, false))
        {
            // TODO
        }

        if (EditorWidgets::MenuItem("Copy", "Ctrl+V", false, false))
        {
            // TODO
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
                if (!ImGui::IsAnyItemHovered())
                {
                    EditorEngine->ClearSelection();

                    RenamingActor       = nullptr;
                    bRequestRenameFocus = false;
                }

                bSelectionActiveInTable = true;
            }
        }
    }

    ApplyPendingAttachment();
}

void FEditorSceneHierarchyWidget::DrawActorRow(FActor* Actor, float Indent)
{
    if (!Actor)
    {
        return;
    }

    constexpr float DefaultRowHeight = 30.0f;

    const CHAR* Type      = GetActorTypeLabel(Actor);
    const bool  bSelected = (EditorEngine->GetSelectedActor() == Actor);

    const auto BeginActorRename = [this](FActor* InActor)
    {
        bRequestRenameFocus = true;
        RenamingActor       = InActor;

        ActorRenameBuffer.Fill(0);
        ActorRenameBufferOriginal.Fill(0);

        const String& Name = InActor->GetName();
        if (!Name.IsEmpty())
        {
            CString::Strncpy(ActorRenameBuffer.Data(), *Name, ActorRenameBuffer.Size());
            CString::Strncpy(ActorRenameBufferOriginal.Data(), *Name, ActorRenameBufferOriginal.Size());
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
            RenamingActor->SetName(String(ActorRenameBuffer.Data()));
            RenamingActor = nullptr;
        }

        bRequestRenameFocus = false;
    };

    // -------------------------------------------------------------------------------------------
    // Colors
    // -------------------------------------------------------------------------------------------

    const ImVec4 ActorNameTextColor    = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
    const ImVec4 ActorTypeTextColor    = ImVec4(124.0f / 255.0f, 124.0f / 255.0f, 124.0f / 255.0f, 1.0f);
    const ImU32  SelectedActiveColor   = IM_COL32(0, 112, 224, 255);
    const ImU32  SelectedInactiveColor = IM_COL32(64, 87, 111, 255);
    const ImU32  SelectedColor         = bSelectionActiveInTable ? SelectedActiveColor : SelectedInactiveColor;
    const ImU32  RowBlue_Rename        = IM_COL32(0x3f, 0x7b, 0xb6, 160);
    const ImVec4 SearchBg              = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
    const ImU32  BorderNormal          = IM_COL32(51, 51, 51, 255);
    const ImU32  BorderHovered         = IM_COL32(74, 74, 74, 255);
    const ImU32  BorderActive          = IM_COL32(9, 92, 176, 255);

    ImGuiStyle& Style = ImGui::GetStyle();
    ImGui::PushID(Actor);

    bool bIsRenamingThis = (RenamingActor == Actor);
    if (bIsRenamingThis && !bSelected)
    {
        CommitActorRename();
        bIsRenamingThis = false;
    }

    {
        const ImGuiIO& IO = ImGui::GetIO();

        const bool bWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (bSelected && bSelectionActiveInTable && bWindowFocused && !bIsRenamingThis && !IO.WantTextInput)
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
        ImGuiSelectableFlags_AllowItemOverlap;

    const ImU32  RowSelectedColor = bIsRenamingThis ? RowBlue_Rename : SelectedColor;
    const ImVec4 RowHoverBg       = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

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
                RequestAttachment(DraggedActor, Actor);
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

        if (bHasChildren && bInArrowCol)
        {
            SetActorExpanded(Actor, !IsActorExpanded(Actor));
        }
        else if (!bSelected)
        {
            EditorEngine->SetSelectedActor(Actor);
            CancelActorRename();
            bIsRenamingThis = false;
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

        ImGui::PushStyleColor(ImGuiCol_FrameBg, SearchBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, SearchBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, SearchBg);
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

        const bool bEnter = ImGui::InputText("##RenameActor", ActorRenameBuffer.Data(), ActorRenameBuffer.Size(), InputFlags);

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

            const ImU32 BorderColor = bActive ? BorderActive : (bHovered ? BorderHovered : BorderNormal);

            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            DrawList->AddRect(ItemMin, ItemMax, BorderColor, BorderRounding, ImDrawListFlags_AntiAliasedLines, BorderThickness);
        }

        if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            CString::Strncpy(ActorRenameBuffer.Data(), ActorRenameBufferOriginal.Data(), ActorRenameBuffer.Size());
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

        TStaticArray<CHAR, 256> FilterBuf{};
        
        const CHAR* FilterText  = EditorHelpers::GetTrimmedQuery(ActorSearchFilterBuffer.Data(), FilterBuf.Data(), static_cast<int32>(FilterBuf.Size()));
        const ImU32 BaseTextU32 = ImGui::GetColorU32(ImGuiCol_Text);

        const String& Name = Actor->GetName();
        const CHAR* NameText = Name.IsEmpty() ? "Actor" : *Name;
        EditorWidgets::DrawTextWithSearchHighlight(ImGui::GetWindowDrawList(), ImVec2(LabelStartX, NameTextY), NameText, FilterText, BaseTextU32, 1.0f, 1.0f, &RowMin, &RowMax);

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

void FEditorSceneHierarchyWidget::RequestAttachment(FActor* ChildActor, FActor* ParentActor)
{
    PendingAttachChild  = ChildActor;
    PendingAttachParent = ParentActor;
    bPendingAttachment  = true;
}

void FEditorSceneHierarchyWidget::ApplyPendingAttachment()
{
    if (!bPendingAttachment)
    {
        return;
    }

    if (PendingAttachChild)
    {
        if (PendingAttachParent)
        {
            if (PendingAttachChild->AttachToActor(PendingAttachParent, EAttachmentRule::KeepWorld))
            {
                // Make sure the newly attached actor is not hidden inside a collapsed parent
                SetActorExpanded(PendingAttachParent, true);
            }
        }
        else
        {
            PendingAttachChild->DetachFromParent(EAttachmentRule::KeepWorld);
        }
    }

    PendingAttachChild  = nullptr;
    PendingAttachParent = nullptr;
    bPendingAttachment  = false;
}
