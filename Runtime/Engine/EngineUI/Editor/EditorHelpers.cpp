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
		const ImVec4 HoveredColor = ImVec4(17.0f / 255.0f, 103.0f / 255.0f, 177.0f / 255.0f, 1.0f);
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
			ShortcutX = CheckMinX - GapRight - ShortcutSize.x;
			
			// Shortcut becomes the left-most right-side content
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

	// Avoid invalid clip rect if popup is extremely narrow
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
		ImGui::PushStyleColor(ImGuiCol_Button, BrightPopupBg);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, BrightPopupBg);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, BrightPopupBg);
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

	ImGui::PushStyleColor(ImGuiCol_PopupBg, BrightPopupBg);

	ImGui::SetNextWindowSizeConstraints(ImVec2(MinWidth, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
	return ImGui::BeginPopup(PopupId);
}

void EditorWidgets::EditorResetMenuPopup()
{
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(5);
}
