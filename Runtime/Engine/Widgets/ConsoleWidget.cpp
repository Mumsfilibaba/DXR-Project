#include "ConsoleWidget.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Templates/CString.h"
#include "Core/Threading/ScopedLock.h"
#include "Application/ApplicationInterface.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

FConsoleWidget::FConsoleWidget()
    : IOutputDevice()
    , InputHandler(MakeSharedPtr<FConsoleInputHandler>())
    , ImGuiDelegateHandle()
    , PopupSelectedText()
    , Candidates()
    , SelectedCandidateIndex(-1)
    , HistoryIndex(-1)
    , Messages()
    , MessagesCS()
    , TextBuffer() 
    , bUpdateCursorPosition(false)
    , bIsActive(false)
    , bCandidateSelectionChanged(false)
    , bScrollDown(false)
{
    if (FOutputDeviceLogger* OutputDeviceManager = FOutputDeviceLogger::Get())
    {
        OutputDeviceManager->RegisterOutputDevice(this);
    }

    if (FApplicationInterface::IsInitialized())
    {
        InputHandler->HandleKeyEventDelegate.BindRaw(this, &FConsoleWidget::HandleKeyPressedEvent);
        FApplicationInterface::Get().RegisterInputHandler(InputHandler);
    }

    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FConsoleWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }

    TextBuffer.Fill(0);
}

