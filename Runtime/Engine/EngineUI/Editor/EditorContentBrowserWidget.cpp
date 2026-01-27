#include "Engine/EngineUI/Editor/EditorContentBrowserWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include <imgui.h>
#include <imgui_internal.h>

// -------------------------------------------------------------------------------------------------
// Scroll shadow helpers
// -------------------------------------------------------------------------------------------------

struct FContentBrowserScrollShadowState
{
    bool       bActive      = false;
    bool       bShowTop     = false;
    bool       bShowBottom  = false;
    ImVec2     ClipMin      = ImVec2(0.0f, 0.0f);
    ImVec2     ClipMax      = ImVec2(0.0f, 0.0f);
    ImDrawList* DrawList    = nullptr;
};

static FContentBrowserScrollShadowState CaptureScrollShadowState()
{
    FContentBrowserScrollShadowState State;

    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || !Window->ScrollbarY)
    {
        return State;
    }

    const float ScrollY    = ImGui::GetScrollY();
    const float ScrollMaxY = ImGui::GetScrollMaxY();

    constexpr float Epsilon = 1.0f;

    State.bShowTop    = ScrollY > Epsilon;
    State.bShowBottom = ScrollY < (ScrollMaxY - Epsilon);

    if (!(State.bShowTop || State.bShowBottom))
    {
        return State;
    }

    State.ClipMin = Window->OuterRectClipped.Min;
    State.ClipMax = Window->OuterRectClipped.Max;

    if (State.ClipMax.x <= State.ClipMin.x || State.ClipMax.y <= State.ClipMin.y)
    {
        return State;
    }

    State.bActive   = true;
    State.DrawList  = ImGui::GetWindowDrawList();
    return State;
}

static void DrawScrollShadows(const FContentBrowserScrollShadowState& State)
{
    if (!State.bActive || !State.DrawList)
    {
        return;
    }

    constexpr float ShadowHeight = 10.0f;

    const ImU32 Dark  = IM_COL32(0, 0, 0, 140);
    const ImU32 Clear = IM_COL32(0, 0, 0, 0);

    State.DrawList->PushClipRect(State.ClipMin, State.ClipMax, true);

    if (State.bShowTop)
    {
        const ImVec2 ShadowMin = State.ClipMin;
        const ImVec2 ShadowMax = ImVec2(State.ClipMax.x, State.ClipMin.y + ShadowHeight);
        State.DrawList->AddRectFilledMultiColor(ShadowMin, ShadowMax, Dark, Dark, Clear, Clear);
    }

    if (State.bShowBottom)
    {
        const ImVec2 ShadowMin = ImVec2(State.ClipMin.x, State.ClipMax.y - ShadowHeight);
        const ImVec2 ShadowMax = State.ClipMax;
        State.DrawList->AddRectFilledMultiColor(ShadowMin, ShadowMax, Clear, Clear, Dark, Dark);
    }

    State.DrawList->PopClipRect();
}

FEditorContentBrowserWidget::FEditorContentBrowserWidget()
    : ImGuiDelegateHandle()
    , SelectedFolderIndex(0)
    , SelectedItemIndex(-1)
    , bSelectionActiveInBrowser(false)
    , bVisible(true)
    , bPendingMove(false)
    , PendingMoveSourceIndex(-1)
    , bDragPreviewInvalidSelfMove(false)
    , bDragPreviewActive(false)
    , DragPreviewIcon(nullptr)
    , bDragPreviewIsFolder(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorContentBrowserWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }

    FolderSearchBuffer.Fill(0);
    AssetSearchBuffer.Fill(0);

    ResetDragPreviewState();

    RootFolders =
    {
        { "Content", true, { } },
        { "MoreContent", true, { } },
    };

    for (FileInfo& Folder : RootFolders)
    {
        Folder.FolderContents =
        {
            { "MyOtherContent", true, { } },
            { "Materials", true, { } },
            { "Geometry", true, { } },
            { "Textures", true, { } },
            { "Scenes", true, { } },
        };

        for (int32 i = 0; i < Folder.FolderContents.Size(); i++)
        {
            FileInfo& SubFolder = Folder.FolderContents[i];
            if (i % 2 == 0)
            {
                SubFolder.FolderContents =
                {
                    { "Meshes", true, { } },
                    { "Materials", true, { } },
                    { "Textures", true, { } },
                    { "Car.asset", false, { } },
                    { "Door.asset", false, { } },
                    { "Wood.asset", false, { } },
                    { "Stone.asset", false, { } },
                    { "Gold.asset", false, { } },
                };
            }
            else
            {
                SubFolder.FolderContents =
                {
                    { "Animations", true, { } },
                    { "Shaders", true, { } },
                    { "Icons", true, { } },
                    { "Bus.asset", false, { } },
                    { "Train.asset", false, { } },
                    { "Metal.asset", false, { } },
                    { "Lava.asset", false, { } },
                    { "Silver.asset", false, { } },
                    { "WalkAnimation.asset", false, { } },
                    { "JumpAnimation.asset", false, { } },
                };
            }

            for (int32 j = 0; j < SubFolder.FolderContents.Size(); j++)
            {
                FileInfo& SubSubFolder = SubFolder.FolderContents[j];
                if (SubSubFolder.bIsFolder)
                {
                    if (j % 2 == 0)
                    {
                        SubSubFolder.FolderContents =
                        {
                            { "WalkAnimation.asset", false, { } },
                            { "JumpAnimation.asset", false, { } },
                            { "LavaTexture.asset", false, { } },
                            { "GoldTexture.asset", false, { } },
                            { "SpaceshipModel.asset", false, { } },
                        };
                    }
                }
            }
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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(36, 36, 36, 255));
    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, IM_COL32(110, 110, 110, 120));
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, IM_COL32(160, 160, 160, 200));
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive,  IM_COL32(200, 200, 200, 255));

    const ImGuiWindowFlags WindowFlags = 
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing;

    if (ImGui::Begin("Content Browser", &bVisible, WindowFlags))
    {
        DrawLayoutTable();
    }

    ImGui::End();

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);
}

