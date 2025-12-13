#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorDockspaceWidget.h"
#include "Engine/EngineUI/Editor/EditorConsoleInputFieldWidget.h"
#include "Engine/EngineUI/Editor/EditorLogOutputWidget.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include <imgui.h>
#include <imgui_internal.h>

static bool GShowContentBrowser  = true;
static bool GShowSceneHierarchy  = true;
static bool GShowPropertiesPanel = true;

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
	if (IImguiPlugin::IsEnabled())
	{
		IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
	}

	EditorEngine = nullptr;
}

void FEditorDockspaceWidget::InitializeEditorStyle()
{
    ImGuiStyle& Style = ImGui::GetStyle();
    Style.WindowRounding       = 6.0f;
    Style.FrameRounding        = 4.0f;
    Style.GrabRounding         = 4.0f;
    Style.TabRounding          = 4.0f;
    Style.ScrollbarRounding    = 4.0f;
    Style.FramePadding         = ImVec2(10, 6);
    Style.ItemSpacing          = ImVec2(8, 6);
    Style.WindowPadding        = ImVec2(10, 10);
    Style.SeparatorTextPadding = ImVec2(6, 6);

    Style.Colors[ImGuiCol_WindowBg]           = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
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
}

void FEditorDockspaceWidget::BuildDockingLayout(FLayoutIds& Ids)
{
	Ids.DockRight        = 0;
	Ids.DockRightTop     = 0;
	Ids.DockRightBottom  = 0;
	Ids.DockCenter       = 0;
	Ids.DockCenterTop    = 0;
	Ids.DockCenterBottom = 0;

	// Cleanup previous dockspace and add a new one
	ImGui::DockBuilderRemoveNodeDockedWindows(Ids.Dockspace, true);
	ImGui::DockBuilderRemoveNode(Ids.Dockspace);
	ImGui::DockBuilderAddNode(Ids.Dockspace, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);

	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::DockBuilderSetNodePos(Ids.Dockspace, Viewport->WorkPos);
	ImGui::DockBuilderSetNodeSize(Ids.Dockspace, Viewport->WorkSize);

	ImGui::DockBuilderSplitNode(Ids.Dockspace, ImGuiDir_Right, 0.22f, &Ids.DockRight, &Ids.DockCenter);
	ImGui::DockBuilderSplitNode(Ids.DockCenter, ImGuiDir_Down, 0.28f, &Ids.DockCenterBottom, &Ids.DockCenterTop);
	ImGui::DockBuilderSplitNode(Ids.DockRight, ImGuiDir_Up, 0.55f, &Ids.DockRightTop, &Ids.DockRightBottom);

	// Assign windows to the dockspace items
	ImGui::DockBuilderDockWindow("Viewport", Ids.DockCenterTop);
	ImGui::DockBuilderDockWindow("Scene Hierarchy", Ids.DockRightTop);
	ImGui::DockBuilderDockWindow("Properties Panel", Ids.DockRightBottom);
	ImGui::DockBuilderDockWindow("Output Log", Ids.DockCenterBottom);
	ImGui::DockBuilderDockWindow("Content Browser", Ids.DockCenterBottom);

	ImGui::DockBuilderFinish(Ids.Dockspace);
}

void FEditorDockspaceWidget::Draw()
{
	// Create a window for the viewport
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
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

	// Window for the dockspace
	ImGui::Begin("##DockspaceHost", nullptr, HostFlags);
	
	ImGui::PopStyleVar(3);
	
	// Draw all of the contents
	DrawMenuBar();
	DrawDockSpace();
	DrawConsole();
	DrawEngineWindows();

	ImGui::End(); // Dockspace Host Window
}

