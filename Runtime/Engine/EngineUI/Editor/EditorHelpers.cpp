#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include <imgui_internal.h>

float EditorStyleVars::MainMenuBarHeight = 28.0f;

ImVec2 EditorStyleVars::InputFieldFramePadding    = ImVec2(12.0f, 6.0f);
float  EditorStyleVars::InputFieldBorderThickness = 2.0f;
float  EditorStyleVars::InputFieldBorderRounding  = 16.0f;
ImU32  EditorStyleVars::InputFieldBorderColor     = IM_COL32(100, 136, 234, 255);

ImVec2 EditorStyleVars::SceneHierarchyItemSpacing    = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::SceneHierarchyWindowPadding  = ImVec2(8.0f, 8.0f);
float  EditorStyleVars::SceneHierarchyTableRowHeight = 32.0f;

ImVec2 EditorStyleVars::PropertiesItemSpacing                 = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::PropertiesWindowPadding               = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::PropertiesCollapsingHeaderItemSpacing = ImVec2(8.0f, 2.0f);
ImVec2 EditorStyleVars::PropertiesCollapsingFramePadding      = ImVec2(10.0f, 8.0f);
float  EditorStyleVars::PropertiesCollapsingFrameRounding     = 2.0f;

static void ApplyHoveredRowBg(bool bRowHovered)
{
	if (!bRowHovered)
	{
		return;
	}

	ImGuiTable* Table = ImGui::GetCurrentTable();
	if (!Table)
	{
		return;
	}

	const ImU32 HoverBg = IM_COL32(47, 47, 47, 255);

	const int32 ColumnCount = ImGui::TableGetColumnCount();
	for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
	{
		ImGui::TableSetColumnIndex(ColumnIndex);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, HoverBg);
	}
}

static bool BeginFullRowHoverCatcher(float RowHeight)
{
	ImGuiTable* Table = ImGui::GetCurrentTable();
	if (!Table)
	{
		return false;
	}

	ImGui::TableSetColumnIndex(0);

	const ImVec2 Cursor = ImGui::GetCursorScreenPos();

	// Invisible item spanning all columns so hover works even over empty cell areas.
	// NOTE: We reset cursor position afterward so the row contents draw on top.
	const ImGuiSelectableFlags HoverFlags =
		ImGuiSelectableFlags_SpanAllColumns |
		ImGuiSelectableFlags_AllowOverlap |
		ImGuiSelectableFlags_Disabled;

	ImGui::Selectable("##RowHover", false, HoverFlags, ImVec2(0.0f, RowHeight));
	const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	ImGui::SetCursorScreenPos(Cursor);
	return bHovered;
}

static void DrawInputBorderLastItem(float Rounding = -1.0f, float Thickness = 2.0f)
{
	ImGuiWindow* Window = ImGui::GetCurrentWindow();
	if (!Window || Window->SkipItems)
	{
		return;
	}

	if (Rounding < 0.0f)
	{
		ImGuiStyle& Style = ImGui::GetStyle();
		Rounding = Style.FrameRounding;
	}

	const bool bActive  = ImGui::IsItemActive();
	const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	const ImU32 ColorIdle   = IM_COL32(54, 54, 54, 255);
	const ImU32 ColorHover  = IM_COL32(84, 84, 84, 255);
	const ImU32 ColorActive = IM_COL32(0, 112, 224, 255);
	const ImU32 Color       = bActive ? ColorActive : (bHovered ? ColorHover : ColorIdle);

	ImVec2 Min = ImGui::GetItemRectMin();
	ImVec2 Max = ImGui::GetItemRectMax();
	Window->DrawList->AddRect(Min, Max, Color, Rounding, 0, Thickness);
}

bool EditorWidgets::ButtonCenteredOnLine(const CHAR* Label, float Alignment)
{
	ImGuiStyle& Style = ImGui::GetStyle();

	const float Size   = ImGui::CalcTextSize(Label).x + Style.FramePadding.x * 2.0f;
	const float Offset = (ImGui::GetContentRegionAvail().x - Size) * Alignment;

	if (Offset > 0.0f)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Offset);
	}

	return ImGui::Button(Label);
}

