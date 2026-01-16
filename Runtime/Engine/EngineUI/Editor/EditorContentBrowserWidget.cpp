#include "Engine/EngineUI/Editor/EditorContentBrowserWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include <imgui.h>
#include <imgui_internal.h>

FEditorContentBrowserWidget::FEditorContentBrowserWidget()
    : ImGuiDelegateHandle()
    , SelectedFolderIndex(0)
    , SelectedItemIndex(-1)
    , bSelectionActiveInBrowser(false)
    , bVisible(true)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorContentBrowserWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }

    FolderSearchBuffer.Fill(0);
    AssetSearchBuffer.Fill(0);

    RootFolders =
    {
        { "Content", true, { } },
        { "MoreContent", true, { } },
    };

    for (FileInfo& Folder : RootFolders)
    {
        Folder.FolderContents =
        {
            { "Content", true, { } },
            { "Materials", true, { } },
            { "Meshes", true, { } },
            { "Textures", true, { } },
            { "Scenes", true, { } },
        };

        for (FileInfo& SubFolder : Folder.FolderContents)
        {
            SubFolder.FolderContents =
            {
                { "Meshes", true, { } },
                { "Materials", true, { } },
                { "Textures", true, { } },
                { "Crate_01.asset", false, { } },
                { "Door.asset", false, { } },
                { "Wood.asset", false, { } },
                { "Stone.asset", false, { } },
                { "Metal.asset", false, { } },
            };
        }
    }

    SelectedFolderPath = { 0 };
}

FEditorContentBrowserWidget::~FEditorContentBrowserWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FEditorContentBrowserWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, EditorStyleVars::SceneHierarchyItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, EditorStyleVars::SceneHierarchyWindowPadding);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f));

    const ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin("Content Browser", &bVisible, WindowFlags))
    {
        DrawLayoutTable();
    }

    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void FEditorContentBrowserWidget::DrawLayoutTable()
{
    const ImGuiTableFlags LayoutFlags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_NoPadOuterX |
        ImGuiTableFlags_BordersInnerV;

    if (!ImGui::BeginTable("##ContentBrowserLayout", 2, LayoutFlags))
    {
        return;
    }

    ImGui::TableSetupColumn("##Folders", ImGuiTableColumnFlags_WidthFixed, 260.0f);
    ImGui::TableSetupColumn("##Content", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    DrawFolderPanel();

    ImGui::TableSetColumnIndex(1);
    DrawContentPanel();

    ImGui::EndTable();
}

const CHAR* FEditorContentBrowserWidget::GetTrimmedQuery(const TStaticArray<CHAR, 256>& InBuf) const
{
    const CHAR* Q = InBuf.Data();
    while (Q && *Q && FCharTraits::IsWhitespace(*Q))
    {
        ++Q;
    }

    return Q;
}

bool FEditorContentBrowserWidget::MatchesSearch(const CHAR* InName, const TStaticArray<CHAR, 256>& InBuf) const
{
    if (!InName)
    {
        return false;
    }

    const CHAR* Query = GetTrimmedQuery(InBuf);
    if (!Query || *Query == 0)
    {
        return true;
    }

    return FCString::Stristr(InName, Query) != nullptr;
}

bool FEditorContentBrowserWidget::HasChildFolders(const FileInfo& InFolder) const
{
    for (int32 i = 0; i < InFolder.FolderContents.Size(); ++i)
    {
        if (InFolder.FolderContents[i].bIsFolder)
        {
            return true;
        }
    }

    return false;
}

bool FEditorContentBrowserWidget::FolderTreeMatches(const FileInfo& InFolder) const
{
    const CHAR* Query = GetTrimmedQuery(FolderSearchBuffer);
    const bool bFolderSearchActive = (Query && *Query != 0);

    if (!bFolderSearchActive)
    {
        return true;
    }

    if (MatchesSearch(InFolder.Name, FolderSearchBuffer))
    {
        return true;
    }

    for (int32 i = 0; i < InFolder.FolderContents.Size(); ++i)
    {
        const FileInfo& Child = InFolder.FolderContents[i];
        if (!Child.bIsFolder)
        {
            continue;
        }

        if (FolderTreeMatches(Child))
        {
            return true;
        }
    }

    return false;
}

bool FEditorContentBrowserWidget::IsPathPrefixOfSelected(const TArray<int32>& InPath) const
{
    if (InPath.Size() <= 0)
    {
        return false;
    }

    if (SelectedFolderPath.Size() < InPath.Size())
    {
        return false;
    }

    for (int32 i = 0; i < InPath.Size(); ++i)
    {
        if (SelectedFolderPath[i] != InPath[i])
        {
            return false;
        }
    }

    return true;
}

void FEditorContentBrowserWidget::CenteredMessage(const char* InText, const ImVec4& InMutedTextColor)
{
    const ImVec2 Avail = ImGui::GetContentRegionAvail();
    const ImVec2 Size  = ImGui::CalcTextSize(InText);

    ImGui::SetCursorPos(ImVec2(Math::Max(0.0f, (Avail.x - Size.x) * 0.5f), Math::Max(0.0f, (Avail.y - Size.y) * 0.5f)));
    ImGui::PushStyleColor(ImGuiCol_Text, InMutedTextColor);
    ImGui::TextUnformatted(InText);
    ImGui::PopStyleColor();
}

void FEditorContentBrowserWidget::DrawSearchField(const char* InId, const char* InHint, TStaticArray<CHAR, 256>& InOutBuffer)
{
    EditorWidgets::EditorSearchField(InId, InHint, InOutBuffer.Data(), InOutBuffer.Size(), -1.0f, true);
}

void FEditorContentBrowserWidget::DrawFolderPanel()
{
    const ImVec4 PanelBg       = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
    const ImVec4 NameTextColor = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);

    const ImU32 FolderActiveColor   = IM_COL32(0, 112, 224, 255);
    const ImU32 FolderInactiveColor = IM_COL32(64, 87, 111, 255);
    const ImU32 FolderHoverColor    = IM_COL32(56, 56, 56, 255);
    const ImU32 FolderPathColor     = IM_COL32(44, 50, 58, 255);

    const bool bFolderSearchActive = (GetTrimmedQuery(FolderSearchBuffer) && *GetTrimmedQuery(FolderSearchBuffer) != 0);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, PanelBg);

    if (ImGui::BeginChild("##CB_Folders", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
    {
        DrawSearchField("##CB_FolderSearch", "Search Paths", FolderSearchBuffer);

        ImGui::Dummy(ImVec2(0.0f, 6.0f));

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, PanelBg);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(87, 87, 87, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(127, 127, 127, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(127, 127, 127, 255));

        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 16.0f);

        if (ImGui::BeginChild("##CB_FoldersScroll", ImVec2(0, 0), false, 0))
        {
            if (RootFolders.Size() > 0)
            {
                if (SelectedFolderPath.Size() <= 0 || !RootFolders.IsValidIndex(SelectedFolderPath[0]))
                {
                    SelectedFolderPath = { 0 };
                }
            }

            ImGuiStorage* Storage = ImGui::GetStateStorage();

            for (int32 RootIndex = 0; RootIndex < RootFolders.Size(); ++RootIndex)
            {
                FileInfo& Root = RootFolders[RootIndex];
                if (!Root.bIsFolder)
                {
                    continue;
                }

                if (bFolderSearchActive && !FolderTreeMatches(Root))
                {
                    continue;
                }

                TArray<int32> Path;
                Path.Add(RootIndex);

                DrawFolderTreeRecursive(Root, Path, 0, Storage, NameTextColor, FolderActiveColor, FolderInactiveColor, FolderHoverColor, FolderPathColor, bFolderSearchActive);
            }
        }

        ImGui::EndChild(); // ##CB_FoldersScroll

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void FEditorContentBrowserWidget::DrawFolderTreeRecursive(FileInfo& InFolder, TArray<int32>& InPath, int32 InDepth, ImGuiStorage* InStorage, const ImVec4& InNameTextColor,
    const ImU32 InFolderActiveColor, const ImU32 InFolderInactiveColor, const ImU32 InFolderHoverColor, const ImU32 InFolderPathColor, bool bFolderSearchActive)
{
    if (!FolderTreeMatches(InFolder))
    {
        return;
    }

    const bool bOpen = DrawFolderRow(InFolder, InPath, InDepth, InStorage, InNameTextColor, InFolderActiveColor, InFolderInactiveColor, InFolderHoverColor, InFolderPathColor, bFolderSearchActive);
    if (!bOpen)
    {
        return;
    }

    for (int32 ChildIndex = 0; ChildIndex < InFolder.FolderContents.Size(); ++ChildIndex)
    {
        FileInfo& Child = InFolder.FolderContents[ChildIndex];
        if (!Child.bIsFolder)
        {
            continue;
        }

        if (bFolderSearchActive && !FolderTreeMatches(Child))
        {
            continue;
        }

        InPath.Add(ChildIndex);
        DrawFolderTreeRecursive(Child, InPath, InDepth + 1, InStorage, InNameTextColor, InFolderActiveColor, InFolderInactiveColor, InFolderHoverColor, InFolderPathColor, bFolderSearchActive);
        InPath.Pop();
    }
}

bool FEditorContentBrowserWidget::DrawFolderRow(FileInfo& InFolder, const TArray<int32>& InPath, int32 InDepth, ImGuiStorage* InStorage, const ImVec4& InNameTextColor,
    const ImU32 InFolderActiveColor, const ImU32 InFolderInactiveColor, const ImU32 InFolderHoverColor, const ImU32 InFolderPathColor, bool bFolderSearchActive)
{
    const auto IsSelectedFolderPath = [&](const TArray<int32>& Path) -> bool
    {
        if (Path.Size() != SelectedFolderPath.Size())
        {
            return false;
        }

        for (int32 i = 0; i < Path.Size(); ++i)
        {
            if (Path[i] != SelectedFolderPath[i])
            {
                return false;
            }
        }

        return true;
    };

    const bool bSelected       = IsSelectedFolderPath(InPath);
    const bool bInSelectedPath = (!bSelected && IsPathPrefixOfSelected(InPath));
    const bool bWindowFocused  = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    const ImU32 SelectedColor = (bWindowFocused && bSelectionActiveInBrowser) ? InFolderActiveColor : InFolderInactiveColor;

    ImGui::PushID(reinterpret_cast<void*>(&InFolder));

    const bool bHasChildFolders = HasChildFolders(InFolder);
    const ImGuiID OpenId = ImGui::GetID("##CB_Open");

    bool bOpen = false;
    if (bHasChildFolders)
    {
        bOpen = bFolderSearchActive ? true : InStorage->GetBool(OpenId, (InDepth == 0));
    }

    if (bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, SelectedColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SelectedColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, InFolderActiveColor);
    }
    else if (bInSelectedPath)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, InFolderPathColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, InFolderHoverColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, InFolderActiveColor);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, InFolderHoverColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, InFolderActiveColor);
    }

    const float RowHeightPx = 20.0f;

    const ImGuiSelectableFlags SelFlags =
        ImGuiSelectableFlags_SpanAllColumns |
        ImGuiSelectableFlags_AllowItemOverlap;

    const bool bRowPressed = ImGui::Selectable("##FolderRow", (bSelected || bInSelectedPath), SelFlags, ImVec2(0.0f, RowHeightPx));
    const bool bRowHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (bRowPressed)
    {
        SelectedFolderPath        = InPath;
        SelectedItemIndex         = -1;
        bSelectionActiveInBrowser = true;
    }

    if (!bFolderSearchActive && bHasChildFolders && bRowHovered && ImGui::IsMouseClicked(0))
    {
        const int32 ClickCount = ImGui::GetMouseClickedCount(0);
        if ((ClickCount > 0) && ((ClickCount & 1) == 0))
        {
            bOpen = !bOpen;
            InStorage->SetBool(OpenId, bOpen);
        }
    }

    if (bSelected || bInSelectedPath)
    {
        ImGui::PopStyleColor(3);
    }
    else
    {
        ImGui::PopStyleColor(2);
    }

    const ImVec2 RowMin     = ImGui::GetItemRectMin();
    const ImVec2 RowMax     = ImGui::GetItemRectMax();
    const float  H          = RowMax.y - RowMin.y;
    const float  FontSize   = ImGui::GetFontSize();
    const float  TextHeight = ImGui::GetTextLineHeight();
    const float  TextY      = RowMin.y + (H - TextHeight) * 0.5f;
    const float  ArrowY     = RowMin.y + (H - FontSize) * 0.5f;

    const float EdgePadPx    = 2.0f;
    const float ArrowGapPx   = 4.0f;
    const float IconGapPx    = 4.0f;
    const float IconSizePx   = 16.0f;
    const float IndentStepPx = 18.0f;
    const float IndentPx     = (float)InDepth * IndentStepPx;

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const ImVec2 ArrowPos = ImVec2(RowMin.x + EdgePadPx + IndentPx, ArrowY);

    float X = ArrowPos.x;

    if (bHasChildFolders)
    {
        const float  ArrowSizePx = FontSize;
        const ImRect ArrowRect   = ImRect(ImVec2(ArrowPos.x, RowMin.y), ImVec2(ArrowPos.x + ArrowSizePx + ArrowGapPx, RowMax.y));

        if (!bFolderSearchActive && bRowHovered && ImGui::IsMouseClicked(0))
        {
            const int32 ClickCount = ImGui::GetMouseClickedCount(0);
            if (ClickCount == 1)
            {
                const ImVec2 Mouse = ImGui::GetMousePos();

                const bool bInsideArrow = (Mouse.x >= ArrowRect.Min.x && Mouse.x <= ArrowRect.Max.x && Mouse.y >= ArrowRect.Min.y && Mouse.y <= ArrowRect.Max.y);
                if (bInsideArrow)
                {
                    bOpen = !bOpen;
                    InStorage->SetBool(OpenId, bOpen);
                }
            }
        }

        const ImGuiDir Dir = bOpen ? ImGuiDir_Down : ImGuiDir_Right;
        ImGui::RenderArrow(DrawList, ArrowPos, IM_COL32(220, 220, 220, 255), Dir, 1.0f);
        X += FontSize + ArrowGapPx;
    }
    else
    {
        X += FontSize + ArrowGapPx;
    }

    ImTextureID FolderIcon = (bHasChildFolders && bOpen) ? EditorIcons::FolderOpenSmallIcon : EditorIcons::FolderSmallIcon;
    if (FolderIcon)
    {
        const float  IconY   = RowMin.y + (H - IconSizePx) * 0.5f;
        const ImVec2 IconMin = ImVec2(X, IconY);
        const ImVec2 IconMax = ImVec2(IconMin.x + IconSizePx, IconMin.y + IconSizePx);

        DrawList->AddImage(FolderIcon, IconMin, IconMax);
        X = IconMax.x + IconGapPx;
    }

    DrawList->AddText(ImVec2(X, TextY), ImGui::GetColorU32(InNameTextColor), InFolder.Name);

    ImGui::PopID();
    return bHasChildFolders && bOpen;
}

