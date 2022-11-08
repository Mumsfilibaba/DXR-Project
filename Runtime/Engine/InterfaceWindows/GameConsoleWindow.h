#pragma once
#include "Application/Window.h"
#include "Application/InputHandler.h"
#include "Application/Events.h"

#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Misc/Console/ConsoleManager.h"

#include <imgui.h>

class FConsoleInputHandler final 
    : public FInputHandler
{
public:
    DECLARE_DELEGATE(FHandleKeyEventDelegate, const FKeyEvent&);
    FHandleKeyEventDelegate HandleKeyEventDelegate;

    FConsoleInputHandler() = default;
    ~FConsoleInputHandler() = default;

    virtual bool HandleKeyEvent(const FKeyEvent& KeyEvent) override final
    {
        HandleKeyEventDelegate.Execute(KeyEvent);
        return bConsoleToggled;
    }

    bool bConsoleToggled = false;
};

class FGameConsoleWindow final 
    : public FWindow
{
    INTERFACE_GENERATE_BODY();

    FGameConsoleWindow();
    ~FGameConsoleWindow() = default;

public:
    static TSharedRef<FGameConsoleWindow> Make();

    virtual void Tick() override final;

    virtual bool IsTickable() override final;

private:

    /** Callback from the input */
    int32 TextCallback(struct ImGuiInputTextCallbackData* Data);

    /** Called when a key is pressed */
    void HandleKeyPressedEvent(const FKeyEvent& Event);

    TSharedPtr<FConsoleInputHandler> InputHandler;

    // Text to display in the input box when browsing through the history
    FString PopupSelectedText;

    // The current candidates of registered console-objects
    TArray<TPair<IConsoleObject*, FString>> Candidates;
    int32 CandidatesIndex = -1;

    // Index in the history
    int32 HistoryIndex = -1;

    TStaticArray<CHAR, 256> TextBuffer;

    bool bUpdateCursorPosition      = false;
    bool bIsActive                  = false;
    bool bCandidateSelectionChanged = false;
    bool bScrollDown                = false;
};