bool EditorWidgets::DrawFloat3Control(const CHAR* Label, FVector3& OutValue, float ResetValue, float ColumnWidth, float Speed, const FVector3* InRevertValue)
{
	ImGuiTable* Table = ImGui::GetCurrentTable();
	if (!Table)
	{
		return false;
	}

	ImGuiStyle& Style = ImGui::GetStyle();

	const float Gap        = 2.0f;
	const float LineHeight = ImGui::GetFontSize() + Style.FramePadding.y * 2.0f;
	const float RowHeight  = LineHeight;

	ImGui::TableNextRow(0, RowHeight);

	bool bResult     = false;
	bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

	// Label
	ImGui::TableSetColumnIndex(0);
	
	const float LabelIndentPx = 24.0f;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);

	ImGui::AlignTextToFramePadding();
	ImGui::Text("%s", Label);

	// Value (always single row)
	ImGui::TableSetColumnIndex(1);
	ImGui::PushID(Label);

	const float  ButtonWidth = LineHeight * 0.90f;
	const ImVec2 ButtonSize  = ImVec2(ButtonWidth, LineHeight);

	const float Avail        = ImGui::GetContentRegionAvail().x;
	const float TotalButtons = 3.0f * ButtonSize.x;
	const float TotalGaps    = 5.0f * Gap;

	float DragWidth = (Avail - TotalButtons - TotalGaps) / 3.0f;
	DragWidth = ImMax(DragWidth, 1.0f); // clamp instead of stacking

	const auto Axis = [&](const char* AxisLabel, float& OutValue, const ImVec4& Btn, const ImVec4& Hover, const ImVec4& Active, float InDragWidth, bool bSameLine)
	{
		if (bSameLine)
		{
			ImGui::SameLine(0.0f, Gap);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(Gap, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, Btn);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Hover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Active);

		if (ImGui::Button(AxisLabel, ButtonSize))
		{
			OutValue = ResetValue;
			bResult = true;
		}

		ImGui::PopStyleColor(3);

		ImGui::SameLine(0.0f, Gap);
		ImGui::SetNextItemWidth(InDragWidth);

		const char* DragId = (AxisLabel[0] == 'X') ? "##X" : (AxisLabel[0] == 'Y') ? "##Y" : "##Z";
		bResult |= ImGui::DragFloat(DragId, &OutValue, Speed, 0.0f, 0.0f, "%.3f");

		// Per-state border for each component input
		ImGuiStyle& Style = ImGui::GetStyle();
		DrawInputBorderLastItem(Style.FrameRounding);

		bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		bRowHovered |= ImGui::IsItemActive();

		ImGui::PopStyleVar(2);
	};

	Axis("X", OutValue.X, ImVec4(0.8f, 0.1f, 0.15f, 1.0f), ImVec4(0.9f, 0.2f, 0.2f, 1.0f), ImVec4(0.8f, 0.1f, 0.15f, 1.0f), DragWidth, false);
	Axis("Y", OutValue.Y, ImVec4(0.2f, 0.7f, 0.2f, 1.0f), ImVec4(0.3f, 0.8f, 0.3f, 1.0f), ImVec4(0.2f, 0.7f, 0.2f, 1.0f), DragWidth, true);
	Axis("Z", OutValue.Z, ImVec4(0.1f, 0.25f, 0.8f, 1.0f), ImVec4(0.2f, 0.35f, 0.9f, 1.0f), ImVec4(0.1f, 0.25f, 0.8f, 1.0f), DragWidth, true);

	ImGui::PopID();

	// Revert
	ImGui::TableSetColumnIndex(2);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);

	const bool bCanRevert = (InRevertValue != nullptr);
	if (!bCanRevert)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::SmallButton("R"))
	{
		OutValue = *InRevertValue;
		bResult = true;
	}

	bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	if (!bCanRevert)
	{
		ImGui::EndDisabled();
	}

	ApplyHoveredRowBg(bRowHovered);
	return bResult;
}

