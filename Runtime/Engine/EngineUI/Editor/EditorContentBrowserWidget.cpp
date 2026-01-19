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

const CHAR* FEditorContentBrowserWidget::GetTrimmedQuery(const TStaticArray<CHAR, 256>& InBuffer) const
{
    const CHAR* Query = InBuffer.Data();
    while (Query && *Query && FCharTraits::IsWhitespace(*Query))
    {
        ++Query;
    }

    return Query;
}

bool FEditorContentBrowserWidget::MatchesSearch(const CHAR* InName, const TStaticArray<CHAR, 256>& InBuffer) const
{
    if (!InName)
    {
        return false;
    }

    const CHAR* Query = GetTrimmedQuery(InBuffer);
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
    const ImVec4 PanelBg             = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
    const ImVec4 NameTextColor       = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
    const ImU32  FolderActiveColor   = IM_COL32(0, 112, 224, 255);
    const ImU32  FolderInactiveColor = IM_COL32(64, 87, 111, 255);
    const ImU32  FolderHoverColor    = IM_COL32(56, 56, 56, 255);
    const ImU32  FolderPathColor     = IM_COL32(44, 50, 58, 255);

    const CHAR* TrimmedQuery        = GetTrimmedQuery(FolderSearchBuffer);
    const bool  bFolderSearchActive = TrimmedQuery && *TrimmedQuery != 0;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, PanelBg);

    if (ImGui::BeginChild("##CB_Folders", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
    {
        constexpr float HeaderHeightPx  = 42.0f;
        constexpr float HeaderInputPadX = 8.0f;
        constexpr float HeaderBorderPx  = 2.0f;

        const ImU32 HeaderBg     = IM_COL32(47, 47, 47, 255);
        const ImU32 HeaderBorder = IM_COL32(26, 26, 26, 255);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();

        const ImVec2 CursorMin = ImGui::GetCursorScreenPos();
        const float  Width     = ImGui::GetContentRegionAvail().x;
        const ImVec2 BarMin    = CursorMin;
        const ImVec2 BarMax    = ImVec2(CursorMin.x + Width, CursorMin.y + HeaderHeightPx);

        DrawList->AddRectFilled(BarMin, BarMax, HeaderBg, 0.0f);
        DrawList->AddRectFilled(BarMin, ImVec2(BarMax.x, BarMin.y + HeaderBorderPx), HeaderBorder, 0.0f);
        DrawList->AddRectFilled(ImVec2(BarMin.x, BarMax.y - HeaderBorderPx), BarMax, HeaderBorder, 0.0f);

        const float InputHeightPx = ImGui::GetFontSize() + EditorStyleVars::InputFieldFramePadding.y * 2.0f;
        const float InputY        = BarMin.y + (HeaderHeightPx - InputHeightPx) * 0.5f;

        ImGui::SetCursorScreenPos(ImVec2(BarMin.x + HeaderInputPadX, InputY));
        DrawSearchField("##CB_FolderSearch", "Search Paths", FolderSearchBuffer);

        ImGui::SetCursorScreenPos(ImVec2(CursorMin.x, CursorMin.y + HeaderHeightPx));

        // ---------------------------------------------------------------------------------
        // Scrollbar styling
        // ---------------------------------------------------------------------------------

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, PanelBg);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(87, 87, 87, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(127, 127, 127, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(127, 127, 127, 255));

        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 16.0f);

        float  ScrollY    = 0.0f;
        float  ScrollMaxY = 0.0f;
        ImVec2 ScrollPos  = ImVec2(0, 0);
        ImVec2 ScrollSize = ImVec2(0, 0);

        bool bScrollHasScrollbarY = false;
        if (ImGui::BeginChild("##CB_FoldersScroll", ImVec2(0, 0), false, 0))
        {
            if (RootFolders.Size() > 0)
            {
                if (SelectedFolderPath.Size() > 0 && !RootFolders.IsValidIndex(SelectedFolderPath[0]))
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

            ScrollY    = ImGui::GetScrollY();
            ScrollMaxY = ImGui::GetScrollMaxY();

            ImGuiWindow* ScrollWindow = ImGui::GetCurrentWindow();
            if (ScrollWindow)
            {
                bScrollHasScrollbarY = ScrollWindow->ScrollbarY;
                ScrollPos            = ScrollWindow->Pos;
                ScrollSize           = ScrollWindow->Size;
            }
        }

        ImGui::EndChild(); // ##CB_FoldersScroll

        // ---------------------------------------------------------------------------------
        // Shadows overlay
        // ---------------------------------------------------------------------------------
        if (bScrollHasScrollbarY && ScrollSize.x > 0.0f && ScrollSize.y > 0.0f)
        {
            constexpr float ShadowHeightPx = 10.0f;
            constexpr float EpsilonPx      = 1.0f;

            const bool bShowTopShadow    = (ScrollY > EpsilonPx);
            const bool bShowBottomShadow = (ScrollY < (ScrollMaxY - EpsilonPx));

            if (bShowTopShadow || bShowBottomShadow)
            {
                ImDrawList* OverlayDrawList = ImGui::GetForegroundDrawList();

                const ImVec2 ClipMin = ScrollPos;
                const ImVec2 ClipMax = ImVec2(ScrollPos.x + ScrollSize.x, ScrollPos.y + ScrollSize.y);

                OverlayDrawList->PushClipRect(ClipMin, ClipMax, true);

                const ImU32 Dark  = IM_COL32(0, 0, 0, 140);
                const ImU32 Clear = IM_COL32(0, 0, 0, 0);

                if (bShowTopShadow)
                {
                    const ImVec2 ShadowMin = ScrollPos;
                    const ImVec2 ShadowMax = ImVec2(ScrollPos.x + ScrollSize.x, ScrollPos.y + ShadowHeightPx);

                    OverlayDrawList->AddRectFilledMultiColor(ShadowMin, ShadowMax, Dark, Dark, Clear, Clear);
                }

                if (bShowBottomShadow)
                {
                    const ImVec2 ShadowMin = ImVec2(ScrollPos.x, ScrollPos.y + ScrollSize.y - ShadowHeightPx);
                    const ImVec2 ShadowMax = ImVec2(ScrollPos.x + ScrollSize.x, ScrollPos.y + ScrollSize.y);

                    OverlayDrawList->AddRectFilledMultiColor(ShadowMin, ShadowMax, Clear, Clear, Dark, Dark);
                }

                OverlayDrawList->PopClipRect();
            }
        }

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

    const float RowHeightPx = 24.0f;

    const ImGuiSelectableFlags SelFlags =
        ImGuiSelectableFlags_SpanAllColumns |
        ImGuiSelectableFlags_AllowItemOverlap;

    const bool bRowPressed = ImGui::Selectable("##FolderRow", (bSelected || bInSelectedPath), SelFlags, ImVec2(0.0f, RowHeightPx));
    const bool bRowHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (bRowPressed)
    {
        NavigateToFolderPath(InPath, true);
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

    const ImVec2 RowMin       = ImGui::GetItemRectMin();
    const ImVec2 RowMax       = ImGui::GetItemRectMax();
    const float  H            = RowMax.y - RowMin.y;
    const float  FontSize     = ImGui::GetFontSize();
    const float  TextHeight   = ImGui::GetTextLineHeight();
    const float  TextY        = RowMin.y + (H - TextHeight) * 0.5f;
    const float  ArrowY       = RowMin.y + (H - FontSize) * 0.5f;
    const float  EdgePadPx    = 2.0f;
    const float  ArrowGapPx   = 4.0f;
    const float  IconGapPx    = 4.0f;
    const float  IconSizePx   = 16.0f;
    const float  IndentStepPx = 18.0f;
    const float  IndentPx     = (float)InDepth * IndentStepPx;

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
        // -----------------------------------------------------------------------------
        // Header bar
        // -----------------------------------------------------------------------------

        DrawContentHeaderBar();

        // -----------------------------------------------------------------------------
        // Search field
        // -----------------------------------------------------------------------------

        DrawSearchField("##CB_AssetSearch", "Search Assets", AssetSearchBuffer);
        ImGui::Dummy(ImVec2(0.0f, 6.0f));

        // -----------------------------------------------------------------------------
        // Scrollbar styling
        // -----------------------------------------------------------------------------

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, RightBg);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(87, 87, 87, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(127, 127, 127, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(127, 127, 127, 255));

        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 16.0f);

        float  ScrollY    = 0.0f;
        float  ScrollMaxY = 0.0f;
        ImVec2 GridPos    = ImVec2(0, 0);
        ImVec2 GridSize   = ImVec2(0, 0);

        bool bGridHasScrollbarY = false;
        if (ImGui::BeginChild("##CB_GridScroll", ImVec2(0, 0), false, 0))
        {
            FileInfo* CurrentFolder = GetFolderFromPath(SelectedFolderPath);

            TArray<FileInfo>* ItemsPtr = nullptr;
            if (SelectedFolderPath.Size() <= 0)
            {
                ItemsPtr = &RootFolders;
            }
            else
            {
                ItemsPtr = CurrentFolder ? &CurrentFolder->FolderContents : nullptr;
            }

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
                                TArray<int32> NewPath = SelectedFolderPath;
                                NewPath.Add(i);
                                NavigateToFolderPath(NewPath, true);

                                SelectedItemIndex = -1;
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

                        // -----------------------------------------------------------------------------
                        // Tooltip
                        // -----------------------------------------------------------------------------

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

            // Capture scroll metrics for shadow conditions
            ScrollY    = ImGui::GetScrollY();
            ScrollMaxY = ImGui::GetScrollMaxY();

            ImGuiWindow* GridWindow = ImGui::GetCurrentWindow();
            if (GridWindow)
            {
                bGridHasScrollbarY = GridWindow->ScrollbarY;
                GridPos            = GridWindow->Pos;
                GridSize           = GridWindow->Size;
            }
        }

        ImGui::EndChild(); // ##CB_GridScroll

        // -----------------------------------------------------------------------------
        // Shadows
        // -----------------------------------------------------------------------------

        if (bGridHasScrollbarY && GridSize.x > 0.0f && GridSize.y > 0.0f)
        {
            constexpr float ShadowHeightPx = 10.0f;
            constexpr float EpsilonPx      = 1.0f;

            const bool bShowTopShadow    = ScrollY > EpsilonPx;
            const bool bShowBottomShadow = ScrollY < (ScrollMaxY - EpsilonPx);

            if (bShowTopShadow || bShowBottomShadow)
            {
                ImDrawList* OverlayDrawList = ImGui::GetForegroundDrawList();

                const ImVec2 ClipMin = GridPos;
                const ImVec2 ClipMax = ImVec2(GridPos.x + GridSize.x, GridPos.y + GridSize.y);

                OverlayDrawList->PushClipRect(ClipMin, ClipMax, true);

                const ImU32 Dark  = IM_COL32(0, 0, 0, 140);
                const ImU32 Clear = IM_COL32(0, 0, 0, 0);

                if (bShowTopShadow)
                {
                    const ImVec2 ShadowMin = GridPos;
                    const ImVec2 ShadowMax = ImVec2(GridPos.x + GridSize.x, GridPos.y + ShadowHeightPx);

                    OverlayDrawList->AddRectFilledMultiColor(ShadowMin, ShadowMax, Dark, Dark, Clear, Clear);
                }

                if (bShowBottomShadow)
                {
                    const ImVec2 ShadowMin = ImVec2(GridPos.x, GridPos.y + GridSize.y - ShadowHeightPx);
                    const ImVec2 ShadowMax = ImVec2(GridPos.x + GridSize.x, GridPos.y + GridSize.y);

                    OverlayDrawList->AddRectFilledMultiColor(ShadowMin, ShadowMax, Clear, Clear, Dark, Dark);
                }

                OverlayDrawList->PopClipRect();
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

bool FEditorContentBrowserWidget::ArePathsEqual(const TArray<int32>& A, const TArray<int32>& B) const
{
    if (A.Size() != B.Size())
    {
        return false;
    }

    for (int32 i = 0; i < A.Size(); ++i)
    {
        if (A[i] != B[i])
        {
            return false;
        }
    }

    return true;
}

void FEditorContentBrowserWidget::NavigateToFolderPath(const TArray<int32>& InNewPath, bool bAddToHistory)
{
    if (ArePathsEqual(SelectedFolderPath, InNewPath))
    {
        return;
    }

    if (bAddToHistory)
    {
        BackHistory.Add(SelectedFolderPath);
        ForwardHistory.Clear();
    }

    SelectedFolderPath        = InNewPath;
    SelectedItemIndex         = -1;
    bSelectionActiveInBrowser = true;
}

void FEditorContentBrowserWidget::NavigateBack()
{
    if (BackHistory.Size() <= 0)
    {
        return;
    }

    ForwardHistory.Add(SelectedFolderPath);

    const TArray<int32> Prev = BackHistory.LastElement();
    BackHistory.Pop();

    SelectedFolderPath        = Prev;
    SelectedItemIndex         = -1;
    bSelectionActiveInBrowser = true;
}

void FEditorContentBrowserWidget::NavigateForward()
{
    if (ForwardHistory.Size() <= 0)
    {
        return;
    }

    BackHistory.Add(SelectedFolderPath);

    const TArray<int32> Next = ForwardHistory.LastElement();
    ForwardHistory.Pop();

    SelectedFolderPath        = Next;
    SelectedItemIndex         = -1;
    bSelectionActiveInBrowser = true;
}

void FEditorContentBrowserWidget::DrawContentHeaderBar()
{
    // -------------------------------------------------------------------------------------
    // Layout
    // -------------------------------------------------------------------------------------

    constexpr float HeaderHeightPx    = 40.0f;
    constexpr float HeaderPadX        = 1.0f;
    constexpr float NavIconPx         = 24.0f;
    constexpr float NavBtnH           = 32.0f;
    constexpr float NavBtnExtraX      = 6.0f;
    constexpr float NavBtnW           = NavBtnH + NavBtnExtraX * 2.0f;
    constexpr float NavGapPx          = 2.0f;
    constexpr float AfterNavGapPx     = 2.0f;
    constexpr float NavButtonRounding = 4.0f;
    constexpr float CrumbPadX         = 8.0f;
    constexpr float CrumbPadY         = 1.0f;
    constexpr float CrumbRounding     = 4.0f;
    constexpr float BarRounding       = 4.0f;
    constexpr float BarBorderTh       = 2.0f;
    constexpr float SepIconPx         = 12.0f;
    constexpr float SepGapAfterPx     = 6.0f;

    // -------------------------------------------------------------------------------------
    // Colors
    // -------------------------------------------------------------------------------------

    const ImU32 HeaderBg        = IM_COL32(36, 36, 36, 255);
    const ImU32 NavBgIdle       = IM_COL32(36, 36, 36, 255);
    const ImU32 NavBgHover      = IM_COL32(56, 56, 56, 255);
    const ImU32 NavIconDisabled = IM_COL32(106, 106, 106, 255);
    const ImU32 NavIconEnabled  = IM_COL32(192, 192, 192, 255);
    const ImU32 NavIconHover    = IM_COL32(255, 255, 255, 255);
    const ImU32 BarBg           = IM_COL32(15, 15, 15, 255);
    const ImU32 BarBorderNormal = IM_COL32(51, 51, 51, 255);
    const ImU32 BarBorderHover  = IM_COL32(74, 74, 74, 255);
    const ImU32 CrumbBgIdle     = IM_COL32(15, 15, 15, 255);
    const ImU32 CrumbBgHover    = IM_COL32(56, 56, 56, 255);
    const ImU32 CrumbTextIdle   = IM_COL32(192, 192, 192, 255);
    const ImU32 CrumbTextHover  = IM_COL32(255, 255, 255, 255);
    const ImU32 SepColor        = IM_COL32(106, 106, 106, 255);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImVec2 Start = ImGui::GetCursorScreenPos();
    const float  Width = ImGui::GetContentRegionAvail().x;
    const ImVec2 End   = ImVec2(Start.x + Width, Start.y + HeaderHeightPx);

    // -------------------------------------------------------------------------------------
    // Background
    // -------------------------------------------------------------------------------------

    DrawList->AddRectFilled(Start, End, HeaderBg, 0.0f);

    const float  ControlY = Start.y + (HeaderHeightPx - NavBtnH) * 0.5f;
    const float  BackX    = Start.x + HeaderPadX;
    const float  ForwardX = BackX + NavBtnW + NavGapPx;
    const float  BarX     = ForwardX + NavBtnW + AfterNavGapPx;
    const float  BarRight = End.x - HeaderPadX;
    const float  BarY     = ControlY;
    const float  BarH     = NavBtnH;
    const float  BarW     = Math::Max(1.0f, BarRight - BarX);
    const ImVec2 BarMin   = ImVec2(BarX, BarY);
    const ImVec2 BarMax   = ImVec2(BarX + BarW, BarY + BarH);

    // -------------------------------------------------------------------------------------
    // Back/Forward button
    // -------------------------------------------------------------------------------------

    const auto DrawNavButton = [&](const char* InId, float X, ImTextureID InIcon, bool bEnabled, bool bForward) -> bool
    {
        const ImVec2 BtnMin = ImVec2(X, ControlY);
        const ImVec2 BtnMax = ImVec2(X + NavBtnW, ControlY + NavBtnH);

        ImGui::SetCursorScreenPos(BtnMin);
        ImGui::PushID(InId);

        const bool bPressed = ImGui::InvisibleButton("##NavBtn", ImVec2(NavBtnW, NavBtnH));
        const bool bHovered = bEnabled ? ImGui::IsItemHovered() : false;

        if (bHovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        const ImU32 BgCol = bHovered ? NavBgHover : NavBgIdle;

        ImU32 IconCol = NavIconDisabled;
        if (bEnabled)
        {
            IconCol = bHovered ? NavIconHover : NavIconEnabled;
        }

        DrawList->AddRectFilled(BtnMin, BtnMax, BgCol, NavButtonRounding);

        if (InIcon)
        {
            const float  IconX   = BtnMin.x + (NavBtnW - NavIconPx) * 0.5f;
            const float  IconY   = BtnMin.y + (NavBtnH - NavIconPx) * 0.5f;
            const ImVec2 IconMin = ImVec2(IconX, IconY);
            const ImVec2 IconMax = ImVec2(IconX + NavIconPx, IconY + NavIconPx);

            DrawList->AddImage(InIcon, IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), IconCol);
        }
        else
        {
            const float TriW = 8.0f;
            const float TriH = 10.0f;
            const float Cx   = (BtnMin.x + BtnMax.x) * 0.5f;
            const float Cy   = (BtnMin.y + BtnMax.y) * 0.5f;

            ImVec2 P0;
            ImVec2 P1;
            ImVec2 P2;
            if (bForward)
            {
                P0 = ImVec2(Cx - TriW * 0.5f, Cy - TriH * 0.5f);
                P1 = ImVec2(Cx - TriW * 0.5f, Cy + TriH * 0.5f);
                P2 = ImVec2(Cx + TriW * 0.5f, Cy);
            }
            else
            {
                P0 = ImVec2(Cx + TriW * 0.5f, Cy - TriH * 0.5f);
                P1 = ImVec2(Cx + TriW * 0.5f, Cy + TriH * 0.5f);
                P2 = ImVec2(Cx - TriW * 0.5f, Cy);
            }

            DrawList->AddTriangleFilled(P0, P1, P2, IconCol);
        }

        ImGui::PopID();
        return bEnabled && bPressed;
    };

    const bool bCanBack    = BackHistory.Size() > 0;
    const bool bCanForward = ForwardHistory.Size() > 0;

    ImTextureID BackIcon    = EditorIcons::PreviousIcon;
    ImTextureID ForwardIcon = EditorIcons::NextIcon;

    if (DrawNavButton("Back", BackX, BackIcon, bCanBack, false))
    {
        NavigateBack();
    }

    if (DrawNavButton("Forward", ForwardX, ForwardIcon, bCanForward, true))
    {
        NavigateForward();
    }

    // -------------------------------------------------------------------------------------
    // Draw navigation bar
    // -------------------------------------------------------------------------------------
    
    const bool  bBarHovered = ImGui::IsMouseHoveringRect(BarMin, BarMax, true);
    const ImU32 BorderCol   = bBarHovered ? BarBorderHover : BarBorderNormal;

    DrawList->AddRectFilled(BarMin, BarMax, BarBg, BarRounding);
    DrawList->AddRect(BarMin, BarMax, BorderCol, BarRounding, 0, BarBorderTh);

    // -------------------------------------------------------------------------------------
    // Breadcrumb rendering
    // -------------------------------------------------------------------------------------

    DrawList->PushClipRect(BarMin, BarMax, true);

    float       CursorX = BarMin.x + 8.0f;
    const float CenterY = BarMin.y + BarH * 0.5f;

    const auto DrawSeparator = [&]()
    {
        if (EditorIcons::RightArrowIcon)
        {
            const float  IconY   = CenterY - SepIconPx * 0.5f;
            const ImVec2 IconMin = ImVec2(CursorX, IconY);
            const ImVec2 IconMax = ImVec2(CursorX + SepIconPx, IconY + SepIconPx);

            DrawList->AddImage(EditorIcons::RightArrowIcon, IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), SepColor);

            CursorX += SepIconPx + SepGapAfterPx;
        }
        else
        {
            const char*  Sep     = ">";
            const ImVec2 SepSize = ImGui::CalcTextSize(Sep);
            const ImVec2 SepPos  = ImVec2(CursorX, CenterY - SepSize.y * 0.5f);

            DrawList->AddText(SepPos, SepColor, Sep);

            CursorX += SepSize.x + SepGapAfterPx;
        }
    };

    const auto DrawCrumbButton = [&](const char* InLabel, const TArray<int32>& InTargetPath, int32 InId)
    {
        if (!InLabel || InLabel[0] == 0)
        {
            return;
        }

        ImFont* FontToUse = EditorFonts::SegoeUI_22 ? EditorFonts::SegoeUI_22 : ImGui::GetFont();
        ImGui::PushFont(FontToUse);

        const ImVec2 TextSize = ImGui::CalcTextSize(InLabel);
        const float  ButtonW  = TextSize.x + CrumbPadX * 2.0f;
        const float  ButtonH  = Math::Min(BarH - 4.0f, TextSize.y + CrumbPadY * 2.0f);
        const float  ButtonY  = CenterY - ButtonH * 0.5f;

        if (CursorX + ButtonW > BarMax.x - 6.0f)
        {
            ImGui::PopFont();
            return;
        }

        const ImVec2 ButtonMin = ImVec2(CursorX, ButtonY);
        const ImVec2 ButtonMax = ImVec2(CursorX + ButtonW, ButtonY + ButtonH);

        ImGui::SetCursorScreenPos(ButtonMin);
        ImGui::PushID(InId);

        const bool bPressed = ImGui::InvisibleButton("##CrumbBtn", ImVec2(ButtonW, ButtonH));
        const bool bHovered = ImGui::IsItemHovered();

        if (bHovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        const ImU32 BgCol   = bHovered ? CrumbBgHover : CrumbBgIdle;
        const ImU32 TextCol = bHovered ? CrumbTextHover : CrumbTextIdle;
        DrawList->AddRectFilled(ButtonMin, ButtonMax, BgCol, CrumbRounding);

        const ImVec2 TextPos = ImVec2(ButtonMin.x + CrumbPadX, CenterY - TextSize.y * 0.5f);
        DrawList->AddText(FontToUse, FontToUse->FontSize, TextPos, TextCol, InLabel);

        if (bPressed)
        {
            NavigateToFolderPath(InTargetPath, true);
        }

        ImGui::PopID();
        ImGui::PopFont();

        CursorX += ButtonW + 5.0f;
    };

    {
        TArray<int32> EmptyPath;
        DrawCrumbButton("Root", EmptyPath, 1000);
    }

    if (SelectedFolderPath.Size() > 0)
    {
        TArray<int32> PrefixPath;
        PrefixPath.Reserve(SelectedFolderPath.Size());

        for (int32 Depth = 0; Depth < SelectedFolderPath.Size(); ++Depth)
        {
            DrawSeparator();

            PrefixPath.Add(SelectedFolderPath[Depth]);

            FileInfo*   Folder = GetFolderFromPath(PrefixPath);
            const char* Label  = Folder ? Folder->Name : "<Invalid>";

            DrawCrumbButton(Label, PrefixPath, 1100 + Depth);
        }
    }

    DrawList->PopClipRect();

    ImGui::SetCursorScreenPos(ImVec2(Start.x, Start.y + HeaderHeightPx));
}
