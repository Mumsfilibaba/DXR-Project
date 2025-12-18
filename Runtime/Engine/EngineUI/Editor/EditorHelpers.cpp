#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include <imgui_internal.h>

ImVec2 EditorStyleVars::InputFieldFramePadding    = ImVec2(12.0f, 6.0f);
float  EditorStyleVars::InputFieldBorderThickness = 2.0f;
float  EditorStyleVars::InputFieldBorderRounding  = 16.0f;
ImU32  EditorStyleVars::InputFieldBorderColor     = IM_COL32(100, 136, 234, 255);

ImVec2 EditorStyleVars::SceneHierarchyItemSpacing    = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::SceneHierarchyWindowPadding  = ImVec2(8.0f, 8.0f);
float  EditorStyleVars::SceneHierarchyTableRowHeight = 0.0f;

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