void FEditorDockspaceWidget::DrawMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("New Level", "Ctrl+N");
			ImGui::MenuItem("Open", "Ctrl+O");

			ImGui::Separator();

			ImGui::MenuItem("Save All", "Ctrl+Shift+S");

			ImGui::Separator();

			ImGui::MenuItem("Exit");

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::MenuItem("Undo", "Ctrl+Z");
			ImGui::MenuItem("Redo", "Ctrl+Y");

			ImGui::Separator();

			ImGui::MenuItem("Project Settings");
			ImGui::MenuItem("Editor Preferences");

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Windows"))
		{
			if (ImGui::MenuItem("Reset Layout"))
			{
				ImGuiID DockspaceId = ImGui::GetID("Dockspace");
				ImGui::DockBuilderRemoveNode(DockspaceId);
			}

			ImGui::Separator();

			ImGui::MenuItem("World Outliner", nullptr, true);
			ImGui::MenuItem("Details", nullptr, true);
			ImGui::MenuItem("Content Browser", nullptr, true);
			ImGui::MenuItem("Place Actors", nullptr, false);

			// Engine widgets require engine pointer
			if (EditorEngine)
			{
				if (FEditorLogOutputWidget* LogWidget = EditorEngine->GetLogOutputWidget().Get())
				{
					bool bLogVisible = LogWidget->IsVisible();
					if (ImGui::MenuItem("Output Log", nullptr, bLogVisible))
					{
						LogWidget->SetVisible(!bLogVisible);
					}
				}
				else
				{
					ImGui::MenuItem("Output Log", nullptr, false, false);
				}

				if (FEditorViewportWidget* EditorWidget = EditorEngine->GetEditorViewportWidget().Get())
				{
					bool bLogVisible = EditorWidget->IsVisible();
					if (ImGui::MenuItem("Viewport", nullptr, bLogVisible))
					{
						EditorWidget->SetVisible(!bLogVisible);
					}
				}
				else
				{
					ImGui::MenuItem("Viewport", nullptr, false, false);
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("About");
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void FEditorDockspaceWidget::DrawDockSpace()
{
	const ImGuiChildFlags ChildFlags =
		ImGuiWindowFlags_NoBackground | 
		ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoScrollWithMouse;

	float FooterHeight = 32.0f;
	if (EditorEngine)
	{
		if (FEditorConsoleInputFieldWidget* ConsoleInputWidget = EditorEngine->GetConsoleWidget().Get())
		{
			FooterHeight = ConsoleInputWidget->GetHeight();
		}
	}

	const float AvailableHeight = ImGui::GetContentRegionAvail().y;
	const float DockspaceHeight = AvailableHeight - FooterHeight;

	ImGui::BeginChild("##DockspaceArea", ImVec2(0, DockspaceHeight), false, ChildFlags);

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

void FEditorDockspaceWidget::DrawConsole()
{
	if (EditorEngine)
	{
		if (FEditorConsoleInputFieldWidget* ConsoleInputWidget = EditorEngine->GetConsoleWidget().Get())
		{
			ConsoleInputWidget->Draw();
		}
	}
}

void FEditorDockspaceWidget::DrawEngineWindows()
{
	if (GShowSceneHierarchy)
	{
		if (ImGui::Begin("Scene Hierarchy", &GShowSceneHierarchy))
		{
			ImGui::TextDisabled("Actors");

			ImGui::Separator();

			if (ImGui::TreeNode("PersistentLevel"))
			{
				ImGui::Selectable("Crate_01");
				ImGui::Selectable("Light_01");
				ImGui::Selectable("PlayerStart");
				ImGui::TreePop();
			}
		}

		ImGui::End();
	}

	if (GShowPropertiesPanel)
	{
		if (ImGui::Begin("Properties Panel", &GShowPropertiesPanel))
		{
			ImGui::TextDisabled("Properties");

			ImGui::Separator();

			ImGui::TextUnformatted("Name: Crate_01");

			float Location[3] = { 0,0,0 };
			ImGui::InputFloat3("Location", Location);

			float Rotation[3] = { 0,0,0 };
			ImGui::InputFloat3("Rotation", Rotation);

			float Scale[3] = { 1,1,1 };
			ImGui::InputFloat3("Scale", Scale);

			ImGui::SeparatorText("Materials");

			ImGui::TextUnformatted("Wood_Oak");
		}

		ImGui::End();
	}

	if (GShowContentBrowser)
	{
		if (ImGui::Begin("Content Browser", &GShowContentBrowser))
		{
			ImGui::TextDisabled("Content");

			ImGui::Separator();

			ImGui::TextUnformatted("Path: /Game");

			ImGui::Separator();

			ImGui::Selectable("Crate_01.asset", false);
			ImGui::Selectable("Door.asset", false);
			ImGui::Selectable("Wood.asset", false);
		}

		ImGui::End();
	}
}
