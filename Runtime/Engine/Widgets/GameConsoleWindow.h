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
#include "Application/Widget.h"
#include "Application/ApplicationEventHandler.h"
#include "Application/Core/Events.h"

#include <imgui.h>

struct FConsoleInputHandler final : public FInputPreProcessor
{
    DECLARE_DELEGATE(FHandleKeyEventDelegate, const FKeyEvent&);
    FHandleKeyEventDelegate HandleKeyEventDelegate;

    FConsoleInputHandler() = default;
    ~FConsoleInputHandler() = default;

    virtual bool OnKeyUpEvent(const FKeyEvent& KeyEvent) override final
    {
        HandleKeyEventDelegate.Execute(KeyEvent);
        return bConsoleToggled;
    }

    virtual bool OnKeyDownEvent(const FKeyEvent& KeyEvent) override final
    {
        HandleKeyEventDelegate.Execute(KeyEvent);
        return bConsoleToggled;
    }

    bool bConsoleToggled = false;
};

class FGameConsoleWindow final : public FWidget, public IOutputDevice
{
    DECLARE_WIDGET(FGameConsoleWindow, FWidget);

public:
    FINITIALIZER_START(FGameConsoleWindow)
    FINITIALIZER_END();

public:
    FGameConsoleWindow();
    ~FGameConsoleWindow();

    void Initialize(const FInitializer& Initializer)
    {
    }

    virtual void Paint(const FRectangle& AssignedBounds) override final;

    virtual void Log(const FString& Message) override final;
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

private:
    int32 TextCallback(struct ImGuiInputTextCallbackData* Data);

    void HandleKeyPressedEvent(const FKeyEvent& Event);

    TSharedPtr<FConsoleInputHandler> InputHandler;

    // Text to display in the input box when browsing through the history
    FString PopupSelectedText;

    // The current candidates of registered console-objects
    TArray<TPair<IConsoleObject*, FString>> Candidates;
    int32 CandidatesIndex = -1;

    // Index in the history
    TArray<TPair<FString, ELogSeverity>> Messages;
    FCriticalSection MessagesCS;

    int32 HistoryIndex = -1;

    TStaticArray<CHAR, 256> TextBuffer;

    bool bUpdateCursorPosition      = false;
    bool bIsActive                  = false;
    bool bCandidateSelectionChanged = false;
    bool bScrollDown                = false;
};
