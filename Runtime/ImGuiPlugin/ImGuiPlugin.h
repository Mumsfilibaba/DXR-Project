#pragma once
#include "ImGuiEventHandler.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include <imgui.h>

class FImGuiRenderer;

struct FImGuiViewport
{
    FImGuiViewport()
        : VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
        , VertexCount(0)
        , IndexCount(0)
        , Viewport(nullptr)
        , bDidResize(false)
        , Width(0)
        , Height(0)
    {
    }

    FRHIBufferRef   VertexBuffer;
    FRHIBufferRef   IndexBuffer;
    int32           VertexCount;
    int32           IndexCount;
    FRHIViewportRef Viewport;
    bool            bDidResize;
    uint16          Width;
    uint16          Height;
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
    virtual bool InitializeRenderer() override final;
    virtual void ReleaseRenderer() override final;

    virtual void Tick(float Delta) override final;
    virtual void TickRenderer(FRHICommandList& CommandList) override final;

    virtual void AddWidget(const TSharedPtr<IImGuiWidget>& InWidget) override final;
    virtual void RemoveWidget(const TSharedPtr<IImGuiWidget>& InWidget) override final;

    virtual void SetMainViewport(FViewport* InViewport) override final;

    virtual ImGuiIO* GetImGuiIO() const override final { return PluginImGuiIO; }
    virtual ImGuiContext* GetImGuiContext() const override final { return PluginImGuiContext; }

private:
    ImGuiIO*                         PluginImGuiIO;
    ImGuiContext*                    PluginImGuiContext;
    FImGuiEventHandler               EventHandler;
    TArray<TSharedPtr<IImGuiWidget>> Widgets;
    TUniquePtr<FImGuiRenderer>       Renderer;
};