void FEditorContentBrowserWidget::DrawLayoutTable()
{
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImVec2 RootMin   = ImGui::GetCursorScreenPos();
    const ImVec2 RootAvail = ImGui::GetContentRegionAvail();
    const ImVec2 RootMax   = ImVec2(RootMin.x + RootAvail.x, RootMin.y + RootAvail.y);

    // -----------------------------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------------------------

    constexpr float PanelBorder = 4.0f;
    constexpr float Splitter    = 3.0f;

    // -----------------------------------------------------------------------------------------
    // Color
    // -----------------------------------------------------------------------------------------

    const ImU32 BorderColor        = IM_COL32(21, 21, 21, 255);
    const ImU32 SplitterColor      = IM_COL32(21, 21, 21, 255);
    const ImU32 SplitterHoverColor = IM_COL32(56, 56, 56, 255);

    const float OuterBorder = PanelBorder;

    DrawList->AddRect(RootMin, RootMax, BorderColor, 0.0f, 0, OuterBorder);

    const ImVec2 InnerMin  = ImVec2(RootMin.x + OuterBorder, RootMin.y + OuterBorder);
    const ImVec2 InnerMax  = ImVec2(RootMax.x - OuterBorder, RootMax.y - OuterBorder);
    const ImVec2 InnerSize = ImVec2(Math::Max(1.0f, InnerMax.x - InnerMin.x), Math::Max(1.0f, InnerMax.y - InnerMin.y));

    static float FolderPanelWidth = 300.0f;

    const float MinFolderWidth = 200.0f;
    const float MaxFolderWidth = Math::Max(MinFolderWidth, InnerSize.x - 250.0f);

    FolderPanelWidth = Math::Clamp(FolderPanelWidth, MinFolderWidth, MaxFolderWidth);

    const float LeftWidth  = FolderPanelWidth;
    const float RightWidth = Math::Max(1.0f, InnerSize.x - LeftWidth - Splitter);

    const ImVec2 LeftMin  = InnerMin;
    const ImVec2 LeftMax  = ImVec2(InnerMin.x + LeftWidth, InnerMax.y);
    const ImVec2 SplitMin = ImVec2(LeftMax.x, InnerMin.y);
    const ImVec2 SplitMax = ImVec2(LeftMax.x + Splitter, InnerMax.y);
    const ImVec2 RightMin = ImVec2(SplitMax.x, InnerMin.y);
    const ImVec2 RightMax = InnerMax;

    DrawList->AddRect(LeftMin, LeftMax, BorderColor, 0.0f, 0, PanelBorder);
    DrawList->AddRect(RightMin, RightMax, BorderColor, 0.0f, 0, PanelBorder);

    // -----------------------------------------------------------------------------------------
    // Splitter
    // -----------------------------------------------------------------------------------------

    ImGui::SetCursorScreenPos(SplitMin);
    ImGui::InvisibleButton("##CB_Splitter", ImVec2(Splitter, InnerSize.y));

    const bool bSplitterHovered = ImGui::IsItemHovered();
    const bool bSplitterActive  = ImGui::IsItemActive();

    if (bSplitterHovered || bSplitterActive)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    const ImU32 SplitColor = (bSplitterHovered || bSplitterActive) ? SplitterHoverColor : SplitterColor;
    DrawList->AddRectFilled(SplitMin, SplitMax, SplitColor, 0.0f);

    if (bSplitterActive)
    {
        const float DeltaX = ImGui::GetIO().MouseDelta.x;
        FolderPanelWidth = Math::Clamp(FolderPanelWidth + DeltaX, MinFolderWidth, MaxFolderWidth);
    }

    // -----------------------------------------------------------------------------------------
    // Draw children
    // -----------------------------------------------------------------------------------------

    ImGui::SetCursorScreenPos(LeftMin);

    ImGui::BeginChild("##CB_FolderPanelRoot", ImVec2(LeftWidth, InnerSize.y), false, ImGuiWindowFlags_NoScrollbar);
    DrawFolderPanel();
    ImGui::EndChild();

    ImGui::SetCursorScreenPos(RightMin);

    ImGui::BeginChild("##CB_ContentPanelRoot", ImVec2(RightWidth, InnerSize.y), false, ImGuiWindowFlags_NoScrollbar);
    DrawContentPanel();
    ImGui::EndChild();
}

void FEditorContentBrowserWidget::DrawFolderPanel()
{
    // -----------------------------------------------------------------------------------------
    // Color
    // -----------------------------------------------------------------------------------------

    const ImVec4 ParentBackGround = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    const ImVec4 HeaderBackGround = ImVec4(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);
    const ImVec4 ListBackGround   = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
    const ImVec4 NameTextColor    = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);

    const ImU32 FolderActiveColor   = IM_COL32(0, 112, 224, 255);
    const ImU32 FolderInactiveColor = IM_COL32(64, 87, 111, 255);
    const ImU32 FolderHoverColor    = IM_COL32(56, 56, 56, 255);
    const ImU32 FolderPathColor     = IM_COL32(44, 50, 58, 255);
    const ImU32 BorderColor         = IM_COL32(26, 26, 26, 255);

    // -----------------------------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------------------------

    constexpr float SidePadding     = 4.0f;
    constexpr float TopPadding      = 8.0f;
    constexpr float BottomPadding   = 8.0f;
    constexpr float InnerPadding    = 3.0f;
    constexpr float HeaderHeight    = 42.0f;
    constexpr float BorderThickness = 2.0f;

    // -----------------------------------------------------------------------------------------
    // Outer container
    // -----------------------------------------------------------------------------------------

    const CHAR* TrimmedQuery        = GetTrimmedQuery(FolderSearchBuffer);
    const bool  bFolderSearchActive = TrimmedQuery && *TrimmedQuery != 0;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ParentBackGround);

    if (ImGui::BeginChild("##CB_Folders", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
    {
        ImGuiStyle& Style = ImGui::GetStyle();

        const ImVec2 PrevItemSpacing = Style.ItemSpacing;
        Style.ItemSpacing.y = 0.0f;

        const float AvailableWidth = ImGui::GetContentRegionAvail().x;
        const float InnerWidth     = Math::Max(1.0f, AvailableWidth - SidePadding * 2.0f);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + TopPadding);

        // -------------------------------------------------------------------------------------
        // Header
        // -------------------------------------------------------------------------------------

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePadding);
        const ImVec2 HeaderStartScreen = ImGui::GetCursorScreenPos();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, HeaderBackGround);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        if (ImGui::BeginChild("##CB_FolderHeader", ImVec2(InnerWidth, HeaderHeight), false, ImGuiWindowFlags_NoScrollbar))
        {
            const float PaddedWidth  = Math::Max(1.0f, InnerWidth - InnerPadding * 2.0f);
            const float PaddedHeight = Math::Max(1.0f, HeaderHeight - InnerPadding * 2.0f);
            const float InputHeight  = ImGui::GetFontSize() + EditorStyleVars::InputFieldFramePadding.y * 2.0f;
            const float CenterY      = Math::Max(0.0f, (PaddedHeight - InputHeight) * 0.5f);

            ImGui::SetCursorPos(ImVec2(InnerPadding, InnerPadding + CenterY));
            DrawSearchField("##CB_FolderSearch", "Search Paths", FolderSearchBuffer, PaddedWidth);

            ImDrawList* DrawList = ImGui::GetWindowDrawList();

            const ImVec2 HeaderMin = ImGui::GetWindowPos();
            const ImVec2 HeaderMax = ImVec2(HeaderMin.x + ImGui::GetWindowSize().x, HeaderMin.y + ImGui::GetWindowSize().y);

            DrawList->AddRectFilled(HeaderMin, ImVec2(HeaderMax.x, HeaderMin.y + BorderThickness), BorderColor, 0.0f);
            DrawList->AddRectFilled(ImVec2(HeaderMin.x, HeaderMax.y - BorderThickness), HeaderMax, BorderColor, 0.0f);
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // -------------------------------------------------------------------------------------
        // List
        // -------------------------------------------------------------------------------------

        ImGui::SetCursorScreenPos(ImVec2(HeaderStartScreen.x, HeaderStartScreen.y + HeaderHeight));

        const float AvailableHeight = ImGui::GetContentRegionAvail().y;
        const float ListHeight      = Math::Max(1.0f, AvailableHeight - BottomPadding);

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ListBackGround);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(87, 87, 87, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(127, 127, 127, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(127, 127, 127, 255));

        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 16.0f);

        FContentBrowserScrollShadowState ShadowState;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ListBackGround);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(InnerPadding, InnerPadding));

        if (ImGui::BeginChild("##CB_FoldersScroll", ImVec2(InnerWidth, ListHeight), false, 0))
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

            ShadowState = CaptureScrollShadowState();
        }

        ImGui::EndChild();

        DrawScrollShadows(ShadowState);

        ImGui::PopStyleVar(); // WindowPadding
        ImGui::PopStyleColor(); // ChildBg

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);

        Style.ItemSpacing = PrevItemSpacing;
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}


