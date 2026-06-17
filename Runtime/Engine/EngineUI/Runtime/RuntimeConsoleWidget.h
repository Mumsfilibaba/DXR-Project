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
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "Engine/EngineUI/BaseConsoleWidget.h"

class FRuntimeConsoleWidget final : public IOutputDevice
{
public:
    FRuntimeConsoleWidget();
    ~FRuntimeConsoleWidget();
    
    // IOutputDevice Interface
    virtual void Log(const String& Message) override final;
    virtual void Log(ELogSeverity Severity, const String& Message) override final;
    
    // Draw the interface
    void Draw();
    
private:
    struct FConsoleMessage
    {
        FConsoleMessage() = default;
    
        FConsoleMessage(const String& InMessage, ELogSeverity InSeverity)
            : Message(InMessage)
            , Severity(InSeverity)
        {
        }
    
        String       Message;
        ELogSeverity Severity;
    };

    static constexpr int32 InvalidIndex = -1;
    
    // ImGui callback for the text-input field
    int32 InputTextCallback(struct ImGuiInputTextCallbackData* CallbackData);
    
    // Draw the console
    void DrawConsole();

    // Clear the candidates array and reset the index
    void InvalidateCandidates();
    void HandleKeyPressedEvent(const FKeyEvent& Event);

    TSharedPtr<FConsoleInputHandler>       InputHandler;
    FDelegateHandle                        ImGuiDelegateHandle;
    TArray<TPair<IConsoleObject*, String>> Candidates;
    TArray<FConsoleMessage>                Messages;
    FCriticalSection                       MessagesCS;
    int32                                  SelectedCandidateIndex = InvalidIndex;
    int32                                  HistoryIndex           = InvalidIndex;
    TStaticArray<CHAR, 256>                TextBuffer;
    bool                                   bUpdateCursorPosition : 1;
    bool                                   bIsActive : 1;
    bool                                   bCandidateSelectionChanged : 1;
    bool                                   bShouldScrollText : 1;
};
