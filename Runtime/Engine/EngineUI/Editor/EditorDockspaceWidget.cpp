#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorDockspaceWidget.h"
#include "Engine/EngineUI/Editor/EditorFooterWidget.h"
#include "Engine/EngineUI/Editor/EditorOutputLogWidget.h"
#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "Engine/EngineUI/Editor/EditorPropertiesWidget.h"
#include "Engine/EngineUI/Editor/EditorContentBrowserWidget.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include <imgui.h>
#include <imgui_internal.h>

FEditorDockspaceWidget::FEditorDockspaceWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , ImGuiDelegateHandle()
    , LayoutIds()
    , bResetLayout(true)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorDockspaceWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());

        InitializeEditorStyle();
    }
}

FEditorDockspaceWidget::~FEditorDockspaceWidget()
{
    // Release icons
    EditorIcons::Release();

    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }

    EditorEngine = nullptr;
}

bool FEditorDockspaceWidget::InitializeEditorStyle()
{
    ImGuiStyle& Style = ImGui::GetStyle();
    Style.WindowRounding       = 0.0f;
    Style.FrameRounding        = 4.0f;
    Style.GrabRounding         = 4.0f;
    Style.TabRounding          = 4.0f;
    Style.FramePadding         = ImVec2(10.0f, 6.0f);
    Style.ItemSpacing          = ImVec2(8.0f, 6.0f);
    Style.WindowPadding        = ImVec2(10.0f, 10.0f);
    Style.SeparatorTextPadding = ImVec2(6.0f, 6.0f);
    Style.WindowBorderSize     = 0.0f;
    Style.ChildBorderSize      = 0.0f;
    Style.FrameBorderSize      = 0.0f;
    Style.TabBarBorderSize     = 0.0f;
    Style.DockingSeparatorSize = 4.0f;

    // ------------------------------------------------------------
    // Default
    // ------------------------------------------------------------

    Style.Colors[ImGuiCol_WindowBg]       = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_ChildBg]        = ImVec4(0.06f, 0.06f, 0.07f, 1.0f);
    Style.Colors[ImGuiCol_PopupBg]        = ImVec4(0.09f, 0.09f, 0.10f, 1.0f);
    Style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.13f, 0.13f, 0.14f, 1.0f);
    Style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
    Style.Colors[ImGuiCol_FrameBgActive]  = ImVec4(0.20f, 0.20f, 0.23f, 1.0f);
    Style.Colors[ImGuiCol_Button]         = ImVec4(0.15f, 0.15f, 0.17f, 1.0f);
    Style.Colors[ImGuiCol_ButtonHovered]  = ImVec4(0.23f, 0.23f, 0.26f, 1.0f);
    Style.Colors[ImGuiCol_ButtonActive]   = ImVec4(0.28f, 0.28f, 0.32f, 1.0f);
    Style.Colors[ImGuiCol_Header]         = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
    Style.Colors[ImGuiCol_HeaderHovered]  = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
    Style.Colors[ImGuiCol_HeaderActive]   = ImVec4(0.26f, 0.26f, 0.30f, 1.0f);
    Style.Colors[ImGuiCol_NavHighlight]   = ImVec4(0.37f, 0.37f, 0.80f, 1.0f);
    Style.Colors[ImGuiCol_MenuBarBg]      = ImVec4(0.09f, 0.09f, 0.10f, 1.0f);

    // ------------------------------------------------------------
    // Docking split-line / Seam-colors
    // ------------------------------------------------------------

    const ImVec4 SplitterIdle    = ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);
    const ImVec4 SplitterHovered = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
    const ImVec4 SplitterActive  = SplitterHovered;

    Style.Colors[ImGuiCol_Border]            = SplitterIdle;
    Style.Colors[ImGuiCol_BorderShadow]      = SplitterIdle;
    Style.Colors[ImGuiCol_SeparatorHovered]  = SplitterHovered;
    Style.Colors[ImGuiCol_SeparatorActive]   = SplitterActive;
    Style.Colors[ImGuiCol_Separator]         = SplitterIdle;
    Style.Colors[ImGuiCol_ResizeGripHovered] = SplitterHovered;
    Style.Colors[ImGuiCol_ResizeGripActive]  = SplitterActive;
    Style.Colors[ImGuiCol_ResizeGrip]        = SplitterIdle;

    // ------------------------------------------------------------
    // Docking Tabs
    // ------------------------------------------------------------

    Style.Colors[ImGuiCol_Tab]                = ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_TabHovered]         = ImVec4(33.0f / 255.0f, 33.0f / 255.0f, 33.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_TabActive]          = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

    // ------------------------------------------------------------
    // Title Bar
    // ------------------------------------------------------------

    Style.Colors[ImGuiCol_TitleBg]          = ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);

    // ------------------------------------------------------------
    // Scrollbars
    // ------------------------------------------------------------

    Style.ScrollbarRounding = 12.0f;
    Style.ScrollbarSize     = 16.0f;

    Style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(87.0f / 255.0f, 87.0f / 255.0f, 87.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(127.0f / 255.0f, 127.0f / 255.0f, 127.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(127.0f / 255.0f, 127.0f / 255.0f, 127.0f / 255.0f, 1.0f);

    // ------------------------------------------------------------
    // Icons
    // ------------------------------------------------------------

    if (!EditorIcons::Initialize())
    {
        return false;
    }

    return true;
}

void FEditorDockspaceWidget::BuildDockingLayout(FLayoutIds& Ids)
{
    Ids.DockRight        = 0;
    Ids.DockRightTop     = 0;
    Ids.DockRightBottom  = 0;
    Ids.DockCenter       = 0;
    Ids.DockCenterTop    = 0;
    Ids.DockCenterBottom = 0;

    ImGui::DockBuilderRemoveNodeDockedWindows(Ids.Dockspace, true);
    ImGui::DockBuilderRemoveNode(Ids.Dockspace);
    ImGui::DockBuilderAddNode(Ids.Dockspace, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);

    ImGuiViewport* Viewport = ImGui::GetMainViewport();
    ImGui::DockBuilderSetNodePos(Ids.Dockspace, Viewport->WorkPos);
    ImGui::DockBuilderSetNodeSize(Ids.Dockspace, Viewport->WorkSize);

    ImGui::DockBuilderSplitNode(Ids.Dockspace, ImGuiDir_Right, 0.22f, &Ids.DockRight, &Ids.DockCenter);
    ImGui::DockBuilderSplitNode(Ids.DockCenter, ImGuiDir_Down, 0.28f, &Ids.DockCenterBottom, &Ids.DockCenterTop);
    ImGui::DockBuilderSplitNode(Ids.DockRight, ImGuiDir_Up, 0.55f, &Ids.DockRightTop, &Ids.DockRightBottom);

    // Assign windows to the Dockspace items
    ImGui::DockBuilderDockWindow("Viewport", Ids.DockCenterTop);
    ImGui::DockBuilderDockWindow("Scene Hierarchy", Ids.DockRightTop);
    ImGui::DockBuilderDockWindow("Properties", Ids.DockRightBottom);
    ImGui::DockBuilderDockWindow("Output Log", Ids.DockCenterBottom);
    ImGui::DockBuilderDockWindow("Content Browser", Ids.DockCenterBottom);

    ImGui::DockBuilderFinish(Ids.Dockspace);
}

void FEditorDockspaceWidget::Draw()
{
    ImGuiViewport* MainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(MainViewport->WorkPos);
    ImGui::SetNextWindowSize(MainViewport->WorkSize);
    ImGui::SetNextWindowViewport(MainViewport->ID);

    const ImGuiWindowFlags HostFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

    ImGui::Begin("##DockspaceHost", nullptr, HostFlags);

    ImGui::PopStyleVar(3);

    DrawMenuBar();
    DrawDockSpace();
    DrawFooter();

    ImGui::End();
}

void FEditorDockspaceWidget::DrawMenuBar()
{
    const ImGuiWindowFlags ToolbarFlags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings;

    const ImGuiStyle& Style = ImGui::GetStyle();

    const ImVec4 ToolbarBg    = Style.Colors[ImGuiCol_MenuBarBg];
    const ImVec4 PopupBg      = Style.Colors[ImGuiCol_PopupBg];
    const ImVec4 HoveredColor = ImVec4(87.0f / 255.0f, 87.0f / 255.0f, 87.0f / 255.0f, 1.0f);
    const ImVec4 PressedColor = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ToolbarBg);
    ImGui::PushStyleColor(ImGuiCol_Button, ToolbarBg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, PressedColor);

    if (ImGui::BeginChild("##EditorToolbar", ImVec2(0.0f, EditorStyleVars::MainMenuBarHeight), ImGuiChildFlags_None, ToolbarFlags))
    {
        ImGui::SetCursorPosY(0.0f);

        const CHAR* PopupFile    = "##ToolbarPopup_File";
        const CHAR* PopupEdit    = "##ToolbarPopup_Edit";
        const CHAR* PopupWindows = "##ToolbarPopup_Windows";
        const CHAR* PopupHelp    = "##ToolbarPopup_Help";

        const bool bAnyPopupOpen =
            ImGui::IsPopupOpen(PopupFile, ImGuiPopupFlags_None) ||
            ImGui::IsPopupOpen(PopupEdit, ImGuiPopupFlags_None) ||
            ImGui::IsPopupOpen(PopupWindows, ImGuiPopupFlags_None) ||
            ImGui::IsPopupOpen(PopupHelp, ImGuiPopupFlags_None);

        // File
        {
            PopupAnchor FileAnchor;
            EditorWidgets::MenuButton("File", PopupFile, bAnyPopupOpen, EditorStyleVars::MainMenuBarHeight, FileAnchor);

            if (EditorWidgets::BeginMenuPopup(PopupFile, FileAnchor))
            {
                EditorWidgets::MenuLabeledSeparator("Open");
                EditorWidgets::MenuItem("New Level", "Ctrl+N");
                EditorWidgets::MenuItem("Open Level", "Ctrl+O");
                EditorWidgets::MenuLabeledSeparator("Save");
                EditorWidgets::MenuItem("Save All", "Ctrl+Shift+S");
                EditorWidgets::MenuLabeledSeparator("Exit");
                EditorWidgets::MenuItem("Exit");
                EditorWidgets::EndMenuPopup();
            }
        }

        ImGui::SameLine(0.0f, 0.0f);

        // Edit
        {
            PopupAnchor EditAnchor;
            EditorWidgets::MenuButton("Edit", PopupEdit, bAnyPopupOpen, EditorStyleVars::MainMenuBarHeight, EditAnchor);

            if (EditorWidgets::BeginMenuPopup(PopupEdit, EditAnchor))
            {
                EditorWidgets::MenuLabeledSeparator("Settings");
                EditorWidgets::MenuItem("Project Settings");
                EditorWidgets::MenuItem("Editor Preferences");
                EditorWidgets::EndMenuPopup();
            }
        }

        ImGui::SameLine(0.0f, 0.0f);

        // Windows
        {
            PopupAnchor WindowsAnchor;
            EditorWidgets::MenuButton("Windows", PopupWindows, bAnyPopupOpen, EditorStyleVars::MainMenuBarHeight, WindowsAnchor);

            if (EditorWidgets::BeginMenuPopup(PopupWindows, WindowsAnchor))
            {
                EditorWidgets::MenuLabeledSeparator("Windows");

                if (EditorEngine)
                {
                    if (FEditorOutputLogWidget* LogWidget = EditorEngine->GetOutputLogWidget().Get())
                    {
                        bool bVisible = LogWidget->IsVisible();
                        if (EditorWidgets::MenuItem("Output Log", nullptr, bVisible))
                        {
                            LogWidget->SetVisible(!bVisible);
                        }
                    }
                    else
                    {
                        EditorWidgets::MenuItem("Output Log", nullptr, false, false);
                    }

                    if (FEditorViewportWidget* EditorWidget = EditorEngine->GetEditorViewportWidget().Get())
                    {
                        bool bVisible = EditorWidget->IsVisible();
                        if (EditorWidgets::MenuItem("Viewport", nullptr, bVisible))
                        {
                            EditorWidget->SetVisible(!bVisible);
                        }
                    }
                    else
                    {
                        EditorWidgets::MenuItem("Viewport", nullptr, false, false);
                    }

                    if (FEditorSceneHierarchyWidget* SceneHierarchyWidget = EditorEngine->GetSceneHierarchyWidget().Get())
                    {
                        bool bVisible = SceneHierarchyWidget->IsVisible();
                        if (EditorWidgets::MenuItem("Scene Hierarchy", nullptr, bVisible))
                        {
                            SceneHierarchyWidget->SetVisible(!bVisible);
                        }
                    }
                    else
                    {
                        EditorWidgets::MenuItem("Scene Hierarchy", nullptr, false, false);
                    }

                    if (FEditorPropertiesWidget* PropertiesWidget = EditorEngine->GetPropertiesWidget().Get())
                    {
                        bool bVisible = PropertiesWidget->IsVisible();
                        if (EditorWidgets::MenuItem("Properties", nullptr, bVisible))
                        {
                            PropertiesWidget->SetVisible(!bVisible);
                        }
                    }
                    else
                    {
                        EditorWidgets::MenuItem("Properties", nullptr, false, false);
                    }

                    if (FEditorContentBrowserWidget* ContentBrowserWidget = EditorEngine->GetContentBrowserWidget().Get())
                    {
                        bool bVisible = ContentBrowserWidget->IsVisible();
                        if (EditorWidgets::MenuItem("Content Browser", nullptr, bVisible))
                        {
                            ContentBrowserWidget->SetVisible(!bVisible);
                        }
                    }
                    else
                    {
                        EditorWidgets::MenuItem("Content Browser", nullptr, false, false);
                    }
                }

                EditorWidgets::EndMenuPopup();
            }
        }

        ImGui::SameLine(0.0f, 0.0f);

        // Help
        {
            PopupAnchor HelpAnchor;
            EditorWidgets::MenuButton("Help", PopupHelp, bAnyPopupOpen, EditorStyleVars::MainMenuBarHeight, HelpAnchor);

            if (EditorWidgets::BeginMenuPopup(PopupHelp, HelpAnchor))
            {
                EditorWidgets::MenuLabeledSeparator("About");
                EditorWidgets::MenuItem("About");
                EditorWidgets::EndMenuPopup();
            }
        }
    }

    ImGui::EndChild();

    ImGui::PopStyleColor(4); // ChildBg + Button + ButtonHovered + ButtonActive
    ImGui::PopStyleVar(3);   // WindowPadding + ItemSpacing + FrameRounding
}