void FEditorContentBrowserWidget::DrawContentPanel()
{
    // -----------------------------------------------------------------------------------------
    // Colors
    // -----------------------------------------------------------------------------------------

    const ImVec4 RightBackGround = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

    // -----------------------------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------------------------

    constexpr float SidePadding           = 8.0f;
    constexpr float SearchRowHeight       = 42.0f;
    constexpr float SearchBarExtraPadding = 4.0f;
    constexpr float GridEdgePadding       = 8.0f;
    constexpr float GridSpacingX          = 4.0f;
    constexpr float GridSpacingY          = 8.0f;
    constexpr float ScrollBottomPaddingY  = 8.0f;

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImVec2 PanelMin = ImGui::GetCursorScreenPos();
    const ImVec2 PanelMax = ImVec2(PanelMin.x + ImGui::GetContentRegionAvail().x, PanelMin.y + ImGui::GetContentRegionAvail().y);

    DrawList->AddRectFilled(PanelMin, PanelMax, IM_COL32(36, 36, 36, 255));

    // -----------------------------------------------------------------------------------------
    // Header
    // -----------------------------------------------------------------------------------------

    DrawContentHeaderArea(RightBackGround, SidePadding, SearchRowHeight);

    // -----------------------------------------------------------------------------------------
    // Grid scroll region
    // -----------------------------------------------------------------------------------------

    ImGui::PushStyleColor(ImGuiCol_ChildBg, RightBackGround);

    const ImVec2 ContentRegionAvailable = ImGui::GetContentRegionAvail();
    const float  ScrollAvailableWidth   = ContentRegionAvailable.x;
    const float  ScrollAvailableHeight  = ContentRegionAvailable.y;
    const float  ScrollWidth            = Math::Max(1.0f, ScrollAvailableWidth - SidePadding * 2.0f);
    const float  ScrollHeight           = Math::Max(1.0f, ScrollAvailableHeight - ScrollBottomPaddingY);

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePadding);

    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, RightBackGround);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(87, 87, 87, 255));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(127, 127, 127, 255));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(127, 127, 127, 255));

    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 16.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(GridEdgePadding, GridEdgePadding));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(GridSpacingX, GridSpacingY));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GridSpacingX, GridSpacingY));

    FContentBrowserScrollShadowState ShadowState;

    if (ImGui::BeginChild("##CB_GridScroll", ImVec2(ScrollWidth, ScrollHeight), false, 0))
    {
        DrawContentGrid();
        ShadowState = CaptureScrollShadowState();
    }

    ImGui::EndChild();

    DrawScrollShadows(ShadowState);

    ImGui::PopStyleVar(3);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
}


void FEditorContentBrowserWidget::DrawItemTooltip(const FileInfo& InItem)
{
    const ImVec4 TooltipBg     = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
    const ImVec4 TooltipBorder = ImVec4(71.0f / 255.0f, 71.0f / 255.0f, 71.0f / 255.0f, 1.0f);
    const ImVec4 TextWhite     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 TextGrey      = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);

    const bool bIsFolder = InItem.bIsFolder;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 2.0f);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, TooltipBg);
    ImGui::PushStyleColor(ImGuiCol_Border, TooltipBorder);
    ImGui::PushStyleColor(ImGuiCol_Separator, TooltipBorder);

    ImGui::BeginTooltip();

    ImGui::PushStyleColor(ImGuiCol_Text, TextWhite);
    ImGui::TextUnformatted(InItem.Name);
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

    const float IconSize   = 16.0f;
    const float LineHeight = ImGui::GetTextLineHeight();
    const float CursorY    = ImGui::GetCursorPosY();
    const float IconOffset = Math::Max(0.0f, (LineHeight - IconSize) * 0.5f);

    if (TypeIcon)
    {
        ImGui::SetCursorPosY(CursorY + IconOffset);
        ImGui::Image(TypeIcon, ImVec2(IconSize, IconSize));
        ImGui::SameLine();
        ImGui::SetCursorPosY(CursorY);
    }

    ImGui::PushStyleColor(ImGuiCol_Text, TextGrey);
    ImGui::TextUnformatted(bIsFolder ? "Folder" : "Document");
    ImGui::PopStyleColor();

    ImGui::Separator();

    CHAR FolderPathBuf[512] = {};
    BuildFolderPathString(SelectedFolderPath, FolderPathBuf, (int32)sizeof(FolderPathBuf));

    CHAR FullPathBuf[768] = {};
    if (FolderPathBuf[0] != 0)
    {
        FCString::Snprintf(FullPathBuf, sizeof(FullPathBuf), "%s/%s", FolderPathBuf, InItem.Name);
    }
    else
    {
        FCString::Snprintf(FullPathBuf, sizeof(FullPathBuf), "%s", InItem.Name);
    }

    const ImVec4 MutedTextColor = ImVec4(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);
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

