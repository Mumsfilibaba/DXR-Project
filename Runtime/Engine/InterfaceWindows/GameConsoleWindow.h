#pragma once
#include "Canvas/Window.h"
#include "Canvas/InputHandler.h"
#include "Canvas/Events.h"

#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Debug/Console/IConsoleObject.h"

#include <imgui.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CConsoleInputHandler

class CConsoleInputHandler final : public FInputHandler
{
public:

    DECLARE_DELEGATE(CHandleKeyEventDelegate, const FKeyEvent&);
    CHandleKeyEventDelegate HandleKeyEventDelegate;

    CConsoleInputHandler() = default;
    ~CConsoleInputHandler() = default;

    virtual bool HandleKeyEvent(const FKeyEvent& KeyEvent) override final
    {
        HandleKeyEventDelegate.Execute(KeyEvent);
        return bConsoleToggled;
    }

    bool bConsoleToggled = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGameConsoleWindow

class CGameConsoleWindow final : public FWindow
{
    INTERFACE_GENERATE_BODY();

public:

    static TSharedRef<CGameConsoleWindow> Make();

    virtual void Tick() override final;

    virtual bool IsTickable() override final;

private:

    CGameConsoleWindow();
    ~CGameConsoleWindow() = default;

    /** Callback from the input */
    int32 TextCallback(struct ImGuiInputTextCallbackData* Data);

    /** Called when a key is pressed */
    void HandleKeyPressedEvent(const FKeyEvent& Event);

    TSharedPtr<CConsoleInputHandler> InputHandler;

    // Text to display in the input box when browsing through the history
    FString PopupSelectedText;

    // The current candidates of registered console-objects
    TArray<TPair<IConsoleObject*, FString>> Candidates;
    int32 CandidatesIndex = -1;

    // Index in the history
    int32 HistoryIndex = -1;

    TStaticArray<char, 256> TextBuffer;

    bool bUpdateCursorPosition = false;
    bool bIsActive = false;
    bool bCandidateSelectionChanged = false;
    bool bScrollDown = false;
};
