#pragma once
#include "RendererAPI.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIResourceViews.h"

#include "Core/Application/InputHandler.h"
#include "Core/Application/UI/IUIRenderer.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Time/Timer.h"

#include <imgui.h>

class CUIInputHandler : public CInputHandler
{
public:

    CUIInputHandler() = default;
    ~CUIInputHandler() = default;

    DECLARE_DELEGATE( CKeyEventDelegate, const SKeyEvent& );
    CKeyEventDelegate KeyEventDelegate;

    virtual bool HandleKeyEvent( const SKeyEvent& KeyEvent ) override final
    {
        KeyEventDelegate.Execute( KeyEvent );
        return ImGui::GetIO().WantCaptureKeyboard;
    }

    DECLARE_DELEGATE( CKeyTypedDelegate, SKeyTypedEvent );
    CKeyTypedDelegate KeyTypedDelegate;

    virtual bool HandleKeyTyped( SKeyTypedEvent KeyTypedEvent ) override final
    {
        KeyTypedDelegate.Execute( KeyTypedEvent );
        return ImGui::GetIO().WantCaptureKeyboard;
    }

    DECLARE_DELEGATE( CMouseButtonDelegate, const SMouseButtonEvent& );
    CMouseButtonDelegate MouseButtonDelegate;

    virtual bool HandleMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent ) override final
    {
        MouseButtonDelegate.Execute( MouseButtonEvent );
        return ImGui::GetIO().WantCaptureMouse;
    }

    DECLARE_DELEGATE( CMouseScrolledDelegate, const SMouseScrolledEvent& );
    CMouseScrolledDelegate MouseScrolledDelegate;

    virtual bool HandleMouseScrolled( const SMouseScrolledEvent& MouseScrolledEvent ) override final
    {
        MouseScrolledDelegate.Execute( MouseScrolledEvent );
        return ImGui::GetIO().WantCaptureMouse;
    }
};

class RENDERER_API CUIRenderer final : public IUIRenderer
{
public:

    static TSharedRef<CUIRenderer> Make();

    /* Start the update of the UI, after the call to this function, calls to UI window's tick are valid */
    virtual void BeginTick() override final;

    /* End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid  */
    virtual void EndTick() override final;

    /* Render all the UI for this frame */
    virtual void Render( class CRHICommandList& Commandlist ) override final;

    /* Retrieve the context handle */
    virtual UIContextHandle GetContext() const override final;

private:

    CUIRenderer() = default;
    ~CUIRenderer();

    bool Init();

    void OnKeyEvent( const SKeyEvent& Event );
    void OnKeyTyped( SKeyTypedEvent Event );
    
    void OnMouseButtonEvent( const SMouseButtonEvent& Event );
    void OnMouseScrolled( const SMouseScrolledEvent& Event );
    
    CUIInputHandler InputHandler;

    CTimer FrameClock;

    TSharedRef<CRHITexture2D>             FontTexture;
    TSharedRef<CRHIGraphicsPipelineState> PipelineState;
    TSharedRef<CRHIGraphicsPipelineState> PipelineStateNoBlending;
    TSharedRef<CRHIPixelShader>           PShader;
    TSharedRef<CRHIVertexBuffer>          VertexBuffer;
    TSharedRef<CRHIIndexBuffer>           IndexBuffer;
    TSharedRef<CRHISamplerState>          PointSampler;

    ImGuiContext* Context = nullptr;
};