void FEditorContentBrowserWidget::DrawContentGrid()
{
    ResetDragPreviewState();

    bool bDragHoverSelfMove = false;

    // -----------------------------------------------------------------------------------------
    // Tile Layout
    // -----------------------------------------------------------------------------------------

    const ImVec4 NameTextColor     = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
    const ImVec4 MutedTextColor    = ImVec4(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);
    const ImU32  TileSelectedColor = IM_COL32(0, 112, 224, 255);
    const ImU32  TileHoverColor    = IM_COL32(47, 47, 47, 255);
    const ImU32  TileIdleColor     = IM_COL32(31, 31, 31, 255);

    const float TileWidth       = 132.0f;
    const float TileHeight      = 158.0f;
    const float LabelAreaHeight = 40.0f;
    const float CornerRounding  = 6.0f;
    const float IconPadding     = 4.0f;

    // -----------------------------------------------------------------------------------------
    // Helper Lambdas
    // -----------------------------------------------------------------------------------------

    const auto BuildFullPathForItem = [&](const FileInfo& InItem, CHAR* OutBuf, int32 OutBufSize)
    {
        if (!OutBuf || OutBufSize <= 0)
        {
            return;
        }

        CHAR FolderPathBuf[512] = {};
        BuildFolderPathString(SelectedFolderPath, FolderPathBuf, (int32)sizeof(FolderPathBuf));

        if (FolderPathBuf[0] != 0)
        {
            FCString::Snprintf(OutBuf, OutBufSize, "%s/%s", FolderPathBuf, InItem.Name);
        }
        else
        {
            FCString::Snprintf(OutBuf, OutBufSize, "%s", InItem.Name);
        }
    };

    // -----------------------------------------------------------------------------------------
    // Find folder items
    // -----------------------------------------------------------------------------------------

    FileInfo* Folder = GetFolderFromPath(SelectedFolderPath);
    if (!Folder)
    {
        DrawCenteredMessage("No folder selected", MutedTextColor);
        return;
    }

    TArray<FileInfo>& Items = Folder->FolderContents;

    int32 VisibleCount = 0;
    for (int32 i = 0; i < Items.Size(); ++i)
    {
        if (MatchesSearch(Items[i].Name, AssetSearchBuffer))
        {
            ++VisibleCount;
        }
    }

    if (Items.Size() <= 0)
    {
        DrawCenteredMessage("Folder is empty", MutedTextColor);
        return;
    }

    if (VisibleCount <= 0)
    {
        DrawCenteredMessage("No results", MutedTextColor);
        return;
    }

    // -----------------------------------------------------------------------------------------
    // Grid layout
    // -----------------------------------------------------------------------------------------

    const float CellWidth  = TileWidth + ImGui::GetStyle().CellPadding.x * 2.0f;
    const float AvailableX = ImGui::GetContentRegionAvail().x;

    int32 ColumnCount = (int32)(AvailableX / CellWidth);
    if (ColumnCount < 1)
    {
        ColumnCount = 1;
    }

    // -----------------------------------------------------------------------------------------
    // Draw tiles
    // -----------------------------------------------------------------------------------------

    if (ImGui::BeginTable("##CB_AssetGrid", ColumnCount, ImGuiTableFlags_SizingFixedFit))
    {
        struct FCBDndPayload
        {
            int32 Depth;
            int32 Indices[32];
            int32 SourceIndex;
            bool  bIsFolder;
        };

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
            const ImVec2 TileEnd   = ImVec2(TileStart.x + TileWidth, TileStart.y + TileHeight);

            ImGui::InvisibleButton("##TileBtn", ImVec2(TileWidth, TileHeight));

            const bool bHovered     = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            const bool bPressed     = ImGui::IsItemClicked();
            const bool bDoubleClick = bPressed && ImGui::IsMouseDoubleClicked(0);

            if (bPressed)
            {
                SelectedItemIndex = i;

                if (bDoubleClick && bIsFolder)
                {
                    TArray<int32> NewPath = SelectedFolderPath;
                    NewPath.Add(i);
                    NavigateToFolderPath(NewPath, true);

                    SelectedItemIndex = -1;
                }
            }

            ImDrawList* WindowDrawList = ImGui::GetWindowDrawList();
            const ImU32 BackGround     = bSelected ? TileSelectedColor : (bHovered ? TileHoverColor : TileIdleColor);

            // -----------------------------------------------------------------------------
            // Tile shadow
            // -----------------------------------------------------------------------------

            if (bHovered || bSelected)
            {
                constexpr float ShadowOffsetY = 3.0f;
                constexpr int32 ShadowLayers  = 3;

                for (int32 Layer = 0; Layer < ShadowLayers; ++Layer)
                {
                    const float  Expand    = (float)Layer;
                    const float  Rounding  = CornerRounding + Expand;
                    const int32  Alpha     = (Layer == 0) ? 55 : (Layer == 1) ? 30 : 16;
                    const ImU32  ShadowCol = IM_COL32(0, 0, 0, Alpha);
                    const ImVec2 ShadowMin = ImVec2(TileStart.x - Expand, TileStart.y - Expand + ShadowOffsetY);
                    const ImVec2 ShadowMax = ImVec2(TileEnd.x + Expand, TileEnd.y + Expand + ShadowOffsetY);

                    WindowDrawList->AddRectFilled(ShadowMin, ShadowMax, ShadowCol, Rounding);
                }
            }

            WindowDrawList->AddRectFilled(TileStart, TileEnd, BackGround, CornerRounding);

            ImTextureID Icon = bIsFolder ? EditorIcons::FolderIcon : EditorIcons::DocumentIcon;
            if (!Icon && bIsFolder)
            {
                Icon = EditorIcons::FolderSmallIcon;
            }
            if (!Icon && !bIsFolder)
            {
                Icon = EditorIcons::DocumentSmallIcon;
            }

            if (Icon)
            {
                const float  IconAreaHeight = TileHeight - LabelAreaHeight;
                const float  MaxIconSz      = Math::Min((TileWidth - IconPadding * 2.0f), (IconAreaHeight - IconPadding * 2.0f));
                const float  IconSz         = Math::Max(1.0f, MaxIconSz);
                const ImVec2 IconMin        = ImVec2(TileStart.x + (TileWidth - IconSz) * 0.5f, TileStart.y + (IconAreaHeight - IconSz) * 0.5f);
                const ImVec2 IconMax        = ImVec2(IconMin.x + IconSz, IconMin.y + IconSz);

                WindowDrawList->AddImage(Icon, IconMin, IconMax);
            }

            {
                const ImVec2 LabelMin = ImVec2(TileStart.x + 8.0f, TileEnd.y - LabelAreaHeight + 6.0f);
                const ImVec2 LabelMax = ImVec2(TileEnd.x - 8.0f, TileEnd.y - 6.0f);

                ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);
                ImGui::RenderTextClipped(LabelMin, LabelMax, Item.Name, nullptr, nullptr, ImVec2(0.5f, 0.0f));
                ImGui::PopStyleColor();
            }

            // ---------------------------------------------------------------------------------
            // Drag source (Folders and Files)
            // ---------------------------------------------------------------------------------

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID | ImGuiDragDropFlags_SourceNoPreviewTooltip))
            {
                FCBDndPayload Payload = {};
                Payload.Depth = Math::Min(SelectedFolderPath.Size(), 32);
                
                for (int32 P = 0; P < Payload.Depth; ++P)
                {
                    Payload.Indices[P] = SelectedFolderPath[P];
                }

                Payload.SourceIndex = i;
                Payload.bIsFolder   = bIsFolder;

                ImGui::SetDragDropPayload("CB_MOVE_ITEM", &Payload, sizeof(FCBDndPayload));

                bDragPreviewActive   = true;
                DragPreviewIcon      = Icon;
                bDragPreviewIsFolder = bIsFolder;

                FCString::Strncpy(DragPreviewSourceName, Item.Name, (int32)sizeof(DragPreviewSourceName));

                ImGui::EndDragDropSource();
            }

            if (bIsFolder && bHovered && ImGui::IsDragDropActive())
            {
                const ImGuiPayload* ActivePayload = ImGui::GetDragDropPayload();
                if (ActivePayload && ActivePayload->IsDataType("CB_MOVE_ITEM") && ActivePayload->DataSize == static_cast<int32>(sizeof(FCBDndPayload)))
                {
                    const FCBDndPayload* Data = reinterpret_cast<const FCBDndPayload*>(ActivePayload->Data);
                    if (Data && Data->bIsFolder)
                    {
                        TArray<int32> SourceParentPath;
                        SourceParentPath.Reserve(Data->Depth);

                        for (int32 P = 0; P < Data->Depth; ++P)
                        {
                            SourceParentPath.Add(Data->Indices[P]);
                        }

                        const bool bSameFolder = ArePathsEqual(SourceParentPath, SelectedFolderPath);
                        const bool bSelfDrop   = bSameFolder && (Data->SourceIndex == i);

                        if (bSelfDrop)
                        {
                            bDragHoverSelfMove       = true;
                            DragPreviewTargetName[0] = 0;
                        }
                    }
                }
            }

            // ---------------------------------------------------------------------------------
            // Drag target (Folders)
            // ---------------------------------------------------------------------------------

            if (bIsFolder)
            {
                ImGui::PushStyleColor(ImGuiCol_DragDropTarget, IM_COL32(0, 0, 0, 0));

                if (ImGui::BeginDragDropTarget())
                {
                    const ImGuiDragDropFlags DragDropFlags =
                        ImGuiDragDropFlags_AcceptBeforeDelivery |
                        ImGuiDragDropFlags_AcceptNoDrawDefaultRect;

                    if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("CB_MOVE_ITEM", DragDropFlags))
                    {
                        if (const FCBDndPayload* Data = reinterpret_cast<const FCBDndPayload*>(Payload->Data))
                        {
                            TArray<int32> SourceParentPath;
                            SourceParentPath.Reserve(Data->Depth);

                            for (int32 P = 0; P < Data->Depth; ++P)
                            {
                                SourceParentPath.Add(Data->Indices[P]);
                            }

                            const int32 SourceIndex = Data->SourceIndex;

                            const bool bSameFolder = ArePathsEqual(SourceParentPath, SelectedFolderPath);
                            const bool bSelfDrop   = bSameFolder && (SourceIndex == i);

                            if (Data->bIsFolder && bSelfDrop)
                            {
                                bDragHoverSelfMove = true;
                                DragPreviewTargetName[0] = 0;
                            }
                            else
                            {
                                bDragHoverSelfMove = false;
                                FCString::Strncpy(DragPreviewTargetName, Item.Name, (int32)sizeof(DragPreviewTargetName));
                            }

                            if (Payload->IsDelivery())
                            {
                                TArray<int32> TargetPath = SelectedFolderPath;
                                TargetPath.Add(i);

                                bool bInvalidDescendant = false;
                                if (Data->bIsFolder)
                                {
                                    TArray<int32> SourceFullPath = SourceParentPath;
                                    SourceFullPath.Add(SourceIndex);

                                    if (TargetPath.Size() >= SourceFullPath.Size())
                                    {
                                        bInvalidDescendant = true;
                                        
                                        for (int32 P = 0; P < SourceFullPath.Size(); ++P)
                                        {
                                            if (TargetPath[P] != SourceFullPath[P])
                                            {
                                                bInvalidDescendant = false;
                                                break;
                                            }
                                        }
                                    }
                                }

                                if (!(Data->bIsFolder && bSelfDrop) && !bInvalidDescendant)
                                {
                                    bPendingMove                = true;
                                    PendingMoveSourceParentPath = SourceParentPath;
                                    PendingMoveSourceIndex      = SourceIndex;
                                    PendingMoveTargetFolderPath = TargetPath;
                                }
                            }
                        }
                    }

                    ImGui::EndDragDropTarget();
                }

                ImGui::PopStyleColor();
            }

            if (bHovered && !ImGui::IsDragDropActive())
            {
                DrawItemTooltip(Item);
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    if (bPendingMove)
    {
        MoveItemToFolder(PendingMoveSourceParentPath, PendingMoveSourceIndex, PendingMoveTargetFolderPath);
        bPendingMove      = false;
        SelectedItemIndex = -1;
    }

    if (ImGui::IsDragDropActive() && bDragPreviewActive && DragPreviewSourceName[0] != 0)
    {
        const bool bHasFolderHoverTarget = (DragPreviewTargetName[0] != 0);
        const bool bShowSelfWarning      = bDragHoverSelfMove;
        const bool bShowTextAndDivider   = bHasFolderHoverTarget || bShowSelfWarning;

        const ImVec4 PreviewBg     = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
        const ImVec4 PreviewBorder = ImVec4(48.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_PopupBg, PreviewBg);
        ImGui::PushStyleColor(ImGuiCol_Border, PreviewBorder);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize,  2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,  0.0f);

        ImGui::BeginTooltip();
        {
            constexpr float IconSize         = 64.0f;
            constexpr float DividerThickness = 2.0f;
            constexpr float GapLeft          = 12.0f;
            constexpr float GapRight         = 12.0f;

            if (DragPreviewIcon)
            {
                ImGui::Image(DragPreviewIcon, ImVec2(IconSize, IconSize));
            }
            else
            {
                ImGui::Dummy(ImVec2(IconSize, IconSize));
            }

            if (bShowTextAndDivider)
            {
                const ImVec2 IconMax = ImGui::GetItemRectMax();

                {
                    ImDrawList* PreviewDrawList = ImGui::GetWindowDrawList();

                    const ImU32  DividerCol = ImGui::GetColorU32(PreviewBorder);
                    const float  DividerX   = IconMax.x + GapLeft;

                    const ImVec2 WinMin = ImGui::GetWindowPos();
                    const ImVec2 WinMax = ImVec2(WinMin.x + ImGui::GetWindowSize().x, WinMin.y + ImGui::GetWindowSize().y);

                    PreviewDrawList->AddRectFilled(ImVec2(DividerX, WinMin.y), ImVec2(DividerX + DividerThickness, WinMax.y), DividerCol);
                }

                ImGui::SameLine(0.0f, GapLeft + DividerThickness + GapRight);

                ImGui::BeginGroup();
                {
                    const float TextHeight = ImGui::GetTextLineHeight();
                    const float CenteredY  = ImGui::GetCursorPosY() + Math::Max(0.0f, (IconSize - TextHeight) * 0.5f);
                    ImGui::SetCursorPosY(CenteredY);

                    ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);

                    if (bShowSelfWarning)
                    {
                        ImGui::TextUnformatted("Cannot move a folder into itself");
                    }
                    else
                    {
                        ImGui::Text("Move %s to %s", DragPreviewSourceName, DragPreviewTargetName);
                    }

                    ImGui::PopStyleColor();
                }

                ImGui::EndGroup();
            }
        }

        ImGui::EndTooltip();

        ImGui::PopStyleVar(5);
        ImGui::PopStyleColor(2);
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
    {
        SelectedItemIndex = -1;
    }
}

