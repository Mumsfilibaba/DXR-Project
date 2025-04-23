#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Templates/CString.h"
#include "Core/Threading/ScopedLock.h"
#include "Application/Application.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include "Engine/EngineUI/InGameConsoleWidget.h"

FInGameConsoleWidget::FInGameConsoleWidget()
    : IOutputDevice()
    , InputHandler(MakeSharedPtr<FConsoleInputHandler>())
    , ImGuiDelegateHandle()
    , Candidates()
    , SelectedCandidateIndex(InvalidIndex)
    , HistoryIndex(InvalidIndex)
    , Messages()
    , MessagesCS()
    , TextBuffer() 
    , bUpdateCursorPosition(false)
    , bIsActive(false)
    , bCandidateSelectionChanged(false)
    , bShouldScrollText(false)
{
    if (FOutputDeviceLogger* OutputDeviceManager = FOutputDeviceLogger::Get())
    {
        OutputDeviceManager->RegisterOutputDevice(this);
    }

    if (FApplication::IsInitialized())
    {
        InputHandler->HandleKeyEventDelegate.BindRaw(this, &FInGameConsoleWidget::HandleKeyPressedEvent);
        FApplication::Get().RegisterInputHandler(InputHandler);
    }

    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FInGameConsoleWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }

    TextBuffer.Fill(0);
}

