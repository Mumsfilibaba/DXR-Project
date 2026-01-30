#include "Engine/EngineUI/Editor/EditorContentBrowserWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include "Core/Misc/OutputDeviceLogger.h"
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

    State.bActive  = true;
    State.DrawList = ImGui::GetWindowDrawList();
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

static void SplitNameAndExtension(const CHAR* InName, TStaticArray<CHAR, 256>& OutBase, TStaticArray<CHAR, 64>& OutExtension)
{
    OutBase.Fill(0);
    OutExtension.Fill(0);

    if (!InName || InName[0] == 0)
    {
        return;
    }

    const int32 Len = static_cast<int32>(FCString::Strlen(InName));
    if (Len <= 0)
    {
        return;
    }

    int32 DotIndex = -1;
    for (int32 Index = Len - 1; Index > 0; --Index)
    {
        if (InName[Index] == '.')
        {
            DotIndex = Index;
            break;
        }
    }

    if (DotIndex <= 0)
    {
        FCString::Strncpy(OutBase.Data(), InName, static_cast<int32>(OutBase.Size()));
        return;
    }

    const int32 BaseLen     = DotIndex;
    const int32 BaseCopyLen = Math::Min(BaseLen, static_cast<int32>(OutBase.Size()) - 1);

    FCString::Strncpy(OutBase.Data(), InName, BaseCopyLen);
    OutBase[BaseCopyLen] = 0;

    const int32 ExtLen     = Len - DotIndex;
    const int32 ExtCopyLen = Math::Min(ExtLen, static_cast<int32>(OutExtension.Size()) - 1);
    
    FCString::Strncpy(OutExtension.Data(), InName + DotIndex, ExtCopyLen);
    OutExtension[ExtCopyLen] = 0;
}

FEditorContentBrowserWidget::FEditorContentBrowserWidget()
    : ImGuiDelegateHandle()
    , LastSelectedItemIndex(-1)
    , bSelectionActiveInBrowser(false)
    , bVisible(true)
    , FolderPanelWidth(300.0f)
    , bPendingMove(false)
    , bFolderSelectionAnchorValid(false)
    , bHasLastActiveFolderPath(false)
    , bDragPreviewActive(false)
    , DragPreviewIcon(nullptr)
    , bDragPreviewIsFolder(false)
    , DragPreviewSelectionCount(0)
    , bDragPreviewHasAnyLegalMove(false)
    , DragPreviewIllegalMoveCount(0)
    , bRequestFolderRenameFocus(false)
    , RenamingItemIndex(-1)
    , bRequestItemRenameFocus(false)
    , bClipboardValid(false)
    , bClipboardFromContentPanel(false)
    , bPendingDeleteContent(false)
    , bPendingDeleteFolder(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorContentBrowserWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }

    FolderSearchBuffer.Fill(0);
    AssetSearchBuffer.Fill(0);
    FolderRenameBuffer.Fill(0);
    FolderRenameBufferOriginal.Fill(0);
    ItemRenameBuffer.Fill(0);
    ItemRenameBufferOriginal.Fill(0);
    ItemRenameExtension.Fill(0);

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

static FString AppendNameToPath(const FString& BasePath, const FString& Name)
{
    if (Name.IsEmpty())
    {
        return BasePath;
    }

    FString Result = BasePath;
    if (!Result.IsEmpty())
    {
        Result += "/";
    }

    Result += Name;
    return Result;
}

void FEditorContentBrowserWidget::Draw()
{
    if (!bVisible)
    {
        EditorWidgets::DrawErrorWindow(FailedMoveErrorContext);
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

    EditorWidgets::DrawErrorWindow(FailedMoveErrorContext);
}

void FEditorContentBrowserWidget::DrawLayoutTable()
{
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    ResetDragPreviewState();

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

    DrawList->AddRect(RootMin, RootMax, BorderColor, 0.0f, ImDrawListFlags_AntiAliasedLines, OuterBorder);

    const ImVec2 InnerMin  = ImVec2(RootMin.x + OuterBorder, RootMin.y + OuterBorder);
    const ImVec2 InnerMax  = ImVec2(RootMax.x - OuterBorder, RootMax.y - OuterBorder);
    const ImVec2 InnerSize = ImVec2(Math::Max(1.0f, InnerMax.x - InnerMin.x), Math::Max(1.0f, InnerMax.y - InnerMin.y));

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

    DrawList->AddRect(LeftMin, LeftMax, BorderColor, 0.0f, ImDrawListFlags_AntiAliasedLines, PanelBorder);
    DrawList->AddRect(RightMin, RightMax, BorderColor, 0.0f, ImDrawListFlags_AntiAliasedLines, PanelBorder);

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

    if (EditorWidgets::DrawConfirmDialog(DeleteConfirmContext))
    {
        if (bPendingDeleteContent)
        {
            DeleteSelectedContentItems();
            bPendingDeleteContent = false;
        }
        else if (bPendingDeleteFolder)
        {
            DeleteSelectedFolderInTree();
            bPendingDeleteFolder = false;
        }
    }
}

void FEditorContentBrowserWidget::DrawFolderPanel()
{
    // -----------------------------------------------------------------------------------------
    // Color
    // -----------------------------------------------------------------------------------------

    const ImVec4 ParentBackGround    = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    const ImVec4 HeaderBackGround    = ImVec4(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);
    const ImVec4 ListBackGround      = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
    const ImVec4 NameTextColor       = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
    const ImU32  FolderActiveColor   = IM_COL32(0, 112, 224, 255);
    const ImU32  FolderInactiveColor = IM_COL32(64, 87, 111, 255);
    const ImU32  FolderHoverColor    = IM_COL32(56, 56, 56, 255);
    const ImU32  FolderPathColor     = IM_COL32(44, 50, 58, 255);
    const ImU32  BorderColor         = IM_COL32(26, 26, 26, 255);

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

    TStaticArray<CHAR, 256> TrimmedQueryBuf{};

    const CHAR* TrimmedQuery        = EditorHelpers::GetTrimmedQuery(FolderSearchBuffer.Data(), TrimmedQueryBuf.Data(), static_cast<int32>(TrimmedQueryBuf.Size()));
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
            EditorWidgets::DrawSearchField("##CB_FolderSearch", "Search Paths", FolderSearchBuffer.Data(), FolderSearchBuffer.Size(), PaddedWidth, true);

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

            if (!bHasLastActiveFolderPath || !ArePathsEqual(LastActiveFolderPath, SelectedFolderPath))
            {
                FolderSelectionPaths.Clear();

                if (SelectedFolderPath.Size() > 0)
                {
                    FolderSelectionPaths.Add(SelectedFolderPath);

                    FolderSelectionAnchor       = SelectedFolderPath;
                    bFolderSelectionAnchorValid = true;
                }
                else
                {
                    FolderSelectionAnchor.Clear();
                    bFolderSelectionAnchorValid = false;
                }

                LastActiveFolderPath     = SelectedFolderPath;
                bHasLastActiveFolderPath = true;
            }

            FolderVisiblePaths.Clear();

            ImGuiStorage* Storage = ImGui::GetStateStorage();

            const auto CollectVisiblePathsRecursive = [&](auto&& Self, FileInfo& Folder, TArray<int32>& Path, int32 Depth) -> void
            {
                if (!FolderTreeMatches(Folder, TrimmedQuery))
                {
                    return;
                }

                ImGui::PushID("FolderTreeNode");
                for (int32 IdIndex = 0; IdIndex < Path.Size(); ++IdIndex)
                {
                    ImGui::PushID(Path[IdIndex]);
                }

                FolderVisiblePaths.Add(Path);

                const bool bHasChildFolders = HasChildFolders(Folder);

                bool bOpen = false;
                if (bHasChildFolders)
                {
                    const ImGuiID OpenId = ImGui::GetID("##CB_Open");
                    bOpen = Storage->GetBool(OpenId, (Depth == 0));
                }

                for (int32 IdIndex = 0; IdIndex < Path.Size(); ++IdIndex)
                {
                    ImGui::PopID();
                }

                ImGui::PopID();

                if (!bOpen)
                {
                    return;
                }

                for (int32 ChildIndex = 0; ChildIndex < Folder.FolderContents.Size(); ++ChildIndex)
                {
                    FileInfo& Child = Folder.FolderContents[ChildIndex];
                    if (!Child.bIsFolder)
                    {
                        continue;
                    }

                    if (bFolderSearchActive && !FolderTreeMatches(Child, TrimmedQuery))
                    {
                        continue;
                    }

                    Path.Add(ChildIndex);
                    Self(Self, Child, Path, Depth + 1);
                    Path.Pop();
                }
            };

            for (int32 RootIndex = 0; RootIndex < RootFolders.Size(); ++RootIndex)
            {
                FileInfo& Root = RootFolders[RootIndex];
                if (!Root.bIsFolder)
                {
                    continue;
                }

                if (bFolderSearchActive && !FolderTreeMatches(Root, TrimmedQuery))
                {
                    continue;
                }

                TArray<int32> Path;
                Path.Add(RootIndex);

                CollectVisiblePathsRecursive(CollectVisiblePathsRecursive, Root, Path, 0);
            }

            for (int32 RootIndex = 0; RootIndex < RootFolders.Size(); ++RootIndex)
            {
                FileInfo& Root = RootFolders[RootIndex];
                if (!Root.bIsFolder)
                {
                    continue;
                }

                if (bFolderSearchActive && !FolderTreeMatches(Root, TrimmedQuery))
                {
                    continue;
                }

                TArray<int32> Path;
                Path.Add(RootIndex);

                DrawFolderTreeRecursive(Root, Path, 0, Storage, NameTextColor, FolderActiveColor, FolderInactiveColor, FolderHoverColor, FolderPathColor, TrimmedQuery);
            }

            ShadowState = CaptureScrollShadowState();

            if (EditorWidgets::BeginPopupContextWindow("FolderPanelContextMenu"))
            {
                EditorWidgets::MenuLabeledSeparator("Create");

                const bool bHasFolderSelection = SelectedFolderPath.Size() > 0;
                if (EditorWidgets::MenuItem("New folder", nullptr, false, true))
                {
                    AddNewFolderInCurrentPath();
                }

                EditorWidgets::MenuLabeledSeparator("Common");

                if (EditorWidgets::MenuItem("Delete", "Delete", false, bHasFolderSelection))
                {
                    DeleteSelectedFolderInTree();
                }
                
                if (EditorWidgets::MenuItem("Rename", "F2", false, bHasFolderSelection))
                {
                    if (bHasFolderSelection)
                    {
                        const FileInfo* Folder = GetFolderFromPath(SelectedFolderPath);
                        if (Folder)
                        {
                            BeginFolderRename(SelectedFolderPath, *Folder);
                        }
                    }
                }
                
                if (EditorWidgets::MenuItem("Copy", nullptr, false, bHasFolderSelection))
                {
                    CopySelectedFolderPaths();
                }
                
                if (EditorWidgets::MenuItem("Paste", nullptr, false, HasClipboardContent()))
                {
                    PasteInCurrentFolder();
                }
                
                EditorWidgets::EndPopupContext();
            }

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete) && SelectedFolderPath.Size() > 0)
            {
                DeleteConfirmContext.Title = "Delete Folder";
                
                const FileInfo* Folder = GetFolderFromPath(SelectedFolderPath);
                if (Folder)
                {
                    const FString FolderName = Folder->Name.IsEmpty() ? "this folder" : Folder->Name;
                    DeleteConfirmContext.Message.Format("Are you sure you want to delete \"%s\" and its contents?", *FolderName);
                }
                else
                {
                    DeleteConfirmContext.Message = "Are you sure you want to delete this folder and its contents?";
                }
                
                DeleteConfirmContext.bVisible = true;
                bPendingDeleteFolder          = true;
            }
        }

        ImGui::EndChild();

        DrawScrollShadows(ShadowState);

        if (PendingFolderMoves.Size() > 0)
        {
            for (int32 MoveIndex = 0; MoveIndex < PendingFolderMoves.Size(); ++MoveIndex)
            {
                const FFolderMoveRequest& Request = PendingFolderMoves[MoveIndex];
                MoveItemsToFolder(Request.SourceParentPath, Request.SourceIndices, Request.TargetFolderPath);
            }

            PendingFolderMoves.Clear();

            FolderSelectionPaths.Clear();
            if (SelectedFolderPath.Size() > 0)
            {
                FolderSelectionPaths.Add(SelectedFolderPath);

                FolderSelectionAnchor       = SelectedFolderPath;
                bFolderSelectionAnchorValid = true;
            }
            else
            {
                FolderSelectionAnchor.Clear();
                bFolderSelectionAnchorValid = false;
            }
        }

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

        if (EditorWidgets::BeginPopupContextWindow("ContentPanelContextMenu"))
        {
            EditorWidgets::MenuLabeledSeparator("Create");

            const bool bHasSelection = SelectedItemIndices.Size() > 0;
            if (EditorWidgets::MenuItem("New folder", nullptr, false, true))
            {
                AddNewFolderInCurrentPath();
            }

            EditorWidgets::MenuLabeledSeparator("Common");

            if (EditorWidgets::MenuItem("Delete", "Delete", false, bHasSelection))
            {
                DeleteSelectedContentItems();
            }

            if (EditorWidgets::MenuItem("Rename", "F2", false, bHasSelection))
            {
                if (bHasSelection && SelectedFolderPath.Size() > 0)
                {
                    FileInfo* Folder = GetFolderFromPath(SelectedFolderPath);
                    if (Folder && Folder->FolderContents.IsValidIndex(SelectedItemIndices[0]))
                    {
                        BeginItemRename(SelectedFolderPath, SelectedItemIndices[0], Folder->FolderContents[SelectedItemIndices[0]]);
                    }
                }
            }

            if (EditorWidgets::MenuItem("Copy", nullptr, false, bHasSelection))
            {
                CopySelectedContent();
            }

            if (EditorWidgets::MenuItem("Paste", nullptr, false, HasClipboardContent()))
            {
                PasteInCurrentFolder();
            }

            EditorWidgets::EndPopupContext();
        }

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete) && SelectedItemIndices.Size() > 0)
        {
            DeleteConfirmContext.Title = "Delete";
            
            FileInfo* Folder = GetFolderFromPath(SelectedFolderPath);
            if (Folder && SelectedItemIndices.Size() == 1)
            {
                const int32 Index = SelectedItemIndices[0];
                if (Folder->FolderContents.IsValidIndex(Index))
                {
                    const FileInfo& Item = Folder->FolderContents[Index];
                    const FString ItemName = Item.Name.IsEmpty() ? "this item" : Item.Name;
                    DeleteConfirmContext.Message.Format("Are you sure you want to delete \"%s\"?", *ItemName);
                }
                else
                {
                    DeleteConfirmContext.Message = "Are you sure you want to delete the selected item?";
                }
            }
            else
            {
                DeleteConfirmContext.Message = "Are you sure you want to delete the selected items?";
            }
            
            DeleteConfirmContext.bVisible = true;
            bPendingDeleteContent         = true;
        }
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

    const bool  bIsFolder = InItem.bIsFolder;
    const CHAR* ItemName  = InItem.Name.IsEmpty() ? "" : *InItem.Name;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 2.0f);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, TooltipBg);
    ImGui::PushStyleColor(ImGuiCol_Border, TooltipBorder);
    ImGui::PushStyleColor(ImGuiCol_Separator, TooltipBorder);

    ImGui::BeginTooltip();

    ImGui::PushStyleColor(ImGuiCol_Text, TextWhite);
    ImGui::TextUnformatted(ItemName);
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

    TStaticArray<CHAR, 512> FolderPathBuf{};
    BuildFolderPathString(SelectedFolderPath, FolderPathBuf.Data(), static_cast<int32>(FolderPathBuf.Size()));

    TStaticArray<CHAR, 768> FullPathBuf{};
    if (FolderPathBuf[0] != 0)
    {
        FCString::Snprintf(FullPathBuf.Data(), static_cast<int32>(FullPathBuf.Size()), "%s/%s", FolderPathBuf.Data(), ItemName);
    }
    else
    {
        FCString::Snprintf(FullPathBuf.Data(), static_cast<int32>(FullPathBuf.Size()), "%s", ItemName);
    }

    const ImVec4 MutedTextColor = ImVec4(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, MutedTextColor);
    ImGui::TextUnformatted("Path:");
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Text, TextWhite);
    ImGui::TextUnformatted(FullPathBuf.Data());
    ImGui::PopStyleColor();

    ImGui::EndTooltip();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(4);
}

