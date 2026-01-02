#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorDockspaceWidget.h"
#include "Engine/EngineUI/Editor/EditorConsoleInputFieldWidget.h"
#include "Engine/EngineUI/Editor/EditorLogOutputWidget.h"
#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
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
    Style.FramePadding         = ImVec2(10.0f, 6.0f);
    Style.ItemSpacing          = ImVec2(8.0f, 6.0f);
    Style.WindowPadding        = ImVec2(10.0f, 10.0f);
    Style.SeparatorTextPadding = ImVec2(6.0f, 6.0f);
	Style.WindowBorderSize     = 0.0f;
	Style.ChildBorderSize      = 0.0f;
	Style.FrameBorderSize      = 0.0f;
	Style.TabBarBorderSize     = 0.0f;

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
		//ImGuiWindowFlags_MenuBar |
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

#if 0 
void FEditorDockspaceWidget::DrawMenuBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f)); // affects item height
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 8.0f));  // spacing between items
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f)); // padding inside the bar

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
			// TODO: Add a undo/redo system
			//ImGui::MenuItem("Undo", "Ctrl+Z");
			//ImGui::MenuItem("Redo", "Ctrl+Y");

			//ImGui::Separator();

			ImGui::MenuItem("Project Settings");
			ImGui::MenuItem("Editor Preferences");

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Windows"))
		{
			// TODO: Add this to the Editor Preferences
			//if (ImGui::MenuItem("Reset Layout"))
			//{
			//	ImGuiID DockspaceId = ImGui::GetID("Dockspace");
			//	ImGui::DockBuilderRemoveNode(DockspaceId);
			//}

			//ImGui::Separator();

			ImGui::MenuItem("Properties", nullptr, true);
			ImGui::MenuItem("Content Browser", nullptr, true);

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

				if (FEditorSceneHierarchyWidget* SceneHierarchyWidget = EditorEngine->GetSceneHierarchyWidget().Get())
				{
					bool bLogVisible = SceneHierarchyWidget->IsVisible();
					if (ImGui::MenuItem("Scene Hierarchy", nullptr, bLogVisible))
					{
						SceneHierarchyWidget->SetVisible(!bLogVisible);
					}
				}
				else
				{
					ImGui::MenuItem("Scene Hierarchy", nullptr, false, false);
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

	ImGui::PopStyleVar(3);
}
#endif

#if 0
static bool StylizedMenuItem(const char* Label, const char* Shortcut = nullptr, bool bSelected = false, bool bEnabled = true)
{
	const ImGuiStyle& S = ImGui::GetStyle();

	const float PadX = 8.0f;
	const float PadY = 6.0f;

	// Explicit row height (independent of FramePadding)
	const float RowH = ImGui::GetFontSize() + PadY * 2.0f;

	// Explicit row width (span full popup work width)
	const float RowW = ImGui::GetContentRegionAvail().x;

	// Use an ID that won't collide if labels repeat
	ImGui::PushID(Label);

	if (!bEnabled) ImGui::BeginDisabled();

	// Create the full-width, full-height hit box
	const ImGuiSelectableFlags Flags =
		ImGuiSelectableFlags_SpanAvailWidth |
		ImGuiSelectableFlags_NoPadWithHalfSpacing;

	bool pressed = ImGui::Selectable("##row", bSelected, Flags, ImVec2(RowW, RowH));

	const bool hovered = ImGui::IsItemHovered();
	const bool active = ImGui::IsItemActive();

	// Row rect (screen space)
	const ImVec2 rMin = ImGui::GetItemRectMin();
	const ImVec2 rMax = ImGui::GetItemRectMax();

	// Draw background using Header colors (Selectable uses these semantics)
	ImU32 bg = ImGui::GetColorU32(ImGuiCol_Header);
	if (active)  bg = ImGui::GetColorU32(ImGuiCol_HeaderActive);
	else if (hovered) bg = ImGui::GetColorU32(ImGuiCol_HeaderHovered);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddRectFilled(rMin, rMax, bg, 0.0f);

	// Draw label text with padding
	const ImVec2 labelSize = ImGui::CalcTextSize(Label);
	const float textY = rMin.y + (RowH - labelSize.y) * 0.5f;
	dl->AddText(ImVec2(rMin.x + PadX, textY), ImGui::GetColorU32(ImGuiCol_Text), Label);

	// Draw shortcut right-aligned (optional)
	if (Shortcut && Shortcut[0] != '\0')
	{
		const ImVec2 scSize = ImGui::CalcTextSize(Shortcut);
		dl->AddText(ImVec2(rMax.x - PadX - scSize.x, textY),
			ImGui::GetColorU32(ImGuiCol_TextDisabled),
			Shortcut);
	}

	if (!bEnabled) ImGui::EndDisabled();

	ImGui::PopID();
	return bEnabled && pressed;
}
#endif

#if 0
static void DrawCheckMark(ImDrawList* DL, ImVec2 Pos, ImU32 Col, float Sz)
{
	// Simple check-mark using 2 lines (looks very close to ImGui’s menu checkmark)
	// Pos is top-left of the check mark box.
	const float thickness = ImMax(1.0f, Sz / 6.0f);

	const ImVec2 a(Pos.x + Sz * 0.15f, Pos.y + Sz * 0.55f);
	const ImVec2 b(Pos.x + Sz * 0.40f, Pos.y + Sz * 0.80f);
	const ImVec2 c(Pos.x + Sz * 0.85f, Pos.y + Sz * 0.20f);

	DL->AddLine(a, b, Col, thickness);
	DL->AddLine(b, c, Col, thickness);
}

static bool StylizedMenuItem(const char* Label, const char* Shortcut = nullptr, bool bSelected = false, bool bEnabled = true)
{
	const float PadX = 8.0f;
	const float PadY = 6.0f;

	const float RowH = ImGui::GetFontSize() + PadY * 2.0f;
	const float RowW = ImGui::GetContentRegionAvail().x;

	// Reserve a "gutter" on the left for checkmark (always), so labels align
	const float CheckBoxSize = ImGui::GetFontSize() * 0.85f;      // size of check-box
	const float CheckGutter = CheckBoxSize + PadX;               // gutter width

	ImGui::PushID(Label);

	if (!bEnabled) ImGui::BeginDisabled();

	const ImGuiSelectableFlags Flags =
		ImGuiSelectableFlags_SpanAvailWidth |
		ImGuiSelectableFlags_NoPadWithHalfSpacing;

	bool pressed = ImGui::Selectable("##row", false, Flags, ImVec2(RowW, RowH));

	const bool hovered = ImGui::IsItemHovered();
	const bool active = ImGui::IsItemActive();

	const ImVec2 rMin = ImGui::GetItemRectMin();
	const ImVec2 rMax = ImGui::GetItemRectMax();

	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Background
	ImU32 bg = ImGui::GetColorU32(ImGuiCol_Header);
	if (active)       bg = ImGui::GetColorU32(ImGuiCol_HeaderActive);
	else if (hovered) bg = ImGui::GetColorU32(ImGuiCol_HeaderHovered);

	dl->AddRectFilled(rMin, rMax, bg, 0.0f);

	// Text baseline (center vertically)
	const ImVec2 labelSize = ImGui::CalcTextSize(Label);
	const float textY = rMin.y + (RowH - labelSize.y) * 0.5f;

	// Checkmark (if selected)
	if (bSelected)
	{
		const float checkY = rMin.y + (RowH - CheckBoxSize) * 0.5f;
		const ImVec2 checkPos(rMin.x + PadX, checkY);
		DrawCheckMark(dl, checkPos, ImGui::GetColorU32(ImGuiCol_Text), CheckBoxSize);
	}

	// Label text (shifted by gutter so it aligns whether checked or not)
	dl->AddText(ImVec2(rMin.x + CheckGutter, textY),
		ImGui::GetColorU32(ImGuiCol_Text),
		Label);

	// Shortcut (right aligned)
	if (Shortcut && Shortcut[0] != '\0')
	{
		const ImVec2 scSize = ImGui::CalcTextSize(Shortcut);
		dl->AddText(ImVec2(rMax.x - PadX - scSize.x, textY),
			ImGui::GetColorU32(ImGuiCol_TextDisabled),
			Shortcut);
	}

	if (!bEnabled) ImGui::EndDisabled();

	ImGui::PopID();
	return bEnabled && pressed;
}
#endif

static void DrawCheckMark(ImDrawList* DL, ImVec2 Pos, ImU32 Col, float Sz)
{
	const float thickness = ImMax(1.0f, Sz / 6.0f);

	const ImVec2 a(Pos.x + Sz * 0.15f, Pos.y + Sz * 0.55f);
	const ImVec2 b(Pos.x + Sz * 0.40f, Pos.y + Sz * 0.80f);
	const ImVec2 c(Pos.x + Sz * 0.85f, Pos.y + Sz * 0.20f);

	DL->AddLine(a, b, Col, thickness);
	DL->AddLine(b, c, Col, thickness);
}

static bool StylizedMenuItem(const char* Label, const char* Shortcut = nullptr, bool bSelected = false, bool bEnabled = true)
{
	const float PadX = 8.0f;
	const float PadY = 6.0f;

	const float RowH = ImGui::GetFontSize() + PadY * 2.0f;
	const float RowW = ImGui::GetContentRegionAvail().x;

	const float CheckSize = ImGui::GetFontSize() * 0.85f;
	const float GapRight = 8.0f; // gap between shortcut and checkmark (if both exist)

	ImGui::PushID(Label);

	if (!bEnabled) ImGui::BeginDisabled();

	const ImGuiSelectableFlags Flags =
		ImGuiSelectableFlags_SpanAvailWidth |
		ImGuiSelectableFlags_NoPadWithHalfSpacing;

	const bool pressed = ImGui::Selectable("##row", false, Flags, ImVec2(RowW, RowH));
	const bool hovered = ImGui::IsItemHovered();
	const bool active = ImGui::IsItemActive();

	const ImVec2 rMin = ImGui::GetItemRectMin();
	const ImVec2 rMax = ImGui::GetItemRectMax();

	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Background
	ImU32 bg = ImGui::GetColorU32(ImGuiCol_Header);
	if (active)       bg = ImGui::GetColorU32(ImGuiCol_HeaderActive);
	else if (hovered) bg = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
	dl->AddRectFilled(rMin, rMax, bg, 0.0f);

	// Vertical centering reference
	const ImVec2 labelSize = ImGui::CalcTextSize(Label);
	const float textY = rMin.y + (RowH - labelSize.y) * 0.5f;

	// Compute right-side layout
	const bool hasShortcut = (Shortcut && Shortcut[0] != '\0');
	const ImVec2 scSize = hasShortcut ? ImGui::CalcTextSize(Shortcut) : ImVec2(0, 0);

	// Right edge starts from the inside padding
	float rightX = rMax.x - PadX;

	// Checkmark position:
	// - If shortcut exists: draw shortcut first (right-aligned), then check to its right.
	// - If no shortcut: check goes flush to the right padding.
	ImVec2 checkPos(0, 0);
	if (bSelected)
	{
		const float checkY = rMin.y + (RowH - CheckSize) * 0.5f;
		if (hasShortcut)
		{
			// reserve space: [shortcut][GapRight][check]
			checkPos = ImVec2(rightX - CheckSize, checkY);
			rightX -= (CheckSize + GapRight + scSize.x); // shrink available space for label
		}
		else
		{
			checkPos = ImVec2(rightX - CheckSize, checkY);
			rightX -= CheckSize; // shrink available space for label
		}
	}
	else
	{
		// no checkmark: only shortcut eats the right side
		if (hasShortcut)
			rightX -= scSize.x;
	}

	// Draw shortcut (so it sits left of the checkmark when checked)
	if (hasShortcut)
	{
		// If checked, shortcut should end at: (checkPos.x - GapRight)
		// If not checked, shortcut ends at: (rMax.x - PadX)
		const float scRightEdge = bSelected ? (checkPos.x - GapRight) : (rMax.x - PadX);
		const float scX = scRightEdge - scSize.x;
		const float scY = rMin.y + (RowH - scSize.y) * 0.5f;

		dl->AddText(ImVec2(scX, scY),
			ImGui::GetColorU32(ImGuiCol_TextDisabled),
			Shortcut);
	}

	// Draw checkmark last so it’s always crisp on top
	if (bSelected)
	{
		DrawCheckMark(dl, checkPos, ImGui::GetColorU32(ImGuiCol_Text), CheckSize);
	}

	// Draw label (left aligned), but keep it from colliding with right-side stuff
	// Clip label to [left, rightX] so long labels don't overlap shortcut/check.
	const float labelX = rMin.x + PadX;
	const float clipMaxX = (hasShortcut || bSelected) ? ((bSelected && hasShortcut) ? (checkPos.x - GapRight - 4.0f) : (rMax.x - PadX - (hasShortcut ? scSize.x : 0.0f) - 4.0f))
		: (rMax.x - PadX);

	dl->PushClipRect(ImVec2(labelX, rMin.y), ImVec2(clipMaxX, rMax.y), true);
	dl->AddText(ImVec2(labelX, textY),
		ImGui::GetColorU32(ImGuiCol_Text),
		Label);
	dl->PopClipRect();

	if (!bEnabled) ImGui::EndDisabled();

	ImGui::PopID();
	return bEnabled && pressed;
}


static void StylizedMenuSeparator(float Thickness = 1.0f, float PaddingY = 0.0f)
{
	ImGuiWindow* Window = ImGui::GetCurrentWindow();
	if (Window->SkipItems)
		return;

	// Space above
	if (PaddingY > 0.0f)
		ImGui::Dummy(ImVec2(0.0f, PaddingY));

	const ImVec2 Min = ImGui::GetCursorScreenPos();
	const float W = ImGui::GetContentRegionAvail().x;

	// Use ImGui's separator color
	const ImU32 Col = ImGui::GetColorU32(ImGuiCol_Separator);

	// Draw line
	ImGui::GetWindowDrawList()->AddLine(
		Min,
		ImVec2(Min.x + W, Min.y),
		Col,
		Thickness
	);

	// Advance cursor by line thickness
	ImGui::Dummy(ImVec2(0.0f, Thickness));

	// Space below
	if (PaddingY > 0.0f)
		ImGui::Dummy(ImVec2(0.0f, PaddingY));
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
	const ImVec4 HoveredColor = ImVec4(17.0f / 255.0f, 103.0f / 255.0f, 177.0f / 255.0f, 1.0f);

	ImVec4 BrightPopupBg = PopupBg;
	BrightPopupBg.x = (BrightPopupBg.x + 0.1f > 1.0f) ? 1.0f : (BrightPopupBg.x + 0.1f);
	BrightPopupBg.y = (BrightPopupBg.y + 0.1f > 1.0f) ? 1.0f : (BrightPopupBg.y + 0.1f);
	BrightPopupBg.z = (BrightPopupBg.z + 0.1f > 1.0f) ? 1.0f : (BrightPopupBg.z + 0.1f);

	struct PopupAnchor
	{
		ImVec2 Min = ImVec2(0.0f, 0.0f);
		ImVec2 Max = ImVec2(0.0f, 0.0f);
		bool   bRequestPosition = false;
	};

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ToolbarBg);
	ImGui::PushStyleColor(ImGuiCol_Button, ToolbarBg);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HoveredColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, PopupBg);

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

		const auto DrawMenuButton = [&](const char* Label, const char* PopupId, PopupAnchor& OutAnchor)
		{
			const bool bThisPopupOpen = ImGui::IsPopupOpen(PopupId, ImGuiPopupFlags_None);
			OutAnchor.bRequestPosition = false;

			if (bThisPopupOpen)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, BrightPopupBg);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, BrightPopupBg);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, BrightPopupBg);
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 0.0f));
			bool bPressed = ImGui::Button(Label, ImVec2(0.0f, EditorStyleVars::MainMenuBarHeight));
			ImGui::PopStyleVar();

			if (bPressed)
			{
				ImGui::OpenPopup(PopupId);
				OutAnchor.bRequestPosition = true;
			}

			if (bAnyPopupOpen && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && !bThisPopupOpen)
			{
				ImGui::OpenPopup(PopupId);
				OutAnchor.bRequestPosition = true;
			}

			OutAnchor.Min = ImGui::GetItemRectMin();
			OutAnchor.Max = ImGui::GetItemRectMax();

			if (bThisPopupOpen)
			{
				ImGui::PopStyleColor(3);
			}
		};

		const auto BeginMenuPopup = [&](const char* PopupId, const PopupAnchor& Anchor) -> bool
		{
			if (Anchor.bRequestPosition || ImGui::IsPopupOpen(PopupId, ImGuiPopupFlags_None))
			{
				ImGui::SetNextWindowPos(ImVec2(Anchor.Min.x, Anchor.Max.y), ImGuiCond_Always);
			}

			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

			ImGui::PushStyleColor(ImGuiCol_PopupBg, BrightPopupBg);

			ImGui::SetNextWindowSizeConstraints(ImVec2(160.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

			return ImGui::BeginPopup(PopupId);
		};

		const auto ResetMenuPopup = []()
		{
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(5);
		};

		//const auto StylizedMenuItem = [](const char* Label, const char* Shortcut = nullptr, bool bSelected = false, bool bEnabled = true)
		//{
		//	//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 10.0f));
		//	//ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 10.0f));

		//	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 12.0f));

		//	//const bool bResult = ImGui::MenuItem(Label, Shortcut, bSelected, bEnabled);

		//	const ImGuiSelectableFlags Flags =
		//		ImGuiSelectableFlags_SpanAvailWidth |
		//		ImGuiSelectableFlags_NoPadWithHalfSpacing;

		//	if (!bEnabled)
		//	{
		//		ImGui::BeginDisabled();
		//	}

		//	const bool bResult = ImGui::Selectable(Label, bSelected, Flags);
		//	
		//	if (!bEnabled)
		//	{
		//		ImGui::EndDisabled();
		//	}

		//	ImGui::PopStyleVar(1);
		//	return bResult;
		//};

		// File
		{
			PopupAnchor FileAnchor;
			DrawMenuButton("File", PopupFile, FileAnchor);

			if (BeginMenuPopup(PopupFile, FileAnchor))
			{
				StylizedMenuItem("New Level", "Ctrl+N");
				StylizedMenuItem("Open", "Ctrl+O");
				StylizedMenuSeparator();
				StylizedMenuItem("Save All", "Ctrl+Shift+S");
				StylizedMenuSeparator();
				StylizedMenuItem("Exit");
				ImGui::EndPopup();
			}
				
			ResetMenuPopup();
		}

		ImGui::SameLine(0.0f, 0.0f);

		// Edit
		{
			PopupAnchor EditAnchor;
			DrawMenuButton("Edit", PopupEdit, EditAnchor);

			if (BeginMenuPopup(PopupEdit, EditAnchor))
			{
				StylizedMenuItem("Project Settings");
				StylizedMenuItem("Editor Preferences");

				ImGui::EndPopup();
			}

			ResetMenuPopup();
		}

		ImGui::SameLine(0.0f, 0.0f);

		// Windows
		{
			PopupAnchor WindowsAnchor;
			DrawMenuButton("Windows", PopupWindows, WindowsAnchor);

			if (BeginMenuPopup(PopupWindows, WindowsAnchor))
			{
				{
					bool bProps = GShowPropertiesPanel;
					if (StylizedMenuItem("Properties", nullptr, bProps))
					{
						GShowPropertiesPanel = !bProps;
					}
				}

				{
					bool bContent = GShowContentBrowser;
					if (StylizedMenuItem("Content Browser", nullptr, bContent))
					{
						GShowContentBrowser = !bContent;
					}
				}

				if (EditorEngine)
				{
					if (FEditorLogOutputWidget* LogWidget = EditorEngine->GetLogOutputWidget().Get())
					{
						bool bVisible = LogWidget->IsVisible();
						if (StylizedMenuItem("Output Log", nullptr, bVisible))
						{
							LogWidget->SetVisible(!bVisible);
						}
					}
					else
					{
						StylizedMenuItem("Output Log", nullptr, false, false);
					}

					if (FEditorViewportWidget* EditorWidget = EditorEngine->GetEditorViewportWidget().Get())
					{
						bool bVisible = EditorWidget->IsVisible();
						if (StylizedMenuItem("Viewport", nullptr, bVisible))
						{
							EditorWidget->SetVisible(!bVisible);
						}
					}
					else
					{
						StylizedMenuItem("Viewport", nullptr, false, false);
					}

					if (FEditorSceneHierarchyWidget* SceneHierarchyWidget = EditorEngine->GetSceneHierarchyWidget().Get())
					{
						bool bVisible = SceneHierarchyWidget->IsVisible();
						if (StylizedMenuItem("Scene Hierarchy", nullptr, bVisible))
						{
							SceneHierarchyWidget->SetVisible(!bVisible);
						}
					}
					else
					{
						StylizedMenuItem("Scene Hierarchy", nullptr, false, false);
					}
				}

				ImGui::EndPopup();
			}

			ResetMenuPopup();
		}

		ImGui::SameLine(0.0f, 0.0f);

		// Help
		{
			PopupAnchor HelpAnchor;
			DrawMenuButton("Help", PopupHelp, HelpAnchor);

			if (BeginMenuPopup(PopupHelp, HelpAnchor))
			{
				StylizedMenuItem("About");
				ImGui::EndPopup();
			}

			ResetMenuPopup();
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
		if (FEditorConsoleInputFieldWidget* ConsoleInputWidget = EditorEngine->GetConsoleWidget().Get())
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
