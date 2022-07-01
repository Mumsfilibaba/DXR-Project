#include "ConsoleManager.h"

#include "Core/Logging/Log.h"

#include "Canvas/CanvasApplication.h"

// TODO: Remove (Make own? Slow?)
#include <regex>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console commands for the console

CAutoConsoleCommand GClearHistory("ClearHistory", 
                                  CExecutedDelegateType::CreateRaw(&CConsoleManager::Get(), &CConsoleManager::ClearHistory));

TAutoConsoleVariable<FString> GEcho("Echo", "", 
                                   CConsoleVariableChangedDelegateType::CreateLambda([](IConsoleVariable* InVariable) -> void
{
    if (InVariable->IsString())
    {
        CConsoleManager& ConsoleManager = CConsoleManager::Get();
        ConsoleManager.PrintMessage(InVariable->GetString(), EConsoleSeverity::Info);
    }
}));

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CConsoleManager

TOptional<CConsoleManager>& CConsoleManager::GetConsoleManagerInstance()
{
    static TOptional<CConsoleManager> Instance(InPlace);
    return Instance;
}

CConsoleManager& CConsoleManager::Get()
{
    TOptional<CConsoleManager>& ConsoleManager = GetConsoleManagerInstance();
    return ConsoleManager.GetValue();
}

void CConsoleManager::RegisterCommand(const FString& Name, IConsoleCommand* Command)
{
    if (!RegisterObject(Name, Command))
    {
        LOG_WARNING("ConsoleCommand '%s' is already registered", Name.CStr());
    }
}

void CConsoleManager::RegisterVariable(const FString& Name, IConsoleVariable* Variable)
{
    if (!RegisterObject(Name, Variable))
    {
        LOG_WARNING("ConsoleVariable '%s' is already registered", Name.CStr());
    }
}

void CConsoleManager::UnregisterObject(const FString& Name)
{
    auto ExistingObject = ConsoleObjects.find(Name);
    if (ExistingObject == ConsoleObjects.end())
    {
        ConsoleObjects.erase(ExistingObject);
    }
}

bool CConsoleManager::IsConsoleObject(const FString& Name) const
{
    auto ExistingObject = ConsoleObjects.find(Name);
    return ExistingObject != ConsoleObjects.end();
}

IConsoleCommand* CConsoleManager::FindCommand(const FString& Name)
{
    IConsoleObject* Object = FindConsoleObject(Name);
    if (!Object)
    {
        LOG_ERROR("Could not find ConsoleCommand '%s'", Name.CStr());
        return nullptr;
    }

    IConsoleCommand* Command = Object->AsCommand();
    if (!Command)
    {
        LOG_ERROR("'%s' is not a ConsoleCommand'", Name.CStr());
        return nullptr;
    }
    else
    {
        return Command;
    }
}

IConsoleVariable* CConsoleManager::FindVariable(const FString& Name)
{
    IConsoleObject* Object = FindConsoleObject(Name);
    if (!Object)
    {
        LOG_ERROR("Could not find ConsoleVariable '%s'", Name.CStr());
        return nullptr;
    }

    IConsoleVariable* Variable = Object->AsVariable();
    if (!Variable)
    {
        LOG_ERROR("'%s' is not a ConsoleVariable", Name.CStr());
        return nullptr;
    }
    else
    {
        return Variable;
    }
}

void CConsoleManager::PrintMessage(const FString& Message, EConsoleSeverity Severity)
{
    ConsoleMessages.Emplace(Message, Severity);
}

void CConsoleManager::ClearHistory()
{
    History.Clear();
    ConsoleMessages.Clear();
}

void CConsoleManager::FindCandidates(const FStringView& CandidateName, TArray<TPair<IConsoleObject*, FString>>& OutCandidates)
{
    for (const auto& Object : ConsoleObjects)
    {
        const FString& ObjectName = Object.first;

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

void CConsoleManager::Execute(const FString& Command)
{
    PrintMessage(Command, EConsoleSeverity::Info);

    // Erase history
    History.Emplace(Command);
    if (History.Size() > HistoryLength)
    {
        History.RemoveAt(History.StartIterator());
    }

    int32 Pos = Command.FindOneOf(" ");
    if (Pos == FString::NPos)
    {
        IConsoleCommand* CommandObject = FindCommand(Command);
        if (!CommandObject)
        {
            PrintMessage("'" + Command + "' is not a registered command", EConsoleSeverity::Error);
        }
        else
        {
            CommandObject->Execute();
        }
    }
    else
    {
        FString VariableName(Command.CStr(), Pos);

        IConsoleVariable* VariableObject = FindVariable(VariableName);
        if (!VariableObject)
        {
            PrintMessage("'" + Command + "' is not a registered variable", EConsoleSeverity::Error);
            return;
        }

        Pos++;

        FString Value(Command.CStr() + Pos, Command.Length() - Pos);
        if (std::regex_match(Value.CStr(), std::regex("[-]?[0-9]+")))
        {
            VariableObject->SetString(Value);
        }
        else if (std::regex_match(Value.CStr(), std::regex("[-]?[0-9]*[.][0-9]+")) && VariableObject->IsFloat())
        {
            VariableObject->SetString(Value);
        }
        else if (std::regex_match(Value.CStr(), std::regex("(false)|(true)")) && VariableObject->IsBool())
        {
            VariableObject->SetString(Value);
        }
        else
        {
            if (VariableObject->IsString())
            {
                VariableObject->SetString(Value);
            }
            else
            {
                PrintMessage("'" + Value + "' Is an invalid value for '" + VariableName + "'", EConsoleSeverity::Error);
            }
        }
    }
}

bool CConsoleManager::RegisterObject(const FString& Name, IConsoleObject* Object)
{
    auto ExistingObject = ConsoleObjects.find(Name);
    if (ExistingObject == ConsoleObjects.end())
    {
        LOG_INFO("Registered ConsoleObject '%s'", Name.CStr());

        ConsoleObjects.insert(std::make_pair(Name, Object));
        return true;
    }
    else
    {
        return false;
    }
}

IConsoleObject* CConsoleManager::FindConsoleObject(const FString& Name)
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