FEditorContentBrowserWidget::FileInfo* FEditorContentBrowserWidget::GetFolderFromPath(const TArray<int32>& InPath)
{
    if (InPath.Size() <= 0)
    {
        return nullptr;
    }

    const int32 RootIndex = InPath[0];
    if (!RootFolders.IsValidIndex(RootIndex))
    {
        return nullptr;
    }

    FileInfo* Current = &RootFolders[RootIndex];

    for (int32 Depth = 1; Depth < InPath.Size(); ++Depth)
    {
        const int32 ChildIndex = InPath[Depth];
        if (!Current->FolderContents.IsValidIndex(ChildIndex))
        {
            return Current;
        }

        Current = &Current->FolderContents[ChildIndex];
    }

    return Current;
}

void FEditorContentBrowserWidget::BuildFolderPathString(const TArray<int32>& InPath, char* OutBuf, int32 OutBufSize) const
{
    if (!OutBuf || OutBufSize <= 0)
    {
        return;
    }

    OutBuf[0] = 0;

    int32 Offset = 0;
    if (InPath.Size() <= 0)
    {
        return;
    }

    const int32 RootIndex = InPath[0];
    if (!RootFolders.IsValidIndex(RootIndex))
    {
        return;
    }

    const FileInfo* Current = &RootFolders[RootIndex];
    Offset += FCString::Snprintf(OutBuf + Offset, OutBufSize - Offset, "%s", Current->Name);

    for (int32 Depth = 1; Depth < InPath.Size(); ++Depth)
    {
        const int32 ChildIndex = InPath[Depth];
        if (!Current->FolderContents.IsValidIndex(ChildIndex))
        {
            break;
        }

        Current = &Current->FolderContents[ChildIndex];
        Offset += FCString::Snprintf(OutBuf + Offset, OutBufSize - Offset, "/%s", Current->Name);
    }
}

