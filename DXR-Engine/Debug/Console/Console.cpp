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
            GConsole.DrawInterface();
        });
    }
}

void Console::RegisterCommand(const std::string& Name, ConsoleCommand* Command)
{
    if (!RegisterObject(Name, Command))
    {
        LOG_WARNING("ConsoleCommand '" + Name + "' is already registered");
    }
}

void Console::RegisterVariable(const std::string& Name, ConsoleVariable* Variable)
{
    if (!RegisterObject(Name, Variable))
    {
        LOG_WARNING("ConsoleVariable '" + Name + "' is already registered");
    }
}

ConsoleCommand* Console::FindCommand(const std::string& Name)
{
    ConsoleObject* Object = FindConsoleObject(Name);
    if (!Object)
    {
        LOG_ERROR("Could not find ConsoleCommand '" + Name + '\'');
        return nullptr;
    }

    ConsoleCommand* Command = Object->AsCommand();
    if (!Command)
    {
        LOG_ERROR('\'' + Name + "'Is not a ConsoleCommand'");
        return nullptr;
    }
    else
    {
        return Command;
    }
}

ConsoleVariable* Console::FindVariable(const std::string& Name)
{
    ConsoleObject* Object = FindConsoleObject(Name);
    if (!Object)
    {
        LOG_ERROR("Could not find ConsoleVariable '" + Name + '\'');
        return nullptr;
    }

    ConsoleVariable* Variable = Object->AsVariable();
    if (!Variable)
    {
        LOG_ERROR('\'' + Name + "'Is not a ConsoleVariable'");
        return nullptr;
    }
    else
    {
        return Variable;
    }
}

