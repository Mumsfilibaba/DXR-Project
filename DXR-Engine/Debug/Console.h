#pragma once
#include "Core/Application/Events.h"

#include <unordered_map>
#include <cstring>

#define INIT_CONSOLE_VARIABLE(VarName, Var)    GConsole.RegisterVariable(VarName, &Var)
#define INIT_CONSOLE_COMMAND(CmdName, CmdFunc) GConsole.RegisterCommand(CmdName, CmdFunc)

typedef void(*ConsoleCommand)();

enum class EConsoleVariableType : uint8
{
    Unknown = 0,
    Bool    = 1,
    Int     = 2,
    Float   = 3,
    String  = 4,
};

struct ConsoleVariable
{
    ConsoleVariable() = default;

    ConsoleVariable(EConsoleVariableType InType)
        : Type(InType)
    {
    }

    ~ConsoleVariable()
    {
        Free();
    }
    
    FORCEINLINE void Free()
    {
        if (Type == EConsoleVariableType::String)
        {
            Memory::Free(StringValue);
            StringValue = nullptr;
        }
    }

    FORCEINLINE void SetBool(bool Value)
    {
        Assert(Type == EConsoleVariableType::Bool);
        BoolValue = Value;
    }

    FORCEINLINE void SetInt(int32 Value)
    {
        Assert(Type == EConsoleVariableType::Int);
        IntValue = Value;
    }

    FORCEINLINE void SetFloat(float Value)
    {
        Assert(Type == EConsoleVariableType::Float);
        FloatValue = Value;
    }

    FORCEINLINE void SetAndConvertInt(int32 Value)
    {
        if (Type == EConsoleVariableType::Int)
        {
            IntValue = Value;
        }
        else if (Type == EConsoleVariableType::Bool)
        {
            BoolValue = bool(Value);
        }
        else if (Type == EConsoleVariableType::Float)
        {
            FloatValue = float(Value);
        }
        else
        {
            Assert(false);
        }
    }

    FORCEINLINE void SetString(const char* Value)
    {
        Assert(Type == EConsoleVariableType::String);
        
        const int32 Len = int32(strlen(Value));
        if (Len < Length)
        {
            Memory::Free(StringValue);
            
            StringValue = Memory::Malloc<char>(Len);
            Length      = Len;
        }

        Memory::Strcpy(StringValue, Value);
    }

    FORCEINLINE bool GetBool() const
    {
        Assert(Type == EConsoleVariableType::Bool);
        return BoolValue;
    }

    FORCEINLINE int32 GetInt32() const
    {
        Assert(Type == EConsoleVariableType::Int);
        return IntValue;
    }

    FORCEINLINE float GetFloat() const
    {
        Assert(Type == EConsoleVariableType::Float);
        return FloatValue;
    }

    FORCEINLINE const char* GetString() const
    {
        Assert(Type == EConsoleVariableType::String);
        return StringValue;
    }

    bool IsBool() const { return Type == EConsoleVariableType::Bool; }
    bool IsInt() const { return Type == EConsoleVariableType::Int; }
    bool IsFloat() const { return Type == EConsoleVariableType::Float; }
    bool IsString() const { return Type == EConsoleVariableType::String; }

    bool CanBeInteger() const
    {
        return IsBool() || IsInt() || IsFloat();
    }

    EConsoleVariableType Type = EConsoleVariableType::Unknown;
    union
    {
        bool  BoolValue;
        int32 IntValue;
        float FloatValue;
        struct
        {
            char* StringValue;
            int32 Length;
        };
    };
};

class Console
{
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
        Candidate() = default;

        Candidate(const std::string& InText, const std::string& InPostFix)
            : Text(InText)
            , PostFix(InPostFix)
        {
            TextSize = ImGui::CalcTextSize(Text.c_str());
            TextSize.x += 20.0f;
        }

        std::string Text;
        std::string PostFix;
        ImVec2 TextSize;
    };

public:
    void Init();
    void Tick();

    void RegisterCommand(const std::string& CmdName, ConsoleCommand Command);
    void RegisterVariable(const std::string& VarName, ConsoleVariable* Variable);

    ConsoleCommand FindCommand(const std::string& CmdName);
    ConsoleVariable* FindVariable(const std::string& VarName);

    void PrintMessage(const std::string& Message);
    void PrintWarning(const std::string& Message);
    void PrintError(const std::string& Message);

    void ClearHistory();

private:
    void OnKeyPressedEvent(const KeyPressedEvent& Event);

    int32 TextCallback(ImGuiInputTextCallbackData* Data);
    void HandleCommand(const std::string& CmdString);

    std::string PopupSelectedText;
    std::unordered_map<std::string, int32> CmdIndexMap;
    std::unordered_map<std::string, int32> VarIndexMap;
    TArray<ConsoleCommand>   Commands;
    TArray<ConsoleVariable*> Variables;

    TArray<Candidate>     Candidates;
    int32 CandidatesIndex = -1;

    TStaticArray<char, 256> TextBuffer;
    TArray<Line>        Lines;
    TArray<std::string> History;
    uint32 HistoryLength = 50;
    int32  HistoryIndex   = -1;

    bool UpdateCursorPosition      = false;
    bool CandidateSelectionChanged = false;
    bool ScrollDown                = false;
    bool IsActive                  = false;
};

extern Console GConsole;