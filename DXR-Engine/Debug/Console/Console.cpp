#include "Console.h"

#include "Rendering/DebugUI.h"

#include "Main/EngineLoop.h"

#include "Core/Application/Application.h"

#include <regex>

Console GConsole;

ConsoleCommand GClearHistory;

void Console::Init()
{
    GClearHistory.OnExecute.AddObject(this, &Console::ClearHistory);
    INIT_CONSOLE_COMMAND("ClearHistory", &GClearHistory);
    
    GApplication->OnKeyPressedEvent.AddObject(this, &Console::OnKeyPressedEvent);
}

void Console::Tick()
{
    if (IsActive)
    {
        DebugUI::DrawUI([]()
        {
            const uint32 WindowWidth  = GApplication->Window->GetWidth();
            const uint32 WindowHeight = GApplication->Window->GetHeight();
            const float Width         = float(WindowWidth);
            const float Height        = float(WindowHeight) * 0.125f;

            ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
            ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
            ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(Width, 0.0f), ImGuiCond_FirstUseEver);

            ImGui::SetNextWindowSizeConstraints(ImVec2(Width, 100), ImVec2(Width, WindowHeight * 0.5f));

            ImGui::Begin(
                "Console Window",
                nullptr,
                ImGuiWindowFlags_NoMove            |
                ImGuiWindowFlags_NoTitleBar        |
                ImGuiWindowFlags_NoScrollbar       |
                ImGuiWindowFlags_NoCollapse        |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoSavedSettings);

            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));

            const ImVec2 ParentSize      = ImGui::GetWindowSize();
            const float TextWindowWidth  = Width * 0.985f;
            const float TextWindowHeight = ParentSize.y - 40.0f;
            ImGui::BeginChild("##TextWindow", ImVec2(TextWindowWidth, TextWindowHeight), false, ImGuiWindowFlags_None);

            for (const Line& Text : GConsole.Lines)
            {
                ImGui::TextColored(Text.Color, "%s", Text.String.c_str());
            }

            if (GConsole.ScrollDown)
            {
                ImGui::SetScrollHereY();
                GConsole.ScrollDown = false;
            }

            ImGui::EndChild();

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

            ImGui::Text(">");
            ImGui::SameLine();
            
            ImGui::PushItemWidth(TextWindowWidth - 25.0f);

            const ImGuiInputTextFlags InputFlags =
                ImGuiInputTextFlags_EnterReturnsTrue   |
                ImGuiInputTextFlags_CallbackCompletion |
                ImGuiInputTextFlags_CallbackHistory    |
                ImGuiInputTextFlags_CallbackAlways     |
                ImGuiInputTextFlags_CallbackEdit;

            auto Callback = [](ImGuiInputTextCallbackData* Data)->int32
            {
                Console* This = reinterpret_cast<Console*>(Data->UserData);
                return This->TextCallback(Data);
            };

            const bool Result = ImGui::InputText("###Input", GConsole.TextBuffer.Data(), GConsole.TextBuffer.Size(), InputFlags, Callback, reinterpret_cast<void*>(&GConsole));
            if (Result && GConsole.TextBuffer[0] != 0)
            {
                if (GConsole.CandidatesIndex != -1)
                {
                    strcpy(GConsole.TextBuffer.Data(), GConsole.PopupSelectedText.c_str());
                    GConsole.Candidates.Clear();
                    GConsole.CandidatesIndex      = -1;
                    GConsole.UpdateCursorPosition = true;
                }
                else
                {
                    const std::string Text = std::string(GConsole.TextBuffer.Data());
                    GConsole.HandleCommand(Text);
                    GConsole.TextBuffer[0] = 0;
                    GConsole.ScrollDown    = true;

                    ImGui::SetItemDefaultFocus();
                    ImGui::SetKeyboardFocusHere(-1);
                }
            }

            if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
            {
                ImGui::SetKeyboardFocusHere(-1);
            }

            if (!GConsole.Candidates.IsEmpty())
            {
                bool IsActiveIndex = false;
                bool PopupOpen     = true;
                const ImGuiWindowFlags PopupFlags =
                    ImGuiWindowFlags_NoTitleBar      |
                    ImGuiWindowFlags_NoResize        |
                    ImGuiWindowFlags_NoMove          |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoFocusOnAppearing;

                constexpr uint8 MaxCandidates = 5;

                float SizeY = 0.0f;
                if (GConsole.Candidates.Size() < MaxCandidates)
                {
                    SizeY = (ImGui::GetTextLineHeight() + 10.0f) * GConsole.Candidates.Size();
                }
                else
                {
                    SizeY = ImGui::GetTextLineHeight() * MaxCandidates + 10.0f;
                }

                ImGui::SetNextWindowPos(ImVec2(0.0f, ParentSize.y));

                ImGui::SetNextWindowSize(ImVec2(ParentSize.x, 0.0f));

                ImGui::Begin("CandidatesWindow", &PopupOpen, PopupFlags);

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
                ImGui::PushAllowKeyboardFocus(false);

                float ColumnWidth = 0.0f;
                for (const Candidate& Candidate : GConsole.Candidates)
                {
                    if (Candidate.TextSize.x > ColumnWidth)
                    {
                        ColumnWidth = Candidate.TextSize.x;
                    }
                }

                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

                for (int32 i = 0; i < (int32)GConsole.Candidates.Size(); i++)
                {
                    const Candidate& Candidate = GConsole.Candidates[i];
                    IsActiveIndex = (GConsole.CandidatesIndex == i);
                    
                    ImGui::PushID(i);
                    if (ImGui::Selectable(Candidate.Text.c_str(), &IsActiveIndex))
                    {
                        strcpy(GConsole.TextBuffer.Data(), Candidate.Text.c_str());
                        GConsole.PopupSelectedText = Candidate.Text;
                        
                        GConsole.Candidates.Clear();
                        GConsole.CandidatesIndex      = -1;
                        GConsole.UpdateCursorPosition = true;
                        
                        ImGui::PopID();
                        break;
                    }

                    ImGui::SameLine(ColumnWidth);

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                    ImGui::Text(Candidate.PostFix.c_str());
                    ImGui::PopStyleColor();

                    ImGui::PopID();

                    if (IsActiveIndex && GConsole.CandidateSelectionChanged)
                    {
                        ImGui::SetScrollHere();
                        GConsole.PopupSelectedText         = Candidate.Text;
                        GConsole.CandidateSelectionChanged = false;
                    }
                }

                ImGui::PopStyleColor();
                ImGui::PopStyleColor();

                ImGui::PopAllowKeyboardFocus();
                ImGui::PopStyleVar();
                ImGui::End();
            }

            ImGui::PopItemWidth();

            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            ImGui::End();
        });
    }
}

void Console::RegisterCommand(const std::string& CmdName, ConsoleCommand* Command)
{
    ConsoleCommand* CurrCmd = FindCommand(CmdName);
    if (!CurrCmd)
    {
        const uint32 Index = Commands.Size();
        Commands.EmplaceBack(Command);
        CmdIndexMap[CmdName] = Index;
    }
    else
    {
        LOG_WARNING("ConsoleCommand '" + CmdName + "' is already registered");
    }
}

void Console::RegisterVariable(const std::string& VarName, ConsoleVariable* Variable)
{
    ConsoleVariable* Var = FindVariable(VarName);
    if (!Var)
    {
        const uint32 Index = Variables.Size();
        Variables.EmplaceBack(Variable);
        VarIndexMap[VarName] = Index;
    }
    else
    {
        LOG_WARNING("ConsoleVariable '" + VarName + "' is already registered");
    }
}

ConsoleCommand* Console::FindCommand(const std::string& CmdName)
{
    auto CmdIndex = CmdIndexMap.find(CmdName.c_str());
    if (CmdIndex != CmdIndexMap.end())
    {
        Assert(CmdIndex->second >= 0);
        return Commands[CmdIndex->second];
    }

    return nullptr;
}

ConsoleVariable* Console::FindVariable(const std::string& VarName)
{
    auto VarIndex = VarIndexMap.find(VarName.c_str());
    if (VarIndex != VarIndexMap.end())
    {
        Assert(VarIndex->second >= 0);
        return Variables[VarIndex->second];
    }

    return nullptr;
}