FConsoleWidget::~FConsoleWidget()
{
    if (FOutputDeviceLogger* OutputDeviceManager = FOutputDeviceLogger::Get())
    {
        OutputDeviceManager->UnregisterOutputDevice(this);
    }

    if (FApplicationInterface::IsInitialized())
    {
        FApplicationInterface::Get().UnregisterInputHandler(InputHandler);
    }

    if (IImguiPlugin::IsEnabled())
    {
         IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FConsoleWidget::Draw()
{
    if (!bIsActive)
    {
        return;
    }

    const ImVec2 MainViewportPos  = ImGuiExtensions::GetMainViewportPos();
    const ImVec2 MainViewportSize = ImGuiExtensions::GetMainViewportSize();
    const ImVec2 FrameBufferScale = ImGuiExtensions::GetDisplayFramebufferScale();

    const float Scale          = FrameBufferScale.x;
    const float TotalWidth     = MainViewportSize.x;
    const float TextAreaHeight = 384.0f * Scale;

    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

    const ImGuiStyle& Style = ImGui::GetStyle();

    const ImVec4 WindowBG = Style.Colors[ImGuiCol_WindowBg];
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(WindowBG.x, WindowBG.y, WindowBG.z, 0.7f));

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
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));

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
                    if (ImGui::Selectable(*Candidate.Second, &bIsActiveIndex, ImGuiSelectableFlags_None, SelectableSize))
                    {
                        FCString::Strcpy(TextBuffer.Data(), *Candidate.Second);
                        PopupSelectedText = Candidate.Second;

                        Candidates.Clear();
                        SelectedCandidateIndex = -1;

                        bUpdateCursorPosition = true;

                        ImGui::PopID();
                        break;
                    }

                    ImGui::SameLine(VariableNameWidth);

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

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

                    const CHAR* HelpString = Candidate.First->GetHelpString();
                    ImGui::Text(" [Help: %s]", HelpString);

                    ImGui::PopStyleColor();

                    ImGui::PopID();

                    if (bIsActiveIndex && bCandidateSelectionChanged)
                    {
                        ImGui::SetScrollHereY();

                        PopupSelectedText = Candidate.Second;
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

                for (const TPair<FString, ELogSeverity>& Text : Messages)
                {
                    const ImVec4 Color = GetColorFromLogSeverity(Text.Second);
                    ImGui::TextColored(Color, "%s", *Text.First);
                }

                ImGui::PopStyleVar();

                if (bScrollDown)
                {
                    ImGui::SetScrollHereY();
                    bScrollDown = false;
                }
            }

            ImGui::EndChild();
        }

        // Text Input
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));

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
                FConsoleWidget* ConsoleWidget = reinterpret_cast<FConsoleWidget*>(CallbackData->UserData);
                return ConsoleWidget->TextCallback(CallbackData);
            };

            const bool bResult = ImGui::InputText("###Input", TextBuffer.Data(), TextBuffer.Size(), InputFlags, TextInputCallback, reinterpret_cast<void*>(this));
            if (bResult && TextBuffer[0] != 0)
            {
                if (SelectedCandidateIndex >= 0)
                {
                    FCString::Strcpy(TextBuffer.Data(), *PopupSelectedText);

                    SelectedCandidateIndex = -1;
                    bUpdateCursorPosition  = true;

                    Candidates.Clear();
                }
                else
                {
                    const FString Text = FString(TextBuffer.Data());
                    FConsoleManager::Get().ExecuteCommand(*this, Text);

                    TextBuffer[0] = 0;
                    bScrollDown   = true;

                    ImGui::SetItemDefaultFocus();
                    ImGui::SetKeyboardFocusHere(-1);
                }
            }

            if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
            {
                ImGui::SetKeyboardFocusHere(-1);
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

void FConsoleWidget::Log(const FString& Message)
{
    Log(ELogSeverity::Info, Message);
}

void FConsoleWidget::Log(ELogSeverity Severity, const FString& Message)
{
    SCOPED_LOCK(MessagesCS);

    constexpr int32 MaxMessages = 100;

    // Insert in the beginning to get the correct order
    Messages.Add(MakePair<FString, ELogSeverity>(Message, Severity));

    if (Messages.Size() > MaxMessages)
    {
        Messages.RemoveAt(0);
    }

    bScrollDown = true;
}

int32 FConsoleWidget::TextCallback(ImGuiInputTextCallbackData* CallbackData)
{
    if (bUpdateCursorPosition)
    {
        CallbackData->CursorPos = int32(PopupSelectedText.Length());
        bUpdateCursorPosition = false;

        PopupSelectedText.Clear();
    }

    switch (CallbackData->EventFlag)
    {
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

            Candidates.Clear();

            bCandidateSelectionChanged = true;
            SelectedCandidateIndex = -1;

            const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
            if (WordLength > 0)
            {
                const FStringView CandidateName(WordStart, WordLength);
                FConsoleManager::Get().FindCandidates(CandidateName, Candidates);
            }

            break;
        }
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
                if (Candidates.Size() == 1)
                {
                    const int32 Pos   = static_cast<int32>(WordStart - CallbackData->Buf);
                    const int32 Count = WordLength;

                    CallbackData->DeleteChars(Pos, Count);
                    CallbackData->InsertChars(CallbackData->CursorPos, *Candidates[0].Second);

                    SelectedCandidateIndex = -1;
                    bCandidateSelectionChanged = true;

                    Candidates.Clear();
                }
                else if (!Candidates.IsEmpty() && SelectedCandidateIndex != -1)
                {
                    const int32 Pos   = static_cast<int32>(WordStart - CallbackData->Buf);
                    const int32 Count = WordLength;

                    CallbackData->DeleteChars(Pos, Count);
                    CallbackData->InsertChars(CallbackData->CursorPos, *PopupSelectedText);

                    PopupSelectedText = "";

                    SelectedCandidateIndex = -1;
                    bCandidateSelectionChanged = true;

                    Candidates.Clear();
                }
            }

            break;
        }
        case ImGuiInputTextFlags_CallbackHistory:
        {
            if (Candidates.IsEmpty())
            {
                const TArray<FString>& History = FConsoleManager::Get().GetHistory();
                if (History.IsEmpty())
                {
                    HistoryIndex = -1;
                }

                const int32 PrevHistoryIndex = HistoryIndex;
                if (CallbackData->EventKey == ImGuiKey_UpArrow)
                {
                    if (HistoryIndex == -1)
                    {
                        HistoryIndex = History.Size() - 1;
                    }
                    else if (HistoryIndex > 0)
                    {
                        HistoryIndex--;
                    }
                }
                else if (CallbackData->EventKey == ImGuiKey_DownArrow)
                {
                    if (HistoryIndex != -1)
                    {
                        HistoryIndex++;
                        if (HistoryIndex >= static_cast<int32>(History.Size()))
                        {
                            HistoryIndex = -1;
                        }
                    }
                }

                if (PrevHistoryIndex != HistoryIndex)
                {
                    const char* HistoryStr = (HistoryIndex >= 0) ? *History[HistoryIndex] : "";
                    CallbackData->DeleteChars(0, CallbackData->BufTextLen);
                    CallbackData->InsertChars(0, HistoryStr);
                }
            }
            else
            {
                if (CallbackData->EventKey == ImGuiKey_UpArrow)
                {
                    bCandidateSelectionChanged = true;
                    if (SelectedCandidateIndex <= 0)
                    {
                        SelectedCandidateIndex = Candidates.Size() - 1;
                    }
                    else
                    {
                        SelectedCandidateIndex--;
                    }
                }
                else if (CallbackData->EventKey == ImGuiKey_DownArrow)
                {
                    bCandidateSelectionChanged = true;
                    if (SelectedCandidateIndex >= int32(Candidates.Size()) - 1)
                    {
                        SelectedCandidateIndex = 0;
                    }
                    else
                    {
                        SelectedCandidateIndex++;
                    }
                }
            }

            break;
        }
    }

    return 0;
}

void FConsoleWidget::HandleKeyPressedEvent(const FKeyEvent& Event)
{
    CHECK(InputHandler.IsValid());

    if (Event.IsDown())
    {
        const bool bIsEnableKey = Event.GetKey() == EKeys::GraveAccent || Event.GetKey() == EKeys::World1;
        if (!Event.IsRepeat() && bIsEnableKey)
        {
            bIsActive = !bIsActive;
            InputHandler->bConsoleToggled = bIsActive;
        }
    }
}
