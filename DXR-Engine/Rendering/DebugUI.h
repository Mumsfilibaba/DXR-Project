#pragma once
#include "Core/Application/InputHandler.h"

#include "RenderLayer/Resources.h"
#include "RenderLayer/ResourceViews.h"

#include "Core/Delegates/Delegate.h"

#include "Core/Containers/SharedRef.h"

#include <imgui.h>

// Used when rendering images with ImGui
struct ImGuiImage
{
    ImGuiImage() = default;

    ImGuiImage( const TSharedRef<ShaderResourceView>& InImageView, const TSharedRef<Texture>& InImage, EResourceState InBefore, EResourceState InAfter )
        : ImageView( InImageView )
        , Image( InImage )
        , BeforeState( InBefore )
        , AfterState( InAfter )
    {
    }

    TSharedRef<ShaderResourceView> ImageView;
    TSharedRef<Texture>  Image;
    EResourceState BeforeState;
    EResourceState AfterState;
    bool AllowBlending = false;
};

class CUIInputHandler : public CInputHandler
{
public:

    CUIInputHandler() = default;
    ~CUIInputHandler() = default;

    virtual bool OnKeyEvent( const SKeyEvent& KeyEvent ) override final
    {
        KeyEventDelegate.Execute( KeyEvent );
        return ImGui::GetIO().WantCaptureKeyboard ? true : false;
    }

    virtual bool OnKeyTyped( SKeyTypedEvent KeyTypedEvent ) override final
    {
        KeyTypedDelegate.Execute( KeyTypedEvent );
        return ImGui::GetIO().WantCaptureKeyboard ? true : false;
    }

    virtual bool OnMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent ) override final
    {
        MouseButtonDelegate.Execute( MouseButtonEvent );
        return ImGui::GetIO().WantCaptureMouse ? true : false;
    }

    virtual bool OnMouseScrolled( const SMouseScrolledEvent& MouseScrolledEvent ) override final
    {
        MouseScrolledDelegate.Execute( MouseScrolledEvent );
        return ImGui::GetIO().WantCaptureMouse ? true : false;
    }

    DECLARE_DELEGATE( CKeyEventDelegate, const SKeyEvent& );
    CKeyEventDelegate KeyEventDelegate;

    DECLARE_DELEGATE( CKeyTypedDelegate, SKeyTypedEvent );
    CKeyTypedDelegate KeyTypedDelegate;

    DECLARE_DELEGATE( CMouseButtonDelegate, const SMouseButtonEvent& );
    CMouseButtonDelegate MouseButtonDelegate;

    DECLARE_DELEGATE( CMouseScrolledDelegate, const SMouseScrolledEvent& );
    CMouseScrolledDelegate MouseScrolledDelegate;
};

class DebugUI
{
public:
    typedef void(*UIDrawFunc)();

    static bool Init();
    static void Release();

    static void DrawUI( UIDrawFunc DrawFunc );
    static void DrawDebugString( const std::string& DebugString );

    static void OnKeyEvent( const SKeyEvent& Event );
    static void OnKeyTyped( SKeyTypedEvent Event );

    static void OnMouseButtonEvent( const SMouseButtonEvent& Event );
    static void OnMouseScrolled( const SMouseScrolledEvent& Event );

    // Should only be called by the renderer
    static void Render( class CommandList& CmdList );

    static ImGuiContext* GetCurrentContext();
};
