#include "ConsoleWidget.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Templates/CString.h"
#include "Core/Threading/ScopedLock.h"
#include "Application/Application.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

FConsoleWidget::FConsoleWidget()
    : IOutputDevice()
    , InputHandler(MakeSharedPtr<FConsoleInputHandler>())
    , ImGuiDelegateHandle()
    , PopupSelectedText()
    , Candidates()
    , CandidatesIndex(-1)
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

    const float Scale  = FrameBufferScale.x;
    const float Width  = MainViewportSize.x;
    const float Height = 256.0f * Scale;

    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

    const ImGuiStyle& Style = ImGui::GetStyle();
    ImVec4 WindowBG = Style.Colors[ImGuiCol_WindowBg];
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{ WindowBG.x, WindowBG.y, WindowBG.z, 0.8f });

    ImGui::SetNextWindowPos(MainViewportPos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, 0.0f), ImGuiCond_Always);

    const ImGuiWindowFlags StyleFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * Scale, 10.0f * Scale));

    if (ImGui::Begin("Console Window", nullptr, StyleFlags))
    {
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));

        const ImGuiWindowFlags PopupFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing;

        ImGui::BeginChild("##ChildWindow", ImVec2(Width, Height), false, PopupFlags);
        if (!Candidates.IsEmpty())
        {
            bool bIsActiveIndex = false;

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
            ImGui::PushAllowKeyboardFocus(false);

            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

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
            for (int32 i = 0; i < Candidates.Size(); i++)
            {
                const TPair<IConsoleObject*, FString>& Candidate = Candidates[i];
                bIsActiveIndex = (CandidatesIndex == i);

                // VariableName
                ImGui::PushID(i);
                if (ImGui::Selectable(*Candidate.Second, &bIsActiveIndex))
                {
                    FCString::Strcpy(TextBuffer.Data(), *Candidate.Second);
                    PopupSelectedText = Candidate.Second;

                    Candidates.Clear();
                    CandidatesIndex = -1;

                    bUpdateCursorPosition = true;

                    ImGui::PopID();
                    break;
                }

                ImGui::SameLine(VariableNameWidth);

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

                const CHAR* PostFix = "";
                const CHAR* SetBy   = "";

                // Value
                static float PostFixSize = 
                    FMath::Max(ImGui::CalcTextSize("Bool").x,
                    FMath::Max(ImGui::CalcTextSize("Int").x,
                    FMath::Max(ImGui::CalcTextSize("Float").x,
                    ImGui::CalcTextSize("String").x)));

                static float SetBySize = 
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
                        PostFix = "Bool";
                    }
                    else if (ConsoleVariable->IsVariableInt())
                    {
                        PostFix = "Int";
                    }
                    else if (ConsoleVariable->IsVariableFloat())
                    {
                        PostFix = "Float";
                    }
                    else if (ConsoleVariable->IsVariableString())
                    {
                        PostFix = "String";
                    }

                    const EConsoleVariableFlags VariableFlags = (ConsoleVariable->GetFlags() & EConsoleVariableFlags::SetByMask);
                    SetBy = SetByFlagToString(VariableFlags);
                }
                else if (Candidate.First->AsCommand())
                {
                    PostFix = "Command";
                }

                // Offset from the start is name + value
                const float PostFixOffset = VariableNameWidth + VariableValueWidth;
                ImGui::SameLine(PostFixOffset);

                // PostFix
                ImGui::Text("[%s]", PostFix);

                const float SetByOffset = PostFixOffset + PostFixSize + 20.0f * Scale;
                if (ConsoleVariable)
                {
                    ImGui::SameLine(SetByOffset);
                    ImGui::Text("[%s]", SetBy);
                }

                const float HelpStringOffset = SetByOffset + SetBySize + 20.0f * Scale;
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

            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            ImGui::PopAllowKeyboardFocus();
            ImGui::PopStyleVar();
        }
        else
        {
            SCOPED_LOCK(MessagesCS);
            
            for (const TPair<FString, ELogSeverity>& Text : Messages)
            {
                ImVec4 Color;
                if (Text.Second == ELogSeverity::Info)
                {
                    Color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                }
                else if (Text.Second == ELogSeverity::Warning)
                {
                    Color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                }
                else if (Text.Second == ELogSeverity::Error)
                {
                    Color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                }

                ImGui::TextColored(Color, "%s", *Text.First);
            }

            if (bScrollDown)
            {
                ImGui::SetScrollHereY();
                bScrollDown = false;
            }
        }

        ImGui::EndChild();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));

        // Draw the Input Sign for the text input 
        {
            ImVec2 CursorPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(CursorPos.x, CursorPos.y + 2.0f * Scale));

            ImGui::Text(">");
            ImGui::SameLine();

            CursorPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(CursorPos.x, CursorPos.y - 2.0f * Scale));
        }

        // Text Input
        ImGui::PushItemWidth(Width - 32.0f * Scale);

        const ImGuiInputTextFlags InputFlags =
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_CallbackCompletion |
            ImGuiInputTextFlags_CallbackHistory |
            ImGuiInputTextFlags_CallbackAlways |
            ImGuiInputTextFlags_CallbackEdit;

        // Prepare callback for ImGui
        auto Callback = [](ImGuiInputTextCallbackData* Data) -> int32
        {
            FConsoleWidget* This = reinterpret_cast<FConsoleWidget*>(Data->UserData);
            return This->TextCallback(Data);
        };

        const bool bResult = ImGui::InputText("###Input", TextBuffer.Data(), TextBuffer.Size(), InputFlags, Callback, reinterpret_cast<void*>(this));
        if (bResult && TextBuffer[0] != 0)
        {
            if (CandidatesIndex != -1)
            {
                FCString::Strcpy(TextBuffer.Data(), *PopupSelectedText);

                Candidates.Clear();
                CandidatesIndex = -1;
                bUpdateCursorPosition = true;
            }
            else
            {
                const FString Text = FString(TextBuffer.Data());
                FConsoleManager::Get().ExecuteCommand(*this, Text);

                TextBuffer[0] = 0;
                bScrollDown = true;

                ImGui::SetItemDefaultFocus();
                ImGui::SetKeyboardFocusHere(-1);
            }
        }

        if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
        {
            ImGui::SetKeyboardFocusHere(-1);
        }

        ImGui::PopItemWidth();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
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

