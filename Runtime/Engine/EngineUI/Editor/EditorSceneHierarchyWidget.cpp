#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EditorEngine.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include <imgui.h>
#include <imgui_internal.h>

static const CHAR* GetTrimmedQuery(const CHAR* InText, CHAR* OutBuf, int32 OutBufSize)
{
    if (!OutBuf || OutBufSize <= 0)
    {
        return nullptr;
    }

    OutBuf[0] = 0;

    if (!InText)
    {
        return nullptr;
    }

    // Skip leading whitespace
    const CHAR* Start = InText;
    while (*Start && (*Start == ' ' || *Start == '\t' || *Start == '\n' || *Start == '\r'))
    {
        ++Start;
    }

    // Find end
    const CHAR* End = Start;
    while (*End)
    {
        ++End;
    }

    // Trim trailing whitespace
    while (End > Start && (End[-1] == ' ' || End[-1] == '\t' || End[-1] == '\n' || End[-1] == '\r'))
    {
        --End;
    }

    const int32 Len = (int32)(End - Start);
    if (Len <= 0)
    {
        return nullptr;
    }

    const int32 CopyLen = Math::Min(Len, OutBufSize - 1);
    FCString::Strncpy(OutBuf, Start, CopyLen + 1);
    OutBuf[CopyLen] = 0;

    return OutBuf[0] ? OutBuf : nullptr;
}

static void DrawTextWithSearchHighlight(ImDrawList* InDrawList, const ImVec2& InTextPos, const ImVec2& InRowMin, const ImVec2& InRowMax, const CHAR* InText, const CHAR* InFilterText, ImU32 InBaseTextU32)
{
    if (!InDrawList || !InText)
    {
        return;
    }

    const CHAR* FilterText = (InFilterText && *InFilterText != 0) ? InFilterText : nullptr;

    int32 MatchStart = -1;
    int32 MatchLen   = 0;

    if (FilterText)
    {
        if (const CHAR* MatchPtr = FCString::Stristr(InText, FilterText))
        {
            MatchStart = static_cast<int32>(MatchPtr - InText);
            MatchLen   = static_cast<int32>(FCString::Strlen(FilterText));
        }
    }

    if (MatchStart >= 0 && MatchLen > 0)
    {
        const ImGuiIO& IO = ImGui::GetIO();

        const float Scale = IO.DisplayFramebufferScale.x;

        const ImU32 HighlightBgU32   = IM_COL32(139, 194, 74, 255);
        const ImU32 HighlightTextU32 = IM_COL32(0, 0, 0, 255);

        const ImVec2 PrefixSize = ImGui::CalcTextSize(InText, InText + MatchStart);
        const ImVec2 MatchSize  = ImGui::CalcTextSize(InText + MatchStart, InText + MatchStart + MatchLen);

        const ImVec2 PrefixPos = InTextPos;
        const ImVec2 MatchPos  = ImVec2(InTextPos.x + PrefixSize.x, InTextPos.y);
        const ImVec2 SuffixPos = ImVec2(MatchPos.x + MatchSize.x, InTextPos.y);

        if (MatchStart > 0)
        {
            InDrawList->AddText(PrefixPos, InBaseTextU32, InText, InText + MatchStart);
        }

        const float PadX = 1.0f * Scale;
        const float PadY = 1.0f * Scale;

        const float TextMinY = MatchPos.y;
        const float TextMaxY = MatchPos.y + MatchSize.y;

        ImVec2 HighlightMin = ImVec2(MatchPos.x - PadX, TextMinY - PadY);
        ImVec2 HighlightMax = ImVec2(MatchPos.x + MatchSize.x + PadX, TextMaxY + PadY);

        HighlightMin.y = ImMax(HighlightMin.y, InRowMin.y);
        HighlightMax.y = ImMin(HighlightMax.y, InRowMax.y);

        InDrawList->AddRectFilled(HighlightMin, HighlightMax, HighlightBgU32, 0.0f);

        InDrawList->AddText(MatchPos, HighlightTextU32, InText + MatchStart, InText + MatchStart + MatchLen);

        const CHAR* Suffix = InText + MatchStart + MatchLen;
        if (Suffix && *Suffix != 0)
        {
            InDrawList->AddText(SuffixPos, InBaseTextU32, Suffix);
        }
    }
    else
    {
        InDrawList->AddText(InTextPos, InBaseTextU32, InText);
    }
}

