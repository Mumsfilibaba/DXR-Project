#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDevice.h"
#include "Core/Platform/CriticalSection.h"
#include "Application/InputHandler.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

struct FConsoleInputHandler final : public FInputHandler
{
    DECLARE_DELEGATE(FHandleKeyEventDelegate, const FKeyEvent&);
    FHandleKeyEventDelegate HandleKeyEventDelegate;

    virtual bool OnKeyUp(const FKeyEvent& KeyEvent) override final
    {
        HandleKeyEventDelegate.Execute(KeyEvent);
        return bConsoleToggled;
    }

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override final
    {
        HandleKeyEventDelegate.Execute(KeyEvent);
        return bConsoleToggled;
    }

    bool bConsoleToggled = false;
};

class FConsoleWidget final : public IOutputDevice
{
public:
    FConsoleWidget();
    ~FConsoleWidget();

    virtual void Log(const FString& Message) override final;
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

    void Draw();

private:
    int32 TextCallback(struct ImGuiInputTextCallbackData* Data);
    void HandleKeyPressedEvent(const FKeyEvent& Event);

    TSharedPtr<FConsoleInputHandler> InputHandler;
    FDelegateHandle                  ImGuiDelegateHandle;

    // Text to display in the input box when browsing through the history
    FString PopupSelectedText;

    // The current candidates of registered console-objects
    TArray<TPair<IConsoleObject*, FString>> Candidates;
    int32 CandidatesIndex = -1;
    int32 HistoryIndex    = -1;

    // Index in the history
    TArray<TPair<FString, ELogSeverity>> Messages;
    FCriticalSection                     MessagesCS;

    TStaticArray<CHAR, 256> TextBuffer;

    bool bUpdateCursorPosition;
    bool bIsActive;
    bool bCandidateSelectionChanged;
    bool bScrollDown;
};
