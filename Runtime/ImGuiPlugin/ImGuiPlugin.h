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

    void OnCreatePlatformWindow(ImGuiViewport* Viewport);
    void OnDestroyPlatformWindow(ImGuiViewport* Viewport);
    void OnShowPlatformWindow(ImGuiViewport* Viewport);
    void OnUpdatePlatformWindow(ImGuiViewport* Viewport);
    ImVec2 OnGetPlatformWindowPosition(ImGuiViewport* Viewport);
    void OnSetPlatformWindowPosition(ImGuiViewport* Viewport, ImVec2 Position);
    ImVec2 OnGetPlatformWindowSize(ImGuiViewport* Viewport);
    void OnSetPlatformWindowSize(ImGuiViewport* Viewport, ImVec2 Size);
    void OnSetPlatformWindowFocus(ImGuiViewport* Viewport);
    bool OnGetPlatformWindowFocus(ImGuiViewport* Viewport);
    bool OnGetPlatformWindowMinimized(ImGuiViewport* Viewport);
    void OnSetPlatformWindowTitle(ImGuiViewport* Viewport, const CHAR* Title);
    void OnSetPlatformWindowAlpha(ImGuiViewport* Viewport, float Alpha);
    float OnGetPlatformWindowDpiScale(ImGuiViewport* Viewport);
    void OnPlatformChangedViewport(ImGuiViewport* Viewport);
    
private:
    static void StaticOnCreatePlatformWindow(ImGuiViewport* Viewport);
    static void StaticOnDestroyPlatformWindow(ImGuiViewport* Viewport);
    static void StaticOnShowPlatformWindow(ImGuiViewport* Viewport);
    static void StaticOnUpdatePlatformWindow(ImGuiViewport* Viewport);
    static ImVec2 StaticOnGetPlatformWindowPosition(ImGuiViewport* Viewport);
    static void StaticOnSetPlatformWindowPosition(ImGuiViewport* Viewport, ImVec2 Position);
    static ImVec2 StaticOnGetPlatformWindowSize(ImGuiViewport* Viewport);
    static void StaticOnSetPlatformWindowSize(ImGuiViewport* Viewport, ImVec2 Size);
    static void StaticOnSetPlatformWindowFocus(ImGuiViewport* Viewport);
    static bool StaticOnGetPlatformWindowFocus(ImGuiViewport* Viewport);
    static bool StaticOnGetPlatformWindowMinimized(ImGuiViewport* Viewport);
    static void StaticOnSetPlatformWindowTitle(ImGuiViewport* Viewport, const CHAR* Title);
    static void StaticOnSetPlatformWindowAlpha(ImGuiViewport* Viewport, float Alpha);
    static float StaticOnGetPlatformWindowDpiScale(ImGuiViewport* Viewport);
    static void StaticOnPlatformChangedViewport(ImGuiViewport* Viewport);

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