FEditorSceneHierarchyWidget::FEditorSceneHierarchyWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , RenamingActor(nullptr)
    , bVisible(true)
    , bRequestRenameFocus(false)
    , bSelectionActiveInTable(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorSceneHierarchyWidget::Draw));
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
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
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

    const ImVec4 RowHoverBg            = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    const ImVec4 SearchBg              = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
    const ImVec4 SearchTextColor       = ImVec4(77.0f / 255.0f, 77.0f / 255.0f, 77.0f / 255.0f, 1.0f);
    const ImVec4 NameTextColor         = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
    const ImVec4 TypeTextColor         = ImVec4(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);
    const ImU32  SelectedActiveColor   = IM_COL32(0, 112, 224, 255);
    const ImU32  SelectedInactiveColor = IM_COL32(64, 87, 111, 255);

    // -----------------------------------------------------------------------------------------
    // Row helpers
    // -----------------------------------------------------------------------------------------

    const auto DrawFolderRow = [&](const CHAR* Label, const CHAR* Type, const CHAR* OpenKey, bool bDefaultOpen, float IndentPx) -> bool
    {
        ImGuiStyle& Style = ImGui::GetStyle();
        ImGui::PushID(OpenKey);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        const ImGuiSelectableFlags SelectableFlags =
            ImGuiSelectableFlags_SpanAllColumns |
            ImGuiSelectableFlags_AllowItemOverlap;

        const ImGuiID OpenId = ImGui::GetID("Open");

        ImGuiStorage* Storage = ImGui::GetStateStorage();
        bool bOpen = Storage->GetBool(OpenId, bDefaultOpen);

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, RowHoverBg);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, RowHoverBg);

        float RowHeight = ImGui::GetTextLineHeight() + Style.CellPadding.y * 2.0f;
        if (ImGui::Selectable("##Row", false, SelectableFlags, ImVec2(0.0f, RowHeight)))
        {
            bSelectionActiveInTable = true; // clicked in table on something
            bOpen = !bOpen;
            Storage->SetBool(OpenId, bOpen);
        }

        ImGui::PopStyleColor(2);

        const ImVec2 RowMin = ImGui::GetItemRectMin();
        const ImVec2 RowMax = ImGui::GetItemRectMax();
        RowHeight = RowMax.y - RowMin.y;

        const float TextHeight   = ImGui::GetTextLineHeight();
        const float FontSize     = ImGui::GetFontSize();
        const float TextY        = RowMin.y + (RowHeight - TextHeight) * 0.5f;
        const float ArrowY       = RowMin.y + (RowHeight - FontSize) * 0.5f;
        const float ArrowTextGap = 6.0f;

        // Folder icon between the arrow and the label.
        const float FolderIconGapPx  = 6.0f;
        const float FolderIconSizePx = Math::Min(16.0f, Math::Max(1.0f, RowHeight - 6.0f));

        ImGui::TableSetColumnIndex(0);
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));

        ImGui::TableSetColumnIndex(1);

        const ImVec2 CollumnPosition = ImGui::GetCursorScreenPos();
        const ImVec2 ArrowPosition   = ImVec2(CollumnPosition.x + IndentPx, ArrowY);

        ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();

        const float  IconSizePx = 16.0f;
        const ImVec2 IconPos    = ImVec2(CollumnPosition.x + IndentPx, RowMin.y + (RowHeight - IconSizePx) * 0.5f);
        const ImU32  Tint       = IM_COL32(101, 101, 101, 255);

        ImTextureID ArrowIcon = bOpen ? EditorIcons::CollapseArrowDown : EditorIcons::CollapseArrowRight;
        if (ArrowIcon)
        {
            DrawList->AddImage(ArrowIcon, IconPos, ImVec2(IconPos.x + IconSizePx, IconPos.y + IconSizePx), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), Tint);
        }
        else
        {
            const ImGuiDir Dir = bOpen ? ImGuiDir_Down : ImGuiDir_Right;
            ImGui::RenderArrow(DrawList, IconPos, Tint, Dir, 1.0f);
        }

        float LabelX = IconPos.x + IconSizePx + ArrowTextGap;
        if (EditorIcons::FolderSmallIcon || EditorIcons::FolderOpenSmallIcon)
        {
            ImTextureID FolderIcon = bOpen ? EditorIcons::FolderOpenSmallIcon : EditorIcons::FolderSmallIcon;
            if (FolderIcon)
            {
                const float  IconY   = RowMin.y + (RowHeight - FolderIconSizePx) * 0.5f;
                const ImVec2 IconMin = ImVec2(LabelX, IconY);
                const ImVec2 IconMax = ImVec2(IconMin.x + FolderIconSizePx, IconMin.y + FolderIconSizePx);

                DrawList->AddImage(FolderIcon, IconMin, IconMax);
                LabelX = IconMax.x + FolderIconGapPx;
            }
        }

        ImGui::SetCursorScreenPos(ImVec2(LabelX, TextY));

        // Highlight matches from the Scene Hierarchy search field (same style as Content Browser).
        CHAR FilterBuf[256];
        const CHAR* FilterText  = GetTrimmedQuery(ActorSearchFilterBuffer.Data(), FilterBuf, static_cast<int32>(sizeof(FilterBuf)));
        const ImU32 BaseTextU32 = ImGui::GetColorU32(ImGuiCol_Text);
        DrawTextWithSearchHighlight(DrawList, ImVec2(LabelX, TextY), RowMin, RowMax, Label, FilterText, BaseTextU32);

        ImGui::PopStyleColor();

        ImGui::TableSetColumnIndex(2);
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));

        ImGui::PushStyleColor(ImGuiCol_Text, TypeTextColor);
        ImGui::TextUnformatted(Type);
        ImGui::PopStyleColor();

        ImGui::PopID();
        return bOpen;
    };

    const auto DrawLeafRow = [&](const CHAR* Label, const CHAR* Type, bool bSelected, void* Id, float IndentPx, auto&& OnClick)
    {
        ImGuiStyle& Style = ImGui::GetStyle();
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGui::PushID(Id);

        const ImGuiSelectableFlags SelectableFlags =
            ImGuiSelectableFlags_SpanAllColumns |
            ImGuiSelectableFlags_AllowItemOverlap;

        if (bSelected)
        {
            const ImU32 SelColor = bSelectionActiveInTable ? SelectedActiveColor : SelectedInactiveColor;
            ImGui::PushStyleColor(ImGuiCol_Header, SelColor);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SelColor);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, SelColor);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, RowHoverBg);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, RowHoverBg);
        }

        float RowHeight = ImGui::GetTextLineHeight() + Style.CellPadding.y * 2.0f;
        if (ImGui::Selectable("##Row", bSelected, SelectableFlags, ImVec2(0.0f, RowHeight)))
        {
            bSelectionActiveInTable = true;
            OnClick();
        }

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
        RowHeight = RowMax.y - RowMin.y;

        const float TextHeight   = ImGui::GetTextLineHeight();
        const float FontSize     = ImGui::GetFontSize();
        const float TextY        = RowMin.y + (RowHeight - TextHeight) * 0.5f;
        const float ArrowTextGap = 6.0f;
        const float ArrowAdvance = FontSize;

        ImGui::TableSetColumnIndex(0);
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));

        ImGui::TableSetColumnIndex(1);

        const ImVec2 Col1Pos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(Col1Pos.x + IndentPx + ArrowAdvance + ArrowTextGap, TextY));

        ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);

        CHAR FilterBuf[256];
        const CHAR* FilterText  = GetTrimmedQuery(ActorSearchFilterBuffer.Data(), FilterBuf, static_cast<int32>(sizeof(FilterBuf)));
        const ImU32 BaseTextU32 = ImGui::GetColorU32(ImGuiCol_Text);
        DrawTextWithSearchHighlight(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos(), RowMin, RowMax, Label, FilterText, BaseTextU32);

        ImGui::PopStyleColor();

        ImGui::TableSetColumnIndex(2);
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));

        ImGui::PushStyleColor(ImGuiCol_Text, TypeTextColor);
        ImGui::TextUnformatted(Type);
        ImGui::PopStyleColor();

        ImGui::PopID();
    };

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

    // Pull current selection
    FActor*      SelectedActor      = EditorEngine->GetSelectedActor();
    FLightProbe* SelectedLightProbe = EditorEngine->GetSelectedLightProbe();
    FLight*      SelectedLight      = EditorEngine->GetSelectedLight();
    FCamera*     SelectedCamera     = EditorEngine->GetSelectedCamera();

    const bool bHasAnySelection = (SelectedActor != nullptr) || (SelectedLightProbe != nullptr) || (SelectedLight != nullptr) || (SelectedCamera != nullptr);

    // Pull camera (Currently only a single camera)
    FCamera* Camera = World->GetCamera();

    // Scene data
    const TArray<FActor*>&      Actors      = World->GetActors();
    const TArray<FLight*>&      Lights      = World->GetLights();
    const TArray<FLightProbe*>& LightProbes = World->GetLightProbes();

    const bool bHasActors   = !Actors.IsEmpty();
    const bool bHasLights   = !Lights.IsEmpty();
    const bool bHasProbes   = !LightProbes.IsEmpty();
    const bool bHasCameras  = Camera != nullptr;
    const bool bHasLighting = bHasLights || bHasProbes;

    // -----------------------------------------------------------------------------------------
    // Search Field
    // -----------------------------------------------------------------------------------------

    const ImVec2 OldWindowPadding = ImGui::GetStyle().WindowPadding;
    const ImVec2 OldItemSpacing   = ImGui::GetStyle().ItemSpacing;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(OldWindowPadding.x, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(OldItemSpacing.x, 0.0f));

    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    EditorWidgets::EditorSearchField("##SceneHierarchySearch", "Search Actors", ActorSearchFilterBuffer.Data(), ActorSearchFilterBuffer.Size());
    ImGui::Dummy(ImVec2(0.0f, 12.0f));

    ImGui::PopStyleVar(2);

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

    ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f));

    const ImVec4 BorderDarkGray = ImVec4(37.0f / 255.0f, 37.0f / 255.0f, 37.0f / 255.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, BorderDarkGray);
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, BorderDarkGray);

    const float SavedCursorX = ImGui::GetCursorPosX();
    ImGui::SetCursorPosX(0.0f);

    const float TableHeight = ImGui::GetContentRegionAvail().y;
    const float FullWidth   = ImGui::GetContentRegionAvail().x + Style.WindowPadding.x;

    const ImVec2 TableSize = ImVec2(FullWidth, TableHeight);

    // Track the table rect so we can apply your click rules.
    const ImVec2 TableRectMin = ImGui::GetCursorScreenPos();
    const ImVec2 TableRectMax = ImVec2(TableRectMin.x + TableSize.x, TableRectMin.y + TableSize.y);

    if (!ImGui::BeginTable("##SceneOutliner", 3, TableFlags, TableSize))
    {
        ImGui::SetCursorPosX(SavedCursorX);
        ImGui::PopStyleColor(4); // RowBg/Alt + BorderLight/Strong
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);

    // Fill the whole table area (including empty space below rows)
    {
        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        const ImU32 Bg = IM_COL32(26, 26, 26, 255);
        DrawList->AddRectFilled(TableRectMin, TableRectMax, Bg);
    }

    const float TypeColWidth = ImGui::CalcTextSize("DirectionalLight").x + Style.CellPadding.x * 2.0f + 12.0f;
    ImGui::TableSetupColumn("##Gutter", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_NoResize, 8.0f);
    ImGui::TableSetupColumn("Item Label", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, TypeColWidth);

    const float CellPaddingY = Math::Max(0.0f, EditorStyleVars::SceneHierarchyTableRowHeight - ImGui::GetTextLineHeight()) * 0.5f;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0, CellPaddingY));
    
    const ImVec4 HeaderBg      = ImVec4(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);
    const ImVec4 HeaderBgHover = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, HeaderBg);
    ImGui::PushStyleColor(ImGuiCol_Header, HeaderBg);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HeaderBgHover);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, HeaderBgHover);

    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

    float HeaderMinY = 0.0f;
    float HeaderMaxY = 0.0f;
    float Sep01X     = 0.0f; // between column 0 and 1
    float Sep12X     = 0.0f; // between column 1 and 2

    for (int32 Column = 0; Column < 3; Column++)
    {
        ImGui::TableSetColumnIndex(Column);
        ImGui::TableHeader(ImGui::TableGetColumnName(Column));

        const ImVec2 CellMin = ImGui::GetItemRectMin();
        const ImVec2 CellMax = ImGui::GetItemRectMax();

        if (Column == 1)
        {
            HeaderMinY = CellMin.y;
            HeaderMaxY = CellMax.y;
            Sep01X     = CellMin.x;
        }
        else if (Column == 2)
        {
            Sep12X = CellMin.x;
        }
    }

    {
        const ImU32 BorderCol = ImGui::GetColorU32(ImGuiCol_TableBorderStrong);
        const float Thickness = 2.0f;

        float X0 = Math::Floor(Sep01X) + 0.5f;
        float X1 = Math::Floor(Sep12X) + 0.5f;

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddLine(ImVec2(X0, HeaderMinY), ImVec2(X0, HeaderMaxY), BorderCol, Thickness);
        DrawList->AddLine(ImVec2(X1, HeaderMinY), ImVec2(X1, HeaderMaxY), BorderCol, Thickness);
    }

    ImGui::PopStyleColor(4); // Header colors
    ImGui::PopStyleVar();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));

    // Cameras
    if (bHasCameras)
    {
        const bool bCamerasOpen = DrawFolderRow("Cameras", "Folder", "CamerasFolder", true, 0.0f);
        if (bCamerasOpen)
        {
            bool bCameraFound = true;

            const CHAR* Search = ActorSearchFilterBuffer.Data();
            if (Search && Search[0] != '\0')
            {
                // Match either the displayed name or the type (nice when user searches "camera")
                const CHAR* CameraName = "Main Camera";
                if (!FCString::Stristr(CameraName, Search) && !FCString::Stristr("Camera", Search))
                {
                    bCameraFound = false;
                }
            }

            if (bCameraFound)
            {
                DrawLeafRow("Main Camera", "Camera", Camera == SelectedCamera, (void*)Camera, ChildIndent, [&]()
                {
                    EditorEngine->SetSelectedCamera(Camera);
                });
            }
        }
    }

    // Actors folder
    if (bHasActors)
    {
        const bool bActorsOpen = DrawFolderRow("Actors", "Folder", "ActorsFolder", true, 0.0f);
        if (bActorsOpen)
        {
            for (FActor* Actor : Actors)
            {
                if (!Actor)
                {
                    continue;
                }

                const CHAR* Search = ActorSearchFilterBuffer.Data();
                if (Search && Search[0] != '\0')
                {
                    const FString& Name = Actor->GetName();
                    if (Name.IsEmpty() || !FCString::Stristr(*Name, Search))
                    {
                        continue;
                    }
                }

                DrawActorRow(Actor, "Actor", Actor == SelectedActor, ChildIndent);
            }
        }
    }

    // Lighting folder
    if (bHasLighting)
    {
        const bool bLightingOpen = DrawFolderRow("Lighting", "Folder", "LightingFolder", true, 0.0f);
        if (bLightingOpen)
        {
            // Lights
            if (bHasLights)
            {
                int32 LightIndex = 0;
                for (FLight* Light : Lights)
                {
                    if (!Light)
                    {
                        continue;
                    }

                    const CHAR* TypeLabel = "Light";
                    if (Cast<FPointLight>(Light))
                    {
                        TypeLabel = "PointLight";
                    }
                    else if (Cast<FDirectionalLight>(Light))
                    {
                        TypeLabel = "DirectionalLight";
                    }

                    constexpr uint32 LabelLength = 256;
                    CHAR Label[LabelLength];
                    FCString::Snprintf(Label, LabelLength, "%s %d", TypeLabel, LightIndex++);

                    const CHAR* Search = ActorSearchFilterBuffer.Data();
                    if (Search && Search[0] != '\0')
                    {
                        if (!FCString::Stristr(Label, Search))
                        {
                            continue;
                        }
                    }

                    DrawLeafRow(Label, TypeLabel, Light == SelectedLight, (void*)Light, ChildIndent, [&]()
                    {
                        EditorEngine->SetSelectedLight(Light);
                    });
                }
            }

            // Light Probes
            if (bHasProbes)
            {
                int32 ProbeIndex = 0;
                for (FLightProbe* Probe : LightProbes)
                {
                    if (!Probe)
                    {
                        continue;
                    }

                    constexpr uint32 LabelLength = 256;
                    CHAR Label[LabelLength];
                    FCString::Snprintf(Label, LabelLength, "LightProbe %d", ProbeIndex++);

                    const CHAR* Search = ActorSearchFilterBuffer.Data();
                    if (Search && Search[0] != '\0')
                    {
                        if (!FCString::Stristr(Label, Search))
                        {
                            continue;
                        }
                    }

                    DrawLeafRow(Label, "LightProbe", Probe == SelectedLightProbe, (void*)Probe, ChildIndent, [&]()
                    {
                        EditorEngine->SetSelectedLightProbe(Probe);
                    });
                }
            }
        }
    }

    ImGui::PopStyleVar(); // CellPadding
    ImGui::EndTable();

    ImGui::SetCursorPosX(SavedCursorX);

    ImGui::PopStyleColor(4); // RowBg/Alt + BorderLight/Strong

    // -----------------------------------------------------------------------------------------
    // Click rules
    // -----------------------------------------------------------------------------------------
    if (EditorEngine && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        // If another window is on overlapping, the scene hierarchy panel is NOT hovered.
        const bool bHierarchyHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        if (!bHierarchyHovered)
        {
            // Click went to another window -> keep selection but make it inactive.
            if (bHasAnySelection)
            {
                bSelectionActiveInTable = false;
            }
        }
        else
        {
            // Click is inside Scene Hierarchy window.
            const ImVec2 MousePos = ImGui::GetIO().MousePos;

            const bool bInTableRect = (MousePos.x >= TableRectMin.x && MousePos.x < TableRectMax.x) && (MousePos.y >= TableRectMin.y && MousePos.y < TableRectMax.y);
            if (!bInTableRect)
            {
                // Clicked in this window but outside the table (search bar/background) -> inactive selection.
                if (bHasAnySelection)
                {
                    bSelectionActiveInTable = false;
                }
            }
            else
            {
                // Clicked in the table area. If we didn't click any row/header/scrollbar item -> clear selection.
                if (!ImGui::IsAnyItemHovered())
                {
                    EditorEngine->ClearSelection();
                    RenamingActor       = nullptr;
                    bRequestRenameFocus = false;
                }

                // Any click in table makes selection "active" again (blue).
                bSelectionActiveInTable = true;
            }
        }
    }
}

