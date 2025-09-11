#include "Engine/EngineUI/DockspaceWidget.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include <imgui.h>
#include <imgui_internal.h>

inline void EditorStyle()
{
    ImGuiStyle& Style = ImGui::GetStyle();
    Style.WindowRounding = 6.0f;
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

inline void BuildDockingLayout(const FLayoutIds& Ids)
{
	// Remove previous layout if any, and build afresh
	ImGui::DockBuilderRemoveNode(Ids.Dockspace);
	ImGui::DockBuilderAddNode(Ids.Dockspace, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::DockBuilderSetNodeSize(Ids.Dockspace, ImGui::GetMainViewport()->Size);

	// Start from the root and split
	// Root -> Right (width ~ 22%), Bottom (height ~ 28%), Left (width ~ 16%), Center is remainder
	ImGuiID DockMainId = Ids.Dockspace;

    ImGuiID DockRightId;
    ImGuiID DockMainAfterRight;
	DockRightId = ImGui::DockBuilderSplitNode(DockMainId, ImGuiDir_Right, 0.22f, nullptr, &DockMainAfterRight);

    ImGuiID DockBottomId;
    ImGuiID DockCenterAfterBottom;
	DockBottomId = ImGui::DockBuilderSplitNode(DockMainAfterRight, ImGuiDir_Down, 0.28f, nullptr, &DockCenterAfterBottom);

    ImGuiID DockLeftId;
    ImGuiID DockCenterId;
	DockLeftId = ImGui::DockBuilderSplitNode(DockCenterAfterBottom, ImGuiDir_Left, 0.16f, nullptr, &DockCenterId);

	// Right column: split into top (Outliner) and bottom (Details)
    ImGuiID DockRightTopId;
    ImGuiID DockRightBottomId;
	DockRightTopId = ImGui::DockBuilderSplitNode(DockRightId, ImGuiDir_Up, 0.55f, nullptr, &DockRightBottomId);

	// Assign windows to nodes
	ImGui::DockBuilderDockWindow("Viewport", DockCenterId);
	ImGui::DockBuilderDockWindow("World Outliner", DockRightTopId);
	ImGui::DockBuilderDockWindow("Details", DockRightBottomId);
	ImGui::DockBuilderDockWindow("Content Browser", DockBottomId);
	ImGui::DockBuilderDockWindow("Output Log", DockBottomId);
	ImGui::DockBuilderDockWindow("Console", DockBottomId);
	ImGui::DockBuilderDockWindow("Place Actors", DockLeftId);

	ImGui::DockBuilderFinish(Ids.Dockspace);
}

inline bool BeginDockspace(bool* bOpen, FLayoutIds* OutIds)
{
	ImGuiViewport* MainViewport = ImGui::GetMainViewport();

	// Host window covers the viewport work area (under menu bar), no decoration
	ImGui::SetNextWindowPos(MainViewport->WorkPos);
	ImGui::SetNextWindowSize(MainViewport->WorkSize);
	ImGui::SetNextWindowViewport(MainViewport->ID);

	ImGuiWindowFlags HostFlags = 
		ImGuiWindowFlags_NoDocking | 
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse | 
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus | 
		ImGuiWindowFlags_MenuBar;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	bool bIsOpen = ImGui::Begin("##DockspaceHost", bOpen, HostFlags);
	ImGui::PopStyleVar(2);

	// Menu bar (top)
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Level", "Ctrl+N")) 
			{
			}

			if (ImGui::MenuItem("Open...", "Ctrl+O"))
			{
			}

			ImGui::Separator();
			
			if (ImGui::MenuItem("Save All", "Ctrl+Shift+S"))
			{
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit")) 
			{
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::MenuItem("Undo", "Ctrl+Z");
			ImGui::MenuItem("Redo", "Ctrl+Y");

			ImGui::Separator();
			
			ImGui::MenuItem("Project Settings...");
			ImGui::MenuItem("Editor Preferences...");
			
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Reset Layout"))
			{
				// Force a rebuild next frame by nuking the dock node
				ImGuiID DockspaceId = ImGui::GetID("Dockspace");
				ImGui::DockBuilderRemoveNode(DockspaceId);
			}

			ImGui::Separator();

			ImGui::MenuItem("Viewport", nullptr, true);
			ImGui::MenuItem("World Outliner", nullptr, true);
			ImGui::MenuItem("Details", nullptr, true);
			ImGui::MenuItem("Content Browser", nullptr, true);
			ImGui::MenuItem("Output Log", nullptr, true);
			ImGui::MenuItem("Console", nullptr, true);
			ImGui::MenuItem("Place Actors", nullptr, false);
			
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("About...");
			ImGui::EndMenu();
		}
		
		ImGui::EndMenuBar();
	}

	// Toolbar strip under the menu bar
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 6));

		ImGui::BeginChild("##Toolbar", ImVec2(0, 36), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		
		if (ImGui::Button("Play"))
		{
		}

		ImGui::SameLine();
		
		if (ImGui::Button("Simulate")) 
		{
		}

		ImGui::SameLine();
		
		if (ImGui::Button("Build"))
		{
		}

		ImGui::SameLine();
		
		if (ImGui::Button("Content"))
		{
		}

		ImGui::SameLine();
		
		ImGui::Dummy(ImVec2(16, 0));
		
		ImGui::SameLine();
		
		ImGui::TextUnformatted("|  Platform: Windows  |  Config: Development  |  RHI: D3D12");
		
		ImGui::EndChild();
		
		ImGui::PopStyleVar();
	}

	// Dockspace
	ImGuiID DockspaceId = ImGui::GetID("Dockspace");
	ImGuiDockNodeFlags DockFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoWindowMenuButton;
	ImGui::DockSpace(DockspaceId, ImVec2(0, 0), DockFlags);

	// Build default layout once (if missing)
	ImGuiDockNode* DockNode = ImGui::DockBuilderGetNode(DockspaceId);
	if (!DockNode || (DockNode->IsRootNode() && DockNode->IsSplitNode() == false && DockNode->Windows.Size == 0))
	{
		FLayoutIds Ids{};
		Ids.Dockspace = DockspaceId;
		BuildDockingLayout(Ids);
	}

	if (OutIds)
	{
		OutIds->Dockspace = DockspaceId;
	}

	return bIsOpen;
}

