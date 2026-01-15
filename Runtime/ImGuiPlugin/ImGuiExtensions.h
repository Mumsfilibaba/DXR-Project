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
