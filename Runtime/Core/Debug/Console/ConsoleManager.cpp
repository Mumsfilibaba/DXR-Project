#include "ConsoleManager.h"

#include "Core/Logging/Log.h"

#include "Interface/InterfaceApplication.h"

// TODO: Remove (Make own? Slow?)
#include <regex>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

CConsoleCommand GClearHistory;

TConsoleVariable<CString> GEcho;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

CConsoleManager CConsoleManager::Instance;

void CConsoleManager::Initialize()
{
    GClearHistory.GetExecutedDelgate().AddRaw(&Instance, &CConsoleManager::ClearHistory);
    INIT_CONSOLE_COMMAND("ClearHistory", &GClearHistory);

    GEcho.GetChangedDelegate().AddLambda([](IConsoleVariable* InVariable) -> void
    {
        if (InVariable->IsString())
        {
            Instance.PrintMessage(InVariable->GetString(), EConsoleSeverity::Info);
        }
    });

    INIT_CONSOLE_VARIABLE("Echo", &GEcho);
}

void CConsoleManager::RegisterCommand(const CString& Name, IConsoleCommand* Command)
{
    if (!RegisterObject(Name, Command))
    {
        LOG_WARNING("ConsoleCommand '" + Name + "' is already registered");
    }
}

void CConsoleManager::RegisterVariable(const CString& Name, IConsoleVariable* Variable)
{
    if (!RegisterObject(Name, Variable))
    {
        LOG_WARNING("ConsoleVariable '" + Name + "' is already registered");
    }
}

IConsoleCommand* CConsoleManager::FindCommand(const CString& Name)
{
    IConsoleObject* Object = FindConsoleObject(Name);
    if (!Object)
    {
        LOG_ERROR("Could not find ConsoleCommand '" + Name + '\'');
        return nullptr;
    }

    IConsoleCommand* Command = Object->AsCommand();
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

IConsoleVariable* CConsoleManager::FindVariable(const CString& Name)
{
    IConsoleObject* Object = FindConsoleObject(Name);
    if (!Object)
    {
        LOG_ERROR("Could not find ConsoleVariable '" + Name + '\'');
        return nullptr;
    }

    IConsoleVariable* Variable = Object->AsVariable();
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

void CConsoleManager::PrintMessage(const CString& Message, EConsoleSeverity Severity)
{
    ConsoleMessages.Emplace(Message, Severity);
}

void CConsoleManager::ClearHistory()
{
    History.Clear();
}

void CConsoleManager::FindCandidates(const CStringView& CandidateName, TArray<TPair<IConsoleObject*, CString>>& OutCandidates)
{
    for (const auto& Object : ConsoleObjects)
    {
        const CString& ObjectName = Object.first;

        int32 Length = CandidateName.Length();
        if (Length <= ObjectName.Length())
        {
            const char* Command = ObjectName.CStr();
            const char* WordIt = CandidateName.CStr();

            int32 CharDiff = -1;
            while (Length > 0 && (CharDiff = (toupper(*WordIt) - toupper(*Command))) == 0)
            {
                Command++;
                WordIt++;
                Length--;
            }

            if (CharDiff == 0)
            {
                IConsoleObject* ConsoleObject = Object.second;
                OutCandidates.Emplace(ConsoleObject, ObjectName);
            }
        }
    }
}

void CConsoleManager::Execute(const CString& CmdString)
{
    PrintMessage(CmdString, EConsoleSeverity::Info);

    // Erase history
    History.Emplace(CmdString);
    if (History.Size() > HistoryLength)
    {
        History.RemoveAt(History.StartIterator());
    }

    int32 Pos = CmdString.FindOneOf(" ");
    if (Pos == CString::NPos)
    {
        IConsoleCommand* Command = FindCommand(CmdString);
        if (!Command)
        {
            PrintMessage("'" + CmdString + "' is not a registered command", EConsoleSeverity::Error);
        }
        else
        {
            Command->Execute();
        }
    }
    else
    {
        CString VariableName(CmdString.CStr(), Pos);

        IConsoleVariable* Variable = FindVariable(VariableName);
        if (!Variable)
        {
            PrintMessage("'" + CmdString + "' is not a registered variable", EConsoleSeverity::Error);
            return;
        }

        Pos++;

        CString Value(CmdString.CStr() + Pos, CmdString.Length() - Pos);
        if (std::regex_match(Value.CStr(), std::regex("[-]?[0-9]+")))
        {
            Variable->SetString(Value);
        }
        else if (std::regex_match(Value.CStr(), std::regex("[-]?[0-9]*[.][0-9]+")) && Variable->IsFloat())
        {
            Variable->SetString(Value);
        }
        else if (std::regex_match(Value.CStr(), std::regex("(false)|(true)")) && Variable->IsBool())
        {
            Variable->SetString(Value);
        }
        else
        {
            if (Variable->IsString())
            {
                Variable->SetString(Value);
            }
            else
            {
                PrintMessage("'" + Value + "' Is an invalid value for '" + VariableName + "'", EConsoleSeverity::Error);
            }
        }
    }
}

bool CConsoleManager::RegisterObject(const CString& Name, IConsoleObject* Object)
{
    auto ExistingObject = ConsoleObjects.find(Name);
    if (ExistingObject == ConsoleObjects.end())
    {
        ConsoleObjects.insert(std::make_pair(Name, Object));
        return true;
    }
    else
    {
        return false;
    }
}

IConsoleObject* CConsoleManager::FindConsoleObject(const CString& Name)
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
