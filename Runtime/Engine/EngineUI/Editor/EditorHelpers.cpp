#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include <imgui_internal.h>

float EditorStyleVars::MainMenuBarHeight = 28.0f;

ImVec2 EditorStyleVars::InputFieldFramePadding    = ImVec2(12.0f, 6.0f);
float  EditorStyleVars::InputFieldBorderThickness = 2.0f;
float  EditorStyleVars::InputFieldBorderRounding  = 16.0f;
ImU32  EditorStyleVars::InputFieldBorderColor     = IM_COL32(100, 136, 234, 255);

ImVec2 EditorStyleVars::SceneHierarchyItemSpacing    = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::SceneHierarchyWindowPadding  = ImVec2(8.0f, 8.0f);
float  EditorStyleVars::SceneHierarchyTableRowHeight = 24.0f;

ImVec2 EditorStyleVars::PropertiesItemSpacing                 = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::PropertiesWindowPadding               = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::PropertiesCollapsingHeaderItemSpacing = ImVec2(8.0f, 2.0f);
ImVec2 EditorStyleVars::PropertiesCollapsingFramePadding      = ImVec2(10.0f, 8.0f);
float  EditorStyleVars::PropertiesCollapsingFrameRounding     = 2.0f;

bool EditorWidgets::ButtonCenteredOnLine(const CHAR* Label, float Alignment)
{
	ImGuiStyle& Style = ImGui::GetStyle();

	const float Size = ImGui::CalcTextSize(Label).x + Style.FramePadding.x * 2.0f;
	const float Offset = (ImGui::GetContentRegionAvail().x - Size) * Alignment;
	if (Offset > 0.0f)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Offset);
	}

	return ImGui::Button(Label);
}

bool EditorWidgets::DrawFloat3Control(const CHAR* Label, FVector3& OutValue, float ResetValue, float ColumnWidth, float Speed)
{
	bool bResult = false;

	ImGui::PushID(Label);

	ImGui::Columns(2, nullptr, false);

	// Text
	ImGui::SetColumnWidth(0, ColumnWidth);
	
	ImGui::Text("%s", Label);

	ImGui::NextColumn();

	// Drag Floats
	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

	const float  LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	const ImVec2 ButtonSize = ImVec2(LineHeight + 3.0f, LineHeight);

	// X
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));

	if (ImGui::Button("X", ButtonSize))
	{
		OutValue.X = ResetValue;
		bResult    = true;
	}

	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	
	ImGui::DragFloat("##X", &OutValue.X, Speed);
	
	ImGui::PopItemWidth();

	ImGui::SameLine();

	// Y
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));

	if (ImGui::Button("Y", ButtonSize))
	{
		OutValue.Y = ResetValue;
		bResult    = true;
	}

	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	
	ImGui::DragFloat("##Y", &OutValue.Y, Speed);
	
	ImGui::PopItemWidth();

	ImGui::SameLine();

	// Z
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
	
	if (ImGui::Button("Z", ButtonSize))
	{
		OutValue.Z = ResetValue;
		bResult    = true;
	}

	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	ImGui::DragFloat("##Z", &OutValue.Z, Speed);
	
	ImGui::PopItemWidth();

	// Reset
	ImGui::PopStyleVar(2);
	
	ImGui::Columns(1);

	ImGui::PopID();
	return bResult;
}

void EditorWidgets::EditorDrawCheckMark(ImDrawList* DrawList, ImVec2 Position, ImU32 Color, float CheckMarkSize)
{
	const float Thickness = ImMax(1.0f, CheckMarkSize / 6.0f);

	const ImVec2 PointA = ImVec2(Position.x + CheckMarkSize * 0.15f, Position.y + CheckMarkSize * 0.55f);
	const ImVec2 PointB = ImVec2(Position.x + CheckMarkSize * 0.40f, Position.y + CheckMarkSize * 0.80f);
	const ImVec2 PointC = ImVec2(Position.x + CheckMarkSize * 0.85f, Position.y + CheckMarkSize * 0.20f);

	DrawList->AddLine(PointA, PointB, Color, Thickness);
	DrawList->AddLine(PointB, PointC, Color, Thickness);
}

