#include "GameConsoleWindow.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Templates/CString.h"
#include "Core/Threading/ScopedLock.h"
#include "Application/Application.h"

FGameConsoleWindow::FGameConsoleWindow()
    : FWidget()
    , IOutputDevice()
    , InputHandler(MakeShared<FConsoleInputHandler>())
{
	if (auto OutputDeviceManager = FOutputDeviceLogger::Get())
	{
        OutputDeviceManager->AddOutputDevice(this);
    }

    if (FWindowedApplication::IsInitialized())
    {
        InputHandler->HandleKeyEventDelegate.BindRaw(this, &FGameConsoleWindow::HandleKeyPressedEvent);
        FWindowedApplication::Get().AddInputHandler(InputHandler, uint32(-1));
    }

    TextBuffer.Fill(0);
}

FGameConsoleWindow::~FGameConsoleWindow()
{
    if (auto OutputDeviceManager = FOutputDeviceLogger::Get())
    {
        OutputDeviceManager->RemoveOutputDevice(this);
    }

    if (FWindowedApplication::IsInitialized())
    {
        FWindowedApplication::Get().RemoveInputHandler(InputHandler);
    }
}

void FGameConsoleWindow::Paint(const FRectangle& AssignedBounds)
{
    UNREFERENCED_VARIABLE(AssignedBounds);

    if (!bIsActive)
    {
        return;
    }

    ImGuiIO& GuiIO = ImGui::GetIO();
    const float Scale  = GuiIO.DisplayFramebufferScale.y;
    const float Width  = float(GuiIO.DisplaySize.x);
    const float Height = 256.0f * Scale;

    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
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
                VariableNameWidth = NMath::Max(VariableNameWidth, ImGui::CalcTextSize(Candidate.Second.GetCString()).x);

                if (IConsoleVariable* Variable = Candidate.First->AsVariable())
                {
                    const FString Value = Variable->GetString();
                    VariableValueWidth = NMath::Max(VariableValueWidth, ImGui::CalcTextSize(Value.GetCString()).x);
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
                if (ImGui::Selectable(Candidate.Second.GetCString(), &bIsActiveIndex))
                {
                    FCString::Strcpy(TextBuffer.Data(), Candidate.Second.GetCString());
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
                const CHAR* SetBy  = "";

                // Value
                static float PostFixSize = 
                    NMath::Max(ImGui::CalcTextSize("Bool").x,
                    NMath::Max(ImGui::CalcTextSize("Int").x,
                    NMath::Max(ImGui::CalcTextSize("Float").x,
                    ImGui::CalcTextSize("String").x)));

                static float SetBySize = 
                    NMath::Max(ImGui::CalcTextSize(SetByFlagToString(EConsoleVariableFlags::SetByConstructor)).x,
                    NMath::Max(ImGui::CalcTextSize(SetByFlagToString(EConsoleVariableFlags::SetByCommandLine)).x,
                    NMath::Max(ImGui::CalcTextSize(SetByFlagToString(EConsoleVariableFlags::SetByConfigFile)).x,
                    NMath::Max(ImGui::CalcTextSize(SetByFlagToString(EConsoleVariableFlags::SetByCode)).x,
                    ImGui::CalcTextSize(SetByFlagToString(EConsoleVariableFlags::SetByConsole)).x))));

                IConsoleVariable* ConsoleVariable = Candidate.First->AsVariable();
                if (ConsoleVariable)
                {
                    const FString Value = ConsoleVariable->GetString();
                    ImGui::Text("%s", Value.GetCString());

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

                ImGui::TextColored(Color, "%s", Text.First.GetCString());
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
            FGameConsoleWindow* This = reinterpret_cast<FGameConsoleWindow*>(Data->UserData);
            return This->TextCallback(Data);
        };

        const bool bResult = ImGui::InputText(
            "###Input",
            TextBuffer.Data(),
            TextBuffer.Size(),
            InputFlags, 
            Callback, 
            reinterpret_cast<void*>(this));

        if (bResult && TextBuffer[0] != 0)
        {
            if (CandidatesIndex != -1)
            {
                FCString::Strcpy(TextBuffer.Data(), PopupSelectedText.GetCString());

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
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

    }

    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

void FGameConsoleWindow::Log(const FString& Message)
{
    Log(ELogSeverity::Info, Message);
}

void FGameConsoleWindow::Log(ELogSeverity Severity, const FString& Message)
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

int32 FGameConsoleWindow::TextCallback(ImGuiInputTextCallbackData* Data)
{
    if (bUpdateCursorPosition)
    {
        Data->CursorPos = int32(PopupSelectedText.GetLength());
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
                Data->InsertChars(Data->CursorPos, Candidates[0].Second.GetCString());

                CandidatesIndex = -1;
                bCandidateSelectionChanged = true;
                Candidates.Clear();
            }
            else if (!Candidates.IsEmpty() && CandidatesIndex != -1)
            {
                const int32 Pos = static_cast<int32>(WordStart - Data->Buf);
                const int32 Count = WordLength;
                Data->DeleteChars(Pos, Count);
                Data->InsertChars(Data->CursorPos, PopupSelectedText.GetCString());

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
                const CHAR* HistoryStr = (HistoryIndex >= 0) ? History[HistoryIndex].GetCString() : "";
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

void FGameConsoleWindow::HandleKeyPressedEvent(const FKeyEvent& Event)
{
    CHECK(InputHandler.IsValid());
    InputHandler->bConsoleToggled = false;

    if (Event.IsDown())
    {
        if (!Event.IsRepeat() && Event.GetKey() == EKey::Key_GraveAccent)
        {
            bIsActive = !bIsActive;
            InputHandler->bConsoleToggled = true;
        }
    }
}