void FEditorSceneHierarchyWidget::DrawActorRow(FActor* Actor, const CHAR* Type, const bool bSelected, float IndentPx)
{
    if (!Actor)
    {
        return;
    }

    const auto BeginActorRename = [this](FActor* InActor)
    {
        bRequestRenameFocus = true;
        RenamingActor       = InActor;

        ActorRenameBuffer.Fill(0);
        ActorRenameBufferOriginal.Fill(0);

        const FString& Name = InActor->GetName();
        if (!Name.IsEmpty())
        {
            FCString::Strncpy(ActorRenameBuffer.Data(), *Name, ActorRenameBuffer.Size());
            FCString::Strncpy(ActorRenameBufferOriginal.Data(), *Name, ActorRenameBufferOriginal.Size());
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
            RenamingActor->SetName(FString(ActorRenameBuffer.Data()));
            RenamingActor = nullptr;
        }

        bRequestRenameFocus = false;
    };

    // Colors
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

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    ImGui::PushID(Actor);

    bool bIsRenamingThis = (RenamingActor == Actor);
    if (bIsRenamingThis && !bSelected)
    {
        CommitActorRename();
        bIsRenamingThis = false;
    }

    const ImGuiSelectableFlags SelectableFlags =
        ImGuiSelectableFlags_SpanAllColumns |
        ImGuiSelectableFlags_AllowItemOverlap;

    const ImU32  RowSelectedColor = bIsRenamingThis ? RowBlue_Rename : SelectedColor;
    const ImVec4 RowHoverBg       = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

    // Selected actors use active/inactive color. Non-selected actors use consistent hover.
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

    float RowHeight = ImGui::GetTextLineHeight() + Style.CellPadding.y * 2.0f;

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
    RowHeight = RowMax.y - RowMin.y;

    const float TextHeight   = ImGui::GetTextLineHeight();
    const float TextY        = RowMin.y + (RowHeight - TextHeight) * 0.5f;
    const float FontSize     = ImGui::GetFontSize();
    const float ArrowAdvance = FontSize;
    const float ArrowTextGap = 6.0f;

    // Column 0: Empty
    ImGui::TableSetColumnIndex(0);
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, TextY));

    // Column 1: Label
    ImGui::TableSetColumnIndex(1);

    const ImVec2 Column1Pos   = ImGui::GetCursorScreenPos();
    const float  Column1Width = ImGui::GetContentRegionAvail().x;
    const float  Column1MinX  = Column1Pos.x;
    const float  Column1MaxX  = Column1Pos.x + Column1Width;
    const float  LabelStartX  = Column1Pos.x + IndentPx + ArrowAdvance + ArrowTextGap;
    const ImVec2 MousePos     = ImGui::GetIO().MousePos;

    const bool bClickedInLabelColumn = (MousePos.x >= Column1MinX && MousePos.x <= Column1MaxX);

    if (bRowPressed)
    {
        // Any click on a row should re-activate selection (blue)
        bSelectionActiveInTable = true;

        if (!bSelected)
        {
            EditorEngine->SetSelectedActor(Actor);
            CancelActorRename();
        }
        else
        {
            if (bClickedInLabelColumn && !bIsRenamingThis)
            {
                BeginActorRename(Actor);
                bIsRenamingThis = true;
            }
        }
    }

    const float RenameFramePadY = Style.CellPadding.y;

    if (bIsRenamingThis)
    {
        const float InputY     = TextY - RenameFramePadY;
        const float InputX     = LabelStartX - Style.FramePadding.x;
        const float InputWidth = (Column1MaxX - InputX) - 2.0f;

        ImGui::SetCursorScreenPos(ImVec2(InputX, InputY));
        ImGui::SetNextItemWidth(InputWidth > 0.0f ? InputWidth : 0.0f);

        const float BorderRounding  = 16.0f;
        const float BorderThickness = 2.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, BorderRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Style.FramePadding.x, RenameFramePadY));

        // Same background as search box
        ImGui::PushStyleColor(ImGuiCol_FrameBg, SearchBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, SearchBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, SearchBg);

        // Rename text should match actor name color
        ImGui::PushStyleColor(ImGuiCol_Text, ActorNameTextColor);

        if (bRequestRenameFocus)
        {
            ImGui::SetKeyboardFocusHere();
            bRequestRenameFocus = false;
        }

        const ImGuiInputTextFlags InputFlags =
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_AutoSelectAll;

        const bool bEnter = ImGui::InputText("##RenameActor", ActorRenameBuffer.Data(), ActorRenameBuffer.Size(), InputFlags);

        ImGui::PopStyleColor(4); // FrameBg x3 + Text
        ImGui::PopStyleVar(2);

        // Always draw border like search box
        {
            const ImVec2 ItemMin = ImGui::GetItemRectMin();
            const ImVec2 ItemMax = ImGui::GetItemRectMax();

            const bool bActive  = ImGui::IsItemActive();
            const bool bHovered = ImGui::IsItemHovered();

            const ImU32 BorderColor = bActive ? BorderActive : (bHovered ? BorderHovered : BorderNormal);

            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            DrawList->AddRect(ItemMin, ItemMax, BorderColor, BorderRounding, 0, BorderThickness);
        }

        if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            FCString::Strncpy(ActorRenameBuffer.Data(), ActorRenameBufferOriginal.Data(), ActorRenameBuffer.Size());
            CancelActorRename();
        }
        else if (bEnter || ImGui::IsItemDeactivatedAfterEdit())
        {
            CommitActorRename();
        }
        else if (ImGui::IsItemDeactivated())
        {
            CancelActorRename();
        }
    }
    else
    {
        ImGui::SetCursorScreenPos(ImVec2(LabelStartX, TextY));

        const FString& Name = Actor->GetName();

        ImGui::PushStyleColor(ImGuiCol_Text, ActorNameTextColor);

        const CHAR* NameText = Name.IsEmpty() ? "Actor" : *Name;

        CHAR FilterBuf[256];
        const CHAR* FilterText  = GetTrimmedQuery(ActorSearchFilterBuffer.Data(), FilterBuf, (int32)sizeof(FilterBuf));
        const ImU32 BaseTextU32 = ImGui::GetColorU32(ImGuiCol_Text);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawTextWithSearchHighlight(DrawList, ImVec2(LabelStartX, TextY), RowMin, RowMax, NameText, FilterText, BaseTextU32);

        ImGui::PopStyleColor();
    }

    // Column 2: type
    ImGui::TableSetColumnIndex(2);

    const float TypeLabelX            = ImGui::GetCursorScreenPos().x;
    const float BaselineCompensationY = bIsRenamingThis ? RenameFramePadY : 0.0f;

    ImGui::SetCursorScreenPos(ImVec2(TypeLabelX, TextY - BaselineCompensationY));

    ImGui::PushStyleColor(ImGuiCol_Text, ActorTypeTextColor);
    ImGui::TextUnformatted(Type);
    ImGui::PopStyleColor();

    ImGui::PopID();
}