inline void EndDockspace()
{
	// Optional: status bar
	ImGui::Separator();

	ImGui::BeginChild("##StatusBar", ImVec2(0, 22), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	
	ImGui::TextUnformatted("Ready");
	
	ImGui::SameLine();
	
	ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 200.0f);
	
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	
	ImGui::EndChild();

	ImGui::End(); // host
}

static bool gShowContentBrowser = true;
static bool gShowOutputLog      = true;
static bool gShowConsole        = true;
static bool gShowOutliner       = true;
static bool gShowDetails        = true;
static bool gShowViewport       = true;
static bool gShowPlaceActors    = false;

inline void DrawEngineWindows()
{
	if (gShowViewport)
	{
		if (ImGui::Begin("Viewport", &gShowViewport, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			ImVec2 Size = ImGui::GetContentRegionAvail();
			ImGui::Dummy(ImVec2(Size.x * 0.5f, Size.y * 0.5f));

			ImGui::TextDisabled("Viewport goes here (size: %.0f x %.0f)", Size.x, Size.y);
		}

		ImGui::End();
	}

	if (gShowOutliner)
	{
		if (ImGui::Begin("World Outliner", &gShowOutliner))
		{
			ImGui::TextDisabled("Actors");
			
			ImGui::Separator();
			
			if (ImGui::TreeNode("PersistentLevel"))
			{
				ImGui::Selectable("SM_Crate_01");
				ImGui::Selectable("BP_Light_01");
				ImGui::Selectable("BP_PlayerStart");
				ImGui::TreePop();
			}
		}

		ImGui::End();
	}

	if (gShowDetails)
	{
		if (ImGui::Begin("Details", &gShowDetails))
		{
			ImGui::TextDisabled("Details Panel");
			
			ImGui::Separator();
			
			ImGui::TextUnformatted("Name: SM_Crate_01");

			float Location[3] = { 0,0,0 };
			ImGui::InputFloat3("Location", Location);
			
			float Rotation[3] = { 0,0,0 };
			ImGui::InputFloat3("Rotation", Rotation);
			
			float Scale[3] = { 1,1,1 };
			ImGui::InputFloat3("Scale", Scale);
			
			ImGui::SeparatorText("Materials");
			
			ImGui::TextUnformatted("M_Wood_Oak");
		}

		ImGui::End();
	}

	if (gShowContentBrowser)
	{
		if (ImGui::Begin("Content Browser", &gShowContentBrowser))
		{
			ImGui::TextDisabled("Content");
			
			ImGui::Separator();
			
			ImGui::TextUnformatted("Path: /Game");
			
			ImGui::Separator();

			ImGui::Selectable("SM_Crate_01.uasset", false);
			ImGui::Selectable("BP_Door.uasset", false);
			ImGui::Selectable("TX_Wood_D.uasset", false);
		}

		ImGui::End();
	}

	if (gShowOutputLog)
	{
		if (ImGui::Begin("Output Log", &gShowOutputLog))
		{
			ImGui::TextDisabled("[LogTemp] Editor started...");
			ImGui::TextDisabled("[LogBuild] Build succeeded.");
			ImGui::TextDisabled("[LogPIE] PIE session ended.");
		}

		ImGui::End();
	}

	if (gShowConsole)
	{
		if (ImGui::Begin("Console", &gShowConsole))
		{
			static char Cmd[256]{};
			
			ImGui::TextDisabled("Type commands here:");
			ImGui::InputText("##cmd", Cmd, IM_ARRAYSIZE(Cmd));
			
			if (ImGui::Button("Execute"))
			{
			}
			
			ImGui::SameLine();

			if (ImGui::Button("Clear"))
			{
			}
		}

		ImGui::End();
	}

	if (gShowPlaceActors)
	{
		if (ImGui::Begin("Place Actors", &gShowPlaceActors))
		{
			ImGui::TextDisabled("Place Actors");
			
			ImGui::Separator();

			ImGui::Selectable("Empty Actor");
			ImGui::Selectable("Point Light");
			ImGui::Selectable("Camera");
			ImGui::Selectable("Player Start");
		}

		ImGui::End();
	}
}


FDockspaceWidget::FDockspaceWidget()
    : ImGuiDelegateHandle()
	, LayoutIds()
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FDockspaceWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());

        EditorStyle();
    }
}

FDockspaceWidget::~FDockspaceWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FDockspaceWidget::Draw()
{
	bool bOpen = true;
	BeginDockspace(&bOpen, &LayoutIds);

	DrawEngineWindows();
	
	EndDockspace();
}
