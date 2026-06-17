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
    static constexpr int32 InvalidIndex = -1;

public:
    FEditorFooterWidget(const TSharedPtr<IOutputDevice>& InOutputDevice);
    ~FEditorFooterWidget();

    void Draw();

    float GetHeight() const
    {
        return 40.0f;
    }

private:
    void ApplyCandidateToBuffer(int32 CandidateIndex);

    // Helper that can invalidate the candidate-list
    void InvalidateCandidates();

    // ImGui callback for the text-input field
    int32 InputTextCallback(struct ImGuiInputTextCallbackData* Data);

private:
    TArray<TPair<IConsoleObject*, String>> Candidates;
    String                                 CandidateFilter;
    TStaticArray<CHAR, 256>                TextBuffer;
    TSharedPtr<IOutputDevice>              OutputDevice;
    TSharedPtr<FConsoleInputHandler>       InputHandler;
    int32                                  SelectedCandidateIndex;
    int32                                  HistoryIndex;
    int32                                  LastCursorPosition;
    int32                                  PendingCursorPosition;
    bool                                   bCandidateSelectionChanged : 1;
    bool                                   bRequestCursorPosition : 1;
    bool                                   bRequestInputFocus : 1;
    bool                                   bUpdateCursorPosition : 1;
    bool                                   bScrollToBottom : 1;
    bool                                   bCandidatesOverlayOpen : 1;
};
