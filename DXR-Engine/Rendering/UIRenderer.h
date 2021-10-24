#pragma once
#include "Core/Application/InputHandler.h"

#include "CoreRHI/RHIResources.h"
#include "CoreRHI/RHIResourceViews.h"

#include "Core/Delegates/Delegate.h"

#include "Core/Containers/SharedRef.h"

#include <imgui.h>

// Used when rendering images with ImGui
struct SImGuiImage
{
    SImGuiImage() = default;

    SImGuiImage( const TSharedRef<CRHIShaderResourceView>& InImageView, const TSharedRef<CRHITexture>& InImage, EResourceState InBefore, EResourceState InAfter )
        : ImageView( InImageView )
        , Image( InImage )
        , BeforeState( InBefore )
        , AfterState( InAfter )
    {
    }

    TSharedRef<CRHIShaderResourceView> ImageView;
    TSharedRef<CRHITexture> Image;
    EResourceState BeforeState;
    EResourceState AfterState;
    bool AllowBlending = false;
};

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

class CUIRenderer
{
public:
    typedef void(*UIDrawFunc)();

    static bool Init();
    static void Release();

    static void DrawUI( UIDrawFunc DrawFunc );
    static void DrawDebugString( const CString& DebugString );

    static void OnKeyEvent( const SKeyEvent& Event );
    static void OnKeyTyped( SKeyTypedEvent Event );

    static void OnMouseButtonEvent( const SMouseButtonEvent& Event );
    static void OnMouseScrolled( const SMouseScrolledEvent& Event );

    // Update the UI should be called in the application before handling input
    static void Tick();

    // Should only be called by the renderer
    static void Render( class CRHICommandList& CmdList );

    static ImGuiContext* GetCurrentContext();
};
