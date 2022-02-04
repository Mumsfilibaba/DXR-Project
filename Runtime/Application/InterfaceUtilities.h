#pragma once
#include "Core/Core.h"

#include "Core/Math/Vector3.h"
#include "Core/Containers/String.h"

#include <imgui.h>
#include <imgui_internal.h>

constexpr float MAX_FRAMETIME_MS = 1000.0f / 30.0f;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

inline void DrawFloat3Control(const String& Label, CVector3& OutValue, float ResetValue = 0.0f, float ColumnWidth = 100.0f, float Speed = 0.01f)
{
    ImGui::PushID(Label.CStr());
    ImGui::Columns(2, nullptr, false);

    // Text
    ImGui::SetColumnWidth(0, ColumnWidth);
    ImGui::Text("%s", Label.CStr());
    ImGui::NextColumn();

    // Drag Floats
    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

    float  LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 ButtonSize = ImVec2(LineHeight + 3.0f, LineHeight);

    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    if (ImGui::Button("X", ButtonSize))
    {
        OutValue.x = ResetValue;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &OutValue.x, Speed);
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button("Y", ButtonSize))
    {
        OutValue.y = ResetValue;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &OutValue.y, Speed);
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    if (ImGui::Button("Z", ButtonSize))
    {
        OutValue.z = ResetValue;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &OutValue.z, Speed);
    ImGui::PopItemWidth();

    // Reset
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
}

FORCEINLINE bool DrawColorEdit3(const char* Label, CVector3& OutColor, ImGuiColorEditFlags Flags = 0)
{
    return ImGui::ColorEdit3(Label, OutColor.GetData(), Flags);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

inline void ImGui_PrintTime(float Nanoseconds)
{
    if (Nanoseconds == FLT_MAX || Nanoseconds == -FLT_MAX)
    {
        ImGui::Text("0.0 s");
    }
    else if (Nanoseconds < NTime::FromMicroseconds<float>(1.0f))
    {
        ImGui::Text("%.4f ns", Nanoseconds);
    }
    else if (Nanoseconds < NTime::FromMilliseconds<float>(1.0f))
    {
        ImGui::Text("%.4f qs", NTime::ToMicroseconds<float>(Nanoseconds));
    }
    else if (Nanoseconds < NTime::FromSeconds<float>(1.0f))
    {
        ImGui::Text("%.4f ms", NTime::ToMilliseconds<float>(Nanoseconds));
    }
    else
    {
        ImGui::Text("%.4f s", NTime::ToSeconds<float>(Nanoseconds));
    }
}

#if 0
inline void ImGui_PrintTiming(const char* Text, float Num)
{
    ImGui::Text("%s: ", Text);

    ImGui::NextColumn();

    ImGui_PrintTime(Num);
}

inline void ImGui_PrintTiming_SameLine(const char* Text, float Num)
{
    ImGui::Text("%s: ", Text);
    ImGui::SameLine();
    ImGui_PrintTime(Num);
}
#endif

inline float ImGui_GetMaxLimit(float Num)
{
    if (Num < 0.01f)
    {
        return 0.01f;
    }
    else if (Num < 0.1f)
    {
        return 0.1f;
    }
    else if (Num < 1.0f)
    {
        return 1.0f;
    }
    else if (Num < 3.0f)
    {
        return 3.0f;
    }
    else if (Num < 17.0f)
    {
        return 17.0f;
    }
    else if (Num < 34.0f)
    {
        return 34.0f;
    }
    else if (Num < 100.0f)
    {
        return 100.0f;
    }
    else
    {
        return 1000.0f;
    }
}