void Console::PrintMessage(const std::string& Message)
{
    GConsole.Lines.EmplaceBack(Message, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void Console::PrintWarning(const std::string& Message)
{
    GConsole.Lines.EmplaceBack(Message, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
}

void Console::PrintError(const std::string& Message)
{
    GConsole.Lines.EmplaceBack(Message, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
}

void Console::ClearHistory()
{
    History.Clear();
    HistoryIndex = -1;
}

void Console::OnKeyPressedEvent(const KeyPressedEvent& Event)
{
    if (!Event.IsRepeat && Event.Key == EKey::Key_GraveAccent)
    {
        IsActive = !IsActive;
    }
}

int32 Console::TextCallback(ImGuiInputTextCallbackData* Data)
{
    if (UpdateCursorPosition)
    {
        Data->CursorPos = int32(PopupSelectedText.length());
        PopupSelectedText.clear();
        UpdateCursorPosition = false;
    }

    switch (Data->EventFlag)
    {
        case ImGuiInputTextFlags_CallbackEdit:
        {
            const char* WordEnd   = Data->Buf + Data->CursorPos;
            const char* WordStart = WordEnd;
            while (WordStart > Data->Buf)
            {
                const char c = WordStart[-1];
                if (c == ' ' || c == '\t' || c == ',' || c == ';')
                {
                    break;
                }

                WordStart--;
            }

            Candidates.Clear();
            CandidatesIndex           = -1;
            CandidateSelectionChanged = true;

            const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
            if (WordLength <= 0)
            {
                break;
            }

            for (const std::pair<std::string, int32>& Index : CmdIndexMap)
            {
                if (WordLength <= Index.first.size())
                {
                    const char* Command = Index.first.c_str();
                    int32 d = -1;
                    int32 n = WordLength;
                
                    const char* CmdIt  = Command;
                    const char* WordIt = WordStart;
                    while (n > 0 && (d = toupper(*WordIt) - toupper(*CmdIt)) == 0)
                    {
                        CmdIt++;
                        WordIt++;
                        n--;
                    }

                    if (d == 0)
                    {
                        Candidates.EmplaceBack(Index.first, "[Cmd]");
                    }
                }
            }

            for (const std::pair<std::string, int32>& Index : VarIndexMap)
            {
                if (WordLength <= Index.first.size())
                {
                    const char* Var = Index.first.c_str();
                    int32 d = -1;
                    int32 n = WordLength;

                    const char* VarIt  = Var;
                    const char* WordIt = WordStart;
                    while (n > 0 && (d = toupper(*WordIt) - toupper(*VarIt)) == 0)
                    {
                        VarIt++;
                        WordIt++;
                        n--;
                    }

                    if (d == 0)
                    {
                        ConsoleVariable* Variable = Variables[Index.second];
                        if (Variable->IsBool())
                        {
                            Candidates.EmplaceBack(Index.first, "= " + std::string(Variable->GetBool() ? "true" : "false") + " [Boolean]");
                        }
                        else if (Variable->IsInt())
                        {
                            Candidates.EmplaceBack(Index.first, "= " + std::to_string(Variable->GetInt32()) + " [Integer]");
                        }
                        else if (Variable->IsFloat())
                        {
                            Candidates.EmplaceBack(Index.first, "= " + std::to_string(Variable->GetFloat()) + " [float]");
                        }
                        else if (Variable->IsString())
                        {
                            Candidates.EmplaceBack(Index.first, "= " + std::string(Variable->GetString()) + " [String]");
                        }
                    }
                }
            }

            break;
        }
        case ImGuiInputTextFlags_CallbackCompletion:
        {
            const char* WordEnd   = Data->Buf + Data->CursorPos;
            const char* WordStart = WordEnd;
            if (Data->BufTextLen > 0)
            {
                while (WordStart > Data->Buf)
                {
                    const char c = WordStart[-1];
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
                    const int32 Pos   = static_cast<int32>(WordStart - Data->Buf);
                    const int32 Count = WordLength;
                    Data->DeleteChars(Pos, Count);
                    Data->InsertChars(Data->CursorPos, Candidates[0].Text.c_str());

                    CandidatesIndex           = -1;
                    CandidateSelectionChanged = true;
                    Candidates.Clear();
                }
                else if (!Candidates.IsEmpty() && CandidatesIndex != -1)
                {
                    const int32 Pos        = static_cast<int32>(WordStart - Data->Buf);
                    const int32 Count    = WordLength;
                    Data->DeleteChars(Pos, Count);
                    Data->InsertChars(Data->CursorPos, PopupSelectedText.c_str());

                    PopupSelectedText         = "";
                    CandidatesIndex           = -1;
                    CandidateSelectionChanged = true;
                    Candidates.Clear();
                }
            }

            break;
        }
        case ImGuiInputTextFlags_CallbackHistory:
        {
            if (Candidates.IsEmpty())
            {
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
                    const char* HistoryStr = (HistoryIndex >= 0) ? History[HistoryIndex].c_str() : "";
                    Data->DeleteChars(0, Data->BufTextLen);
                    Data->InsertChars(0, HistoryStr);
                }
            }
            else
            {
                if (Data->EventKey == ImGuiKey_UpArrow)
                {
                    CandidateSelectionChanged = true;
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
                    CandidateSelectionChanged = true;
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

void Console::HandleCommand(const std::string& CmdString)
{
    PrintMessage(CmdString);

    History.EmplaceBack(CmdString);
    if (History.Size() > HistoryLength)
    {
        History.Erase(History.Begin());
    }

    size_t Pos = CmdString.find_first_of(" ");
    if (Pos == std::string::npos)
    {
        auto CmdIndex = CmdIndexMap.find(CmdString);
        if (CmdIndex != CmdIndexMap.end())
        {
            ConsoleCommand Cmd = Commands[CmdIndex->second];
            Assert(Cmd != nullptr);

            Cmd();
        }
        else
        {
            const std::string Message = "'" + CmdString + "' is not a registered command";
            PrintError(Message);
        }
    }
    else
    {
        std::string Variable(CmdString.c_str(), Pos);
        auto VarIndex = VarIndexMap.find(Variable);
        if (VarIndex == VarIndexMap.end())
        {
            PrintError("'" + CmdString + "' is not a registered variable");
            return;
        }

        ConsoleVariable* Var = Variables[VarIndex->second];
        Pos++;

        std::string Value(CmdString.c_str() + Pos, CmdString.length() - Pos);
        if (std::regex_match(Value, std::regex("-[0-9]+")) && Var->CanBeInteger())
        {
            const int32 IntValue = std::stoi(Value);
            Var->SetAndConvertInt(IntValue);
        }
        else if (std::regex_match(Value, std::regex("[0-9]+")) && Var->CanBeInteger())
        {
            const int32 IntValue = std::stoi(Value);
            Var->SetAndConvertInt(IntValue);
        }
        else if (std::regex_match(Value, std::regex("(-[0-9]*\\.[0-9]+)|(-[0-9]+\\.[0-9]*)")) && Var->IsFloat())
        {
            const float FloatValue = std::stof(Value);
            Var->SetFloat(FloatValue);
        }
        else if (std::regex_match(Value, std::regex("([0-9]*\\.[0-9]+)|([0-9]+\\.[0-9]*)")) && Var->IsFloat())
        {
            const float FloatValue = std::stof(Value);
            Var->SetFloat(FloatValue);
        }
        else 
        {
            if (Var->IsBool())
            {
                for (char& c : Value)
                {
                    c = (char)tolower(c);
                }

                if (std::regex_match(Value, std::regex("(false)|(true)")))
                {
                    const bool BoolValue = (Value == "false" ? false : true);
                    Var->SetBool(BoolValue);
                    return;
                }
            }
            else if (Var->IsString())
            {
                Var->SetString(Value.c_str());
                return;
            }
            
            const std::string Message = "'" + Value + "' Is an invalid value for '" + Variable + "'";
            PrintError(Message);
        }
    }
}