void Console::PrintMessage(const std::string& Message)
{
    Lines.EmplaceBack(Message, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void Console::PrintWarning(const std::string& Message)
{
    Lines.EmplaceBack(Message, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
}

void Console::PrintError(const std::string& Message)
{
    Lines.EmplaceBack(Message, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
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

void Console::DrawInterface()
{
    const uint32 WindowWidth  = GApplication->Window->GetWidth();
    const uint32 WindowHeight = GApplication->Window->GetHeight();
    const float Width  = float(WindowWidth);
    const float Height = float(WindowHeight) * 0.125f;

    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, 0.0f), ImGuiCond_FirstUseEver);

    ImGui::SetNextWindowSizeConstraints(ImVec2(Width, 100), ImVec2(Width, WindowHeight * 0.5f));

    const ImGuiWindowFlags StyleFlags =
        ImGuiWindowFlags_NoMove            |
        ImGuiWindowFlags_NoTitleBar        |
        ImGuiWindowFlags_NoScrollbar       |
        ImGuiWindowFlags_NoCollapse        |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Console Window", nullptr, StyleFlags);

    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));

    const ImVec2 ParentSize = ImGui::GetWindowSize();
    const float TextWindowWidth  = Width * 0.985f;
    const float TextWindowHeight = ParentSize.y - 40.0f;
    ImGui::BeginChild("##TextWindow", ImVec2(TextWindowWidth, TextWindowHeight), false, ImGuiWindowFlags_None);

    for (const Line& Text : Lines)
    {
        ImGui::TextColored(Text.Color, "%s", Text.String.c_str());
    }

    if (ScrollDown)
    {
        ImGui::SetScrollHereY();
        ScrollDown = false;
    }

    ImGui::EndChild();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    ImGui::Text(">");
    ImGui::SameLine();

    ImGui::PushItemWidth(TextWindowWidth - 25.0f);

    const ImGuiInputTextFlags InputFlags =
        ImGuiInputTextFlags_EnterReturnsTrue |
        ImGuiInputTextFlags_CallbackCompletion |
        ImGuiInputTextFlags_CallbackHistory |
        ImGuiInputTextFlags_CallbackAlways |
        ImGuiInputTextFlags_CallbackEdit;

    auto Callback = [](ImGuiInputTextCallbackData* Data)->int32
    {
        Console* This = reinterpret_cast<Console*>(Data->UserData);
        return This->TextCallback(Data);
    };

    const bool Result = ImGui::InputText("###Input", TextBuffer.Data(), TextBuffer.Size(), InputFlags, Callback, reinterpret_cast<void*>(&GConsole));
    if (Result && TextBuffer[0] != 0)
    {
        if (CandidatesIndex != -1)
        {
            strcpy(TextBuffer.Data(), PopupSelectedText.c_str());
            
            Candidates.Clear();
            CandidatesIndex = -1;

            UpdateCursorPosition = true;
        }
        else
        {
            const std::string Text = std::string(TextBuffer.Data());
            Execute(Text);

            TextBuffer[0] = 0;
            ScrollDown = true;

            ImGui::SetItemDefaultFocus();
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

    if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
    {
        ImGui::SetKeyboardFocusHere(-1);
    }

    if (!Candidates.IsEmpty())
    {
        bool IsActiveIndex = false;
        bool PopupOpen = true;
        const ImGuiWindowFlags PopupFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing;

        constexpr uint8 MaxCandidates = 5;

        float SizeY = 0.0f;
        if (Candidates.Size() < MaxCandidates)
        {
            SizeY = (ImGui::GetTextLineHeight() + 10.0f) * Candidates.Size();
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
        for (const Candidate& Candidate : Candidates)
        {
            if (Candidate.TextSize.x > ColumnWidth)
            {
                ColumnWidth = Candidate.TextSize.x;
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

        for (int32 i = 0; i < (int32)Candidates.Size(); i++)
        {
            const Candidate& Candidate = Candidates[i];
            IsActiveIndex = (CandidatesIndex == i);

            ImGui::PushID(i);
            if (ImGui::Selectable(Candidate.Text.c_str(), &IsActiveIndex))
            {
                strcpy(TextBuffer.Data(), Candidate.Text.c_str());
                PopupSelectedText = Candidate.Text;

                Candidates.Clear();
                CandidatesIndex = -1;

                UpdateCursorPosition = true;

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
                GConsole.PopupSelectedText = Candidate.Text;
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
            CandidateSelectionChanged = true;
            CandidatesIndex           = -1;

            const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
            if (WordLength <= 0)
            {
                break;
            }

            for (const std::pair<std::string, ConsoleObject*>& Object : ConsoleObjects)
            {
                if (WordLength <= Object.first.size())
                {
                    const char* Command = Object.first.c_str();
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
                        ConsoleObject* ConsoleObject = Object.second;
                        Assert(ConsoleObject != nullptr);

                        if (ConsoleObject->AsCommand())
                        {
                            Candidates.EmplaceBack(Object.first, "[Cmd]");
                        }
                        else
                        {
                            ConsoleVariable* Variable = ConsoleObject->AsVariable();
                            if (Variable->IsBool())
                            {
                                Candidates.EmplaceBack(Object.first, "= " + Variable->GetString() + " [Boolean]");
                            }
                            else if (Variable->IsInt())
                            {
                                Candidates.EmplaceBack(Object.first, "= " + Variable->GetString() + " [Integer]");
                            }
                            else if (Variable->IsFloat())
                            {
                                Candidates.EmplaceBack(Object.first, "= " + Variable->GetString() + " [float]");
                            }
                            else if (Variable->IsString())
                            {
                                Candidates.EmplaceBack(Object.first, "= " + Variable->GetString() + " [String]");
                            }
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
                    const int32 Pos   = static_cast<int32>(WordStart - Data->Buf);
                    const int32 Count = WordLength;
                    Data->DeleteChars(Pos, Count);
                    Data->InsertChars(Data->CursorPos, PopupSelectedText.c_str());

                    PopupSelectedText = "";

                    Candidates.Clear();
                    CandidatesIndex           = -1;
                    CandidateSelectionChanged = true;
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

void Console::Execute(const std::string& CmdString)
{
    PrintMessage(CmdString);

    // Erase history
    History.EmplaceBack(CmdString);
    if (History.Size() > HistoryLength)
    {
        History.Erase(History.Begin());
    }

    const auto IsValid = [](std::string::const_iterator It) -> bool
    {
        char Lower = tolower(*It);
        if (Lower >= 'a' && Lower <= 'z')
        {
            return true;
        }
        else if (Lower == '.')
        {
            return true;
        }
        else if (Lower >= '0' && Lower <= '9')
        {
            return true;
        }

        return false;
    };

    // Parse cmd string
    Token NewToken;
    for (std::string::const_iterator It = CmdString.begin(); It != CmdString.end(); It++)
    {
        if (IsValid(It))
        {
            NewToken.Name.push_back(*It);
        }
        else
        {
            Tokens.EmplaceBack(NewToken);
            NewToken.Name.clear();
        }
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
                ToLower(Value);

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

bool Console::RegisterObject(const std::string& Name, ConsoleObject* Object)
{
    auto ExistingObject = ConsoleObjects.find(Name);
    if (ExistingObject != ConsoleObjects.end())
    {
        ConsoleObjects.insert(std::make_pair(Name, Object));
        return true;
    }
    else
    {
        return false;
    }
}

ConsoleObject* Console::FindConsoleObject(const std::string& Name)
{
    auto ExisitingObject = ConsoleObjects.find(Name);
    if (ExisitingObject != ConsoleObjects.end())
    {
        return ExisitingObject->second;
    }
    else
    {
        return nullptr;
    }
}