void FEditorContentBrowserWidget::DrawContentGrid()
{
    // -----------------------------------------------------------------------------------------
    // Tile Layout
    // -----------------------------------------------------------------------------------------

    const ImVec4 NameTextColor     = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
    const ImVec4 MutedTextColor    = ImVec4(122.0f / 255.0f, 122.0f / 255.0f, 122.0f / 255.0f, 1.0f);
    const ImU32  TileSelectedColor = IM_COL32(0, 112, 224, 255);
    const ImU32  TileHoverColor    = IM_COL32(47, 47, 47, 255);
    const ImU32  TileIdleColor     = IM_COL32(31, 31, 31, 255);
    const ImU32  TileRenameColor   = IM_COL32(0x3f, 0x7b, 0xb6, 160);
    const ImVec4 RenameBg          = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
    const ImU32  BorderNormal      = IM_COL32(51, 51, 51, 255);
    const ImU32  BorderHovered     = IM_COL32(74, 74, 74, 255);
    const ImU32  BorderActive      = IM_COL32(9, 92, 176, 255);

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

        TStaticArray<CHAR, 512> FolderPathBuf{};
        BuildFolderPathString(SelectedFolderPath, FolderPathBuf.Data(), static_cast<int32>(FolderPathBuf.Size()));

        const CHAR* ItemName = InItem.Name.IsEmpty() ? "" : *InItem.Name;

        if (FolderPathBuf[0] != 0)
        {
            FCString::Snprintf(OutBuf, OutBufSize, "%s/%s", FolderPathBuf.Data(), ItemName);
        }
        else
        {
            FCString::Snprintf(OutBuf, OutBufSize, "%s", ItemName);
        }
    };

    const auto DrawLabelWithSearchHighlight = [&](ImDrawList* InDrawList, const ImVec2& InLabelMin, const ImVec2& InLabelMax, const CHAR* InText, const CHAR* InFilterText, ImU32 InBaseTextU32)
    {
        if (!InDrawList)
        {
            return;
        }

        const CHAR* Text       = InText ? InText : "";
        const CHAR* FilterText = (InFilterText && *InFilterText != 0) ? InFilterText : nullptr;

        const ImVec2 FullSize       = ImGui::CalcTextSize(Text);
        const float  AvailableWidth = Math::Max(1.0f, InLabelMax.x - InLabelMin.x);
        const float  X              = InLabelMin.x + Math::Max(0.0f, (AvailableWidth - FullSize.x) * 0.5f);
        const float  Y              = InLabelMin.y;
        const ImVec2 TextStart      = ImVec2(X, Y);

        InDrawList->PushClipRect(InLabelMin, InLabelMax, true);
        EditorWidgets::DrawTextWithSearchHighlight(InDrawList, TextStart, Text, FilterText, InBaseTextU32);
        InDrawList->PopClipRect();
    };

    const auto GetItemIcon = [&](const FileInfo& InItem) -> ImTextureID
    {
        ImTextureID Icon = InItem.bIsFolder ? EditorIcons::FolderIcon : EditorIcons::DocumentIcon;
        if (!Icon && InItem.bIsFolder)
        {
            Icon = EditorIcons::FolderSmallIcon;
        }
        if (!Icon && !InItem.bIsFolder)
        {
            Icon = EditorIcons::DocumentSmallIcon;
        }
        return Icon;
    };

    // -----------------------------------------------------------------------------------------
    // Find folder items
    // -----------------------------------------------------------------------------------------

    FileInfo* Folder = GetFolderFromPath(SelectedFolderPath);
    bool bDrawGrid = true;
    if (!Folder)
    {
        DrawCenteredMessage("No folder selected", MutedTextColor);
        bDrawGrid = false;
    }

    TArray<FileInfo>* ItemsPtr = Folder ? &Folder->FolderContents : nullptr;

    if (bDrawGrid)
    {
        TArray<FileInfo>& Items = *ItemsPtr;
        
        const ImGuiIO& IO = ImGui::GetIO();
        
        TStaticArray<CHAR, 256> AssetQueryBuf{};
        
        const CHAR* AssetQuery = EditorHelpers::GetTrimmedQuery(AssetSearchBuffer.Data(), AssetQueryBuf.Data(), static_cast<int32>(AssetQueryBuf.Size()));
        if (RenamingItemIndex >= 0 && !ArePathsEqual(RenamingItemParentPath, SelectedFolderPath))
        {
            CommitItemRename();
        }

        if (RenamingItemIndex >= 0 && !Items.IsValidIndex(RenamingItemIndex))
        {
            CommitItemRename();
        }

        int32 VisibleCount = 0;
        for (int32 i = 0; i < Items.Size(); ++i)
        {
            if (MatchesSearch(Items[i].Name.IsEmpty() ? "" : *Items[i].Name, AssetQuery))
            {
                ++VisibleCount;
            }
        }

        if (Items.Size() <= 0)
        {
            DrawCenteredMessage("Folder is empty", MutedTextColor);
        }
        else if (VisibleCount <= 0)
        {
            DrawCenteredMessage("No results", MutedTextColor);
        }
        else
        {
            const bool bWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

            if (bWindowFocused && !IO.WantTextInput && RenamingItemIndex < 0 && SelectedItemIndices.Size() > 0)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_F2))
                {
                    int32 TargetIndex = LastSelectedItemIndex;
                    if (!Items.IsValidIndex(TargetIndex) || !IsItemSelected(TargetIndex))
                    {
                        TargetIndex = SelectedItemIndices[0];
                    }

                    if (Items.IsValidIndex(TargetIndex))
                    {
                        BeginItemRename(SelectedFolderPath, TargetIndex, Items[TargetIndex]);
                    }
                }
            }
        }
    // -----------------------------------------------------------------------------------------
    // Grid layout
    // -----------------------------------------------------------------------------------------

    const float CellWidth  = TileWidth + ImGui::GetStyle().CellPadding.x * 2.0f;
    const float AvailableX = ImGui::GetContentRegionAvail().x;

    int32 ColumnCount = static_cast<int32>(AvailableX / CellWidth);
    if (ColumnCount < 1)
    {
        ColumnCount = 1;
    }

    // -----------------------------------------------------------------------------------------
    // Draw tiles
    // -----------------------------------------------------------------------------------------

    if (ImGui::BeginTable("##CB_AssetGrid", ColumnCount, ImGuiTableFlags_SizingFixedFit))
    {
        const bool bCtrlHeld  = IO.KeyCtrl;
        const bool bShiftHeld = IO.KeyShift;

        for (int32 i = 0; i < Items.Size(); ++i)
        {
            FileInfo& Item = Items[i];

            if (!MatchesSearch(Item.Name.IsEmpty() ? "" : *Item.Name, AssetQuery))
            {
                continue;
            }

            ImGui::TableNextColumn();
            ImGui::PushID(i);

            const bool bSelected    = IsItemSelected(i);
            const bool bWasSelected = bSelected;
            const bool bIsFolder    = Item.bIsFolder;
            
            bool bIsRenaming = IsRenamingItem(SelectedFolderPath, i);
            if (bIsRenaming && !bSelected)
            {
                CommitItemRename();
                bIsRenaming = false;
            }

            const ImVec2 TileStart = ImGui::GetCursorScreenPos();
            const ImVec2 TileEnd   = ImVec2(TileStart.x + TileWidth, TileStart.y + TileHeight);
            const ImVec2 LabelMin  = ImVec2(TileStart.x + 8.0f, TileEnd.y - LabelAreaHeight + 6.0f);
            const ImVec2 LabelMax  = ImVec2(TileEnd.x - 8.0f, TileEnd.y - 6.0f);
            const ImVec2 MousePos  = IO.MousePos;

            const bool bMouseInLabel = (MousePos.x >= LabelMin.x && MousePos.x <= LabelMax.x && MousePos.y >= LabelMin.y && MousePos.y <= LabelMax.y);

            ImGui::InvisibleButton("##TileBtn", ImVec2(TileWidth, TileHeight));

            const bool bHovered     = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            const bool bPressed     = ImGui::IsItemClicked();
            const bool bDoubleClick = bPressed && ImGui::IsMouseDoubleClicked(0);

            if (bPressed)
            {
                if (RenamingItemIndex >= 0 && RenamingItemIndex != i)
                {
                    CommitItemRename();
                    bIsRenaming = false;
                }

                if (bShiftHeld)
                {
                    const int32 AnchorIndex = (LastSelectedItemIndex >= 0) ? LastSelectedItemIndex : i;
                    SelectItemRange(AnchorIndex, i, bCtrlHeld);
                }
                else if (bCtrlHeld)
                {
                    ToggleItemSelection(i);
                }
                else
                {
                    if (!IsItemSelected(i) || SelectedItemIndices.Size() <= 1)
                    {
                        SelectSingleItem(i);
                    }
                    else
                    {
                        LastSelectedItemIndex = i;
                    }
                }

                if (bDoubleClick && bIsFolder)
                {
                    TArray<int32> NewPath = SelectedFolderPath;
                    NewPath.Add(i);
                    NavigateToFolderPath(NewPath, true);
                }
                else if (bWasSelected && bMouseInLabel && !bCtrlHeld && !bShiftHeld && !bDoubleClick && !bIsRenaming)
                {
                    BeginItemRename(SelectedFolderPath, i, Item);
                    bIsRenaming = true;
                }
            }

            ImDrawList* WindowDrawList = ImGui::GetWindowDrawList();
            
            const ImU32 BackGround = bIsRenaming ? TileRenameColor : (bSelected ? TileSelectedColor : (bHovered ? TileHoverColor : TileIdleColor));

            // -----------------------------------------------------------------------------
            // Tile shadow
            // -----------------------------------------------------------------------------

            if (bHovered || bSelected)
            {
                constexpr float ShadowOffsetY = 3.0f;
                constexpr int32 ShadowLayers  = 3;

                for (int32 Layer = 0; Layer < ShadowLayers; ++Layer)
                {
                    const float  Expand    = static_cast<float>(Layer);
                    const float  Rounding  = CornerRounding + Expand;
                    const int32  Alpha     = (Layer == 0) ? 55 : (Layer == 1) ? 30 : 16;
                    const ImU32  ShadowCol = IM_COL32(0, 0, 0, Alpha);
                    const ImVec2 ShadowMin = ImVec2(TileStart.x - Expand, TileStart.y - Expand + ShadowOffsetY);
                    const ImVec2 ShadowMax = ImVec2(TileEnd.x + Expand, TileEnd.y + Expand + ShadowOffsetY);

                    WindowDrawList->AddRectFilled(ShadowMin, ShadowMax, ShadowCol, Rounding);
                }
            }

            WindowDrawList->AddRectFilled(TileStart, TileEnd, BackGround, CornerRounding);

            ImTextureID Icon = GetItemIcon(Item);
            if (Icon)
            {
                const float  IconAreaHeight = TileHeight - LabelAreaHeight;
                const float  MaxIconSz      = Math::Min((TileWidth - IconPadding * 2.0f), (IconAreaHeight - IconPadding * 2.0f));
                const float  IconSz         = Math::Max(1.0f, MaxIconSz);
                const ImVec2 IconMin        = ImVec2(TileStart.x + (TileWidth - IconSz) * 0.5f, TileStart.y + (IconAreaHeight - IconSz) * 0.5f);
                const ImVec2 IconMax        = ImVec2(IconMin.x + IconSz, IconMin.y + IconSz);

                WindowDrawList->AddImage(Icon, IconMin, IconMax);
            }

            if (bIsRenaming)
            {
                ImGuiStyle& Style = ImGui::GetStyle();

                const float LabelHeight      = LabelMax.y - LabelMin.y;
                const float DesiredFramePadY = Math::Max(0.0f, (LabelHeight - ImGui::GetFontSize()) * 0.5f);
                const float InputWidth       = Math::Max(1.0f, (LabelMax.x - LabelMin.x));

                ImGui::SetCursorScreenPos(LabelMin);
                ImGui::SetNextItemWidth(InputWidth);

                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Style.FramePadding.x, DesiredFramePadY));

                ImGui::PushStyleColor(ImGuiCol_FrameBg, RenameBg);
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, RenameBg);
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, RenameBg);
                ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);

                if (bRequestItemRenameFocus)
                {
                    ImGui::SetKeyboardFocusHere();
                    bRequestItemRenameFocus = false;
                }

                const ImGuiInputTextFlags InputFlags =
                    ImGuiInputTextFlags_EnterReturnsTrue |
                    ImGuiInputTextFlags_AutoSelectAll;

                const bool bEnter = ImGui::InputText("##RenameItem", ItemRenameBuffer.Data(), ItemRenameBuffer.Size(), InputFlags);

                ImGui::PopStyleColor(4);
                ImGui::PopStyleVar(2);

                {
                    ImVec2 ItemMin = ImGui::GetItemRectMin();
                    ItemMin.x -= 1.0f;
                    ItemMin.y += 1.0f;

                    ImVec2 ItemMax = ImGui::GetItemRectMax();
                    ItemMax.x += 1.0f;
                    ItemMax.y -= 1.0f;

                    const bool bActive        = ImGui::IsItemActive();
                    const bool bInputHovered  = ImGui::IsItemHovered();

                    const ImU32 BorderColor = bActive ? BorderActive : (bInputHovered ? BorderHovered : BorderNormal);
                    WindowDrawList->AddRect(ItemMin, ItemMax, BorderColor, 4.0f, ImDrawListFlags_AntiAliasedLines, 2.0f);
                }

                if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    FCString::Strncpy(ItemRenameBuffer.Data(), ItemRenameBufferOriginal.Data(), ItemRenameBuffer.Size());
                    CancelItemRename();
                }
                else if (bEnter || ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemDeactivated())
                {
                    CommitItemRename();
                }
            }
            else
            {
                const CHAR* FilterText = (AssetQuery && *AssetQuery != 0) ? AssetQuery : nullptr;

                ImGui::PushStyleColor(ImGuiCol_Text, NameTextColor);
                
                const ImU32 BaseTextU32 = ImGui::GetColorU32(ImGuiCol_Text);
                DrawLabelWithSearchHighlight(WindowDrawList, LabelMin, LabelMax, Item.Name.IsEmpty() ? "" : *Item.Name, FilterText, BaseTextU32);
                
                ImGui::PopStyleColor();
            }
            
            // ---------------------------------------------------------------------------------
            // Drag source (Folders and Files)
            // ---------------------------------------------------------------------------------

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID | ImGuiDragDropFlags_SourceNoPreviewTooltip))
            {
                if (!IsItemSelected(i))
                {
                    SelectSingleItem(i);
                }

                int32 PrimaryIndex = i;
                if (SelectedItemIndices.Size() > 0 && Items.IsValidIndex(SelectedItemIndices[0]))
                {
                    PrimaryIndex = SelectedItemIndices[0];
                }

                const FileInfo& PrimaryItem = Items[PrimaryIndex];
                ImTextureID     PrimaryIcon = GetItemIcon(PrimaryItem);

                FCBDndPayload Payload = {};
                Payload.Depth = Math::Min(SelectedFolderPath.Size(), static_cast<int32>(Payload.Indices.Size()));
                
                for (int32 P = 0; P < Payload.Depth; ++P)
                {
                    Payload.Indices[P] = SelectedFolderPath[P];
                }

                Payload.SourceIndex = i;
                Payload.bIsFolder   = bIsFolder;

                ImGui::SetDragDropPayload("CB_MOVE_ITEM", &Payload, sizeof(FCBDndPayload));

                bDragPreviewActive        = true;
                DragPreviewIcon           = PrimaryIcon ? PrimaryIcon : Icon;
                bDragPreviewIsFolder      = PrimaryItem.bIsFolder;
                DragPreviewSelectionCount = Math::Max(1, SelectedItemIndices.Size());

                const CHAR* PrimaryName = PrimaryItem.Name.IsEmpty() ? "" : *PrimaryItem.Name;
                FCString::Strncpy(DragPreviewSourceName.Data(), PrimaryName, static_cast<int32>(DragPreviewSourceName.Size()));

                ImGui::EndDragDropSource();
            }

            if (bIsFolder && bHovered && ImGui::IsDragDropActive())
            {
                const ImGuiPayload* ActivePayload = ImGui::GetDragDropPayload();
                if (ActivePayload && ActivePayload->IsDataType("CB_MOVE_ITEM") && ActivePayload->DataSize == static_cast<int32>(sizeof(FCBDndPayload)))
                {
                    const FCBDndPayload* Data = reinterpret_cast<const FCBDndPayload*>(ActivePayload->Data);
                    if (Data)
                    {
                        const FileInfo* SourceParentFolder = nullptr;

                        TArray<int32> SourceParentPath;
                        TArray<int32> SourceIndices;
                        TArray<TArray<int32>> SourceFolderPaths;
                        BuildDragSourceSelection(*Data, SourceParentPath, SourceParentFolder, SourceIndices, SourceFolderPaths);

                        const CHAR* TargetName = Item.Name.IsEmpty() ? "" : *Item.Name;

                        TArray<int32> TargetPath = SelectedFolderPath;
                        TargetPath.Add(i);

                        SetDragPreviewTarget(TargetName, TargetPath, SourceFolderPaths, SourceParentFolder, &SourceIndices, &SourceParentPath);
                    }
                }
                else if (ActivePayload && ActivePayload->IsDataType("CB_MOVE_FOLDER"))
                {
                    TArray<int32> TargetPath = SelectedFolderPath;
                    TargetPath.Add(i);

                    const CHAR* TargetName = Item.Name.IsEmpty() ? "" : *Item.Name;

                    TArray<TArray<int32>> DragPaths = GetFilteredFolderSelectionPaths();
                    AppendFolderPayloadPath(ActivePayload, DragPaths);
                    SetDragPreviewTarget(TargetName, TargetPath, DragPaths, nullptr, nullptr, nullptr);
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
                            const FileInfo* SourceParentFolder = nullptr;

                            TArray<int32> SourceParentPath;
                            TArray<int32> SourceIndices;
                            TArray<TArray<int32>> SourceFolderPaths;
                            BuildDragSourceSelection(*Data, SourceParentPath, SourceParentFolder, SourceIndices, SourceFolderPaths);

                            const bool bSameFolder = ArePathsEqual(SourceParentPath, SelectedFolderPath);
                            const CHAR* TargetName = Item.Name.IsEmpty() ? "" : *Item.Name;

                            TArray<int32> TargetPath = SelectedFolderPath;
                            TargetPath.Add(i);
                            
                            SetDragPreviewTarget(TargetName, TargetPath, SourceFolderPaths, SourceParentFolder, &SourceIndices, &SourceParentPath);

                            if (Payload->IsDelivery())
                            {
                                bool bHasMoveCandidate = false;
                                for (int32 Index = 0; Index < SourceIndices.Size(); ++Index)
                                {
                                    if (!(bSameFolder && SourceIndices[Index] == i))
                                    {
                                        bHasMoveCandidate = true;
                                        break;
                                    }
                                }

                                if (bHasMoveCandidate && bDragPreviewHasAnyLegalMove)
                                {
                                    bPendingMove                = true;
                                    PendingMoveSourceParentPath = SourceParentPath;
                                    PendingMoveSourceIndices    = SourceIndices;
                                    PendingMoveTargetFolderPath = TargetPath;
                                }
                            }
                        }
                    }

                    if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("CB_MOVE_FOLDER", DragDropFlags))
                    {
                        TArray<int32> TargetPath = SelectedFolderPath;
                        TargetPath.Add(i);

                        const CHAR* TargetName = Item.Name.IsEmpty() ? "" : *Item.Name;

                        TArray<TArray<int32>> DragPaths = GetFilteredFolderSelectionPaths();
                        AppendFolderPayloadPath(Payload, DragPaths);
                        SetDragPreviewTarget(TargetName, TargetPath, DragPaths, nullptr, nullptr, nullptr);

                        if (Payload->IsDelivery() && bDragPreviewHasAnyLegalMove)
                        {
                            QueueFolderMoveRequests(DragPaths, TargetPath);
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
    }

    if (ImGui::IsDragDropActive() && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
        const ImGuiPayload* ActivePayload = ImGui::GetDragDropPayload();
        if (ActivePayload && ActivePayload->IsDataType("CB_MOVE_FOLDER"))
        {
            const CHAR* TargetName = "Root";
            if (SelectedFolderPath.Size() > 0)
            {
                if (FileInfo* TargetFolder = GetFolderFromPath(SelectedFolderPath))
                {
                    if (!TargetFolder->Name.IsEmpty())
                    {
                        TargetName = *TargetFolder->Name;
                    }
                }
            }

            TArray<TArray<int32>> DragPaths = GetFilteredFolderSelectionPaths();
            if (DragPaths.Size() <= 0 && ActivePayload->DataSize == static_cast<int32>(sizeof(FCBFolderDndPayload)))
            {
                const FCBFolderDndPayload* Data = reinterpret_cast<const FCBFolderDndPayload*>(ActivePayload->Data);
                if (Data)
                {
                    TArray<int32> PayloadPath;
                    PayloadPath.Reserve(Data->Depth);

                    for (int32 P = 0; P < Data->Depth; ++P)
                    {
                        PayloadPath.Add(Data->Indices[P]);
                    }

                    if (PayloadPath.Size() > 0)
                    {
                        DragPaths.Add(PayloadPath);
                    }
                }
            }

            SetDragPreviewTarget(TargetName, SelectedFolderPath, DragPaths, nullptr, nullptr, nullptr);

            ImGuiWindow* Window = ImGui::GetCurrentWindow();
            if (Window)
            {
                const ImRect DropRect(Window->InnerRect.Min, Window->InnerRect.Max);
                if (ImGui::BeginDragDropTargetCustom(DropRect, Window->ID))
                {
                    const ImGuiDragDropFlags DragDropFlags =
                        ImGuiDragDropFlags_AcceptBeforeDelivery |
                        ImGuiDragDropFlags_AcceptNoDrawDefaultRect;

                    if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("CB_MOVE_FOLDER", DragDropFlags))
                    {
                        if (Payload->IsDelivery() && bDragPreviewHasAnyLegalMove)
                        {
                            QueueFolderMoveRequests(DragPaths, SelectedFolderPath);
                        }
                    }

                    ImGui::EndDragDropTarget();
                }
            }
        }
    }

    if (bPendingMove)
    {
        MoveItemsToFolder(PendingMoveSourceParentPath, PendingMoveSourceIndices, PendingMoveTargetFolderPath);
        
        bPendingMove = false;
        PendingMoveSourceIndices.Clear();
        
        ClearItemSelection();
    }

    if (ImGui::IsDragDropActive() && !bDragPreviewActive)
    {
        const ImGuiPayload* ActivePayload = ImGui::GetDragDropPayload();
        if (ActivePayload && ActivePayload->IsDataType("CB_MOVE_FOLDER") && ActivePayload->DataSize == static_cast<int32>(sizeof(FCBFolderDndPayload)))
        {
            const FCBFolderDndPayload* Data = reinterpret_cast<const FCBFolderDndPayload*>(ActivePayload->Data);
            if (Data)
            {
                TArray<int32> PayloadPath;
                PayloadPath.Reserve(Data->Depth);

                for (int32 P = 0; P < Data->Depth; ++P)
                {
                    PayloadPath.Add(Data->Indices[P]);
                }

                TArray<TArray<int32>> DragPaths = GetFilteredFolderSelectionPaths();
                if (DragPaths.Size() <= 0 && PayloadPath.Size() > 0)
                {
                    DragPaths.Add(PayloadPath);
                }

                TArray<int32> PrimaryPath = PayloadPath;
                if (bFolderSelectionAnchorValid && ContainsPath(FolderSelectionPaths, FolderSelectionAnchor))
                {
                    PrimaryPath = FolderSelectionAnchor;
                }
                else if (DragPaths.Size() > 0)
                {
                    PrimaryPath = DragPaths[0];
                }

                const FileInfo* PrimaryFolder = GetFolderFromPath(PrimaryPath);
                
                const CHAR* PrimaryName = (PrimaryFolder && !PrimaryFolder->Name.IsEmpty()) ? *PrimaryFolder->Name : "";
                if (PrimaryName[0] != 0)
                {
                    bDragPreviewActive        = true;
                    DragPreviewIcon           = EditorIcons::FolderIcon ? EditorIcons::FolderIcon : EditorIcons::FolderSmallIcon;
                    bDragPreviewIsFolder      = true;
                    DragPreviewSelectionCount = Math::Max(1, DragPaths.Size());

                    FCString::Strncpy(DragPreviewSourceName.Data(), PrimaryName, static_cast<int32>(DragPreviewSourceName.Size()));
                }
            }
        }
    }

    if (ImGui::IsDragDropActive() && bDragPreviewActive && DragPreviewSourceName[0] != 0)
    {
        const bool bHasFolderHoverTarget = (DragPreviewTargetName[0] != 0);
        const bool bShowTextAndDivider   = bHasFolderHoverTarget;
        const bool bHasLegalMove         = bDragPreviewHasAnyLegalMove;

        int32 IllegalMoveCount = DragPreviewIllegalMoveCount;
        if (!bHasLegalMove && IllegalMoveCount <= 0)
        {
            IllegalMoveCount = Math::Max(1, DragPreviewSelectionCount);
        }

        const bool bHasIllegalMoves    = IllegalMoveCount > 0;
        const bool bStatusForbidden    = bHasFolderHoverTarget && !bHasLegalMove;
        const bool bStatusPartial      = bHasFolderHoverTarget && bHasLegalMove && bHasIllegalMoves;
        const bool bStatusAllowed      = bHasFolderHoverTarget && bHasLegalMove && !bHasIllegalMoves;
        const bool bShowStatusIcon     = bHasFolderHoverTarget;

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

            if (DragPreviewSelectionCount > 1)
            {
                ImDrawList* PreviewDrawList = ImGui::GetWindowDrawList();

                const ImVec2 IconMin = ImGui::GetItemRectMin();
                const ImVec2 IconMax = ImGui::GetItemRectMax();

                TStaticArray<CHAR, 16> CountBuf{};
                FCString::Snprintf(CountBuf.Data(), static_cast<int32>(CountBuf.Size()), "+%d", DragPreviewSelectionCount);

                const ImVec2 TextSize = ImGui::CalcTextSize(CountBuf.Data());

                constexpr float LabelPadX = 6.0f;
                constexpr float LabelPadY = 3.0f;

                ImVec2 LabelMin = ImVec2(IconMin.x + 1.0f, IconMax.y - TextSize.y - LabelPadY * 2.0f - 2.0f);
                ImVec2 LabelMax = ImVec2(LabelMin.x + TextSize.x + LabelPadX * 2.0f, LabelMin.y + TextSize.y + LabelPadY * 2.0f);

                const ImU32 LabelBg   = IM_COL32(24, 24, 24, 255);
                const ImU32 LabelText = IM_COL32(230, 230, 230, 255);

                PreviewDrawList->AddRectFilled(LabelMin, LabelMax, LabelBg, 4.0f);
                PreviewDrawList->AddText(ImVec2(LabelMin.x + LabelPadX, LabelMin.y + LabelPadY), LabelText, CountBuf.Data());
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
                    TStaticArray<CHAR, 256> Line1{};
                    TStaticArray<CHAR, 256> Line2{};
                    
                    int32 LineCount = 0;
                    if (bHasFolderHoverTarget)
                    {
                        const bool bIsMultiSelection = DragPreviewSelectionCount > 1;
                        const int32 OtherSelectionCount = Math::Max(0, DragPreviewSelectionCount - 1);

                        if (bIsMultiSelection)
                        {
                            const CHAR* ItemLabel = (OtherSelectionCount == 1) ? "item" : "items";
                            if (bStatusForbidden)
                            {
                                FCString::Snprintf(Line1.Data(), static_cast<int32>(Line1.Size()), "Cannot move %s and %d %s to %s", DragPreviewSourceName.Data(), OtherSelectionCount, ItemLabel, DragPreviewTargetName.Data());
                            }
                            else
                            {
                                FCString::Snprintf(Line1.Data(), static_cast<int32>(Line1.Size()), "Move %s and %d %s to %s", DragPreviewSourceName.Data(), OtherSelectionCount, ItemLabel, DragPreviewTargetName.Data());
                            }
                        }
                        else
                        {
                            if (bStatusForbidden)
                            {
                                FCString::Snprintf(Line1.Data(), static_cast<int32>(Line1.Size()), "Cannot move %s to %s", DragPreviewSourceName.Data(), DragPreviewTargetName.Data());
                            }
                            else
                            {
                                FCString::Snprintf(Line1.Data(), static_cast<int32>(Line1.Size()), "Move %s to %s", DragPreviewSourceName.Data(), DragPreviewTargetName.Data());
                            }
                        }

                        LineCount = 1;

                        if (bStatusPartial)
                        {
                            const CHAR* ItemLabel = (IllegalMoveCount == 1) ? "item" : "items";
                            const CHAR* Pronoun = (IllegalMoveCount == 1) ? "it" : "they";
                            FCString::Snprintf(Line2.Data(), static_cast<int32>(Line2.Size()), "%d %s will be ignored since %s cannot be moved", IllegalMoveCount, ItemLabel, Pronoun);
                            LineCount = 2;
                        }
                    }

                    if (LineCount > 0)
                    {
                        const float TextHeight      = ImGui::GetTextLineHeight();
                        const float LineSpacing     = ImGui::GetStyle().ItemSpacing.y;
                        const float TextBlockHeight = (LineCount * TextHeight) + ((LineCount - 1) * LineSpacing);

                        float MaxLineWidth = 0.0f;
                        if (Line1[0] != 0)
                        {
                            MaxLineWidth = Math::Max(MaxLineWidth, ImGui::CalcTextSize(Line1.Data()).x);
                        }
                        if (Line2[0] != 0)
                        {
                            MaxLineWidth = Math::Max(MaxLineWidth, ImGui::CalcTextSize(Line2.Data()).x);
                        }

                        const ImTextureID StatusIcon = bStatusForbidden ? EditorIcons::ForbiddenIcon : (bShowStatusIcon ? EditorIcons::CircledCheckmarkIcon : nullptr);
                        
                        const bool  bHasStatusIcon = bShowStatusIcon && StatusIcon;
                        const float StatusIconSize = TextHeight;
                        const float StatusIconGap  = 6.0f;
                        const float StatusIndent   = bHasStatusIcon ? (StatusIconSize + StatusIconGap) : 0.0f;

                        const float StatusAlpha = Math::Clamp(ImGui::GetStyle().Alpha, 0.0f, 1.0f);
                        ImVec4 StatusTint = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
                        if (bStatusForbidden)
                        {
                            StatusTint = ImVec4(250.0f / 255.0f, 0.0f, 0.0f, StatusAlpha);
                        }
                        else if (bStatusPartial)
                        {
                            StatusTint = ImVec4(255.0f / 255.0f, 220.0f / 255.0f, 26.0f / 255.0f, StatusAlpha);
                        }
                        else if (bStatusAllowed)
                        {
                            StatusTint = ImVec4(139.0f / 255.0f, 194.0f / 255.0f, 74.0f / 255.0f, StatusAlpha);
                        }

                        const ImVec2 TextBlockMin = ImGui::GetCursorScreenPos();
                        ImGui::Dummy(ImVec2(StatusIndent + MaxLineWidth, IconSize));

                        ImDrawList* PreviewDrawList = ImGui::GetWindowDrawList();
                        const ImU32 TextColor = ImGui::GetColorU32(NameTextColor);

                        const float TextStartY = TextBlockMin.y + Math::Max(0.0f, (IconSize - TextBlockHeight) * 0.5f);
                        const float TextStartX = TextBlockMin.x + StatusIndent;

                        if (Line1[0] != 0)
                        {
                            PreviewDrawList->AddText(ImVec2(TextStartX, TextStartY), TextColor, Line1.Data());
                        }
                        if (Line2[0] != 0)
                        {
                            PreviewDrawList->AddText(ImVec2(TextStartX, TextStartY + TextHeight + LineSpacing), TextColor, Line2.Data());
                        }

                        if (bHasStatusIcon)
                        {
                            const float StatusY = TextStartY + Math::Max(0.0f, (TextBlockHeight - StatusIconSize) * 0.5f);
                            const ImVec2 StatusMin = ImVec2(TextBlockMin.x, StatusY);
                            const ImVec2 StatusMax = ImVec2(StatusMin.x + StatusIconSize, StatusMin.y + StatusIconSize);

                            PreviewDrawList->AddImage(StatusIcon, StatusMin, StatusMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImGui::GetColorU32(StatusTint));
                        }
                    }
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
        CommitItemRename();
        ClearItemSelection();
    }
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
    DrawList->AddRect(BarMin, BarMax, BorderCol, BarRounding, ImDrawListFlags_AntiAliasedLines, BarBorderTh);

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
            const CHAR* Label  = Folder ? (Folder->Name.IsEmpty() ? "" : *Folder->Name) : "<Invalid>";

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

            EditorWidgets::DrawSearchField("##CB_AssetSearch", "Search Content", AssetSearchBuffer.Data(), AssetSearchBuffer.Size(), SearchWidth, true);

            ImGui::SetCursorScreenPos(ImVec2(RowMin.x, RowMin.y));
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void FEditorContentBrowserWidget::DrawFolderTreeRecursive(FileInfo& InFolder, TArray<int32>& InPath, int32 InDepth, ImGuiStorage* InStorage, const ImVec4& InNameTextColor,
    const ImU32 InFolderActiveColor, const ImU32 InFolderInactiveColor, const ImU32 InFolderHoverColor, const ImU32 InFolderPathColor, const CHAR* InFolderSearchQuery)
{
    const bool bFolderSearchActive = (InFolderSearchQuery && *InFolderSearchQuery != 0);

    if (!FolderTreeMatches(InFolder, InFolderSearchQuery))
    {
        return;
    }

    const bool bOpen = DrawFolderRow(InFolder, InPath, InDepth, InStorage, InNameTextColor, InFolderActiveColor, InFolderInactiveColor, InFolderHoverColor, InFolderPathColor, InFolderSearchQuery);
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

        if (bFolderSearchActive && !FolderTreeMatches(Child, InFolderSearchQuery))
        {
            continue;
        }

        InPath.Add(ChildIndex);
        DrawFolderTreeRecursive(Child, InPath, InDepth + 1, InStorage, InNameTextColor, InFolderActiveColor, InFolderInactiveColor, InFolderHoverColor, InFolderPathColor, InFolderSearchQuery);
        InPath.Pop();
    }
}