bool EditorWidgets::EditorMenuItem(const char* Label, const char* Shortcut, bool bSelected, bool bEnabled)
{
	const float PaddingX  = 8.0f;
	const float PaddingY  = 6.0f;
	const float RowHeight = ImGui::GetFontSize() + PaddingY * 2.0f;
	const float RowWidth  = ImGui::GetContentRegionAvail().x;
	const float CheckSize = ImGui::GetFontSize() * 0.85f;
	const float GapRight  = 8.0f; // Gap between shortcut and check-mark (if both exist)

	ImGui::PushID(Label);

	if (!bEnabled)
	{
		ImGui::BeginDisabled();
	}

	const ImGuiSelectableFlags Flags = ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_NoPadWithHalfSpacing;

	const bool bPressed = ImGui::Selectable("##row", false, Flags, ImVec2(RowWidth, RowHeight));
	const bool bHovered = ImGui::IsItemHovered();
	const bool bActive  = ImGui::IsItemActive();

	const ImVec2 RectMin = ImGui::GetItemRectMin();
	const ImVec2 RectMax = ImGui::GetItemRectMax();

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// Background
	ImU32 Background = ImGui::GetColorU32(ImGuiCol_Header);
	if (bActive)
	{
		Background = ImGui::GetColorU32(ImGuiCol_HeaderActive);
	}
	else if (bHovered)
	{
		Background = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
	}

	DrawList->AddRectFilled(RectMin, RectMax, Background, 0.0f);

	// Vertical centering reference
	const ImVec2 LabelSize = ImGui::CalcTextSize(Label);
	const float  TextY     = RectMin.y + (RowHeight - LabelSize.y) * 0.5f;

	// Compute right-side layout
	const bool   bHasShortcut = (Shortcut && Shortcut[0] != '\0');
	const ImVec2 ShortCutSize = bHasShortcut ? ImGui::CalcTextSize(Shortcut) : ImVec2(0, 0);

	// Right edge starts from the inside padding
	float RightX = RectMax.x - PaddingX;

	// Checkmark position:
	// - If shortcut exists: draw shortcut first (right-aligned), then check to its right.
	// - If no shortcut: check goes flush to the right padding.

	ImVec2 CheckPos = ImVec2(0, 0);
	if (bSelected)
	{
		const float CheckY = RectMin.y + (RowHeight - CheckSize) * 0.5f;
		if (bHasShortcut)
		{
			// Reserve space: [shortcut][GapRight][check]
			CheckPos = ImVec2(RightX - CheckSize, CheckY);
			
			// Shrink available space for label
			RightX -= (CheckSize + GapRight + ShortCutSize.x); 
		}
		else
		{
			CheckPos = ImVec2(RightX - CheckSize, CheckY);
			
			// Shrink available space for label
			RightX -= CheckSize; 
		}
	}
	else
	{
		// No checkmark. Only shortcut eats the right side.
		if (bHasShortcut)
		{
			RightX -= ShortCutSize.x;
		}
	}

	// Draw shortcut (so it sits left of the checkmark when checked)
	if (bHasShortcut)
	{
		// If checked, shortcut should end at: (checkPos.x - GapRight)
		// If not checked, shortcut ends at: (RectMax.x - PaddingX)
		const float ShortCutRightEdge = bSelected ? (CheckPos.x - GapRight) : (RectMax.x - PaddingX);
		const float ShortCutX = ShortCutRightEdge - ShortCutSize.x;
		const float ShortCutY = RectMin.y + (RowHeight - ShortCutSize.y) * 0.5f;

		DrawList->AddText(ImVec2(ShortCutX, ShortCutY), ImGui::GetColorU32(ImGuiCol_TextDisabled), Shortcut);
	}

	// Draw checkmark last so it’s always crisp on top
	if (bSelected)
	{
		EditorDrawCheckMark(DrawList, CheckPos, ImGui::GetColorU32(ImGuiCol_Text), CheckSize);
	}

	// Draw label (left aligned), but keep it from colliding with right-side stuff
	// Clip label to [left, rightX] so long labels don't overlap shortcut/check.
	const float LabelX = RectMin.x + PaddingX;
	const float ClipMaxX = (bHasShortcut || bSelected) ? ((bSelected && bHasShortcut) ? (CheckPos.x - GapRight - 4.0f) : (RectMax.x - PaddingX - (bHasShortcut ? ShortCutSize.x : 0.0f) - 4.0f))
		: (RectMax.x - PaddingX);

	DrawList->PushClipRect(ImVec2(LabelX, RectMin.y), ImVec2(ClipMaxX, RectMax.y), true);
	DrawList->AddText(ImVec2(LabelX, TextY), ImGui::GetColorU32(ImGuiCol_Text), Label);
	DrawList->PopClipRect();

	if (!bEnabled)
	{
		ImGui::EndDisabled();
	}

	ImGui::PopID();
	return bEnabled && bPressed;
}

void EditorWidgets::EditorMenuSeparator(float Thickness, float PaddingY)
{
	ImGuiWindow* Window = ImGui::GetCurrentWindow();
	if (Window->SkipItems)
	{
		return;
	}

	// Space above
	if (PaddingY > 0.0f)
	{
		ImGui::Dummy(ImVec2(0.0f, PaddingY));
	}

	const ImVec2 Min = ImGui::GetCursorScreenPos();
	const float  Width = ImGui::GetContentRegionAvail().x;

	// Use ImGui's separator color
	const ImU32 Color = ImGui::GetColorU32(ImGuiCol_Separator);

	// Draw line
	ImGui::GetWindowDrawList()->AddLine(Min, ImVec2(Min.x + Width, Min.y), Color, Thickness);

	// Advance cursor by line thickness
	ImGui::Dummy(ImVec2(0.0f, Thickness));

	// Space below
	if (PaddingY > 0.0f)
	{
		ImGui::Dummy(ImVec2(0.0f, PaddingY));
	}
}
