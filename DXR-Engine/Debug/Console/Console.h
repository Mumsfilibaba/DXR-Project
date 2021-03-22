#pragma once
#include "Core/Application/Events.h"

#include "ConsoleVariable.h"
#include "ConsoleCommand.h"

#include <unordered_map>

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

#define INIT_CONSOLE_VARIABLE(Name, Variable) GConsole.RegisterVariable(Name, Variable)
#define INIT_CONSOLE_COMMAND(Name, Command)   GConsole.RegisterCommand(Name, Command)

class Console
{
    struct Token
    {
        std::string Name;
    };

    struct Line
    {
        Line() = default;

        Line(const std::string& InString, ImVec4 InColor)
            : String(InString)
            , Color(InColor)
        {
        }

        std::string String;
        ImVec4      Color;
    };

    struct Candidate
    {
        static constexpr float TextPadding = 20.0f;

        Candidate() = default;

        Candidate(const std::string& InText, const std::string& InPostFix)
            : Text(InText)
            , PostFix(InPostFix)
        {
            TextSize = ImGui::CalcTextSize(Text.c_str());
            TextSize.x += TextPadding;
        }

        std::string Text;
        std::string PostFix;
        ImVec2 TextSize;
    };

public:
    void Init();
    void Tick();

    void RegisterCommand(const std::string& Name, ConsoleCommand* Object);
    void RegisterVariable(const std::string& Name, ConsoleVariable* Variable);

    ConsoleCommand* FindCommand(const std::string& Name);
    ConsoleVariable* FindVariable(const std::string& Name);

    void PrintMessage(const std::string& Message);
    void PrintWarning(const std::string& Message);
    void PrintError(const std::string& Message);

    void ClearHistory();

private:
    void OnKeyPressedEvent(const KeyPressedEvent& Event);

    void DrawInterface();

    bool RegisterObject(const std::string& Name, ConsoleObject* Variable);

    ConsoleObject* FindConsoleObject(const std::string& Name);

    int32 TextCallback(ImGuiInputTextCallbackData* Data);

    void Execute(const std::string& CmdString);

private:
    std::unordered_map<std::string, ConsoleObject*> ConsoleObjects;

    std::string PopupSelectedText;

    TArray<Candidate> Candidates;
    int32 CandidatesIndex = -1;

    TStaticArray<char, 256> TextBuffer;

    TArray<Line>  Lines;
    TArray<Token> Tokens;

    TArray<std::string> History;
    uint32 HistoryLength = 50;
    int32  HistoryIndex   = -1;

    bool UpdateCursorPosition      = false;
    bool CandidateSelectionChanged = false;
    bool ScrollDown                = false;
    bool IsActive                  = false;
};

extern Console GConsole;

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif