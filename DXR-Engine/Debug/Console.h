#pragma once
#include "Application/Events/EventDispatcher.h"

#include <unordered_map>
#include <cstring>

#define INIT_CONSOLE_VARIABLE(VarName, Var)    gConsole.RegisterVariable(VarName, &Var)
#define INIT_CONSOLE_COMMAND(CmdName, CmdFunc) gConsole.RegisterCommand(CmdName, CmdFunc)

typedef void(*ConsoleCommand)();

enum class EConsoleVariableType : UInt8
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

    FORCEINLINE void SetBool(Bool Value)
    {
        VALIDATE(Type == EConsoleVariableType::Bool);
        BoolValue = Value;
    }

    FORCEINLINE void SetInt(Int32 Value)
    {
        VALIDATE(Type == EConsoleVariableType::Int);
        IntValue = Value;
    }

    FORCEINLINE void SetFloat(Float Value)
    {
        VALIDATE(Type == EConsoleVariableType::Float);
        FloatValue = Value;
    }

    FORCEINLINE void SetAndConvertInt(Int32 Value)
    {
        if (Type == EConsoleVariableType::Int)
        {
            IntValue = Value;
        }
        else if (Type == EConsoleVariableType::Bool)
        {
            BoolValue = Bool(Value);
        }
        else if (Type == EConsoleVariableType::Float)
        {
            FloatValue = Float(Value);
        }
        else
        {
            VALIDATE(false);
        }
    }

    FORCEINLINE void SetString(const Char* Value)
    {
        VALIDATE(Type == EConsoleVariableType::String);
        
        const Int32 Len = Int32(strlen(Value));
        if (Len < Length)
        {
            Memory::Free(StringValue);
            
            StringValue = Memory::Malloc<Char>(Len);
            Length      = Len;
        }

        Memory::Strcpy(StringValue, Value);
    }

    FORCEINLINE Bool GetBool() const
    {
        VALIDATE(Type == EConsoleVariableType::Bool);
        return BoolValue;
    }

    FORCEINLINE Int32 GetInt32() const
    {
        VALIDATE(Type == EConsoleVariableType::Int);
        return IntValue;
    }

    FORCEINLINE Float GetFloat() const
    {
        VALIDATE(Type == EConsoleVariableType::Float);
        return FloatValue;
    }

    FORCEINLINE const Char* GetString() const
    {
        VALIDATE(Type == EConsoleVariableType::String);
        return StringValue;
    }

    Bool IsBool() const { return Type == EConsoleVariableType::Bool; }
    Bool IsInt() const { return Type == EConsoleVariableType::Int; }
    Bool IsFloat() const { return Type == EConsoleVariableType::Float; }
    Bool IsString() const { return Type == EConsoleVariableType::String; }

    Bool CanBeInteger() const
    {
        return IsBool() || IsInt() || IsFloat();
    }

    EConsoleVariableType Type = EConsoleVariableType::Unknown;
    union
    {
        Bool  BoolValue;
        Int32 IntValue;
        Float FloatValue;
        struct
        {
            Char* StringValue;
            Int32 Length;
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
    Console()  = default;
    ~Console() = default;

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
    Int32 TextCallback(ImGuiInputTextCallbackData* Data);
    void HandleCommand(const std::string& CmdString);

    std::string PopupSelectedText;
    std::unordered_map<std::string, Int32> CmdIndexMap;
    std::unordered_map<std::string, Int32> VarIndexMap;
    TArray<ConsoleCommand>   Commands;
    TArray<ConsoleVariable*> Variables;

    TArray<Candidate>     Candidates;
    Int32 CandidatesIndex = -1;

    TStaticArray<Char, 256> TextBuffer;
    TArray<Line>        Lines;
    TArray<std::string> History;
    UInt32 HistoryLength = 50;
    Int32  HistoryIndex   = -1;

    Bool UpdateCursorPosition      = false;
    Bool CandidateSelectionChanged = false;
    Bool ScrollDown                = false;
    Bool IsActive                  = false;
};