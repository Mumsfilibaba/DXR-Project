#pragma once
#include "ImGuiRenderer.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Application/InputHandler.h"

struct FImGuiEventHandler : public FInputHandler
{
    virtual ~FImGuiEventHandler() = default;

    virtual bool OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogEvent);
    virtual bool OnKeyDown(const FKeyEvent& KeyEvent);
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent);
    virtual bool OnKeyChar(const FKeyEvent& KeyTypedEvent);
    virtual bool OnMouseMove(const FCursorEvent& CursorEvent);
    virtual bool OnMouseButtonDown(const FCursorEvent& CursorEvent);
    virtual bool OnMouseButtonUp(const FCursorEvent& CursorEvent);
    virtual bool OnMouseScrolled(const FCursorEvent& CursorEvent);

    bool ProcessKeyEvent(const FKeyEvent& KeyEvent);
    bool ProcessMouseButtonEvent(const FCursorEvent& CursorEvent);
};

class FImGuiPlugin : public IImguiPlugin
{
    friend class FImGuiRenderer;

public:
    FImGuiPlugin();
    virtual ~FImGuiPlugin();

    // FModuleInterface Interface
    virtual bool Load() override final;
    virtual bool Unload() override final;

    // IImguiPlugin Interface
    virtual bool InitRenderer() override final;
    virtual void ReleaseRenderer() override final;

    virtual void Tick(float Delta) override final;
    virtual void Draw(FRHICommandList& CommandList) override final;

    virtual FDelegateHandle AddDelegate(const FImGuiDelegate& Delegate) override final;
    virtual void RemoveDelegate(FDelegateHandle DelegateHandle) override final;

    virtual void SetMainViewport(const TSharedPtr<FViewport>& InViewport) override final;

    virtual ImGuiIO*      GetImGuiIO()      const override final { return PluginImGuiIO; }
    virtual ImGuiContext* GetImGuiContext() const override final { return PluginImGuiContext; }

private:
    static void StaticPlatformCreateWindow(ImGuiViewport* Viewport);
    static void StaticPlatformDestroyWindow(ImGuiViewport* Viewport);
    static void StaticPlatformShowWindow(ImGuiViewport* Viewport);
    static void StaticPlatformUpdateWindow(ImGuiViewport* Viewport);
    static ImVec2 StaticPlatformGetWindowPos(ImGuiViewport* Viewport);
    static void StaticPlatformSetWindowPosition(ImGuiViewport* Viewport, ImVec2 Position);
    static ImVec2 StaticPlatformGetWindowSize(ImGuiViewport* Viewport);
    static void StaticPlatformSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size);
    static void StaticPlatformSetWindowFocus(ImGuiViewport* Viewport);
    static bool StaticPlatformGetWindowFocus(ImGuiViewport* Viewport);
    static bool StaticPlatformGetWindowMinimized(ImGuiViewport* Viewport);
    static void StaticPlatformSetWindowTitle(ImGuiViewport* Viewport, const CHAR* Title);
    static void StaticPlatformSetWindowAlpha(ImGuiViewport* Viewport, float Alpha);
    static float StaticPlatformGetWindowDpiScale(ImGuiViewport* Viewport);
    static void StaticPlatformOnChangedViewport(ImGuiViewport*);

    void UpdateMonitorInfo();

    ImGuiIO*                       PluginImGuiIO;
    ImGuiContext*                  PluginImGuiContext;
    TSharedPtr<FImGuiRenderer>     Renderer;
    TSharedPtr<FImGuiEventHandler> EventHandler;
    TSharedPtr<FWindow>            MainWindow;
    TSharedPtr<FViewport>          MainViewport;
    FImGuiDrawMulticastDelegate    DrawDelegates;
    FDelegateHandle                OnMonitorConfigChangedDelegateHandle;
};

extern FImGuiPlugin* GImGuiPlugin;