#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Color.h"

#include <imgui.h>
#include <imgui_internal.h>

struct FImGuiViewport;

namespace ImGuiExtensions
{
    FORCEINLINE bool IsItemFullyVisible()
    {
        ImGuiWindow* Window = ImGui::GetCurrentWindow();
        if (Window == nullptr)
        {
            return false;
        }

        // Get the item's bounding box in screen space
        ImVec2 ItemMin = ImGui::GetItemRectMin();
        ImVec2 ItemMax = ImGui::GetItemRectMax();

        // Get the current window's clipping rectangle. In a child window, this corresponds to the visible area of the child.
        const ImRect& ClipRect = Window->ClipRect;

        // NOTE: ClipRect.x and ClipRect.y are the top-left; ClipRect.z and ClipRect.w are the bottom-right.
        return (ItemMin.x >= ClipRect.Min.x && ItemMin.y >= ClipRect.Min.y && ItemMax.x <= ClipRect.Max.x && ItemMax.y <= ClipRect.Max.y);
    }

    FORCEINLINE bool ButtonCenteredOnLine(const CHAR* Label, float Alignment = 0.5f)
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

    FORCEINLINE bool DrawFloat3Control(const CHAR* Label, FVector3& OutValue, float ResetValue = 0.0f, float ColumnWidth = 100.0f, float Speed = 0.01f)
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

        const float LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 ButtonSize = ImVec2(LineHeight + 3.0f, LineHeight);

        // X
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
        if (ImGui::Button("X", ButtonSize))
        {
            OutValue.X = ResetValue;
            bResult = true;
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
            bResult = true;
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
            bResult = true;
        }

        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &OutValue.Z, Speed);
        ImGui::PopItemWidth();

        // Reset
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();

        return bResult;
    }

    FORCEINLINE bool DrawColorEdit3(const CHAR* Label, FVector3& OutColor, ImGuiColorEditFlags Flags = 0)
    {
        return ImGui::ColorEdit3(Label, OutColor.XYZ, Flags);
    }
    
    FORCEINLINE bool DrawColorEdit3(const CHAR* Label, FFloatColor& OutColor, ImGuiColorEditFlags Flags = 0)
    {
        return ImGui::ColorEdit3(Label, OutColor.RGBA, Flags);
    }

    FORCEINLINE FImGuiViewport* GetMainViewportData()
    {
        if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
        {
            return reinterpret_cast<FImGuiViewport*>(Viewport->RendererUserData);
        }
        
        return nullptr;
    }

    FORCEINLINE ImVec2 GetMainViewportPos()
    {
        ImVec2 Position = ImVec2(0.0f, 0.0f);
        if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
        {
            Position = Viewport->WorkPos;
        }

        return Position;
    }

    FORCEINLINE ImVec2 GetMainViewportSize()
    {
        ImVec2 Size = ImVec2(0.0f, 0.0f);
        if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
        {
            Size = Viewport->WorkSize;
        }

        return Size;
    }

    FORCEINLINE ImVec2 GetDisplaySize()
    {
        ImGuiIO& IOState = ImGui::GetIO();
        return IOState.DisplaySize;
    }

    FORCEINLINE ImVec2 GetDisplayFramebufferScale()
    {
        ImGuiIO& IOState = ImGui::GetIO();
        return IOState.DisplayFramebufferScale;
    }
}
