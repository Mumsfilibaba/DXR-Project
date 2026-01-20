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

static bool GShowContentBrowser = true;

static const float GStatusBarHeight = 22.0f;

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
    Style.ScrollbarRounding    = 4.0f;
    Style.FramePadding         = ImVec2(10.0f, 6.0f);
    Style.ItemSpacing          = ImVec2(8.0f, 6.0f);
    Style.WindowPadding        = ImVec2(10.0f, 10.0f);
    Style.SeparatorTextPadding = ImVec2(6.0f, 6.0f);
    Style.WindowBorderSize     = 0.0f;
    Style.ChildBorderSize      = 0.0f;
    Style.FrameBorderSize      = 0.0f;
    Style.TabBarBorderSize     = 0.0f;
    Style.DockingSeparatorSize = 4.0f;

    Style.Colors[ImGuiCol_WindowBg]           = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    Style.Colors[ImGuiCol_ChildBg]            = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
    Style.Colors[ImGuiCol_PopupBg]            = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    Style.Colors[ImGuiCol_Border]             = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    Style.Colors[ImGuiCol_FrameBg]            = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);
    Style.Colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    Style.Colors[ImGuiCol_FrameBgActive]      = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    Style.Colors[ImGuiCol_Button]             = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    Style.Colors[ImGuiCol_ButtonHovered]      = ImVec4(0.23f, 0.23f, 0.26f, 1.00f);
    Style.Colors[ImGuiCol_ButtonActive]       = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    Style.Colors[ImGuiCol_Header]             = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    Style.Colors[ImGuiCol_HeaderHovered]      = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
    Style.Colors[ImGuiCol_HeaderActive]       = ImVec4(0.26f, 0.26f, 0.30f, 1.00f);
    Style.Colors[ImGuiCol_Tab]                = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    Style.Colors[ImGuiCol_TabHovered]         = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    Style.Colors[ImGuiCol_TabActive]          = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    Style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    Style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    Style.Colors[ImGuiCol_NavHighlight]       = ImVec4(0.37f, 0.37f, 0.80f, 1.00f);
    Style.Colors[ImGuiCol_TitleBg]            = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    Style.Colors[ImGuiCol_TitleBgActive]      = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    Style.Colors[ImGuiCol_MenuBarBg]          = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    Style.Colors[ImGuiCol_Separator]          = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);

    // Docking split line / seam colors
    const ImVec4 SplitterIdle    = ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);
    const ImVec4 SplitterHovered = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
    const ImVec4 SplitterActive  = SplitterHovered;

    Style.Colors[ImGuiCol_Border]            = SplitterIdle;
    Style.Colors[ImGuiCol_BorderShadow]      = SplitterIdle;
    Style.Colors[ImGuiCol_SeparatorHovered]  = SplitterHovered;
    Style.Colors[ImGuiCol_SeparatorActive]   = SplitterActive;
    Style.Colors[ImGuiCol_ResizeGripHovered] = SplitterHovered;
    Style.Colors[ImGuiCol_ResizeGripActive]  = SplitterActive;
    Style.Colors[ImGuiCol_Separator]         = SplitterIdle;
    Style.Colors[ImGuiCol_ResizeGrip]        = SplitterIdle;

    // Load necessary icons
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

    ImVec4 BrightPopupBg = PopupBg;
    BrightPopupBg.x = (BrightPopupBg.x + 0.1f > 1.0f) ? 1.0f : (BrightPopupBg.x + 0.1f);
    BrightPopupBg.y = (BrightPopupBg.y + 0.1f > 1.0f) ? 1.0f : (BrightPopupBg.y + 0.1f);
    BrightPopupBg.z = (BrightPopupBg.z + 0.1f > 1.0f) ? 1.0f : (BrightPopupBg.z + 0.1f);

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

        const char* PopupFile    = "##ToolbarPopup_File";
        const char* PopupEdit    = "##ToolbarPopup_Edit";
        const char* PopupWindows = "##ToolbarPopup_Windows";
        const char* PopupHelp    = "##ToolbarPopup_Help";

        const bool bAnyPopupOpen =
            ImGui::IsPopupOpen(PopupFile, ImGuiPopupFlags_None) ||
            ImGui::IsPopupOpen(PopupEdit, ImGuiPopupFlags_None) ||
            ImGui::IsPopupOpen(PopupWindows, ImGuiPopupFlags_None) ||
            ImGui::IsPopupOpen(PopupHelp, ImGuiPopupFlags_None);

        // File
        {
            PopupAnchor FileAnchor;
            EditorWidgets::EditorDrawMenuButton("File", PopupFile, bAnyPopupOpen, BrightPopupBg, EditorStyleVars::MainMenuBarHeight, FileAnchor);

            if (EditorWidgets::EditorBeginMenuPopup(PopupFile, FileAnchor))
            {
                EditorWidgets::EditorMenuLabeledSeparator("Open");
                EditorWidgets::EditorMenuItem("New Level", "Ctrl+N");
                EditorWidgets::EditorMenuItem("Open Level", "Ctrl+O");
                EditorWidgets::EditorMenuLabeledSeparator("Save");
                EditorWidgets::EditorMenuItem("Save All", "Ctrl+Shift+S");
                EditorWidgets::EditorMenuLabeledSeparator("Exit");
                EditorWidgets::EditorMenuItem("Exit");
                ImGui::EndPopup();
            }

            EditorWidgets::EditorResetMenuPopup();
        }

        ImGui::SameLine(0.0f, 0.0f);

        // Edit
        {
            PopupAnchor EditAnchor;
            EditorWidgets::EditorDrawMenuButton("Edit", PopupEdit, bAnyPopupOpen, BrightPopupBg, EditorStyleVars::MainMenuBarHeight, EditAnchor);

            if (EditorWidgets::EditorBeginMenuPopup(PopupEdit, EditAnchor))
            {
                EditorWidgets::EditorMenuLabeledSeparator("Settings");
                EditorWidgets::EditorMenuItem("Project Settings");
                EditorWidgets::EditorMenuItem("Editor Preferences");
                ImGui::EndPopup();
            }

            EditorWidgets::EditorResetMenuPopup();
        }

        ImGui::SameLine(0.0f, 0.0f);

        // Windows
        {
            PopupAnchor WindowsAnchor;
            EditorWidgets::EditorDrawMenuButton("Windows", PopupWindows, bAnyPopupOpen, BrightPopupBg, EditorStyleVars::MainMenuBarHeight, WindowsAnchor);

            if (EditorWidgets::EditorBeginMenuPopup(PopupWindows, WindowsAnchor))
            {
                EditorWidgets::EditorMenuLabeledSeparator("Windows");

                if (EditorEngine)
                {
                    if (FEditorOutputLogWidget* LogWidget = EditorEngine->GetOutputLogWidget().Get())
                    {
                        bool bVisible = LogWidget->IsVisible();
                        if (EditorWidgets::EditorMenuItem("Output Log", nullptr, bVisible))
                        {
                            LogWidget->SetVisible(!bVisible);
                        }
                    }
                    else
                    {
                        EditorWidgets::EditorMenuItem("Output Log", nullptr, false, false);
                    }

                    if (FEditorViewportWidget* EditorWidget = EditorEngine->GetEditorViewportWidget().Get())
                    {
                        bool bVisible = EditorWidget->IsVisible();
                        if (EditorWidgets::EditorMenuItem("Viewport", nullptr, bVisible))
                        {
                            EditorWidget->SetVisible(!bVisible);
                        }
                    }
                    else
                    {
                        EditorWidgets::EditorMenuItem("Viewport", nullptr, false, false);
                    }

                    if (FEditorSceneHierarchyWidget* SceneHierarchyWidget = EditorEngine->GetSceneHierarchyWidget().Get())
                    {
                        bool bVisible = SceneHierarchyWidget->IsVisible();
                        if (EditorWidgets::EditorMenuItem("Scene Hierarchy", nullptr, bVisible))
                        {
                            SceneHierarchyWidget->SetVisible(!bVisible);
                        }
                    }
                    else
                    {
                        EditorWidgets::EditorMenuItem("Scene Hierarchy", nullptr, false, false);
                    }

                    if (FEditorPropertiesWidget* PropertiesWidget = EditorEngine->GetPropertiesWidget().Get())
                    {
                        bool bVisible = PropertiesWidget->IsVisible();
                        if (EditorWidgets::EditorMenuItem("Properties", nullptr, bVisible))
                        {
                            PropertiesWidget->SetVisible(!bVisible);
                        }
                    }
                    else
                    {
                        EditorWidgets::EditorMenuItem("Properties", nullptr, false, false);
                    }

                    if (FEditorContentBrowserWidget* ContentBrowserWidget = EditorEngine->GetContentBrowserWidget().Get())
                    {
                        bool bVisible = ContentBrowserWidget->IsVisible();
                        if (EditorWidgets::EditorMenuItem("Content Browser", nullptr, bVisible))
                        {
                            ContentBrowserWidget->SetVisible(!bVisible);
                        }
                    }
                    else
                    {
                        EditorWidgets::EditorMenuItem("Content Browser", nullptr, false, false);
                    }
                }

                ImGui::EndPopup();
            }

            EditorWidgets::EditorResetMenuPopup();
        }

        ImGui::SameLine(0.0f, 0.0f);

        // Help
        {
            PopupAnchor HelpAnchor;
            EditorWidgets::EditorDrawMenuButton("Help", PopupHelp, bAnyPopupOpen, BrightPopupBg, EditorStyleVars::MainMenuBarHeight, HelpAnchor);

            if (EditorWidgets::EditorBeginMenuPopup(PopupHelp, HelpAnchor))
            {
                EditorWidgets::EditorMenuLabeledSeparator("About");
                EditorWidgets::EditorMenuItem("About");
                ImGui::EndPopup();
            }

            EditorWidgets::EditorResetMenuPopup();
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
