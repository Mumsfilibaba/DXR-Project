#pragma once
#include "Core/Application/UI/IUIWindow.h"
#include "Core/Application/InputHandler.h"
#include "Core/Application/Events.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Pair.h"
#include "Core/Debug/Console/IConsoleObject.h"

#include <imgui.h>

class CConsoleInputHandler final : public CInputHandler
{
public:

    DECLARE_DELEGATE( CHandleKeyEventDelegate, const SKeyEvent& );
    CHandleKeyEventDelegate HandleKeyEventDelegate;

    CConsoleInputHandler() = default;
    ~CConsoleInputHandler() = default;

    virtual bool HandleKeyEvent( const SKeyEvent& KeyEvent ) override final
    {
        HandleKeyEventDelegate.Execute( KeyEvent );
        return ConsoleToggled;
    }

    virtual uint32 GetPriority() const override final
    {
        return static_cast<uint32>(-1);
    }

    bool ConsoleToggled = false;
};

/* Panel that render the console window */
class CGameConsoleWindow final : public IUIWindow
{
    INTERFACE_GENERATE_BODY();

public:

    /* Create and initialize the console window */
    static TSharedRef<CGameConsoleWindow> Make();

    /* Update and render the console */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

private:

    CGameConsoleWindow() = default;
    ~CGameConsoleWindow() = default;

    /* Callback from the input */
    int32 TextCallback( struct ImGuiInputTextCallbackData* Data );

    /* Called when a key is pressed */
    void HandleKeyPressedEvent( const SKeyEvent& Event );

    CConsoleInputHandler InputHandler;

    /* Text to display in the input box when browsing through the history */
    CString PopupSelectedText;

    /* The current candidates of registered console-objects */
    TArray<TPair<IConsoleObject*, CString>> Candidates;
    int32 CandidatesIndex = -1;

    /* Index in the history */
    int32 HistoryIndex = -1;

    TStaticArray<char, 256> TextBuffer;

    bool UpdateCursorPosition = false;
    bool IsActive = false;
    bool CandidateSelectionChanged = false;
    bool ScrollDown = false;
};