bool EditorWidgets::DrawFloatProperty(const char* Label, float& InOutValue, float Speed, float MinValue, float MaxValue, const char* Format, bool bUseSlider, const float* InRevertValue, bool bEnabled)
{
	ImGuiTable* Table = ImGui::GetCurrentTable();
	if (!Table)
	{
		return false;
	}

	const float RowHeight = ImGui::GetFrameHeight();
	ImGui::TableNextRow();

	bool bResult = false;
	bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

	// Label
	ImGui::TableSetColumnIndex(0);
	
	const float LabelIndentPx = 24.0f;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(Label);

	// Value
	ImGui::TableSetColumnIndex(1);
	ImGui::PushID(Label);

	if (!bEnabled)
	{
		ImGui::BeginDisabled();
	}

	ImGui::SetNextItemWidth(-FLT_MIN);
	if (bUseSlider)
	{
		bResult = ImGui::SliderFloat("##Value", &InOutValue, MinValue, MaxValue, Format);
	}
	else
	{
		bResult = ImGui::DragFloat("##Value", &InOutValue, Speed, MinValue, MaxValue, Format);
	}

	ImGuiStyle& Style = ImGui::GetStyle();
	DrawInputBorderLastItem(Style.FrameRounding);

	bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	bRowHovered |= ImGui::IsItemActive();

	if (!bEnabled)
	{
		ImGui::EndDisabled();
	}

	ImGui::PopID();

	// Revert
	ImGui::TableSetColumnIndex(2);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);

	const bool bCanRevert = (InRevertValue != nullptr);
	if (!bCanRevert || !bEnabled)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::SmallButton("R"))
	{
		InOutValue = *InRevertValue;
		bResult = true;
	}

	bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	if (!bCanRevert || !bEnabled)
	{
		ImGui::EndDisabled();
	}

	ApplyHoveredRowBg(bRowHovered);
	return bEnabled && bResult;
}

bool EditorWidgets::DrawCheckboxProperty(const char* Label, bool& InOutValue, const bool* InRevertValue, bool bEnabled)
{
	ImGuiTable* Table = ImGui::GetCurrentTable();
	if (!Table)
	{
		return false;
	}

	const float RowHeight = ImGui::GetFrameHeight();

	ImGui::TableNextRow();

	bool bResult     = false;
	bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

	ImGui::TableSetColumnIndex(0);

	const float LabelIndentPx = 24.0f;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(Label);

	ImGui::TableSetColumnIndex(1);
	ImGui::PushID(Label);

	if (!bEnabled)
	{
		ImGui::BeginDisabled();
	}

	bResult = ImGui::Checkbox("##Value", &InOutValue);
	
	ImGuiStyle& Style = ImGui::GetStyle();
	DrawInputBorderLastItem(Style.FrameRounding);

	bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	bRowHovered |= ImGui::IsItemActive();

	if (!bEnabled)
	{
		ImGui::EndDisabled();
	}

	ImGui::PopID();

	ImGui::TableSetColumnIndex(2);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);

	const bool bCanRevert = (InRevertValue != nullptr);
	if (!bCanRevert || !bEnabled)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::SmallButton("R") && bCanRevert)
	{
		InOutValue = *InRevertValue;
		bResult = true;
	}

	bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	if (!bCanRevert || !bEnabled)
	{
		ImGui::EndDisabled();
	}

	ApplyHoveredRowBg(bRowHovered);
	return bEnabled && bResult;
}