bool FEditorContentBrowserWidget::DrawFolderRow(FileInfo& InFolder, const TArray<int32>& InPath, int32 InDepth, ImGuiStorage* InStorage, const ImVec4& InNameTextColor,
    const ImU32 InFolderActiveColor, const ImU32 InFolderInactiveColor, const ImU32 InFolderHoverColor, const ImU32 InFolderPathColor, const CHAR* InFolderSearchQuery)
{
    const auto IsFolderPathSelected = [&](const TArray<int32>& Path) -> bool
    {
        return ContainsPath(FolderSelectionPaths, Path);
    };

    const auto AddFolderPathSelection = [&](const TArray<int32>& Path)
    {
        AddUniquePath(FolderSelectionPaths, Path);
    };

    const auto RemoveFolderPathSelection = [&](const TArray<int32>& Path)
    {
        RemovePath(FolderSelectionPaths, Path);
    };

    const auto FindVisibleIndex = [&](const TArray<int32>& Path) -> int32
    {
        return FindPathIndex(FolderVisiblePaths, Path);
    };

    const auto SelectFolderRange = [&](const TArray<int32>& StartPath, const TArray<int32>& EndPath, bool bAddToExisting)
    {
        int32 StartIndex = FindVisibleIndex(StartPath);
        int32 EndIndex   = FindVisibleIndex(EndPath);

        if (StartIndex < 0 || EndIndex < 0)
        {
            if (!bAddToExisting)
            {
                FolderSelectionPaths.Clear();
            }

            AddFolderPathSelection(EndPath);
            return;
        }

        if (StartIndex > EndIndex)
        {
            const int32 SwapIndex = StartIndex;
            StartIndex = EndIndex;
            EndIndex = SwapIndex;
        }

        if (!bAddToExisting)
        {
            FolderSelectionPaths.Clear();
        }

        for (int32 Index = StartIndex; Index <= EndIndex; ++Index)
        {
            AddFolderPathSelection(FolderVisiblePaths[Index]);
        }
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
    const bool bSelected        = IsFolderPathSelected(InPath);
    const bool bInSelectedPath  = (!bSelected && IsPathPrefixOfSelected(InPath));
    bool       bIsRenaming      = IsRenamingFolderPath(InPath);

    if (bIsRenaming && !bSelected)
    {
        CommitFolderRename();
        bIsRenaming = false;
    }

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

    const ImU32  RowBlue_Rename  = IM_COL32(0x3f, 0x7b, 0xb6, 160);
    const ImVec4 RenameBg        = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
    const ImU32  BorderNormal    = IM_COL32(51, 51, 51, 255);
    const ImU32  BorderHovered   = IM_COL32(74, 74, 74, 255);
    const ImU32  BorderActive    = IM_COL32(9, 92, 176, 255);

    const bool bWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    
    const ImU32 SelectedColor = bIsRenaming ? RowBlue_Rename : ((bWindowFocused && bSelectionActiveInBrowser) ? InFolderActiveColor : InFolderInactiveColor);

    {
        const ImGuiIO& IO = ImGui::GetIO();
        if (bSelected && bWindowFocused && !bIsRenaming && !IO.WantTextInput)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_F2))
            {
                BeginFolderRename(InPath, InFolder);
                bIsRenaming = true;
            }
        }
    }

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
        const ImGuiIO& IO = ImGui::GetIO();

        const bool bCtrlHeld  = IO.KeyCtrl;
        const bool bShiftHeld = IO.KeyShift;

        if (bShiftHeld)
        {
            const TArray<int32>& AnchorPath = (bFolderSelectionAnchorValid ? FolderSelectionAnchor : InPath);
            SelectFolderRange(AnchorPath, InPath, bCtrlHeld);
        }
        else if (bCtrlHeld)
        {
            if (bSelected)
            {
                RemoveFolderPathSelection(InPath);
            }
            else
            {
                AddFolderPathSelection(InPath);
            }
        }
        else
        {
            FolderSelectionPaths.Clear();
            AddFolderPathSelection(InPath);
        }

        FolderSelectionAnchor       = InPath;
        bFolderSelectionAnchorValid = true;
        bSelectionActiveInBrowser   = true;

        if (!bCtrlHeld && !bShiftHeld)
        {
            NavigateToFolderPath(InPath, true);
        }
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

    if (bIsRenaming)
    {
        ImGuiStyle& Style = ImGui::GetStyle();

        const float DesiredFramePadY = Math::Max(0.0f, (Height - FontSize) * 0.5f);
        const float InputX           = X;
        const float InputY           = RowMin.y;
        const float InputWidth       = (RowMax.x - InputX) - 6.0f;
        const float BorderRounding   = 4.0f;
        const float BorderThickness  = 2.0f;

        ImGui::SetCursorScreenPos(ImVec2(InputX, InputY));
        ImGui::SetNextItemWidth(InputWidth > 0.0f ? InputWidth : 0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, BorderRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Style.FramePadding.x, DesiredFramePadY));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, RenameBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, RenameBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, RenameBg);
        ImGui::PushStyleColor(ImGuiCol_Text, InNameTextColor);

        if (bRequestFolderRenameFocus)
        {
            ImGui::SetKeyboardFocusHere();
            bRequestFolderRenameFocus = false;
        }

        const ImGuiInputTextFlags InputFlags =
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_AutoSelectAll;

        const bool bEnter = ImGui::InputText("##RenameFolder", FolderRenameBuffer.Data(), FolderRenameBuffer.Size(), InputFlags);

        ImGui::PopStyleColor(4);
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

            ImDrawList* RenameDrawList = ImGui::GetWindowDrawList();
            RenameDrawList->AddRect(ItemMin, ItemMax, BorderColor, BorderRounding, ImDrawListFlags_AntiAliasedLines, BorderThickness);
        }

        if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            FCString::Strncpy(FolderRenameBuffer.Data(), FolderRenameBufferOriginal.Data(), FolderRenameBuffer.Size());
            CancelFolderRename();
        }
        else if (bEnter || ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemDeactivated())
        {
            CommitFolderRename();
        }
    }
    else
    {
        const CHAR* NameText   = InFolder.Name.IsEmpty() ? "" : *InFolder.Name;
        const CHAR* FilterText = (InFolderSearchQuery && *InFolderSearchQuery != 0) ? InFolderSearchQuery : nullptr;

        const ImU32  BaseTextU32 = ImGui::GetColorU32(InNameTextColor);
        const float  NameTextX   = X + ImGui::GetStyle().FramePadding.x;
        const ImVec2 TextStart   = ImVec2(NameTextX, TextY);

        EditorWidgets::DrawTextWithSearchHighlight(DrawList, TextStart, NameText, FilterText, BaseTextU32, 1.0f, 2.0f, &RowMin, &RowMax);
    }

    // -----------------------------------------------------------------------------------------
    // Drag source/target (Folders)
    // -----------------------------------------------------------------------------------------

    const bool bCanDragFolder = (InPath.Size() > 1);

    if (bCanDragFolder && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID | ImGuiDragDropFlags_SourceNoPreviewTooltip))
    {
        if (!IsFolderPathSelected(InPath))
        {
            FolderSelectionPaths.Clear();
            AddFolderPathSelection(InPath);
            FolderSelectionAnchor = InPath;
            bFolderSelectionAnchorValid = true;
        }

        TArray<TArray<int32>> DragPaths = GetFilteredFolderSelectionPaths();
        if (DragPaths.Size() <= 0)
        {
            DragPaths.Add(InPath);
        }

        TArray<int32> PrimaryPath = InPath;
        if (bFolderSelectionAnchorValid && IsFolderPathSelected(FolderSelectionAnchor))
        {
            PrimaryPath = FolderSelectionAnchor;
        }

        const FileInfo* PrimaryFolder = GetFolderFromPath(PrimaryPath);
        const CHAR* PrimaryName = (PrimaryFolder && !PrimaryFolder->Name.IsEmpty()) ? *PrimaryFolder->Name : (InFolder.Name.IsEmpty() ? "" : *InFolder.Name);

        FCBFolderDndPayload Payload = {};
        Payload.Depth = Math::Min(InPath.Size(), static_cast<int32>(Payload.Indices.Size()));

        for (int32 P = 0; P < Payload.Depth; ++P)
        {
            Payload.Indices[P] = InPath[P];
        }

        ImGui::SetDragDropPayload("CB_MOVE_FOLDER", &Payload, sizeof(FCBFolderDndPayload));

        bDragPreviewActive        = true;
        DragPreviewIcon           = EditorIcons::FolderIcon ? EditorIcons::FolderIcon : EditorIcons::FolderSmallIcon;
        bDragPreviewIsFolder      = true;
        DragPreviewSelectionCount = Math::Max(1, DragPaths.Size());

        FCString::Strncpy(DragPreviewSourceName.Data(), PrimaryName, static_cast<int32>(DragPreviewSourceName.Size()));

        ImGui::EndDragDropSource();
    }

    if (bRowHovered && ImGui::IsDragDropActive())
    {
        if (const ImGuiPayload* ActivePayload = ImGui::GetDragDropPayload())
        {
                    if (ActivePayload->IsDataType("CB_MOVE_ITEM") && ActivePayload->DataSize == static_cast<int32>(sizeof(FCBDndPayload)))
                    {
                        const FCBDndPayload* Data = reinterpret_cast<const FCBDndPayload*>(ActivePayload->Data);
                        if (Data)
                        {
                            const FileInfo* SourceParentFolder = nullptr;
                            
                            TArray<int32>         SourceParentPath;
                            TArray<int32>         SourceIndices;
                            TArray<TArray<int32>> SourceFolderPaths;
                            BuildDragSourceSelection(*Data, SourceParentPath, SourceParentFolder, SourceIndices, SourceFolderPaths);

                            const CHAR* TargetName = InFolder.Name.IsEmpty() ? "" : *InFolder.Name;
                            SetDragPreviewTarget(TargetName, InPath, SourceFolderPaths, SourceParentFolder, &SourceIndices, &SourceParentPath);
                        }
                    }
                    else if (ActivePayload->IsDataType("CB_MOVE_FOLDER"))
                    {
                        const CHAR* TargetName = InFolder.Name.IsEmpty() ? "" : *InFolder.Name;

                        TArray<TArray<int32>> DragPaths = GetFilteredFolderSelectionPaths();
                        AppendFolderPayloadPath(ActivePayload, DragPaths);
                        SetDragPreviewTarget(TargetName, InPath, DragPaths, nullptr, nullptr, nullptr);
                    }
                }
            }

    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiDragDropFlags DragDropFlags =
            ImGuiDragDropFlags_AcceptBeforeDelivery |
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect;

        if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("CB_MOVE_ITEM", DragDropFlags))
        {
            if (const FCBDndPayload* Data = reinterpret_cast<const FCBDndPayload*>(Payload->Data))
            {
                const FileInfo* SourceParentFolder = nullptr;
                
                TArray<int32>         SourceParentPath;
                TArray<int32>         SourceIndices;
                TArray<TArray<int32>> SourceFolderPaths;
                BuildDragSourceSelection(*Data, SourceParentPath, SourceParentFolder, SourceIndices, SourceFolderPaths);

                        const CHAR* TargetName = InFolder.Name.IsEmpty() ? "" : *InFolder.Name;
                        SetDragPreviewTarget(TargetName, InPath, SourceFolderPaths, SourceParentFolder, &SourceIndices, &SourceParentPath);

                if (Payload->IsDelivery() && bDragPreviewHasAnyLegalMove)
                {
                    bPendingMove                = true;
                    PendingMoveSourceParentPath = SourceParentPath;
                    PendingMoveSourceIndices    = SourceIndices;
                    PendingMoveTargetFolderPath = InPath;
                }
            }
        }

        if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("CB_MOVE_FOLDER", DragDropFlags))
        {
                        const CHAR* TargetName = InFolder.Name.IsEmpty() ? "" : *InFolder.Name;

                        TArray<TArray<int32>> DragPaths = GetFilteredFolderSelectionPaths();
                        AppendFolderPayloadPath(Payload, DragPaths);
                        SetDragPreviewTarget(TargetName, InPath, DragPaths, nullptr, nullptr, nullptr);

            if (Payload->IsDelivery() && bDragPreviewHasAnyLegalMove)
            {
                QueueFolderMoveRequests(DragPaths, InPath);
            }
        }

        ImGui::EndDragDropTarget();
    }

    PopFolderNodeIDScope();
    return bOpen;
}

