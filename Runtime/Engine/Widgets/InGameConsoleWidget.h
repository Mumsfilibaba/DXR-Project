#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/IOutputDevice.h"
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

struct FConsoleMessage
{
    FConsoleMessage() = default;

    FConsoleMessage(const FString& InMessage, ELogSeverity InSeverity)
        : Message(InMessage)
        , Severity(InSeverity)
    {
    }

    FString      Message;
    ELogSeverity Severity;
};

class FInGameConsoleWidget final : public IOutputDevice
{
public:
    FInGameConsoleWidget();
    ~FInGameConsoleWidget();

    // IOutputDevice Interface
    virtual void Log(const FString& Message) override final;
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

    // Draw the interface
    void Draw();
    void DrawConsole();

private:
    static constexpr int32 InvalidIndex = -1;

    void HandleKeyPressedEvent(const FKeyEvent& Event);

    // ImGui callback for the text-input field
    int32 InputTextCallback(struct ImGuiInputTextCallbackData* CallbackData);

    // Clear the candidates array and reset the index
    void InvalidateCandidates();

    TSharedPtr<FConsoleInputHandler> InputHandler;
    FDelegateHandle                  ImGuiDelegateHandle;

    // The current candidates of registered console-objects
    TArray<TPair<IConsoleObject*, FString>> Candidates;

    // Index in the history
    TArray<FConsoleMessage> Messages;
    FCriticalSection        MessagesCS;

    int32 SelectedCandidateIndex = InvalidIndex;
    int32 HistoryIndex           = InvalidIndex;

    TStaticArray<CHAR, 256> TextBuffer;

    bool bUpdateCursorPosition;
    bool bIsActive;
    bool bCandidateSelectionChanged;
    bool bShouldScrollText;
};
