#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Misc/IOutputDevice.h"
#include "Core/Misc/ConsoleManager.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "Engine/EngineUI/BaseConsoleWidget.h"

class FEditorConsoleInputFieldWidget final
{
public:
    FEditorConsoleInputFieldWidget(const TSharedPtr<IOutputDevice>& InOutputDevice);
    ~FEditorConsoleInputFieldWidget();

    void Draw();

    void SetVisible(bool bInVisible) 
    { 
        bVisible = bInVisible; 
    }

    bool IsVisible() const 
    { 
        return bVisible; 
    }

private:
    static constexpr int32 InvalidIndex = -1;

    // ImGui callback for the text-input field
    int32 InputTextCallback(struct ImGuiInputTextCallbackData* Data);
    
    // Private function to draw the console input and candidates window
    void DrawConsole();

    // Helper that can invalidate the candidate-list
    void InvalidateCandidates();

    TArray<TPair<IConsoleObject*, FString>> Candidates;
    TStaticArray<CHAR, 256>                 TextBuffer;
    TSharedPtr<IOutputDevice>               OutputDevice;
    TSharedPtr<FConsoleInputHandler>        InputHandler;
    FDelegateHandle                         ImGuiDelegateHandle;

    int32 SelectedCandidateIndex     = InvalidIndex;
    int32 HistoryIndex               = InvalidIndex;
    bool  bCandidateSelectionChanged = false;
    bool  bUpdateCursorPosition      = false;
    bool  bVisible                   = false;
    bool  bIsActive                  = false;
    bool  bAutoScroll                = true;
    bool  bScrollToBottom            = false;
};