bool EditorWidgets::DrawColor3Property(const char* Label, float* InOutColor, const float* InRevertColor, bool bEnabled, ImGuiColorEditFlags Flags)
{
	ImGuiTable* Table = ImGui::GetCurrentTable();
	if (!Table)
	{
		return false;
	}

	const float RowHeight = ImGui::GetFrameHeight();
	ImGui::TableNextRow(0, RowHeight);

	bool bResult     = false;
	bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

	// ------------------------------------------------------------
	// Label
	// ------------------------------------------------------------
	ImGui::TableSetColumnIndex(0);
	{
		// Optional indent (keep/remove depending on your current style)
		const float LabelIndentPx = 24.0f;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(Label);
	}

	// ------------------------------------------------------------
	// Value (ColorButton + 3 DragFloat fields)
	// ------------------------------------------------------------
	ImGui::TableSetColumnIndex(1);
	ImGui::PushID(Label);

	if (!bEnabled)
	{
		ImGui::BeginDisabled();
	}

	ImGuiStyle& Style = ImGui::GetStyle();
	const float Gap         = 4.0f;
	const float FrameHeight = ImGui::GetFrameHeight();

	// Layout widths
	const float Avail       = ImGui::GetContentRegionAvail().x;
	const float ButtonWidth = FrameHeight; // square preview button

	// Gaps: button->R, R->G, G->B  => 3 gaps
	const float TotalGaps = 3.0f * Gap;

	float FieldWidth = (Avail - ButtonWidth - TotalGaps) / 3.0f;
	FieldWidth = ImMax(FieldWidth, 1.0f);

	// Preview button + picker popup
	{
		const ImVec4 Color = ImVec4(InOutColor[0], InOutColor[1], InOutColor[2], 1.0f);

		const ImGuiColorEditFlags ButtonFlags = Flags | ImGuiColorEditFlags_NoTooltip;
		if (ImGui::ColorButton("##ColorBtn", Color, ButtonFlags, ImVec2(ButtonWidth, FrameHeight)))
		{
			ImGui::OpenPopup("##ColorPicker");
		}

		DrawInputBorderLastItem(Style.FrameRounding);

		bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		bRowHovered |= ImGui::IsItemActive();

		if (ImGui::BeginPopup("##ColorPicker"))
		{
			// Keep picker simple
			bResult |= ImGui::ColorPicker3("##Picker", InOutColor, Flags);
			ImGui::EndPopup();
		}
	}

	ImGui::SameLine(0.0f, Gap);

	// R / G / B drag floats (0..1)
	const auto DrawComponentDragFloat = [&](const char* Id, float& OutValue, bool bSameLine)
	{
		if (bSameLine)
		{
			ImGui::SameLine(0.0f, Gap);
		}

		ImGui::SetNextItemWidth(FieldWidth);
		bResult |= ImGui::DragFloat(Id, &OutValue, 0.01f, 0.0f, 1.0f, "%.3f");

		ImGuiStyle& Style = ImGui::GetStyle();
		DrawInputBorderLastItem(Style.FrameRounding);

		bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		bRowHovered |= ImGui::IsItemActive();
	};

	DrawComponentDragFloat("##R", InOutColor[0], false);
	DrawComponentDragFloat("##G", InOutColor[1], true);
	DrawComponentDragFloat("##B", InOutColor[2], true);

	if (!bEnabled)
	{
		ImGui::EndDisabled();
	}

	ImGui::PopID();

	// ------------------------------------------------------------
	// Revert
	// ------------------------------------------------------------
	ImGui::TableSetColumnIndex(2);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);

	const bool bCanRevert = (InRevertColor != nullptr);
	if (!bCanRevert || !bEnabled)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::SmallButton("R") && bCanRevert)
	{
		static constexpr uint64 SizeInBytes = sizeof(float[3]);
		FMemory::Memcpy(InOutColor, InRevertColor, SizeInBytes);
		bResult = true;
	}

	bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	if (!bCanRevert || !bEnabled)
	{
		ImGui::EndDisabled();
	}

	ApplyHoveredRowBg(bRowHovered);
	return bEnabled && bResult;
}

void EditorWidgets::DrawTextProperty(const char* Label, const char* ValueText)
{
	ImGuiTable* Table = ImGui::GetCurrentTable();
	if (!Table)
	{
		ImGui::Text("%s: %s", Label, ValueText ? ValueText : "");
		return;
	}

	const float RowHeight = ImGui::GetFrameHeight();
	ImGui::TableNextRow();

	bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

	ImGui::TableSetColumnIndex(0);

	{
		const float LabelIndentPx = 24.0f;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(Label);
	}

	ImGui::TableSetColumnIndex(1);
	
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(ValueText ? ValueText : "");
	bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	// Column 2: intentionally empty (no reset button for read-only)
	ImGui::TableSetColumnIndex(2);

	ApplyHoveredRowBg(bRowHovered);
}

void EditorWidgets::DrawReadOnlyFloat3Property(const char* Label, const FVector3& Value)
{
	ImGuiTable* Table = ImGui::GetCurrentTable();
	if (!Table)
	{
		ImGui::Text("%s: %.3f %.3f %.3f", Label, Value.X, Value.Y, Value.Z);
		return;
	}

	const float RowHeight = ImGui::GetFrameHeight();
	ImGui::TableNextRow();

	bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

	ImGui::TableSetColumnIndex(0);
	{
		const float LabelIndentPx = 24.0f;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(Label);
	}

	ImGui::TableSetColumnIndex(1);

	float Temp[3] = { Value.X, Value.Y, Value.Z };
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputFloat3("##Value", Temp, "%.3f", ImGuiInputTextFlags_ReadOnly);

	ImGuiStyle& Style = ImGui::GetStyle();
	DrawInputBorderLastItem(Style.FrameRounding);

	bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	// Column 2: intentionally empty (no reset button for read-only)
	ImGui::TableSetColumnIndex(2);

	ApplyHoveredRowBg(bRowHovered);
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

bool EditorWidgets::EditorMenuItem(const char* Label, const char* Shortcut, bool bSelected, bool bEnabled, bool bDrawBorder)
{
	const float PaddingX  = 8.0f;
	const float PaddingY  = 6.0f;
	const float RowHeight = ImGui::GetFontSize() + PaddingY * 2.0f;
	const float RowWidth  = ImGui::GetContentRegionAvail().x;
	const float CheckSize = ImGui::GetFontSize() * 0.85f;
	const float GapRight  = 8.0f; // Gap between shortcut and checkmark (when both exist)
	const float ClipGap   = 4.0f; // Small gap between label clip and right-side content

	ImGui::PushID(Label);

	if (!bEnabled)
	{
		ImGui::BeginDisabled();
	}

	const ImGuiSelectableFlags Flags =
		ImGuiSelectableFlags_SpanAvailWidth |
		ImGuiSelectableFlags_NoPadWithHalfSpacing;

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

	// Hover-only border
	if (bDrawBorder && bHovered)
	{
		// Match the requested menu hover/press blue
		const ImVec4 HoveredColor = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);
		const ImU32  BorderColor  = EditorHelpers::MakeBrighterColorU32(HoveredColor, 0.20f);
		DrawList->AddRect(RectMin, RectMax, BorderColor, 0.0f, 0, 1.0f);
	}

	// Vertical centering reference
	const ImVec2 LabelSize = ImGui::CalcTextSize(Label);
	const float  TextY     = RectMin.y + (RowHeight - LabelSize.y) * 0.5f;

	const bool   bHasShortcut = (Shortcut && Shortcut[0] != '\0');
	const ImVec2 ShortcutSize = bHasShortcut ? ImGui::CalcTextSize(Shortcut) : ImVec2(0.0f, 0.0f);
	const float  RightInnerX  = RectMax.x - PaddingX;

	// Compute right-side layout positions
	bool   bDrawCheckMark = bSelected;
	ImVec2 CheckPos       = ImVec2(0.0f, 0.0f);
	bool   bDrawShortcut  = bHasShortcut;
	float  ShortcutX      = 0.0f;

	// Left-most X of anything on the right (shortcut/check)
	float RightContentMinX = RightInnerX;

	if (bDrawCheckMark)
	{
		const float CheckY    = RectMin.y + (RowHeight - CheckSize) * 0.5f;
		const float CheckMinX = RightInnerX - CheckSize;

		CheckPos         = ImVec2(CheckMinX, CheckY);
		RightContentMinX = CheckMinX;

		if (bDrawShortcut)
		{
			// Shortcut sits to the left of checkmark
			ShortcutX        = CheckMinX - GapRight - ShortcutSize.x;
			RightContentMinX = ShortcutX;
		}
	}
	else if (bDrawShortcut)
	{
		// Shortcut goes flush right when no checkmark
		ShortcutX        = RightInnerX - ShortcutSize.x;
		RightContentMinX = ShortcutX;
	}

	// Draw shortcut (if any)
	if (bDrawShortcut)
	{
		const float ShortcutY = RectMin.y + (RowHeight - ShortcutSize.y) * 0.5f;
		DrawList->AddText(ImVec2(ShortcutX, ShortcutY), ImGui::GetColorU32(ImGuiCol_TextDisabled), Shortcut);
	}

	// Draw checkmark last (crisp on top)
	if (bDrawCheckMark)
	{
		EditorDrawCheckMark(DrawList, CheckPos, ImGui::GetColorU32(ImGuiCol_Text), CheckSize);
	}

	// Draw label + clip to avoid overlap with right-side content
	const float LabelX = RectMin.x + PaddingX;

	float ClipMaxX = RectMax.x - PaddingX;
	if (bDrawShortcut || bDrawCheckMark)
	{
		ClipMaxX = RightContentMinX - ClipGap;
	}

	ClipMaxX = ImMax(ClipMaxX, LabelX + 1.0f);

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

	const ImVec2 Min   = ImGui::GetCursorScreenPos();
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

void EditorWidgets::EditorDrawMenuButton(const char* Label, const char* PopupId, bool bAnyPopupOpen, const ImVec4& BrightPopupBg, float ButtonHeight, PopupAnchor& OutAnchor, bool bDrawBorder)
{
	const bool bThisPopupOpen = ImGui::IsPopupOpen(PopupId, ImGuiPopupFlags_None);
	OutAnchor.bRequestPosition = false;

	if (bThisPopupOpen)
	{
		const ImVec4 PressedBlue = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, PressedBlue);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, PressedBlue);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, PressedBlue);
	}

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 0.0f));
	const bool bPressed = ImGui::Button(Label, ImVec2(0.0f, ButtonHeight));
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

	const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

	OutAnchor.Min = ImGui::GetItemRectMin();
	OutAnchor.Max = ImGui::GetItemRectMax();

	// Hover-only border
	if (bDrawBorder && bHovered)
	{
		const ImU32 BorderColor = ImGui::GetColorU32(ImGuiCol_Border);

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddRect(OutAnchor.Min, OutAnchor.Max, BorderColor, 0.0f, 0, 1.0f);
	}

	if (bThisPopupOpen)
	{
		ImGui::PopStyleColor(3);
	}
}

bool EditorWidgets::EditorBeginMenuPopup(const char* PopupId, const PopupAnchor& Anchor, const ImVec4& BrightPopupBg, float MinWidth)
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

	const ImVec4 MenuHoverBlue = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);

	ImGui::PushStyleColor(ImGuiCol_PopupBg, BrightPopupBg);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, MenuHoverBlue);
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, MenuHoverBlue);

	ImGui::SetNextWindowSizeConstraints(ImVec2(MinWidth, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
	return ImGui::BeginPopup(PopupId);
}

void EditorWidgets::EditorResetMenuPopup()
{
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(5);
}

bool EditorWidgets::BeginPropertyTable(const char* TableId, float LabelColumnWidth, float RevertColumnWidth)
{
	const ImVec4 RowBg       = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
	const ImVec4 TableBorder = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
	const ImVec4 FrameBg     = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6.0f, 4.0f));

	ImGui::PushStyleColor(ImGuiCol_TableRowBg, RowBg);
	ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, RowBg);

	ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, TableBorder);
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight, TableBorder);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, FrameBg);
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, FrameBg);
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, FrameBg);

	const ImGuiTableFlags Flags =
		ImGuiTableFlags_SizingStretchProp |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_NoSavedSettings |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_BordersInnerH |
		ImGuiTableFlags_BordersInnerV;

	// Force table to use full content width (so it matches the collapsing header width)
	const ImVec2 OuterSize(ImGui::GetContentRegionAvail().x, 0.0f);

	if (!ImGui::BeginTable(TableId, 3, Flags, OuterSize))
	{
		ImGui::PopStyleColor(7);
		ImGui::PopStyleVar();
		return false;
	}

	ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_WidthFixed, LabelColumnWidth);
	ImGui::TableSetupColumn("##Value", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("##Revert", ImGuiTableColumnFlags_WidthFixed, RevertColumnWidth);

	return true;
}

void EditorWidgets::EndPropertyTable()
{
	ImGui::EndTable();
	ImGui::PopStyleColor(7);
	ImGui::PopStyleVar();
}

void EditorWidgets::PropertySeparatorRow(float PaddingY)
{
	ImGuiTable* Table = ImGui::GetCurrentTable();
	if (!Table)
	{
		ImGui::Separator();
		return;
	}

	ImGui::TableNextRow();
	const int ColumnCount = ImGui::TableGetColumnCount();
	for (int ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
	{
		ImGui::TableSetColumnIndex(ColumnIndex);
		ImGui::Separator();
	}
}

void EditorWidgets::PropertyRowLabel(const char* Label)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(Label);

	ImGui::TableSetColumnIndex(1);
	ImGui::SetNextItemWidth(-FLT_MIN); // fill available width
}
