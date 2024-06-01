#pragma once
#include "ImGuiRenderer.h"
#include "Viewport.h"

#include <imgui.h>

class APPLICATION_API FImGui
{
public:
    static void CreateContext();
    static void DestroyContext();

    static bool IsInitialized()
    {
        return Context != nullptr;
    }

    static FORCEINLINE ImGuiContext* GetContext()
    {
        return Context;
    }

    static void InitializeStyle();
    static void SetMainViewport(FViewport* InViewport);

    static FResponse OnGamepadButtonEvent(FKey Key, bool bIsDown);
    static FResponse OnGamepadAnalogEvent(EAnalogSourceName::Type AnalogSource, float Analog);
    static FResponse OnKeyEvent(FKey Key, FModifierKeyState ModifierKeyState, bool bIsDown);
    static FResponse OnKeyCharEvent(uint32 Character);
    static FResponse OnMouseMoveEvent(int32 x, int32 y);
    static FResponse OnMouseButtonEvent(FKey Key, bool bIsDown);
    static FResponse OnMouseScrollEvent(float ScrollDelta, bool bVertical);
    static FResponse OnMouseLeft();
    static FResponse OnWindowResize(void* PlatformHandle);
    static FResponse OnWindowMoved(void* PlatformHandle);
    static FResponse OnWindowClose(void* PlatformHandle);
    static FResponse OnFocusLost();
    static FResponse OnFocusGained();

    static FORCEINLINE bool IsMultiViewportEnabled()
    {
        ImGuiIO& ImGuiState = ImGui::GetIO();
        return (ImGuiState.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) == ImGuiBackendFlags_PlatformHasViewports;
    }

    static FORCEINLINE FViewportData* GetMainViewportData()
    {
        if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
        {
            return reinterpret_cast<FViewportData*>(Viewport->RendererUserData);
        }
        
        return nullptr;
    }
    
    static FORCEINLINE ImVec2 GetDisplaySize()
    {
        ImGuiIO& ImGuiState = ImGui::GetIO();
        return ImGuiState.DisplaySize;
    }

    static FORCEINLINE ImVec2 GetDisplayFramebufferScale()
    {
        ImGuiIO& ImGuiState = ImGui::GetIO();
        return ImGuiState.DisplayFramebufferScale;
    }

    static FORCEINLINE ImVec2 GetMainViewportPos()
    {
        ImVec2 Position = ImVec2(0.0f, 0.0f);
        if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
        {
            Position = Viewport->WorkPos;
        }

        return Position;
    }

    static FORCEINLINE ImVec2 GetMainViewportSize()
    {
        ImVec2 Size = ImVec2(0.0f, 0.0f);
        if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
        {
            Size = Viewport->WorkSize;
        }

        return Size;
    }

    static FORCEINLINE ImGuiStyle& GetStyle()
    {
        return ImGui::GetStyle();
    }

public:
    static bool ButtonCenteredOnLine(const CHAR* Label, float Alignment = 0.5f);
    static void DrawFloat3Control(const CHAR* Label, FVector3& OutValue, float ResetValue = 0.0f, float ColumnWidth = 100.0f, float Speed = 0.01f);

    static FORCEINLINE bool DrawColorEdit3(const CHAR* Label, FVector3& OutColor, ImGuiColorEditFlags Flags = 0)
    {
        return ImGui::ColorEdit3(Label, OutColor.Data(), Flags);
    }

private:
    static ImGuiContext* Context;
};