void FEditorContentBrowserWidget::DrawSearchField(const CHAR* InId, const CHAR* InHint, TStaticArray<CHAR, 256>& InOutBuffer, float InWidth)
{
    EditorWidgets::EditorSearchField(InId, InHint, InOutBuffer.Data(), InOutBuffer.Size(), InWidth, true);
}

void FEditorContentBrowserWidget::DrawCenteredMessage(const CHAR* InText, const ImVec4& InColor)
{
    if (!InText || InText[0] == 0)
    {
        return;
    }

    const ImVec2 Available = ImGui::GetContentRegionAvail();
    const ImVec2 Size      = ImGui::CalcTextSize(InText);

    const float X = Math::Max(0.0f, (Available.x - Size.x) * 0.5f);
    const float Y = Math::Max(0.0f, (Available.y - Size.y) * 0.35f);

    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + X, ImGui::GetCursorPosY() + Y));
    ImGui::PushStyleColor(ImGuiCol_Text, InColor);
    ImGui::TextUnformatted(InText);
    ImGui::PopStyleColor();
}

void FEditorContentBrowserWidget::DrawContentHeaderBar()
{
    // -------------------------------------------------------------------------------------
    // Layout
    // -------------------------------------------------------------------------------------

    constexpr float HeaderHeight      = 40.0f;
    constexpr float HeaderPaddingX    = 8.0f;
    constexpr float NavIcon           = 24.0f;
    constexpr float NavButtonHeight   = 32.0f;
    constexpr float NavButtonExtraX   = 6.0f;
    constexpr float NavButtonWidth    = NavButtonHeight + NavButtonExtraX * 2.0f;
    constexpr float NavButtonRounding = 4.0f;
    constexpr float NavGap            = 2.0f;
    constexpr float AfterNavGap       = 2.0f;
    constexpr float CrumbPadX         = 8.0f;
    constexpr float CrumbPadY         = 1.0f;
    constexpr float CrumbRounding     = 4.0f;
    constexpr float BarRounding       = 4.0f;
    constexpr float BarBorderTh       = 2.0f;
    constexpr float BarRightPaddingX  = 8.0f;
    constexpr float BarUpperPaddingY  = 8.0f;
    constexpr float SeperatorIcon     = 12.0f;
    constexpr float SeperatorGapAfter = 6.0f;

    // -------------------------------------------------------------------------------------
    // Colors
    // -------------------------------------------------------------------------------------

    const ImU32 HeaderBackGround     = IM_COL32(36, 36, 36, 255);
    const ImU32 NavBackGroundIdle    = IM_COL32(36, 36, 36, 255);
    const ImU32 NavBackGroundHover   = IM_COL32(56, 56, 56, 255);
    const ImU32 NavIconDisabled      = IM_COL32(106, 106, 106, 255);
    const ImU32 NavIconEnabled       = IM_COL32(192, 192, 192, 255);
    const ImU32 NavIconHover         = IM_COL32(255, 255, 255, 255);
    const ImU32 BarBackGround        = IM_COL32(15, 15, 15, 255);
    const ImU32 BarBorderNormal      = IM_COL32(51, 51, 51, 255);
    const ImU32 BarBorderHover       = IM_COL32(74, 74, 74, 255);
    const ImU32 CrumbBackGroundIdle  = IM_COL32(15, 15, 15, 255);
    const ImU32 CrumbBackGroundHover = IM_COL32(56, 56, 56, 255);
    const ImU32 CrumbTextIdle        = IM_COL32(192, 192, 192, 255);
    const ImU32 CrumbTextHover       = IM_COL32(255, 255, 255, 255);
    const ImU32 SeperatorColor       = IM_COL32(106, 106, 106, 255);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImVec2 CursorScreenPos = ImGui::GetCursorScreenPos();
    const ImVec2 Start           = ImVec2(CursorScreenPos.x, CursorScreenPos.y + BarUpperPaddingY);
    const float  Width           = ImGui::GetContentRegionAvail().x - BarRightPaddingX;
    const ImVec2 End             = ImVec2(Start.x + Width, Start.y + HeaderHeight);

    DrawList->AddRectFilled(Start, End, HeaderBackGround, 0.0f);

    const float  ControlY  = Start.y + (HeaderHeight - NavButtonHeight) * 0.5f;
    const float  BackX     = Start.x + HeaderPaddingX;
    const float  ForwardX  = BackX + NavButtonWidth + NavGap;
    const float  BarX      = ForwardX + NavButtonWidth + AfterNavGap;
    const float  BarRight  = End.x - HeaderPaddingX;
    const float  BarY      = ControlY;
    const float  BarHeight = NavButtonHeight;
    const float  BarWidth  = Math::Max(1.0f, BarRight - BarX);
    const ImVec2 BarMin    = ImVec2(BarX, BarY);
    const ImVec2 BarMax    = ImVec2(BarX + BarWidth, BarY + BarHeight);

    // -----------------------------------------------------------------------------------------
    // Helper Lambdas
    // -----------------------------------------------------------------------------------------

    const auto DrawNavButton = [&](const CHAR* InId, float X, ImTextureID InIcon, bool bEnabled) -> bool
    {
        const ImVec2 ButtonMin = ImVec2(X, ControlY);
        const ImVec2 ButtonMax = ImVec2(X + NavButtonWidth, ControlY + NavButtonHeight);

        ImGui::SetCursorScreenPos(ButtonMin);
        ImGui::PushID(InId);

        const bool bPressed = ImGui::InvisibleButton("##NavBtn", ImVec2(NavButtonWidth, NavButtonHeight));
        const bool bHovered = bEnabled ? ImGui::IsItemHovered() : false;

        if (bHovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        const ImU32 BackGroundColor = bHovered ? NavBackGroundHover : NavBackGroundIdle;

        ImU32 IconTint = NavIconDisabled;
        if (bEnabled)
        {
            IconTint = bHovered ? NavIconHover : NavIconEnabled;
        }

        DrawList->AddRectFilled(ButtonMin, ButtonMax, BackGroundColor, NavButtonRounding);

        if (InIcon)
        {
            const float  IconX   = ButtonMin.x + (NavButtonWidth - NavIcon) * 0.5f;
            const float  IconY   = ButtonMin.y + (NavButtonHeight - NavIcon) * 0.5f;
            const ImVec2 IconMin = ImVec2(IconX, IconY);
            const ImVec2 IconMax = ImVec2(IconX + NavIcon, IconY + NavIcon);

            DrawList->AddImage(InIcon, IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), IconTint);
        }

        ImGui::PopID();
        return bEnabled && bPressed;
    };

    const bool bCanBack    = BackHistory.Size() > 0;
    const bool bCanForward = ForwardHistory.Size() > 0;

    ImTextureID BackIcon    = EditorIcons::PreviousIcon;
    ImTextureID ForwardIcon = EditorIcons::NextIcon;

    if (DrawNavButton("Back", BackX, BackIcon, bCanBack))
    {
        NavigateBack();
    }

    if (DrawNavButton("Forward", ForwardX, ForwardIcon, bCanForward))
    {
        NavigateForward();
    }

    const bool  bBarHovered = ImGui::IsMouseHoveringRect(BarMin, BarMax, true);
    const ImU32 BorderCol   = bBarHovered ? BarBorderHover : BarBorderNormal;

    DrawList->AddRectFilled(BarMin, BarMax, BarBackGround, BarRounding);
    DrawList->AddRect(BarMin, BarMax, BorderCol, BarRounding, 0, BarBorderTh);

    DrawList->PushClipRect(BarMin, BarMax, true);

    float       CursorX = BarMin.x + 8.0f;
    const float CenterY = BarMin.y + BarHeight * 0.5f;

    // -----------------------------------------------------------------------------------------
    // Helper Lambdas
    // -----------------------------------------------------------------------------------------

    const auto DrawSeparator = [&]()
    {
        if (EditorIcons::RightArrowIcon)
        {
            const float  IconY   = CenterY - SeperatorIcon * 0.5f;
            const ImVec2 IconMin = ImVec2(CursorX, IconY);
            const ImVec2 IconMax = ImVec2(CursorX + SeperatorIcon, IconY + SeperatorIcon);

            DrawList->AddImage(EditorIcons::RightArrowIcon, IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), SeperatorColor);
            CursorX += SeperatorIcon + SeperatorGapAfter;
        }
        else
        {
            const CHAR*  Seperator     = ">";
            const ImVec2 SeperatorSize = ImGui::CalcTextSize(Seperator);
            const ImVec2 SeperatorPos  = ImVec2(CursorX, CenterY - SeperatorSize.y * 0.5f);

            DrawList->AddText(SeperatorPos, SeperatorColor, Seperator);
            CursorX += SeperatorSize.x + SeperatorGapAfter;
        }
    };

    const auto DrawCrumbButton = [&](const CHAR* InLabel, const TArray<int32>& InTargetPath, int32 InId)
    {
        if (!InLabel || InLabel[0] == 0)
        {
            return;
        }

        ImFont* FontToUse = EditorFonts::SegoeUI_22 ? EditorFonts::SegoeUI_22 : ImGui::GetFont();
        ImGui::PushFont(FontToUse);

        const ImVec2 TextSize     = ImGui::CalcTextSize(InLabel);
        const float  ButtonWidth  = TextSize.x + CrumbPadX * 2.0f;
        const float  ButtonHeight = Math::Min(BarHeight - 4.0f, TextSize.y + CrumbPadY * 2.0f);
        const float  ButtonY      = CenterY - ButtonHeight * 0.5f;

        if (CursorX + ButtonWidth > BarMax.x - 6.0f)
        {
            ImGui::PopFont();
            return;
        }

        const ImVec2 ButtonMin = ImVec2(CursorX, ButtonY);
        const ImVec2 ButtonMax = ImVec2(CursorX + ButtonWidth, ButtonY + ButtonHeight);

        ImGui::SetCursorScreenPos(ButtonMin);
        ImGui::PushID(InId);

        const bool bPressed = ImGui::InvisibleButton("##CrumbBtn", ImVec2(ButtonWidth, ButtonHeight));
        const bool bHovered = ImGui::IsItemHovered();

        if (bHovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        const ImU32 BackGroundColor = bHovered ? CrumbBackGroundHover : CrumbBackGroundIdle;
        const ImU32 TextColor       = bHovered ? CrumbTextHover : CrumbTextIdle;

        DrawList->AddRectFilled(ButtonMin, ButtonMax, BackGroundColor, CrumbRounding);

        const ImVec2 TextPos = ImVec2(ButtonMin.x + CrumbPadX, CenterY - TextSize.y * 0.5f);
        DrawList->AddText(FontToUse, FontToUse->FontSize, TextPos, TextColor, InLabel);

        if (bPressed)
        {
            NavigateToFolderPath(InTargetPath, true);
        }

        ImGui::PopID();
        ImGui::PopFont();

        CursorX += ButtonWidth + 5.0f;
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
            const CHAR* Label  = Folder ? Folder->Name : "<Invalid>";

            DrawCrumbButton(Label, PrefixPath, 1100 + Depth);
        }
    }

    DrawList->PopClipRect();

    ImGui::SetCursorScreenPos(ImVec2(Start.x, Start.y + HeaderHeight));
}

