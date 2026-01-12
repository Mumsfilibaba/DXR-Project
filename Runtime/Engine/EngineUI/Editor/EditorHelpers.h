#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Color.h"
#include "Core/Containers/UniquePtr.h"
#include <imgui.h>

struct PopupAnchor
{
	ImVec2 Min              = ImVec2(0.0f, 0.0f);
	ImVec2 Max              = ImVec2(0.0f, 0.0f);
	bool   bRequestPosition = false;
};

struct EditorStyleVars
{
	static float MainMenuBarHeight;

    static ImVec2 InputFieldFramePadding;
	static float  InputFieldBorderThickness;
	static float  InputFieldBorderRounding;
	static ImU32  InputFieldBorderColor;
    
	static ImVec2 SceneHierarchyItemSpacing;
	static ImVec2 SceneHierarchyWindowPadding;
	static float  SceneHierarchyTableRowHeight;

	static ImVec2 PropertiesItemSpacing;
	static ImVec2 PropertiesWindowPadding;
	static ImVec2 PropertiesCollapsingHeaderItemSpacing;
	static ImVec2 PropertiesCollapsingFramePadding;
	static float  PropertiesCollapsingFrameRounding;
};

struct ENGINE_API EditorHelpers
{
	static FORCEINLINE ImU32 MakeBrighterColorU32(const ImVec4& HoveredColor, float BrightenAmount = 0.20f)
	{
		ImVec4 BrighterColor = HoveredColor;
		BrighterColor.x = (BrighterColor.x + BrightenAmount > 1.0f) ? 1.0f : (BrighterColor.x + BrightenAmount);
		BrighterColor.y = (BrighterColor.y + BrightenAmount > 1.0f) ? 1.0f : (BrighterColor.y + BrightenAmount);
		BrighterColor.z = (BrighterColor.z + BrightenAmount > 1.0f) ? 1.0f : (BrighterColor.z + BrightenAmount);
		BrighterColor.w = 1.0f;
		return ImGui::GetColorU32(BrighterColor);
	}
};

struct ENGINE_API EditorWidgets
{
	static bool ButtonCenteredOnLine(const CHAR* Label, float Alignment = 0.5f);

	static bool DrawFloat3Control(const CHAR* Label, FVector3& OutValue, float ResetValue = 0.0f, float ColumnWidth = 100.0f, float Speed = 0.01f, const FVector3* InRevertValue = nullptr);
	static bool DrawFloatProperty(const char* Label, float& InOutValue, float Speed, float MinValue, float MaxValue, const char* Format, bool bUseSlider, const float* InRevertValue, bool bEnabled = true);
	static bool DrawCheckboxProperty(const char* Label, bool& InOutValue, const bool* InRevertValue, bool bEnabled = true);
	static void DrawTextProperty(const char* Label, const char* ValueText);
	static void DrawReadOnlyFloat3Property(const char* Label, const FVector3& Value);
	static bool DrawColor3Property(const char* Label, float* InOutColor, const float* InRevertColor, bool bEnabled, ImGuiColorEditFlags Flags);
 
	static FORCEINLINE bool DrawColor3Property(const char* Label, FFloatColor& InOutColor, const FFloatColor& InRevertColor, bool bEnabled = true, ImGuiColorEditFlags Flags = ImGuiColorEditFlags_None)
	{
		return DrawColor3Property(Label, InOutColor.RGBA, InRevertColor.RGBA, bEnabled, Flags);
	}

	static FORCEINLINE bool DrawColor3Property(const char* Label, FVector3& InOutColor, const FVector3& InRevertColor, bool bEnabled = true, ImGuiColorEditFlags Flags = ImGuiColorEditFlags_None)
	{
		return DrawColor3Property(Label, InOutColor.XYZ, InRevertColor.XYZ, bEnabled, Flags);
	}

	static FORCEINLINE bool DrawColorEdit3(const CHAR* Label, FVector3& OutColor, ImGuiColorEditFlags Flags = 0)
	{
		return ImGui::ColorEdit3(Label, OutColor.XYZ, Flags);
	}

	static FORCEINLINE bool DrawColorEdit3(const CHAR* Label, FFloatColor& OutColor, ImGuiColorEditFlags Flags = 0)
	{
		return ImGui::ColorEdit3(Label, OutColor.RGBA, Flags);
	}

	static void EditorDrawCheckMark(ImDrawList* DrawList, ImVec2 Position, ImU32 Color, float CheckMarkSize);
	static void EditorMenuSeparator(float Thickness = 1.0f, float PaddingY = 0.0f);
	static bool EditorMenuItem(const char* Label, const char* Shortcut = nullptr, bool bSelected = false, bool bEnabled = true, bool bDrawBorder = false);
	static void EditorDrawMenuButton(const char* Label, const char* PopupId, bool bAnyPopupOpen, const ImVec4& BrightPopupBg, float ButtonHeight, PopupAnchor& OutAnchor, bool bDrawBorder = false);
	static bool EditorBeginMenuPopup(const char* PopupId, const PopupAnchor& Anchor, const ImVec4& BrightPopupBg, float MinWidth = 180.0f);
	static void EditorResetMenuPopup();

	static bool BeginPropertyTable(const char* TableId, float LabelColumnWidth = 200.0f, float RevertColumnWidth = 20.0f);
	static void EndPropertyTable();
	static void PropertyRowLabel(const char* Label);
	static void PropertySeparatorRow(float PaddingY = 4.0f);
};

struct FImGuiTexture;

struct EditorIcons
{
	static FImGuiTexture* UndoIcon;

	static bool Initialize();
	static void Release();
};
