#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Color.h"
#include <imgui.h>

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

struct ENGINE_API EditorWidgets
{
	static bool ButtonCenteredOnLine(const CHAR* Label, float Alignment = 0.5f);

	static bool DrawFloat3Control(const CHAR* Label, FVector3& OutValue, float ResetValue = 0.0f, float ColumnWidth = 100.0f, float Speed = 0.01f);

	static FORCEINLINE bool DrawColorEdit3(const CHAR* Label, FVector3& OutColor, ImGuiColorEditFlags Flags = 0)
	{
		return ImGui::ColorEdit3(Label, OutColor.XYZ, Flags);
	}

	static FORCEINLINE bool DrawColorEdit3(const CHAR* Label, FFloatColor& OutColor, ImGuiColorEditFlags Flags = 0)
	{
		return ImGui::ColorEdit3(Label, OutColor.RGBA, Flags);
	}
};
