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

    FResponse OnMouseLeft();
    FResponse OnWindowResize(void* PlatformHandle);
    FResponse OnWindowMoved(void* PlatformHandle);
    FResponse OnWindowClose(void* PlatformHandle);
    FResponse OnFocusLost();
    FResponse OnFocusGained();
};

class FImGuiPlugin : public IImguiPlugin
{
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

    virtual void AddWidget(const TSharedPtr<IImGuiWidget>& InWidget) override final;
    virtual void RemoveWidget(const TSharedPtr<IImGuiWidget>& InWidget) override final;

    virtual void SetMainViewport(const TSharedPtr<FViewport>& InViewport) override final;

    virtual ImGuiIO*      GetImGuiIO()      const override final { return PluginImGuiIO; }
    virtual ImGuiContext* GetImGuiContext() const override final { return PluginImGuiContext; }

private:
    ImGuiIO*                         PluginImGuiIO;
    ImGuiContext*                    PluginImGuiContext;
    TSharedPtr<FImGuiRenderer>       Renderer;
    TSharedPtr<FImGuiEventHandler>   EventHandler;
    TSharedPtr<FViewport>            MainViewport;
    TArray<TSharedPtr<IImGuiWidget>> Widgets;
};

extern FImGuiPlugin* GImGuiPlugin;