void FEditorContentBrowserWidget::DrawContentHeaderArea(const ImVec4& InBackGround, float InSidePadding, float InSearchRowHeight)
{
    constexpr float HeaderBarHeight = 44.0f;

    const float HeaderHeight = HeaderBarHeight + InSearchRowHeight;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, InBackGround);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if (ImGui::BeginChild("##CB_ContentHeader", ImVec2(0.0f, HeaderHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        DrawContentHeaderBar();

        {
            const float  FullWidth   = ImGui::GetContentRegionAvail().x;
            const float  InnerWidth  = Math::Max(1.0f, FullWidth - InSidePadding * 2.0f);
            const float  MaxWidth    = Math::Max(120.0f, FullWidth * 0.25f);
            const float  SearchWidth = Math::Min(InnerWidth, MaxWidth);
            const ImVec2 RowMin      = ImGui::GetCursorScreenPos();
            const float  InputHeight = ImGui::GetFontSize() + EditorStyleVars::InputFieldFramePadding.y * 2.0f;
            const float  InputY      = (RowMin.y + (InSearchRowHeight - InputHeight) * 0.5f);

            ImGui::SetCursorScreenPos(ImVec2(RowMin.x + InSidePadding, InputY));

            DrawSearchField("##CB_AssetSearch", "Search Content", AssetSearchBuffer, SearchWidth);

            ImGui::SetCursorScreenPos(ImVec2(RowMin.x, RowMin.y));
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
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

    ImGui::PushID("FolderTreeNode");
    for (int32 i = 0; i < InPath.Size(); ++i)
    {
        ImGui::PushID(InPath[i]);
    }

    const auto PopFolderNodeIDScope = [&]()
    {
        for (int32 i = 0; i < InPath.Size(); ++i)
        {
            ImGui::PopID();
        }

        ImGui::PopID();
    };

    const bool bHasChildFolders = HasChildFolders(InFolder);
    const bool bSelected        = IsSelectedFolderPath(InPath);
    const bool bInSelectedPath  = (!bSelected && IsPathPrefixOfSelected(InPath));

    ImGuiID OpenId = 0;
    if (bHasChildFolders)
    {
        OpenId = ImGui::GetID("##CB_Open");
    }

    bool bOpen = false;
    if (bHasChildFolders)
    {
        bOpen = InStorage->GetBool(OpenId, (InDepth == 0));
    }

    const bool bWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    const ImU32 SelectedColor = (bWindowFocused && bSelectionActiveInBrowser) ? InFolderActiveColor : InFolderInactiveColor;

    int32 NumPushedColors = 0;
    if (bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, SelectedColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SelectedColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, InFolderActiveColor);
        NumPushedColors = 3;
    }
    else if (bInSelectedPath)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, InFolderPathColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, InFolderHoverColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, InFolderActiveColor);
        NumPushedColors = 3;
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, InFolderHoverColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, InFolderActiveColor);
        NumPushedColors = 2;
    }

    const float RowHeight = 24.0f;

    const ImGuiSelectableFlags SelFlags =
        ImGuiSelectableFlags_SpanAllColumns |
        ImGuiSelectableFlags_AllowItemOverlap;

    const bool bRowPressed = ImGui::Selectable("##FolderRow", (bSelected || bInSelectedPath), SelFlags, ImVec2(0.0f, RowHeight));
    const bool bRowHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (bRowPressed)
    {
        NavigateToFolderPath(InPath, true);
    }

    if (bHasChildFolders && bRowHovered && ImGui::IsMouseClicked(0))
    {
        const int32 ClickCount = ImGui::GetMouseClickedCount(0);
        if ((ClickCount > 0) && ((ClickCount & 1) == 0))
        {
            bOpen = !bOpen;
            InStorage->SetBool(OpenId, bOpen);
        }
    }

    if (NumPushedColors > 0)
    {
        ImGui::PopStyleColor(NumPushedColors);
    }

    const ImVec2 RowMin      = ImGui::GetItemRectMin();
    const ImVec2 RowMax      = ImGui::GetItemRectMax();
    const float  Height      = RowMax.y - RowMin.y;
    const float  FontSize    = ImGui::GetFontSize();
    const float  TextHeight  = ImGui::GetTextLineHeight();
    const float  TextY       = RowMin.y + (Height - TextHeight) * 0.5f;
    const float  EdgePadding = 2.0f;
    const float  ArrowGap    = 4.0f;
    const float  IconGap     = 4.0f;
    const float  IconSize    = 16.0f;
    const float  IndentStep  = 18.0f;
    const float  Indentation = static_cast<float>(InDepth) * IndentStep;

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const float  CollapseIconSize = IconSize;
    const float  CollapseIconY    = RowMin.y + (Height - CollapseIconSize) * 0.5f;
    const ImVec2 CollapseIconPos  = ImVec2(RowMin.x + EdgePadding + Indentation, CollapseIconY);
    const ImU32  CollapseIconTint = IM_COL32(101, 101, 101, 255);

    float X = CollapseIconPos.x;
    if (bHasChildFolders)
    {
        const float  ArrowSize = CollapseIconSize;
        const ImRect ArrowRect = ImRect(ImVec2(CollapseIconPos.x, RowMin.y), ImVec2(CollapseIconPos.x + ArrowSize + ArrowGap, RowMax.y));

        if (bRowHovered && ImGui::IsMouseClicked(0))
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

        ImTextureID CollapseIcon = bOpen ? EditorIcons::CollapseArrowDown : EditorIcons::CollapseArrowRight;
        if (CollapseIcon)
        {
            const ImVec2 IconMin = CollapseIconPos;
            const ImVec2 IconMax = ImVec2(IconMin.x + CollapseIconSize, IconMin.y + CollapseIconSize);

            DrawList->AddImage(CollapseIcon, IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), CollapseIconTint);
        }
        else
        {
            const ImGuiDir Dir = bOpen ? ImGuiDir_Down : ImGuiDir_Right;
            ImGui::RenderArrow(DrawList, ImVec2(CollapseIconPos.x, CollapseIconPos.y + (CollapseIconSize - FontSize) * 0.5f), IM_COL32(220, 220, 220, 255), Dir, 1.0f);
        }

        X += CollapseIconSize + ArrowGap;
    }
    else
    {
        X += CollapseIconSize + ArrowGap;
    }

    // -----------------------------------------------------------------------------------------
    // Folder icon
    // -----------------------------------------------------------------------------------------

    ImTextureID FolderIcon = (bHasChildFolders && bOpen) ? EditorIcons::FolderOpenSmallIcon : EditorIcons::FolderSmallIcon;
    if (FolderIcon)
    {
        const float  IconY   = RowMin.y + (Height - IconSize) * 0.5f;
        const ImVec2 IconMin = ImVec2(X, IconY);
        const ImVec2 IconMax = ImVec2(IconMin.x + IconSize, IconMin.y + IconSize);

        DrawList->AddImage(FolderIcon, IconMin, IconMax);
        X = IconMax.x + IconGap;
    }

    // -------------------------------------------------------------------------------------
    // Folder name
    // -------------------------------------------------------------------------------------

    {
        const CHAR* NameText   = InFolder.Name ? InFolder.Name : "";
        const CHAR* Query      = bFolderSearchActive ? GetTrimmedQuery(FolderSearchBuffer) : nullptr;
        const CHAR* FilterText = (Query && *Query != 0) ? Query : nullptr;

        int32 MatchStart = -1;
        int32 MatchLen   = 0;

        if (FilterText)
        {
            if (const CHAR* MatchPtr = FCString::Stristr(NameText, FilterText))
            {
                MatchStart = static_cast<int32>(MatchPtr - NameText);
                MatchLen   = static_cast<int32>(strlen(FilterText));
            }
        }

        ImGuiIO& IO = ImGui::GetIO();

        const float Scale = IO.DisplayFramebufferScale.x;

        const ImU32 BaseTextU32      = ImGui::GetColorU32(InNameTextColor);
        const ImU32 HighlightBgU32   = IM_COL32(139, 194, 74, 255);
        const ImU32 HighlightTextU32 = IM_COL32(0, 0, 0, 255);

        const ImVec2 TextStart = ImVec2(X, TextY);
        if (MatchStart >= 0 && MatchLen > 0)
        {
            const ImVec2 PrefixSize = ImGui::CalcTextSize(NameText, NameText + MatchStart);
            const ImVec2 MatchSize  = ImGui::CalcTextSize(NameText + MatchStart, NameText + MatchStart + MatchLen);
            const ImVec2 PrefixPos  = TextStart;
            const ImVec2 MatchPos   = ImVec2(TextStart.x + PrefixSize.x, TextStart.y);
            const ImVec2 SuffixPos  = ImVec2(MatchPos.x + MatchSize.x, TextStart.y);

            DrawList->AddText(PrefixPos, BaseTextU32, NameText, NameText + MatchStart);

            const float  HighlightPadY = 2.0f * Scale;
            const ImVec2 HighlightMin  = ImVec2(MatchPos.x - 1.0f * Scale, RowMin.y + HighlightPadY);
            const ImVec2 HighlightMax  = ImVec2(MatchPos.x + MatchSize.x + 1.0f * Scale, RowMax.y - HighlightPadY);
            DrawList->AddRectFilled(HighlightMin, HighlightMax, HighlightBgU32, 0.0f);

            DrawList->AddText(MatchPos, HighlightTextU32, NameText + MatchStart, NameText + MatchStart + MatchLen);
            DrawList->AddText(SuffixPos, BaseTextU32, NameText + MatchStart + MatchLen);
        }
        else
        {
            DrawList->AddText(TextStart, BaseTextU32, NameText);
        }
    }

    PopFolderNodeIDScope();
    return bOpen;
}