FInGameConsoleWidget::~FInGameConsoleWidget()
{
    if (FOutputDeviceLogger* OutputDeviceManager = FOutputDeviceLogger::Get())
    {
        OutputDeviceManager->UnregisterOutputDevice(this);
    }

    if (FApplication::IsInitialized())
    {
        FApplication::Get().UnregisterInputHandler(InputHandler);
    }

    if (IImguiPlugin::IsEnabled())
    {
         IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FInGameConsoleWidget::Draw()
{
    if (bIsActive)
    {
        DrawConsole();
    }
}

void FInGameConsoleWidget::DrawConsole()
{
    const ImVec2 MainViewportPos  = ImGuiExtensions::GetMainViewportPos();
    const ImVec2 MainViewportSize = ImGuiExtensions::GetMainViewportSize();
    const ImVec2 FrameBufferScale = ImGuiExtensions::GetDisplayFramebufferScale();

    const float Scale          = FrameBufferScale.x;
    const float TotalWidth     = MainViewportSize.x;
    const float TextAreaHeight = 384.0f * Scale;

    const float Transparency = 0.8f;

    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

    const ImGuiStyle& Style = ImGui::GetStyle();

    const ImVec4 WindowBG = Style.Colors[ImGuiCol_WindowBg];
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(WindowBG.x, WindowBG.y, WindowBG.z, Transparency));

    const ImVec2 WindowPadding = ImVec2(10.0f * Scale, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, WindowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    const ImVec2 WindowSize = ImVec2(TotalWidth, 0.0f);
    ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(MainViewportPos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

    const ImGuiWindowFlags StyleFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Console", nullptr, StyleFlags);
    {
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.3f, 0.3f, 0.3f, Transparency));

        const ImGuiWindowFlags TextChildWindowPopupFlags =
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing;

        const ImVec4 ChildWindowBG = Style.Colors[ImGuiCol_ChildBg];
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ChildWindowBG.x, ChildWindowBG.y, ChildWindowBG.z, 0.0f));

        const ImGuiChildFlags TextChildWindowFlags = ImGuiChildFlags_None;

        const ImVec2 TextChildWindowSize = ImVec2(WindowSize.x - (WindowPadding.x * 2.0f), TextAreaHeight);
        ImGui::BeginChild("##TextChildWindow", TextChildWindowSize, TextChildWindowFlags, TextChildWindowPopupFlags);
        {
            if (!Candidates.IsEmpty())
            {
                ImGui::PushAllowKeyboardFocus(false);

                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));

                const float Padding = 8.0f * Scale;

                float VariableNameWidth  = 30.0f * Scale;
                float VariableValueWidth = 20.0f * Scale;

                // First find the maximum length of each column for the selectable
                Candidates.Foreach([&](const TPair<IConsoleObject*, FString>& Candidate)
                {
                    VariableNameWidth = FMath::Max(VariableNameWidth, ImGui::CalcTextSize(*Candidate.Second).x);

                    if (IConsoleVariable* Variable = Candidate.First->AsVariable())
                    {
                        const FString Value = Variable->GetString();
                        VariableValueWidth = FMath::Max(VariableValueWidth, ImGui::CalcTextSize(*Value).x);
                    }
                });

                VariableNameWidth  += Padding;
                VariableValueWidth += Padding;

                // Draw UI
                bool bIsActiveIndex = false;
                for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Size(); CandidateIndex++)
                {
                    const TPair<IConsoleObject*, FString>& Candidate = Candidates[CandidateIndex];
                    bIsActiveIndex = SelectedCandidateIndex == CandidateIndex;

                    // VariableName
                    ImGui::PushID(CandidateIndex);

                    const ImVec2 SelectableSize(ImGui::GetContentRegionAvail().x, 20.0f);
                    ImGui::Selectable(*Candidate.Second, bIsActiveIndex, ImGuiSelectableFlags_None, SelectableSize);

                    // If the selectable is not visible we want to scroll to it
                    const bool bIsSelectableVisible = ImGuiExtensions::IsItemFullyVisible();

                    ImGui::SameLine(VariableNameWidth);

                    ImGui::AlignTextToFramePadding();

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));

                    const char* PostFixText = "";
                    const char* SetByText   = "";

                    // Value
                    const float PostFixTextLength = 
                        FMath::Max(ImGui::CalcTextSize("Bool").x,
                        FMath::Max(ImGui::CalcTextSize("Int").x,
                        FMath::Max(ImGui::CalcTextSize("Float").x,
                                   ImGui::CalcTextSize("String").x)));

                    const float SetByTextLength =
                        FMath::Max(ImGui::CalcTextSize(SetByFlagToString(EConsoleVariableFlags::SetByConstructor)).x,
                        FMath::Max(ImGui::CalcTextSize(SetByFlagToString(EConsoleVariableFlags::SetByCommandLine)).x,
                        FMath::Max(ImGui::CalcTextSize(SetByFlagToString(EConsoleVariableFlags::SetByConfigFile)).x,
                        FMath::Max(ImGui::CalcTextSize(SetByFlagToString(EConsoleVariableFlags::SetByCode)).x,
                                   ImGui::CalcTextSize(SetByFlagToString(EConsoleVariableFlags::SetByConsole)).x))));

                    IConsoleVariable* ConsoleVariable = Candidate.First->AsVariable();
                    if (ConsoleVariable)
                    {
                        const FString Value = ConsoleVariable->GetString();
                        ImGui::Text("%s", *Value);

                        if (ConsoleVariable->IsVariableBool())
                        {
                            PostFixText = "Bool";
                        }
                        else if (ConsoleVariable->IsVariableInt())
                        {
                            PostFixText = "Int";
                        }
                        else if (ConsoleVariable->IsVariableFloat())
                        {
                            PostFixText = "Float";
                        }
                        else if (ConsoleVariable->IsVariableString())
                        {
                            PostFixText = "String";
                        }

                        const EConsoleVariableFlags VariableFlags = (ConsoleVariable->GetFlags() & EConsoleVariableFlags::SetByMask);
                        SetByText = SetByFlagToString(VariableFlags);
                    }
                    else if (Candidate.First->AsCommand())
                    {
                        PostFixText = "Command";
                    }

                    // Offset from the start is name + value
                    const float PostFixOffset = VariableNameWidth + VariableValueWidth;
                    ImGui::SameLine(PostFixOffset);

                    // PostFix
                    ImGui::Text("[%s]", PostFixText);

                    const float SetByOffset = PostFixOffset + PostFixTextLength + 20.0f * Scale;
                    if (ConsoleVariable)
                    {
                        ImGui::SameLine(SetByOffset);
                        ImGui::Text("[%s]", SetByText);
                    }

                    const float HelpStringOffset = SetByOffset + SetByTextLength + 20.0f * Scale;
                    ImGui::SameLine(HelpStringOffset);

                    const char* HelpString = Candidate.First->GetHelpString();
                    ImGui::Text(" [Help: %s]", HelpString);

                    ImGui::PopStyleColor();

                    ImGui::PopID();

                    // Check if we need to scroll to the current selected item
                    if (bIsActiveIndex && bCandidateSelectionChanged)
                    {
                        // Only scroll if the selectable was is not visible
                        if (!bIsSelectableVisible)
                        {
                            ImGui::SetScrollHereY(0.0f);
                        }

                        bCandidateSelectionChanged = false;
                    }
                }

                ImGui::PopStyleVar();

                ImGui::PopStyleColor();
                ImGui::PopStyleColor();

                ImGui::PopAllowKeyboardFocus();
            }
            else
            {
                SCOPED_LOCK(MessagesCS);

                const auto GetColorFromLogSeverity = [](ELogSeverity Severity)
                {
                    switch (Severity)
                    {
                        case ELogSeverity::Info:    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        case ELogSeverity::Warning: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                        case ELogSeverity::Error:   return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                        default:                    return ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
                    }
                };

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 8.0f));

                for (const FConsoleMessage& Text : Messages)
                {
                    const ImVec4 Color = GetColorFromLogSeverity(Text.Severity);
                    ImGui::TextColored(Color, "%s", *Text.Message);
                }

                ImGui::PopStyleVar();

                // Scroll down so that the last text is visible
                if (bShouldScrollText)
                {
                    ImGui::SetScrollHereY(1.0f);
                    bShouldScrollText = false;
                }
            }

            ImGui::EndChild();
        }

        // Text Input
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, Transparency));

            const float DummyTextInputPadding = 6.0f;
            ImGui::Dummy(ImVec2(0.0f, DummyTextInputPadding));

            const float TextInputWidth = TotalWidth - (WindowPadding.x * 2.0f);
            ImGui::PushItemWidth(TextInputWidth);

            const ImGuiInputTextFlags InputFlags =
                ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_CallbackCompletion |
                ImGuiInputTextFlags_CallbackHistory |
                ImGuiInputTextFlags_CallbackAlways |
                ImGuiInputTextFlags_CallbackEdit;

            // Prepare callback for ImGui
            const auto TextInputCallback = [](ImGuiInputTextCallbackData* CallbackData)
            {
                FInGameConsoleWidget* ConsoleWidget = reinterpret_cast<FInGameConsoleWidget*>(CallbackData->UserData);
                return ConsoleWidget->InputTextCallback(CallbackData);
            };

            // Always set keyboard focus to the input field
            ImGui::SetKeyboardFocusHere();

            // Actually draw and handle text input... (Most logic happens in the callback)
            const bool bResult = ImGui::InputText("###Input", TextBuffer.Data(), TextBuffer.Size(), InputFlags, TextInputCallback, reinterpret_cast<void*>(this));
           
            // ImGui::InputText returns true when enter is pressed (see. ImGuiInputTextFlags_EnterReturnsTrue) ...
            if (bResult)
            {
                // ... if the text-buffer has some input we handle that ...
                if (TextBuffer[0] != 0)
                {
                    if (SelectedCandidateIndex >= 0)
                    {
                        CHECK(Candidates.IsEmpty() == false);

                        const FString& NewTextData = Candidates[SelectedCandidateIndex].Second;
                        FCString::Strcpy(TextBuffer.Data(), *NewTextData);
                        bUpdateCursorPosition = true;
                    }
                    else
                    {
                        const FString Text = FString(TextBuffer.Data());
                        FConsoleManager::Get().ExecuteCommand(*this, Text);

                        TextBuffer[0] = 0;
                        bShouldScrollText = true;
                    }
                }

                // ... however, we always clear the candidates
                InvalidateCandidates();
            }

            ImGui::PopItemWidth();

            ImGui::Dummy(ImVec2(0.0f, DummyTextInputPadding));

            ImGui::PopStyleColor();
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

        ImGui::End();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}

void FInGameConsoleWidget::Log(const FString& Message)
{
    Log(ELogSeverity::Info, Message);
}

void FInGameConsoleWidget::Log(ELogSeverity Severity, const FString& Message)
{
    SCOPED_LOCK(MessagesCS);

    constexpr int32 MaxMessages = 100;

    Messages.Emplace(Message, Severity);
    if (Messages.Size() > MaxMessages)
    {
        Messages.RemoveAt(0);
    }

    bShouldScrollText = true;
}

void FInGameConsoleWidget::InvalidateCandidates()
{
    SelectedCandidateIndex = InvalidIndex;
    bCandidateSelectionChanged = true;
    Candidates.Clear();
}

int32 FInGameConsoleWidget::InputTextCallback(ImGuiInputTextCallbackData* CallbackData)
{
    // If we have completed using the Enter key, then we need to update the cursor-position
    if (bUpdateCursorPosition)
    {
        CallbackData->CursorPos = CallbackData->BufTextLen;
        bUpdateCursorPosition = false;
    }

    switch (CallbackData->EventFlag)
    {
        // This callback is called whenever we edit the text in the InputText field
        case ImGuiInputTextFlags_CallbackEdit:
        {
            const char* WordEnd   = CallbackData->Buf + CallbackData->CursorPos;
            const char* WordStart = WordEnd;

            while (WordStart > CallbackData->Buf)
            {
                const char CurrentChar = WordStart[-1];
                if (CurrentChar == ' ' || CurrentChar == '\t' || CurrentChar == ',' || CurrentChar == ';')
                {
                    break;
                }

                WordStart--;
            }

            // When we edit we want to search for new candidates, we do this by "reseting" candidate index and array
            InvalidateCandidates();

            const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
            if (WordLength > 0)
            {
                const FStringView CandidateName(WordStart, WordLength);
                FConsoleManager::Get().FindCandidates(CandidateName, Candidates);

                // If we found any candidates, then want to reset the history index, otherwise the index will be the 
                // the same if we erase the text in the input-field and start iterating through the history.
                if (!Candidates.IsEmpty())
                {
                    HistoryIndex = InvalidIndex;
                }
            }
            else
            {
                // If we have deleted all characters, then we want to scroll the console text down to the latest again
                bShouldScrollText = true;
            }

            break;
        }

        // This callback is called when we press TAB when the console InputText field has keyboard- focus
        case ImGuiInputTextFlags_CallbackCompletion:
        {
            const char* WordEnd   = CallbackData->Buf + CallbackData->CursorPos;
            const char* WordStart = WordEnd;

            if (CallbackData->BufTextLen > 0)
            {
                while (WordStart > CallbackData->Buf)
                {
                    const char CurrentChar = WordStart[-1];
                    if (CurrentChar == ' ' || CurrentChar == '\t' || CurrentChar == ',' || CurrentChar == ';')
                    {
                        break;
                    }

                    WordStart--;
                }
            }

            const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
            if (WordLength > 0)
            {
                // We might not have a selection-index if we have not pressed the up- or down-key
                if (!Candidates.IsEmpty() && SelectedCandidateIndex > InvalidIndex)
                {
                    const int32 Pos   = static_cast<int32>(WordStart - CallbackData->Buf);
                    const int32 Count = WordLength;

                    const FString& NewTextData = Candidates[SelectedCandidateIndex].Second;
                    CallbackData->DeleteChars(Pos, Count);
                    CallbackData->InsertChars(CallbackData->CursorPos, *NewTextData);

                    // If we used tried to complete the command and there actually was text, then we want to clear the candidate array
                    InvalidateCandidates();

                    // During completion, we also want to scroll down after drawing the text-history, since the candidates field will 
                    // no longer be shown.
                    bShouldScrollText = true;
                }
            }

            break;
        }

        // We have two options when this callback is called (I.e when the user presses either up or down arrows on the keyboard)
        //  1. We want to select one of the options that fits what we have type (auto-completion)
        //  2. We want to use a previous command
        case ImGuiInputTextFlags_CallbackHistory:
        {
            // If the candidates are empty, that means that we have not types anything into the input line
            // so this will route the arrow-keys to flip through the history (The commands previously used)
            if (Candidates.IsEmpty())
            {
                // If we have no candidates we should have and invalid candidate-index
                CHECK(SelectedCandidateIndex == InvalidIndex);

                const int32 PrevHistoryIndex = HistoryIndex;

                const TArray<FString>& History = FConsoleManager::Get().GetHistory();
                if (!History.IsEmpty())
                {
                    // If we have any console-history then we can go through the history by pressing the down-arrow key
                    if (CallbackData->EventKey == ImGuiKey_UpArrow)
                    {
                        // If we have a history-index (non-invalid index), this means that we have already started to go through the 
                        // console history and we can just decrement the index.
                        if (HistoryIndex != InvalidIndex)
                        {
                            HistoryIndex--;

                            // If we try and go further than we have indices, then we clamp the index to zero
                            if (HistoryIndex < 0)
                            {
                                HistoryIndex = 0;
                            }
                        }
                        else
                        {
                            // If we currently have not pressed the history, then we set the index to the last history entry
                            HistoryIndex = History.LastElementIndex();
                        }
                    }
                    else if (CallbackData->EventKey == ImGuiKey_DownArrow)
                    {
                        // if the up arrow is pressed, then we need to have already pressed the down-arrow,
                        // otherwise we do not do anything.

                        if (HistoryIndex != InvalidIndex)
                        {
                            HistoryIndex++;

                            // If we try and go beyond the history array then we "exit" going through the history
                            if (HistoryIndex >= History.Size())
                            {
                                HistoryIndex = InvalidIndex;
                            }
                        }
                    }
                }
                else
                {
                    // If we have no history, then we set the index to invalid
                    HistoryIndex = InvalidIndex;
                }

                // If the index changed then we insert the new history text into the text-field
                if (PrevHistoryIndex != HistoryIndex)
                {
                    // If the history-index is invalid, then we clear the input-text field
                    const char* HistoryStr = (HistoryIndex >= 0) ? *History[HistoryIndex] : "";
                    CallbackData->DeleteChars(0, CallbackData->BufTextLen);
                    CallbackData->InsertChars(0, HistoryStr);
                }
            }
            else
            {
                // If we have candidates we should have an invalid history-index
                CHECK(HistoryIndex == InvalidIndex);

                const int32 PreviousSelectedCandidateIndex = SelectedCandidateIndex;
                if (CallbackData->EventKey == ImGuiKey_UpArrow)
                {
                    if (SelectedCandidateIndex <= 0)
                    {
                        SelectedCandidateIndex = Candidates.LastElementIndex();
                    }
                    else
                    {
                        SelectedCandidateIndex--;
                    }
                }
                else if (CallbackData->EventKey == ImGuiKey_DownArrow)
                {
                    if (SelectedCandidateIndex >= Candidates.LastElementIndex())
                    {
                        SelectedCandidateIndex = 0;
                    }
                    else
                    {
                        SelectedCandidateIndex++;
                    }
                }

                if (PreviousSelectedCandidateIndex != SelectedCandidateIndex)
                {
                    bCandidateSelectionChanged = true;
                }
            }

            break;
        }
    }

    return 0;
}

void FInGameConsoleWidget::HandleKeyPressedEvent(const FKeyEvent& Event)
{
    CHECK(InputHandler.IsValid());

    if (Event.IsDown())
    {
        const bool bIsEnableKey = Event.GetKey() == EKeys::GraveAccent || Event.GetKey() == EKeys::World1;
        if (!Event.IsRepeat() && bIsEnableKey)
        {
            bIsActive = !bIsActive;
            InputHandler->bConsoleToggled = bIsActive;

            HistoryIndex           = InvalidIndex;
            SelectedCandidateIndex = InvalidIndex;
        }
    }
}