void FEditorContentBrowserWidget::BeginFolderRename(const TArray<int32>& InPath, const FileInfo& InFolder)
{
    CommitItemRename();

    RenamingFolderPath        = InPath;
    bRequestFolderRenameFocus = true;

    FolderRenameBuffer.Fill(0);
    FolderRenameBufferOriginal.Fill(0);

    const CHAR* NameText = InFolder.Name.IsEmpty() ? "" : *InFolder.Name;
    if (NameText[0] != 0)
    {
        FCString::Strncpy(FolderRenameBuffer.Data(), NameText, FolderRenameBuffer.Size());
        FCString::Strncpy(FolderRenameBufferOriginal.Data(), NameText, FolderRenameBufferOriginal.Size());
    }
}

void FEditorContentBrowserWidget::CommitFolderRename()
{
    if (RenamingFolderPath.Size() <= 0)
    {
        bRequestFolderRenameFocus = false;
        return;
    }

    FileInfo* Folder = GetFolderFromPath(RenamingFolderPath);
    if (Folder)
    {
        Folder->Name = FString(FolderRenameBuffer.Data());
    }

    RenamingFolderPath.Clear();
    bRequestFolderRenameFocus = false;
}

void FEditorContentBrowserWidget::CancelFolderRename()
{
    RenamingFolderPath.Clear();
    bRequestFolderRenameFocus = false;
}

void FEditorContentBrowserWidget::BeginItemRename(const TArray<int32>& InParentPath, int32 InIndex, const FileInfo& InItem)
{
    CommitFolderRename();

    RenamingItemParentPath  = InParentPath;
    RenamingItemIndex       = InIndex;
    bRequestItemRenameFocus = true;

    ItemRenameBuffer.Fill(0);
    ItemRenameBufferOriginal.Fill(0);
    ItemRenameExtension.Fill(0);

    const CHAR* NameText = InItem.Name.IsEmpty() ? "" : *InItem.Name;
    if (InItem.bIsFolder)
    {
        if (NameText[0] != 0)
        {
            FCString::Strncpy(ItemRenameBuffer.Data(), NameText, ItemRenameBuffer.Size());
            FCString::Strncpy(ItemRenameBufferOriginal.Data(), NameText, ItemRenameBufferOriginal.Size());
        }
    }
    else
    {
        SplitNameAndExtension(NameText, ItemRenameBuffer, ItemRenameExtension);
        FCString::Strncpy(ItemRenameBufferOriginal.Data(), ItemRenameBuffer.Data(), ItemRenameBufferOriginal.Size());
    }
}

void FEditorContentBrowserWidget::CommitItemRename()
{
    if (RenamingItemIndex < 0 || RenamingItemParentPath.Size() <= 0)
    {
        RenamingItemIndex = -1;
        RenamingItemParentPath.Clear();
        bRequestItemRenameFocus = false;
        ItemRenameExtension.Fill(0);
        return;
    }

    FileInfo* ParentFolder = GetFolderFromPath(RenamingItemParentPath);
    if (ParentFolder && ParentFolder->FolderContents.IsValidIndex(RenamingItemIndex))
    {
        FileInfo& Item = ParentFolder->FolderContents[RenamingItemIndex];
        if (Item.bIsFolder || ItemRenameExtension[0] == 0)
        {
            Item.Name = FString(ItemRenameBuffer.Data());
        }
        else
        {
            TStaticArray<CHAR, 320> NewName{};
            FCString::Snprintf(NewName.Data(), static_cast<int32>(NewName.Size()), "%s%s", ItemRenameBuffer.Data(), ItemRenameExtension.Data());
            Item.Name = FString(NewName.Data());
        }
    }

    RenamingItemIndex = -1;
    RenamingItemParentPath.Clear();
    bRequestItemRenameFocus = false;
    ItemRenameExtension.Fill(0);
}