void FEditorDockspaceWidget::DrawDockSpace()
{
    const ImGuiWindowFlags WindowFlags = 
        ImGuiWindowFlags_NoBackground | 
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoScrollWithMouse;

    const ImGuiChildFlags ChildWindowFlags = ImGuiChildFlags_None;

    float FooterHeight = 32.0f;
    if (EditorEngine)
    {
        if (FEditorFooterWidget* ConsoleInputWidget = EditorEngine->GetFooterWidget().Get())
        {
            FooterHeight = ConsoleInputWidget->GetHeight();
        }
    }

    const float AvailableHeight = ImGui::GetContentRegionAvail().y;
    const float DockspaceHeight = AvailableHeight - FooterHeight;

    ImGui::BeginChild("##DockspaceArea", ImVec2(0, DockspaceHeight), ChildWindowFlags, WindowFlags);

    const ImGuiDockNodeFlags DockFlags = 
        ImGuiDockNodeFlags_PassthruCentralNode | 
        ImGuiDockNodeFlags_NoWindowMenuButton;

    ImGuiID DockspaceId = ImGui::GetID("Dockspace");
    ImGui::DockSpace(DockspaceId, ImVec2(0, 0), DockFlags);

    ImGui::EndChild();

    // Build default layout once
    ImGuiDockNode* DockNode = ImGui::DockBuilderGetNode(DockspaceId);
    if (!DockNode || (DockNode->IsRootNode() && !DockNode->IsSplitNode() && DockNode->Windows.Size == 0))
    {
        bResetLayout = true;
    }

    if (bResetLayout)
    {
        LayoutIds = {};
        LayoutIds.Dockspace = DockspaceId;
        BuildDockingLayout(LayoutIds);
        bResetLayout = false;
    }
}

void FEditorDockspaceWidget::DrawFooter()
{
    if (EditorEngine)
    {
        if (FEditorFooterWidget* FooterWidget = EditorEngine->GetFooterWidget().Get())
        {
            FooterWidget->Draw();
        }
    }
}