void FEditorContentBrowserWidget::ResetDragPreviewState()
{
    DragPreviewSourceName[0]    = 0;
    DragPreviewTargetName[0]    = 0;
    bDragPreviewActive          = false;
    DragPreviewIcon             = nullptr;
    bDragPreviewIsFolder        = false;
    bDragPreviewInvalidSelfMove = false;
}

bool FEditorContentBrowserWidget::MoveItemToFolder(const TArray<int32>& InSourceParentPath, int32 InSourceIndex, const TArray<int32>& InTargetFolderPath)
{
    if (InSourceIndex < 0)
    {
        return false;
    }

    if (InTargetFolderPath.Size() <= 0)
    {
        return false;
    }

    TArray<int32> TargetParentPath = InTargetFolderPath;
    const int32 TargetFolderIndexOriginal = TargetParentPath.LastElement();
    TargetParentPath.Pop();

    if (ArePathsEqual(InSourceParentPath, TargetParentPath) && InSourceIndex == TargetFolderIndexOriginal)
    {
        return false;
    }

    FileInfo* SourceParent = GetFolderFromPath(InSourceParentPath);
    if (!SourceParent || !SourceParent->FolderContents.IsValidIndex(InSourceIndex))
    {
        return false;
    }

    const FileInfo& SourceItem = SourceParent->FolderContents[InSourceIndex];
    if (SourceItem.bIsFolder)
    {
        TArray<int32> SourceItemPath = InSourceParentPath;
        SourceItemPath.Add(InSourceIndex);

        const bool bTargetIsDescendant = (InTargetFolderPath.Size() >= SourceItemPath.Size()) &&
            [&]()
            {
                for (int32 i = 0; i < SourceItemPath.Size(); ++i)
                {
                    if (InTargetFolderPath[i] != SourceItemPath[i])
                    {
                        return false;
                    }
                }

                return true;
            }();

        if (bTargetIsDescendant)
        {
            return false;
        }
    }

    int32 TargetFolderIndex = TargetFolderIndexOriginal;
    if (ArePathsEqual(InSourceParentPath, TargetParentPath) && InSourceIndex < TargetFolderIndex)
    {
        TargetFolderIndex = Math::Max(0, TargetFolderIndex - 1);
    }

    FileInfo MovedItem = SourceParent->FolderContents[InSourceIndex];

    const int32 OldSize = SourceParent->FolderContents.Size();
    for (int32 i = InSourceIndex; i < OldSize - 1; ++i)
    {
        SourceParent->FolderContents[i] = SourceParent->FolderContents[i + 1];
    }

    SourceParent->FolderContents.Pop();

    TArray<int32> AdjustedTargetFolderPath = TargetParentPath;
    AdjustedTargetFolderPath.Add(TargetFolderIndex);

    FileInfo* TargetFolder = GetFolderFromPath(AdjustedTargetFolderPath);
    if (!TargetFolder || !TargetFolder->bIsFolder)
    {
        return false;
    }

    TargetFolder->FolderContents.Add(MovedItem);

    SelectedItemIndex = -1;
    return true;
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

void FEditorContentBrowserWidget::BuildFolderPathString(const TArray<int32>& InPath, CHAR* OutBuf, int32 OutBufSize) const
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

bool FEditorContentBrowserWidget::ArePathsEqual(const TArray<int32>& PathA, const TArray<int32>& PathB) const
{
    if (PathA.Size() != PathB.Size())
    {
        return false;
    }

    for (int32 i = 0; i < PathA.Size(); ++i)
    {
        if (PathA[i] != PathB[i])
        {
            return false;
        }
    }

    return true;
}