void FEditorContentBrowserWidget::DrawContentPanel()
{
    const ImVec4 RightBg           = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    const ImVec4 NameTextColor     = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
    const ImVec4 MutedTextColor    = ImVec4(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);
    const ImU32  TileSelectedColor = IM_COL32(0, 112, 224, 255);
    const ImU32  TileHoverColor    = IM_COL32(47, 47, 47, 255);
    const ImU32  TileIdleColor     = IM_COL32(31, 31, 31, 255);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, RightBg);

    if (ImGui::BeginChild("##CB_Content", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
    {
        DrawSearchField("##CB_AssetSearch", "Search Assets", AssetSearchBuffer);
        ImGui::Dummy(ImVec2(0.0f, 6.0f));

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, RightBg);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(87, 87, 87, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(127, 127, 127, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(127, 127, 127, 255));

        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 16.0f);

        if (ImGui::BeginChild("##CB_GridScroll", ImVec2(0, 0), false, 0))
        {
            FileInfo* CurrentFolder    = GetFolderFromPath(SelectedFolderPath);
            TArray<FileInfo>* ItemsPtr = CurrentFolder ? &CurrentFolder->FolderContents : nullptr;

            const float TileW      = 132.0f;
            const float TileH      = 158.0f;
            const float LabelAreaH = 40.0f;
            const float CornerR    = 6.0f;
            const float IconPadPx  = 4.0f;
            const float CellW      = TileW + ImGui::GetStyle().ItemSpacing.x;
            const float AvailX     = ImGui::GetContentRegionAvail().x;

            int32 ColumnCount = (int32)(AvailX / CellW);
            if (ColumnCount < 1)
            {
                ColumnCount = 1;
            }

            if (!ItemsPtr)
            {
                CenteredMessage("No folder selected", MutedTextColor);
            }
            else if (ItemsPtr->Size() <= 0)
            {
                CenteredMessage("Folder is empty", MutedTextColor);
            }
            else
            {
                TArray<FileInfo>& Items = *ItemsPtr;

                int32 VisibleCount = 0;
                for (int32 i = 0; i < Items.Size(); ++i)
                {
                    if (MatchesSearch(Items[i].Name, AssetSearchBuffer))
                    {
                        ++VisibleCount;
                    }
                }

                if (VisibleCount <= 0)
                {
                    CenteredMessage("No results", MutedTextColor);
                }
                else if (ImGui::BeginTable("##CB_AssetGrid", ColumnCount, ImGuiTableFlags_SizingFixedFit))
                {
                    for (int32 i = 0; i < Items.Size(); ++i)
                    {
                        FileInfo& Item = Items[i];

                        if (!MatchesSearch(Item.Name, AssetSearchBuffer))
                        {
                            continue;
                        }

                        ImGui::TableNextColumn();
                        ImGui::PushID(i);

                        const bool bSelected = (SelectedItemIndex == i);
                        const bool bIsFolder = Item.bIsFolder;

                        const ImVec2 TileStart = ImGui::GetCursorScreenPos();
                        const ImVec2 TileEnd   = ImVec2(TileStart.x + TileW, TileStart.y + TileH);

                        ImGui::InvisibleButton("##TileBtn", ImVec2(TileW, TileH));

                        const bool bHovered     = ImGui::IsItemHovered();
                        const bool bPressed     = ImGui::IsItemClicked();
                        const bool bDoubleClick = bPressed && ImGui::IsMouseDoubleClicked(0);

                        if (bPressed)
                        {
                            SelectedItemIndex         = i;
                            bSelectionActiveInBrowser = false;

                            if (bDoubleClick && bIsFolder)
                            {
                                SelectedFolderPath.Add(i);
                                SelectedItemIndex         = -1;
                                bSelectionActiveInBrowser = true;
                            }
                        }

                        ImDrawList* WindowDrawList = ImGui::GetWindowDrawList();

                        const ImU32 Bg = bSelected ? TileSelectedColor : (bHovered ? TileHoverColor : TileIdleColor);
                        WindowDrawList->AddRectFilled(TileStart, TileEnd, Bg, CornerR);

                        ImTextureID Icon = bIsFolder ? EditorIcons::FolderIcon : EditorIcons::DocumentIcon;
                        if (!Icon && bIsFolder)
                        {
                            Icon = EditorIcons::FolderSmallIcon;
                        }

                        if (Icon)
                        {
                            const float  IconAreaH = TileH - LabelAreaH;
                            const float  MaxIconSz = Math::Min((TileW - IconPadPx * 2.0f), (IconAreaH - IconPadPx * 2.0f));
                            const float  IconSz    = Math::Max(1.0f, MaxIconSz);
                            const ImVec2 IconMin   = ImVec2(TileStart.x + (TileW - IconSz) * 0.5f, TileStart.y + (IconAreaH - IconSz) * 0.5f);
                            const ImVec2 IconMax   = ImVec2(IconMin.x + IconSz, IconMin.y + IconSz);

                            WindowDrawList->AddImage(Icon, IconMin, IconMax);
                        }

                        {
                            const ImVec2 LabelMin = ImVec2(TileStart.x + 8.0f, TileEnd.y - LabelAreaH + 6.0f);
                            const ImVec2 LabelMax = ImVec2(TileEnd.x - 8.0f, TileEnd.y - 6.0f);

                            ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);
                            ImGui::RenderTextClipped(LabelMin, LabelMax, Item.Name, nullptr, nullptr, ImVec2(0.5f, 0.0f));
                            ImGui::PopStyleColor();
                        }

                        if (bHovered)
                        {
                            const ImVec4 TooltipBg     = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
                            const ImVec4 TooltipBorder = ImVec4(71.0f / 255.0f, 71.0f / 255.0f, 71.0f / 255.0f, 1.0f);
                            const ImVec4 TextWhite     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                            const ImVec4 TextGrey      = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);

                            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                            ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
                            ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 2.0f);

                            ImGui::PushStyleColor(ImGuiCol_PopupBg, TooltipBg);
                            ImGui::PushStyleColor(ImGuiCol_Border, TooltipBorder);
                            ImGui::PushStyleColor(ImGuiCol_Separator, TooltipBorder);

                            ImGui::BeginTooltip();

                            ImGui::PushStyleColor(ImGuiCol_Text, TextWhite);
                            ImGui::TextUnformatted(Item.Name);
                            ImGui::PopStyleColor();

                            ImGui::Spacing();

                            ImTextureID TypeIcon = nullptr;
                            if (bIsFolder)
                            {
                                TypeIcon = EditorIcons::FolderSmallIcon ? EditorIcons::FolderSmallIcon : EditorIcons::FolderIcon;
                            }
                            else
                            {
                                TypeIcon = EditorIcons::DocumentSmallIcon ? EditorIcons::DocumentSmallIcon : EditorIcons::DocumentIcon;
                            }

                            const float IconSizePx = 16.0f;
                            const float LineH      = ImGui::GetTextLineHeight();
                            const float Y0         = ImGui::GetCursorPosY();
                            const float IconOffset = Math::Max(0.0f, (LineH - IconSizePx) * 0.5f);

                            if (TypeIcon)
                            {
                                ImGui::SetCursorPosY(Y0 + IconOffset);
                                ImGui::Image(TypeIcon, ImVec2(IconSizePx, IconSizePx));
                                ImGui::SameLine();
                                ImGui::SetCursorPosY(Y0);
                            }

                            ImGui::PushStyleColor(ImGuiCol_Text, TextGrey);
                            ImGui::TextUnformatted(bIsFolder ? "Folder" : "Document");
                            ImGui::PopStyleColor();

                            ImGui::Separator();

                            char FolderPathBuf[512];
                            BuildFolderPathString(SelectedFolderPath, FolderPathBuf, (int32)sizeof(FolderPathBuf));

                            char FullPathBuf[768];
                            if (FolderPathBuf[0] != 0)
                            {
                                FCString::Snprintf(FullPathBuf, (int32)sizeof(FullPathBuf), "%s/%s", FolderPathBuf, Item.Name);
                            }
                            else
                            {
                                FCString::Snprintf(FullPathBuf, (int32)sizeof(FullPathBuf), "%s", Item.Name);
                            }

                            ImGui::PushStyleColor(ImGuiCol_Text, MutedTextColor);
                            ImGui::TextUnformatted("Path:");
                            ImGui::PopStyleColor();

                            ImGui::SameLine();

                            ImGui::PushStyleColor(ImGuiCol_Text, TextWhite);
                            ImGui::TextUnformatted(FullPathBuf);
                            ImGui::PopStyleColor();

                            ImGui::EndTooltip();

                            ImGui::PopStyleColor(3);
                            ImGui::PopStyleVar(4);
                        }

                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }
            }

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
            {
                SelectedItemIndex = -1;
            }
        }

        ImGui::EndChild(); // ##CB_GridScroll

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}
