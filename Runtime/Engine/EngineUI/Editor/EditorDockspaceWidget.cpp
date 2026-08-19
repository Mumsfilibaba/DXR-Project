#include "Core/Misc/FrameProfiler.h"
#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorDockspaceWidget.h"
#include "Engine/EngineUI/Editor/EditorTitleBarWidget.h"
#include "Engine/EngineUI/Editor/EditorFooterWidget.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiCore.h"
#include "ImGuiPlugin/ImGuiRenderer.h"

FEditorDockspaceWidget::FEditorDockspaceWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , ImGuiDelegateHandle()
    , LayoutIds()
    , bResetLayout(true)
    , TitleBarWidget(MakeUniquePtr<FEditorTitleBarWidget>(InEditorEngine))
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorDockspaceWidget::Draw));
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
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
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
    // Any stock ImGui button falls back on the same palette the editor's own buttons draw with
    Style.Colors[ImGuiCol_Button]         = ImGui::ColorConvertU32ToFloat4(EditorStyleVars::ButtonBgIdle);
    Style.Colors[ImGuiCol_ButtonHovered]  = ImGui::ColorConvertU32ToFloat4(EditorStyleVars::ButtonBgHovered);
    Style.Colors[ImGuiCol_ButtonActive]   = ImGui::ColorConvertU32ToFloat4(EditorStyleVars::ButtonBgHovered);
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
    Style.Colors[ImGuiCol_ResizeGrip]        = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    Style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    Style.Colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

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

    FEditorTitleBarWidget::RefreshMenuBarHeight(EditorEngine);
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
    Ids.DockLeftTop      = 0;

    ImGui::DockBuilderRemoveNodeDockedWindows(Ids.Dockspace, true);
    ImGui::DockBuilderRemoveNode(Ids.Dockspace);
    ImGui::DockBuilderAddNode(Ids.Dockspace, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);

    ImGuiViewport* Viewport = ImGui::GetMainViewport();
    ImGui::DockBuilderSetNodePos(Ids.Dockspace, Viewport->WorkPos);
    ImGui::DockBuilderSetNodeSize(Ids.Dockspace, Viewport->WorkSize);

    ImGui::DockBuilderSplitNode(Ids.Dockspace, ImGuiDir_Right, 0.22f, &Ids.DockRight, &Ids.DockCenter);
    ImGui::DockBuilderSplitNode(Ids.DockCenter, ImGuiDir_Down, 0.28f, &Ids.DockCenterBottom, &Ids.DockCenterTop);
    ImGui::DockBuilderSplitNode(Ids.DockRight, ImGuiDir_Up, 0.55f, &Ids.DockRightTop, &Ids.DockRightBottom);
    ImGui::DockBuilderSplitNode(Ids.DockCenterTop, ImGuiDir_Left, 0.24f, &Ids.DockLeftTop, &Ids.DockCenterTop);

    // Assign windows to the Dockspace items
    ImGui::DockBuilderDockWindow("Viewport", Ids.DockCenterTop);
    ImGui::DockBuilderDockWindow("Scene Hierarchy", Ids.DockRightTop);
    ImGui::DockBuilderDockWindow("Properties", Ids.DockRightBottom);
    ImGui::DockBuilderDockWindow("Output Log", Ids.DockCenterBottom);
    ImGui::DockBuilderDockWindow("Content Browser", Ids.DockCenterBottom);
    ImGui::DockBuilderDockWindow("Renderer Settings", Ids.DockLeftTop);

    ImGui::DockBuilderFinish(Ids.Dockspace);
}

void FEditorDockspaceWidget::Draw()
{
    TRACE_SCOPE("Dockspace");

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

    DrawTitleBar();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    DrawDockSpace();
    DrawFooter();
    ImGui::PopStyleVar(2);

    ImGui::End();
}

void FEditorDockspaceWidget::DrawTitleBar()
{
    if (TitleBarWidget)
    {
        TitleBarWidget->Draw();
    }
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

    const float SeparatorSize   = ImGui::GetStyle().DockingSeparatorSize;
    const float AvailableHeight = ImGui::GetContentRegionAvail().y;
    const float DockspaceHeight = AvailableHeight - FooterHeight - SeparatorSize;

    ImGui::BeginChild("##DockspaceArea", ImVec2(0, DockspaceHeight), ChildWindowFlags, WindowFlags);

    const ImGuiDockNodeFlags DockFlags = 
        ImGuiDockNodeFlags_PassthruCentralNode | 
        ImGuiDockNodeFlags_NoWindowMenuButton;

    ImGuiID DockspaceId = ImGui::GetID("Dockspace");
    ImGui::DockSpace(DockspaceId, ImVec2(0, 0), DockFlags);

    ImGui::EndChild();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_Separator]);

    ImGui::BeginChild("##DockFooterSeparator", ImVec2(0.0f, SeparatorSize), ImGuiChildFlags_None, 
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs);
    ImGui::EndChild();

    ImGui::PopStyleColor();

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