void FEditorContentBrowserWidget::CancelItemRename()
{
    RenamingItemIndex = -1;
    RenamingItemParentPath.Clear();
    bRequestItemRenameFocus = false;
    ItemRenameExtension.Fill(0);
}

bool FEditorContentBrowserWidget::IsRenamingFolderPath(const TArray<int32>& InPath) const
{
    return (RenamingFolderPath.Size() > 0) && ArePathsEqual(RenamingFolderPath, InPath);
}

bool FEditorContentBrowserWidget::IsRenamingItem(const TArray<int32>& InParentPath, int32 InIndex) const
{
    return (RenamingItemIndex == InIndex) && ArePathsEqual(RenamingItemParentPath, InParentPath);
}

void FEditorContentBrowserWidget::ResetDragPreviewState()
{
    DragPreviewSourceName[0]       = 0;
    DragPreviewTargetName[0]       = 0;
    bDragPreviewActive             = false;
    DragPreviewIcon                = nullptr;
    bDragPreviewIsFolder           = false;
    DragPreviewSelectionCount      = 0;
    bDragPreviewHasAnyLegalMove    = false;
    DragPreviewIllegalMoveCount    = 0;
}

void FEditorContentBrowserWidget::ClearItemSelection()
{
    SelectedItemIndices.Clear();
    LastSelectedItemIndex = -1;
}

void FEditorContentBrowserWidget::SelectSingleItem(int32 InIndex)
{
    SelectedItemIndices.Clear();

    if (InIndex >= 0)
    {
        SelectedItemIndices.Add(InIndex);
        LastSelectedItemIndex = InIndex;
    }
    else
    {
        LastSelectedItemIndex = -1;
    }
}

void FEditorContentBrowserWidget::ToggleItemSelection(int32 InIndex)
{
    for (int32 Index = 0; Index < SelectedItemIndices.Size(); ++Index)
    {
        if (SelectedItemIndices[Index] == InIndex)
        {
            for (int32 ShiftIndex = Index; ShiftIndex < SelectedItemIndices.Size() - 1; ++ShiftIndex)
            {
                SelectedItemIndices[ShiftIndex] = SelectedItemIndices[ShiftIndex + 1];
            }

            SelectedItemIndices.Pop();

            if (LastSelectedItemIndex == InIndex)
            {
                LastSelectedItemIndex = (SelectedItemIndices.Size() > 0) ? SelectedItemIndices.LastElement() : -1;
            }

            return;
        }
    }

    SelectedItemIndices.Add(InIndex);
    LastSelectedItemIndex = InIndex;
}

void FEditorContentBrowserWidget::SelectItemRange(int32 InStartIndex, int32 InEndIndex, bool bAddToExisting)
{
    if (InStartIndex > InEndIndex)
    {
        const int32 SwapIndex = InStartIndex;
        InStartIndex = InEndIndex;
        InEndIndex   = SwapIndex;
    }

    if (!bAddToExisting)
    {
        SelectedItemIndices.Clear();
    }

    for (int32 Index = InStartIndex; Index <= InEndIndex; ++Index)
    {
        bool bAlreadySelected = false;
        for (int32 SelectedIndex = 0; SelectedIndex < SelectedItemIndices.Size(); ++SelectedIndex)
        {
            if (SelectedItemIndices[SelectedIndex] == Index)
            {
                bAlreadySelected = true;
                break;
            }
        }

        if (!bAlreadySelected)
        {
            SelectedItemIndices.Add(Index);
        }
    }

    LastSelectedItemIndex = InEndIndex;
}

bool FEditorContentBrowserWidget::IsItemSelected(int32 InIndex) const
{
    for (int32 Index = 0; Index < SelectedItemIndices.Size(); ++Index)
    {
        if (SelectedItemIndices[Index] == InIndex)
        {
            return true;
        }
    }

    return false;
}

bool FEditorContentBrowserWidget::MoveItemsToFolder(const TArray<int32>& InSourceParentPath, const TArray<int32>& InSourceIndices, const TArray<int32>& InTargetFolderPath)
{
    if (InSourceIndices.Size() <= 0)
    {
        return false;
    }

    if (InTargetFolderPath.Size() <= 0)
    {
        return false;
    }

    FileInfo* SourceParent = GetFolderFromPath(InSourceParentPath);
    if (!SourceParent)
    {
        return false;
    }

    constexpr int32 PathBufSize = 512;
    CHAR SourceParentPathBuf[PathBufSize];
    SourceParentPathBuf[0] = 0;
    BuildFolderPathString(InSourceParentPath, SourceParentPathBuf, PathBufSize);
    const FString SourceParentPathStr = SourceParentPathBuf;

    TArray<int32> TargetParentPath = InTargetFolderPath;

    const int32 TargetFolderIndexOriginal = TargetParentPath.LastElement();
    TargetParentPath.Pop();

    FileInfo* TargetParent = GetFolderFromPath(TargetParentPath);
    if (!TargetParent || !TargetParent->FolderContents.IsValidIndex(TargetFolderIndexOriginal))
    {
        return false;
    }

    TArray<int32> UniqueIndices;
    UniqueIndices.Reserve(InSourceIndices.Size());

    for (int32 Index = 0; Index < InSourceIndices.Size(); ++Index)
    {
        const int32 SourceIndex = InSourceIndices[Index];
        if (!SourceParent->FolderContents.IsValidIndex(SourceIndex))
        {
            continue;
        }

        bool bExists = false;
        for (int32 ExistingIndex = 0; ExistingIndex < UniqueIndices.Size(); ++ExistingIndex)
        {
            if (UniqueIndices[ExistingIndex] == SourceIndex)
            {
                bExists = true;
                break;
            }
        }

        if (!bExists)
        {
            UniqueIndices.Add(SourceIndex);
        }
    }

    if (UniqueIndices.Size() <= 0)
    {
        return false;
    }

    const bool bSameParent = ArePathsEqual(InSourceParentPath, TargetParentPath);
    if (bSameParent)
    {
        for (int32 Index = UniqueIndices.Size() - 1; Index >= 0; --Index)
        {
            if (UniqueIndices[Index] == TargetFolderIndexOriginal)
            {
                for (int32 ShiftIndex = Index; ShiftIndex < UniqueIndices.Size() - 1; ++ShiftIndex)
                {
                    UniqueIndices[ShiftIndex] = UniqueIndices[ShiftIndex + 1];
                }

                UniqueIndices.Pop();
                break;
            }
        }
    }

    if (UniqueIndices.Size() <= 0)
    {
        return false;
    }

    TArray<int32> FilteredIndices;
    FilteredIndices.Reserve(UniqueIndices.Size());

    for (int32 Index = 0; Index < UniqueIndices.Size(); ++Index)
    {
        const int32 SourceIndex = UniqueIndices[Index];
        
        const FileInfo& SourceItem = SourceParent->FolderContents[SourceIndex];
        if (SourceItem.bIsFolder)
        {
            TArray<int32> SourceFullPath = InSourceParentPath;
            SourceFullPath.Add(SourceIndex);

            if (InTargetFolderPath.Size() >= SourceFullPath.Size())
            {
                bool bTargetIsDescendant = true;
                for (int32 P = 0; P < SourceFullPath.Size(); ++P)
                {
                    if (InTargetFolderPath[P] != SourceFullPath[P])
                    {
                        bTargetIsDescendant = false;
                        break;
                    }
                }

                if (bTargetIsDescendant)
                {
                    continue;
                }
            }
        }

        FilteredIndices.Add(SourceIndex);
    }

    if (FilteredIndices.Size() <= 0)
    {
        return false;
    }

    for (int32 Index = 1; Index < FilteredIndices.Size(); ++Index)
    {
        const int32 Key = FilteredIndices[Index];
        int32 InsertIndex = Index - 1;

        while (InsertIndex >= 0 && FilteredIndices[InsertIndex] > Key)
        {
            FilteredIndices[InsertIndex + 1] = FilteredIndices[InsertIndex];
            --InsertIndex;
        }

        FilteredIndices[InsertIndex + 1] = Key;
    }

    TArray<int32> OriginalTargetPath = TargetParentPath;
    OriginalTargetPath.Add(TargetFolderIndexOriginal);

    FileInfo* TargetFolder = GetFolderFromPath(OriginalTargetPath);
    if (!TargetFolder || !TargetFolder->bIsFolder)
    {
        return false;
    }

    bool bDidMove = false;

    TArray<int32> IndicesToRemove;
    IndicesToRemove.Reserve(FilteredIndices.Size());

    TArray<FileInfo> ItemsToMove;
    ItemsToMove.Reserve(FilteredIndices.Size());

    for (int32 Index = 0; Index < FilteredIndices.Size(); ++Index)
    {
        const int32 SourceIndex = FilteredIndices[Index];
        if (!SourceParent->FolderContents.IsValidIndex(SourceIndex))
        {
            continue;
        }

        FileInfo& Item = SourceParent->FolderContents[SourceIndex];
        const FString ItemPath = AppendNameToPath(SourceParentPathStr, Item.Name);
        if (Item.bIsFolder)
        {
            const int32 ExistingFolderIndex = FindChildFolderIndexByName(*TargetFolder, Item.Name);
            if (ExistingFolderIndex >= 0)
            {
                if (MergeFolderContents(TargetFolder->FolderContents[ExistingFolderIndex], Item, ItemPath))
                {
                    bDidMove = true;
                }

                if (Item.FolderContents.Size() <= 0)
                {
                    IndicesToRemove.Add(SourceIndex);
                    bDidMove = true;
                }

                continue;
            }

            ItemsToMove.Add(Item);
            IndicesToRemove.Add(SourceIndex);
            bDidMove = true;
            continue;
        }

        if (FindChildFileIndexByName(*TargetFolder, Item.Name) >= 0)
        {
            ReportFailedMove(ItemPath);
            continue;
        }

        ItemsToMove.Add(Item);
        IndicesToRemove.Add(SourceIndex);
        bDidMove = true;
    }

    if (!bDidMove)
    {
        return false;
    }

    for (int32 Index = IndicesToRemove.Size() - 1; Index >= 0; --Index)
    {
        const int32 RemoveIndex = IndicesToRemove[Index];
        const int32 OldSize     = SourceParent->FolderContents.Size();

        for (int32 ShiftIndex = RemoveIndex; ShiftIndex < OldSize - 1; ++ShiftIndex)
        {
            SourceParent->FolderContents[ShiftIndex] = SourceParent->FolderContents[ShiftIndex + 1];
        }

        SourceParent->FolderContents.Pop();
    }

    int32 AdjustedTargetIndex = TargetFolderIndexOriginal;
    if (bSameParent)
    {
        int32 RemovedBeforeTarget = 0;
        for (int32 Index = 0; Index < IndicesToRemove.Size(); ++Index)
        {
            if (IndicesToRemove[Index] < TargetFolderIndexOriginal)
            {
                ++RemovedBeforeTarget;
            }
        }

        AdjustedTargetIndex = Math::Max(0, TargetFolderIndexOriginal - RemovedBeforeTarget);
    }

    TArray<int32> AdjustedTargetPath = TargetParentPath;
    AdjustedTargetPath.Add(AdjustedTargetIndex);

    TargetFolder = GetFolderFromPath(AdjustedTargetPath);
    if (!TargetFolder || !TargetFolder->bIsFolder)
    {
        return false;
    }

    for (int32 Index = 0; Index < ItemsToMove.Size(); ++Index)
    {
        TargetFolder->FolderContents.Add(ItemsToMove[Index]);
    }

    return true;
}

void FEditorContentBrowserWidget::ReportFailedMove(const FString& InFullPath)
{
    FString Entry = InFullPath;
    if (Entry.IsEmpty())
    {
        Entry = "Unnamed asset";
    }

    LOG_ERROR("Failed to move %s since there already is a file with that name in the directory.", *Entry);

    if (!FailedMoveErrorContext.Entries.Contains(Entry))
    {
        FailedMoveErrorContext.Entries.Add(Entry);
    }

    FailedMoveErrorContext.Title      = "Failed Renames";
    FailedMoveErrorContext.HeaderText = "The following files could not be moved";
    FailedMoveErrorContext.bVisible   = true;
}

bool FEditorContentBrowserWidget::MatchesSearch(const CHAR* InName, const CHAR* InQuery) const
{
    if (!InName)
    {
        return false;
    }

    if (!InQuery || *InQuery == 0)
    {
        return true;
    }

    return FCString::Stristr(InName, InQuery) != nullptr;
}

bool FEditorContentBrowserWidget::FolderTreeMatches(const FileInfo& InFolder, const CHAR* InQuery) const
{
    if (!InQuery || *InQuery == 0)
    {
        return true;
    }

    if (MatchesSearch(InFolder.Name.IsEmpty() ? "" : *InFolder.Name, InQuery))
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

        if (FolderTreeMatches(Child, InQuery))
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

    return IsPathPrefix(InPath, SelectedFolderPath);
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

const FEditorContentBrowserWidget::FileInfo* FEditorContentBrowserWidget::GetFolderFromPath(const TArray<int32>& InPath) const
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

    const FileInfo* Current = &RootFolders[RootIndex];

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

    const CHAR* RootName = Current->Name.IsEmpty() ? "" : *Current->Name;
    Offset += FCString::Snprintf(OutBuf + Offset, OutBufSize - Offset, "%s", RootName);

    for (int32 Depth = 1; Depth < InPath.Size(); ++Depth)
    {
        const int32 ChildIndex = InPath[Depth];
        if (!Current->FolderContents.IsValidIndex(ChildIndex))
        {
            break;
        }

        Current = &Current->FolderContents[ChildIndex];

        const CHAR* ChildName = Current->Name.IsEmpty() ? "" : *Current->Name;
        Offset += FCString::Snprintf(OutBuf + Offset, OutBufSize - Offset, "/%s", ChildName);
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

    SelectedFolderPath = InNewPath;
    ClearItemSelection();

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

    SelectedFolderPath = Prev;
    ClearItemSelection();

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

    SelectedFolderPath = Next;
    ClearItemSelection();

    bSelectionActiveInBrowser = true;
}

bool FEditorContentBrowserWidget::ArePathsEqual(const TArray<int32>& PathA, const TArray<int32>& PathB) const
{
    if (PathA.Size() != PathB.Size())
    {
        return false;
    }

    for (int32 Index = 0; Index < PathA.Size(); ++Index)
    {
        if (PathA[Index] != PathB[Index])
        {
            return false;
        }
    }

    return true;
}

bool FEditorContentBrowserWidget::IsPathPrefix(const TArray<int32>& Prefix, const TArray<int32>& Full) const
{
    if (Prefix.Size() > Full.Size())
    {
        return false;
    }

    for (int32 Index = 0; Index < Prefix.Size(); ++Index)
    {
        if (Prefix[Index] != Full[Index])
        {
            return false;
        }
    }

    return true;
}

int32 FEditorContentBrowserWidget::FindPathIndex(const TArray<TArray<int32>>& Paths, const TArray<int32>& Path) const
{
    for (int32 Index = 0; Index < Paths.Size(); ++Index)
    {
        if (ArePathsEqual(Paths[Index], Path))
        {
            return Index;
        }
    }

    return -1;
}

bool FEditorContentBrowserWidget::ContainsPath(const TArray<TArray<int32>>& Paths, const TArray<int32>& Path) const
{
    return FindPathIndex(Paths, Path) >= 0;
}

void FEditorContentBrowserWidget::AddUniquePath(TArray<TArray<int32>>& Paths, const TArray<int32>& Path) const
{
    if (!ContainsPath(Paths, Path))
    {
        Paths.Add(Path);
    }
}

void FEditorContentBrowserWidget::RemovePath(TArray<TArray<int32>>& Paths, const TArray<int32>& Path) const
{
    const int32 RemoveIndex = FindPathIndex(Paths, Path);
    if (RemoveIndex < 0)
    {
        return;
    }

    for (int32 ShiftIndex = RemoveIndex; ShiftIndex < Paths.Size() - 1; ++ShiftIndex)
    {
        Paths[ShiftIndex] = Paths[ShiftIndex + 1];
    }

    Paths.Pop();
}

TArray<TArray<int32>> FEditorContentBrowserWidget::BuildUniquePaths(const TArray<TArray<int32>>& InPaths) const
{
    TArray<TArray<int32>> UniquePaths;
    UniquePaths.Reserve(InPaths.Size());

    for (int32 Index = 0; Index < InPaths.Size(); ++Index)
    {
        AddUniquePath(UniquePaths, InPaths[Index]);
    }

    return UniquePaths;
}

TArray<TArray<int32>> FEditorContentBrowserWidget::RemoveRootPaths(const TArray<TArray<int32>>& InPaths) const
{
    TArray<TArray<int32>> Result;
    Result.Reserve(InPaths.Size());

    for (int32 Index = 0; Index < InPaths.Size(); ++Index)
    {
        if (InPaths[Index].Size() > 1)
        {
            Result.Add(InPaths[Index]);
        }
    }

    return Result;
}

TArray<TArray<int32>> FEditorContentBrowserWidget::FilterTopLevelPaths(const TArray<TArray<int32>>& InPaths) const
{
    TArray<TArray<int32>> FilteredPaths;
    FilteredPaths.Reserve(InPaths.Size());

    for (int32 Index = 0; Index < InPaths.Size(); ++Index)
    {
        const TArray<int32>& Path = InPaths[Index];

        bool bIsDescendant = false;
        for (int32 OtherIndex = 0; OtherIndex < InPaths.Size(); ++OtherIndex)
        {
            if (OtherIndex == Index)
            {
                continue;
            }

            if (IsPathPrefix(InPaths[OtherIndex], Path))
            {
                bIsDescendant = true;
                break;
            }
        }

        if (!bIsDescendant)
        {
            FilteredPaths.Add(Path);
        }
    }

    return FilteredPaths;
}

TArray<TArray<int32>> FEditorContentBrowserWidget::GetFilteredFolderSelectionPaths() const
{
    return FilterTopLevelPaths(RemoveRootPaths(BuildUniquePaths(FolderSelectionPaths)));
}

void FEditorContentBrowserWidget::QueueFolderMoveRequests(const TArray<TArray<int32>>& DragPaths, const TArray<int32>& TargetPath)
{
    if (TargetPath.Size() <= 0)
    {
        return;
    }

    struct FFolderMoveGroup
    {
        TArray<int32> ParentPath;
        TArray<int32> Indices;
    };

    TArray<FFolderMoveGroup> MoveGroups;
    MoveGroups.Reserve(DragPaths.Size());

    for (int32 Index = 0; Index < DragPaths.Size(); ++Index)
    {
        const TArray<int32>& SourcePath = DragPaths[Index];
        if (SourcePath.Size() <= 1)
        {
            continue;
        }

        TArray<int32> SourceParentPath = SourcePath;
        const int32 SourceIndex = SourceParentPath.LastElement();
        SourceParentPath.Pop();

        int32 GroupIndex = -1;
        for (int32 ExistingGroup = 0; ExistingGroup < MoveGroups.Size(); ++ExistingGroup)
        {
            if (ArePathsEqual(MoveGroups[ExistingGroup].ParentPath, SourceParentPath))
            {
                GroupIndex = ExistingGroup;
                break;
            }
        }

        if (GroupIndex < 0)
        {
            FFolderMoveGroup NewGroup;
            NewGroup.ParentPath = SourceParentPath;
            NewGroup.Indices.Add(SourceIndex);
            MoveGroups.Add(NewGroup);
        }
        else
        {
            bool bAlreadyAdded = false;
            for (int32 AddedIndex = 0; AddedIndex < MoveGroups[GroupIndex].Indices.Size(); ++AddedIndex)
            {
                if (MoveGroups[GroupIndex].Indices[AddedIndex] == SourceIndex)
                {
                    bAlreadyAdded = true;
                    break;
                }
            }

            if (!bAlreadyAdded)
            {
                MoveGroups[GroupIndex].Indices.Add(SourceIndex);
            }
        }
    }

    for (int32 GroupIndex = 0; GroupIndex < MoveGroups.Size(); ++GroupIndex)
    {
        if (MoveGroups[GroupIndex].Indices.Size() <= 0)
        {
            continue;
        }

        FFolderMoveRequest Request;
        Request.SourceParentPath = MoveGroups[GroupIndex].ParentPath;
        Request.SourceIndices    = MoveGroups[GroupIndex].Indices;
        Request.TargetFolderPath = TargetPath;

        PendingFolderMoves.Add(Request);
    }
}

void FEditorContentBrowserWidget::BuildDragSourceSelection(const FCBDndPayload& Data, TArray<int32>& OutSourceParentPath, const FileInfo*& OutSourceParentFolder, TArray<int32>& OutSourceIndices, TArray<TArray<int32>>& OutSourceFolderPaths) const
{
    OutSourceParentPath.Clear();
    OutSourceIndices.Clear();
    OutSourceFolderPaths.Clear();
    OutSourceParentFolder = nullptr;

    OutSourceParentPath.Reserve(Data.Depth);
    for (int32 P = 0; P < Data.Depth; ++P)
    {
        OutSourceParentPath.Add(Data.Indices[P]);
    }

    OutSourceParentFolder = GetFolderFromPath(OutSourceParentPath);

    if (ArePathsEqual(OutSourceParentPath, SelectedFolderPath))
    {
        OutSourceIndices = SelectedItemIndices;
    }

    if (OutSourceIndices.Size() <= 0)
    {
        OutSourceIndices.Add(Data.SourceIndex);
    }

    if (!OutSourceParentFolder)
    {
        return;
    }

    for (int32 Index = 0; Index < OutSourceIndices.Size(); ++Index)
    {
        const int32 SourceIndex = OutSourceIndices[Index];
        if (!OutSourceParentFolder->FolderContents.IsValidIndex(SourceIndex))
        {
            continue;
        }

        if (!OutSourceParentFolder->FolderContents[SourceIndex].bIsFolder)
        {
            continue;
        }

        TArray<int32> SourcePath = OutSourceParentPath;
        SourcePath.Add(SourceIndex);
        OutSourceFolderPaths.Add(SourcePath);
    }
}

void FEditorContentBrowserWidget::AppendFolderPayloadPath(const ImGuiPayload* Payload, TArray<TArray<int32>>& InOutPaths) const
{
    if (InOutPaths.Size() > 0 || !Payload || !Payload->IsDataType("CB_MOVE_FOLDER") || Payload->DataSize != static_cast<int32>(sizeof(FCBFolderDndPayload)))
    {
        return;
    }

    const FCBFolderDndPayload* Data = reinterpret_cast<const FCBFolderDndPayload*>(Payload->Data);
    if (!Data)
    {
        return;
    }

    TArray<int32> PayloadPath;
    PayloadPath.Reserve(Data->Depth);

    for (int32 P = 0; P < Data->Depth; ++P)
    {
        PayloadPath.Add(Data->Indices[P]);
    }

    if (PayloadPath.Size() > 0)
    {
        InOutPaths.Add(PayloadPath);
    }
}

int32 FEditorContentBrowserWidget::FindChildFolderIndexByName(const FileInfo& ParentFolder, const FString& FolderName) const
{
    for (int32 Index = 0; Index < ParentFolder.FolderContents.Size(); ++Index)
    {
        const FileInfo& Item = ParentFolder.FolderContents[Index];
        if (!Item.bIsFolder)
        {
            continue;
        }

        if (Item.Name.Equals(FolderName))
        {
            return Index;
        }
    }

    return -1;
}

int32 FEditorContentBrowserWidget::FindChildFileIndexByName(const FileInfo& ParentFolder, const FString& FileName) const
{
    for (int32 Index = 0; Index < ParentFolder.FolderContents.Size(); ++Index)
    {
        const FileInfo& Item = ParentFolder.FolderContents[Index];
        if (Item.bIsFolder)
        {
            continue;
        }

        if (Item.Name.Equals(FileName))
        {
            return Index;
        }
    }

    return -1;
}

void FEditorContentBrowserWidget::UpdateDragPreviewNameConflicts(const TArray<TArray<int32>>& SourceFolderPaths, const FileInfo* SourceParentFolder, const TArray<int32>* SourceIndices, 
    const TArray<int32>* SourceParentPath, const TArray<int32>& TargetPath)
{
    bDragPreviewHasAnyLegalMove = false;
    DragPreviewIllegalMoveCount = 0;

    if (TargetPath.Size() <= 0)
    {
        return;
    }

    const FileInfo* TargetFolder = GetFolderFromPath(TargetPath);
    if (!TargetFolder || !TargetFolder->bIsFolder)
    {
        return;
    }

    TArray<TArray<int32>> FolderPaths;
    FolderPaths.Reserve(DragPreviewSelectionCount);
    TArray<const FileInfo*> FileEntries;
    FileEntries.Reserve(DragPreviewSelectionCount);

    if (SourceParentFolder && SourceIndices && SourceParentPath)
    {
        for (int32 Index = 0; Index < SourceIndices->Size(); ++Index)
        {
            const int32 SourceIndex = (*SourceIndices)[Index];
            if (!SourceParentFolder->FolderContents.IsValidIndex(SourceIndex))
            {
                continue;
            }

            const FileInfo& Item = SourceParentFolder->FolderContents[SourceIndex];
            TArray<int32> ItemPath = *SourceParentPath;
            ItemPath.Add(SourceIndex);

            if (Item.bIsFolder)
            {
                AddUniquePath(FolderPaths, ItemPath);
            }
            else
            {
                FileEntries.Add(&Item);
            }
        }
    }

    for (int32 PathIndex = 0; PathIndex < SourceFolderPaths.Size(); ++PathIndex)
    {
        const TArray<int32>& FolderPath = SourceFolderPaths[PathIndex];
        if (FolderPath.Size() <= 0)
        {
            continue;
        }

        const FileInfo* FolderItem = GetFolderFromPath(FolderPath);
        if (!FolderItem || !FolderItem->bIsFolder)
        {
            continue;
        }

        AddUniquePath(FolderPaths, FolderPath);
    }

    if (FolderPaths.Size() <= 0 && FileEntries.Size() <= 0)
    {
        return;
    }

    for (const TArray<int32>& FolderPath : FolderPaths)
    {
        if (IsPathPrefix(FolderPath, TargetPath))
        {
            ++DragPreviewIllegalMoveCount;
            continue;
        }

        bDragPreviewHasAnyLegalMove = true;
    }

    for (const FileInfo* FileItem : FileEntries)
    {
        if (!FileItem)
        {
            continue;
        }

        if (FindChildFileIndexByName(*TargetFolder, FileItem->Name) >= 0)
        {
            ++DragPreviewIllegalMoveCount;
            continue;
        }

        bDragPreviewHasAnyLegalMove = true;
    }

    if (!bDragPreviewHasAnyLegalMove && DragPreviewIllegalMoveCount <= 0)
    {
        DragPreviewIllegalMoveCount = Math::Max(1, DragPreviewSelectionCount);
    }
}

void FEditorContentBrowserWidget::SetDragPreviewTarget(const CHAR* TargetName, const TArray<int32>& TargetPath, const TArray<TArray<int32>>& SourceFolderPaths,
    const FileInfo* SourceParentFolder, const TArray<int32>* SourceIndices, const TArray<int32>* SourceParentPath)
{
    const CHAR* NameText = (TargetName && TargetName[0]) ? TargetName : "";
    FCString::Strncpy(DragPreviewTargetName.Data(), NameText, static_cast<int32>(DragPreviewTargetName.Size()));
    UpdateDragPreviewNameConflicts(SourceFolderPaths, SourceParentFolder, SourceIndices, SourceParentPath, TargetPath);
}

bool FEditorContentBrowserWidget::MergeFolderContents(FileInfo& TargetFolder, FileInfo& SourceFolder, const FString& SourceFolderPath)
{
    if (!TargetFolder.bIsFolder || !SourceFolder.bIsFolder)
    {
        return false;
    }

    bool bMovedAny = false;

    const auto RemoveSourceItemAt = [&](int32 RemoveIndex)
    {
        const int32 OldSize = SourceFolder.FolderContents.Size();
        for (int32 ShiftIndex = RemoveIndex; ShiftIndex < OldSize - 1; ++ShiftIndex)
        {
            SourceFolder.FolderContents[ShiftIndex] = SourceFolder.FolderContents[ShiftIndex + 1];
        }

        SourceFolder.FolderContents.Pop();
    };

    for (int32 Index = 0; Index < SourceFolder.FolderContents.Size();)
    {
        FileInfo& Item = SourceFolder.FolderContents[Index];
        const FString ItemPath = AppendNameToPath(SourceFolderPath, Item.Name);
        if (Item.bIsFolder)
        {
            const int32 ExistingFolderIndex = FindChildFolderIndexByName(TargetFolder, Item.Name);
            if (ExistingFolderIndex >= 0)
            {
                if (MergeFolderContents(TargetFolder.FolderContents[ExistingFolderIndex], Item, ItemPath))
                {
                    bMovedAny = true;
                }

                if (Item.FolderContents.Size() <= 0)
                {
                    bMovedAny = true;
                    RemoveSourceItemAt(Index);
                    continue;
                }

                ++Index;
                continue;
            }

            TargetFolder.FolderContents.Add(Item);
            bMovedAny = true;
            RemoveSourceItemAt(Index);
            continue;
        }

        if (FindChildFileIndexByName(TargetFolder, Item.Name) >= 0)
        {
            ReportFailedMove(ItemPath);
            ++Index;
            continue;
        }

        TargetFolder.FolderContents.Add(Item);
        bMovedAny = true;
        RemoveSourceItemAt(Index);
    }

    return bMovedAny;
}

FEditorContentBrowserWidget::FileInfo FEditorContentBrowserWidget::DeepCopyFileInfo(const FileInfo& In)
{
    FileInfo Out;
    Out.Name = In.Name;
    Out.bIsFolder = In.bIsFolder;
    for (int32 i = 0; i < In.FolderContents.Size(); ++i)
    {
        Out.FolderContents.Add(DeepCopyFileInfo(In.FolderContents[i]));
    }
    return Out;
}

void FEditorContentBrowserWidget::AddNewFolderInCurrentPath()
{
    CommitItemRename();
    CommitFolderRename();

    FileInfo* ParentFolder = GetFolderFromPath(SelectedFolderPath);
    if (!ParentFolder)
    {
        return;
    }

    const FString NewFolderName("New folder");
    FileInfo NewFolder;
    NewFolder.Name = NewFolderName;
    NewFolder.bIsFolder = true;

    ParentFolder->FolderContents.Add(NewFolder);
    const int32 NewIndex = ParentFolder->FolderContents.Size() - 1;

    // Select the new folder so the rename input is shown (avoids bIsRenaming && !bSelected committing immediately)
    SelectSingleItem(NewIndex);
    BeginItemRename(SelectedFolderPath, NewIndex, ParentFolder->FolderContents[NewIndex]);
}

void FEditorContentBrowserWidget::DeleteSelectedContentItems()
{
    if (SelectedFolderPath.Size() <= 0 || SelectedItemIndices.IsEmpty())
    {
        return;
    }

    FileInfo* ParentFolder = GetFolderFromPath(SelectedFolderPath);
    if (!ParentFolder)
    {
        return;
    }

    TArray<int32> SortedIndices = SelectedItemIndices;
    SortedIndices.Sort();
    for (int32 i = SortedIndices.Size() - 1; i >= 0; --i)
    {
        const int32 Index = SortedIndices[i];
        if (ParentFolder->FolderContents.IsValidIndex(Index))
        {
            ParentFolder->FolderContents.RemoveAt(Index);
        }
    }

    ClearItemSelection();
}

void FEditorContentBrowserWidget::DeleteSelectedFolderInTree()
{
    if (SelectedFolderPath.Size() <= 0)
    {
        return;
    }

    TArray<int32> ParentPath = SelectedFolderPath;
    const int32 FolderIndex = ParentPath.LastElement();
    ParentPath.Pop();

    FileInfo* ParentFolder = GetFolderFromPath(ParentPath);
    if (!ParentFolder || !ParentFolder->FolderContents.IsValidIndex(FolderIndex))
    {
        return;
    }

    ParentFolder->FolderContents.RemoveAt(FolderIndex);
    NavigateToFolderPath(ParentPath, false);
    FolderSelectionPaths.Clear();
    if (ParentPath.Size() > 0)
    {
        FolderSelectionPaths.Add(ParentPath);
        FolderSelectionAnchor = ParentPath;
        bFolderSelectionAnchorValid = true;
    }
}

void FEditorContentBrowserWidget::CopySelectedContent()
{
    if (SelectedFolderPath.Size() <= 0 || SelectedItemIndices.IsEmpty())
    {
        return;
    }

    bClipboardValid = true;
    bClipboardFromContentPanel = true;
    ClipboardContentParentPath = SelectedFolderPath;
    ClipboardContentIndices = SelectedItemIndices;
    ClipboardFolderPaths.Clear();
}

void FEditorContentBrowserWidget::CopySelectedFolderPaths()
{
    TArray<TArray<int32>> Paths = GetFilteredFolderSelectionPaths();
    if (Paths.IsEmpty() && SelectedFolderPath.Size() > 0)
    {
        Paths.Add(SelectedFolderPath);
    }
    if (Paths.IsEmpty())
    {
        return;
    }

    bClipboardValid = true;
    bClipboardFromContentPanel = false;
    ClipboardContentParentPath.Clear();
    ClipboardContentIndices.Clear();
    ClipboardFolderPaths = Paths;
}

bool FEditorContentBrowserWidget::HasClipboardContent() const
{
    return bClipboardValid && (
        (bClipboardFromContentPanel && ClipboardContentIndices.Size() > 0) ||
        (!bClipboardFromContentPanel && ClipboardFolderPaths.Size() > 0));
}

void FEditorContentBrowserWidget::PasteInCurrentFolder()
{
    if (!HasClipboardContent() || SelectedFolderPath.Size() <= 0)
    {
        return;
    }

    FileInfo* TargetFolder = GetFolderFromPath(SelectedFolderPath);
    if (!TargetFolder || !TargetFolder->bIsFolder)
    {
        return;
    }

    if (bClipboardFromContentPanel)
    {
        FileInfo* SourceParent = GetFolderFromPath(ClipboardContentParentPath);
        if (!SourceParent)
        {
            return;
        }

        for (int32 Index = 0; Index < ClipboardContentIndices.Size(); ++Index)
        {
            const int32 SourceIndex = ClipboardContentIndices[Index];
            if (!SourceParent->FolderContents.IsValidIndex(SourceIndex))
            {
                continue;
            }

            const FileInfo& SourceItem = SourceParent->FolderContents[SourceIndex];
            FString BaseName = SourceItem.Name;
            int32 Suffix = 0;
            TStaticArray<CHAR, 256> Buf{};
            while (FindChildFolderIndexByName(*TargetFolder, BaseName) >= 0 ||
                   (!SourceItem.bIsFolder && FindChildFileIndexByName(*TargetFolder, BaseName) >= 0))
            {
                ++Suffix;
                if (SourceItem.bIsFolder)
                {
                    FCString::Snprintf(Buf.Data(), static_cast<int32>(Buf.Size()), "%s (%d)", *SourceItem.Name, Suffix);
                    BaseName = Buf.Data();
                }
                else
                {
                    int32 Dot = -1;
                    const CHAR* NameStr = *SourceItem.Name;
                    const int32 Len = static_cast<int32>(FCString::Strlen(NameStr));
                    for (int32 i = Len - 1; i > 0; --i)
                    {
                        if (NameStr[i] == '.')
                        {
                            Dot = i;
                            break;
                        }
                    }
                    if (Dot >= 0)
                    {
                        FCString::Snprintf(Buf.Data(), static_cast<int32>(Buf.Size()), "%.*s (%d)%s", Dot, NameStr, Suffix, NameStr + Dot);
                        BaseName = Buf.Data();
                    }
                    else
                    {
                        FCString::Snprintf(Buf.Data(), static_cast<int32>(Buf.Size()), "%s (%d)", NameStr, Suffix);
                        BaseName = Buf.Data();
                    }
                }
            }

            FileInfo CopyItem = DeepCopyFileInfo(SourceItem);
            CopyItem.Name = BaseName;
            TargetFolder->FolderContents.Add(CopyItem);
        }
    }
    else
    {
        for (int32 PathIndex = 0; PathIndex < ClipboardFolderPaths.Size(); ++PathIndex)
        {
            const TArray<int32>& FolderPath = ClipboardFolderPaths[PathIndex];
            if (FolderPath.Size() <= 0)
            {
                continue;
            }

            if (IsPathPrefix(FolderPath, SelectedFolderPath))
            {
                continue;
            }

            const FileInfo* SourceFolder = GetFolderFromPath(FolderPath);
            if (!SourceFolder || !SourceFolder->bIsFolder)
            {
                continue;
            }

            FString BaseName = SourceFolder->Name;
            int32 Suffix = 0;
            while (FindChildFolderIndexByName(*TargetFolder, BaseName) >= 0)
            {
                ++Suffix;
                TStaticArray<CHAR, 256> Buf{};
                FCString::Snprintf(Buf.Data(), static_cast<int32>(Buf.Size()), "%s (%d)", *SourceFolder->Name, Suffix);
                BaseName = Buf.Data();
            }

            FileInfo CopyFolder = DeepCopyFileInfo(*SourceFolder);
            CopyFolder.Name = BaseName;
            TargetFolder->FolderContents.Add(CopyFolder);
        }
    }
}
