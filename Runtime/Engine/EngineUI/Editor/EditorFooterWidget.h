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

class FEditorFooterWidget final
{
public:
    FEditorFooterWidget(const TSharedPtr<IOutputDevice>& InOutputDevice);
    ~FEditorFooterWidget();

    void Draw();

    float GetHeight() const
    {
        return 40.0f;
    }

private:
    static constexpr int32 InvalidIndex = -1;

    // ImGui callback for the text-input field
    int32 InputTextCallback(struct ImGuiInputTextCallbackData* Data);
    
    // Helper that can invalidate the candidate-list
    void InvalidateCandidates();
    void ApplyCandidateToBuffer(int32 CandidateIndex);

    TArray<TPair<IConsoleObject*, FString>> Candidates;
    FString                                 CandidateFilter;
    TStaticArray<CHAR, 256>                 TextBuffer;
    TSharedPtr<IOutputDevice>               OutputDevice;
    TSharedPtr<FConsoleInputHandler>        InputHandler;
    int32                                   SelectedCandidateIndex;
    int32                                   HistoryIndex;
    int32                                   LastCursorPosition;
    int32                                   PendingCursorPosition;
    bool                                    bCandidateSelectionChanged;
    bool                                    bRequestCursorPosition;
    bool                                    bRequestInputFocus;
    bool                                    bUpdateCursorPosition;
    bool                                    bScrollToBottom;
    bool                                    bCandidatesOverlayOpen;
};