int32 FConsoleWidget::TextCallback(ImGuiInputTextCallbackData* Data)
{
    if (bUpdateCursorPosition)
    {
        Data->CursorPos = int32(PopupSelectedText.Length());
        PopupSelectedText.Clear();
        bUpdateCursorPosition = false;
    }

    switch (Data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackEdit:
    {
        const CHAR* WordEnd   = Data->Buf + Data->CursorPos;
        const CHAR* WordStart = WordEnd;
        while (WordStart > Data->Buf)
        {
            const CHAR c = WordStart[-1];
            if (c == ' ' || c == '\t' || c == ',' || c == ';')
            {
                break;
            }

            WordStart--;
        }

        Candidates.Clear();
        bCandidateSelectionChanged = true;
        CandidatesIndex = -1;

        const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
        if (WordLength <= 0)
        {
            break;
        }

        const FStringView CandidateName(WordStart, WordLength);
        FConsoleManager::Get().FindCandidates(CandidateName, Candidates);
        break;
    }
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        const CHAR* WordEnd = Data->Buf + Data->CursorPos;
        const CHAR* WordStart = WordEnd;
        if (Data->BufTextLen > 0)
        {
            while (WordStart > Data->Buf)
            {
                const CHAR c = WordStart[-1];
                if (c == ' ' || c == '\t' || c == ',' || c == ';')
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
                const int32 Pos = static_cast<int32>(WordStart - Data->Buf);
                const int32 Count = WordLength;
                Data->DeleteChars(Pos, Count);
                Data->InsertChars(Data->CursorPos, *Candidates[0].Second);

                CandidatesIndex = -1;
                bCandidateSelectionChanged = true;
                Candidates.Clear();
            }
            else if (!Candidates.IsEmpty() && CandidatesIndex != -1)
            {
                const int32 Pos = static_cast<int32>(WordStart - Data->Buf);
                const int32 Count = WordLength;
                Data->DeleteChars(Pos, Count);
                Data->InsertChars(Data->CursorPos, *PopupSelectedText);

                PopupSelectedText = "";

                Candidates.Clear();
                CandidatesIndex = -1;
                bCandidateSelectionChanged = true;
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
            if (Data->EventKey == ImGuiKey_UpArrow)
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
            else if (Data->EventKey == ImGuiKey_DownArrow)
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
                const CHAR* HistoryStr = (HistoryIndex >= 0) ? *History[HistoryIndex] : "";
                Data->DeleteChars(0, Data->BufTextLen);
                Data->InsertChars(0, HistoryStr);
            }
        }
        else
        {
            if (Data->EventKey == ImGuiKey_UpArrow)
            {
                bCandidateSelectionChanged = true;
                if (CandidatesIndex <= 0)
                {
                    CandidatesIndex = Candidates.Size() - 1;
                }
                else
                {
                    CandidatesIndex--;
                }
            }
            else if (Data->EventKey == ImGuiKey_DownArrow)
            {
                bCandidateSelectionChanged = true;
                if (CandidatesIndex >= int32(Candidates.Size()) - 1)
                {
                    CandidatesIndex = 0;
                }
                else
                {
                    CandidatesIndex++;
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
    InputHandler->bConsoleToggled = false;

    if (Event.IsDown())
    {
        const bool bIsEnableKey = Event.GetKey() == EKeys::GraveAccent || Event.GetKey() == EKeys::World1;
        if (!Event.IsRepeat() && bIsEnableKey)
        {
            bIsActive = !bIsActive;
            InputHandler->bConsoleToggled = true;
        }
    }
}
