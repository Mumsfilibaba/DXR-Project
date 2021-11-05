#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Time/Timer.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIResourceViews.h"

#include "Interface/InputHandler.h"
#include "Interface/IInterfaceRenderer.h"

#if PLATFORM_WINDOWS

#if INTERFACE_RENDERER_API_EXPORT
#define INTERFACE_RENDERER_API __declspec(dllexport)
#else
#define INTERFACE_RENDERER_API

#endif

#else
#define INTERFACE_RENDERER_API
#endif

// TODO: Remove since application should handle it directly
class CInputHandler : public CInputHandler
{
public:

    CUIInputHandler() = default;
    ~CUIInputHandler() = default;

    DECLARE_DELEGATE( CKeyEventDelegate, const SKeyEvent& );
    CKeyEventDelegate KeyEventDelegate;

    virtual bool HandleKeyEvent( const SKeyEvent& KeyEvent ) override final;

    DECLARE_DELEGATE( CKeyTypedDelegate, SKeyTypedEvent );
    CKeyTypedDelegate KeyTypedDelegate;

    virtual bool HandleKeyTyped( SKeyTypedEvent KeyTypedEvent ) override final;

    DECLARE_DELEGATE( CMouseButtonDelegate, const SMouseButtonEvent& );
    CMouseButtonDelegate MouseButtonDelegate;

    virtual bool HandleMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent ) override final;

    DECLARE_DELEGATE( CMouseScrolledDelegate, const SMouseScrolledEvent& );
    CMouseScrolledDelegate MouseScrolledDelegate;

    virtual bool HandleMouseScrolled( const SMouseScrolledEvent& MouseScrolledEvent ) override final;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class INTERFACE_RENDERER_API CInterfaceRenderer final : public IInterfaceRenderer
{
public:

    static TSharedRef<CInterfaceRenderer> Make();

    /* Start the update of the UI, after the call to this function, calls to UI window's tick are valid */
    virtual void BeginTick() override final;

    /* End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid  */
    virtual void EndTick() override final;

    /* Render all the UI for this frame */
    virtual void Render( class CRHICommandList& Commandlist ) override final;

    /* Retrieve the context handle */
    virtual InterfaceContext GetContext() const override final;

private:

    CInterfaceRenderer() = default;
    ~CInterfaceRenderer();

    bool Init();

    void OnKeyEvent( const SKeyEvent & Event );
    void OnKeyTyped( SKeyTypedEvent Event );

    void OnMouseButtonEvent( const SMouseButtonEvent & Event );
    void OnMouseScrolled( const SMouseScrolledEvent & Event );

    CUIInputHandler InputHandler;

    TArray<SUIImage*> RenderedImages;

    CTimer FrameClock;

    TSharedRef<CRHITexture2D>             FontTexture;
    TSharedRef<CRHIGraphicsPipelineState> PipelineState;
    TSharedRef<CRHIGraphicsPipelineState> PipelineStateNoBlending;
    TSharedRef<CRHIPixelShader>           PShader;
    TSharedRef<CRHIVertexBuffer>          VertexBuffer;
    TSharedRef<CRHIIndexBuffer>           IndexBuffer;
    TSharedRef<CRHISamplerState>          PointSampler;

    struct ImGuiContext* Context = nullptr;
};
