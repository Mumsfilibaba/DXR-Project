#include "ConsoleManager.h"

#include "Core/Logging/Log.h"

#include "Canvas/Application.h"

// TODO: Remove (Make own? Slow?)
#include <regex>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console commands for the console

FAutoConsoleCommand GClearHistory("ClearHistory", FCommandDelegateType::CreateRaw(&FConsoleManager::Get(), &FConsoleManager::ClearHistory));

TAutoConsoleVariable<FString> GEcho("Echo", "", FCVarChangedDelegateType::CreateLambda([](IConsoleVariable* InVariable) -> void
{
    if (InVariable->IsString())
    {
        FConsoleManager& ConsoleManager = FConsoleManager::Get();
        ConsoleManager.PrintMessage(InVariable->GetString(), EConsoleSeverity::Info);
    }
}));

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FConsoleManager

TOptional<FConsoleManager>& FConsoleManager::GetConsoleManagerInstance()
{
    static TOptional<FConsoleManager> Instance(InPlace);
    return Instance;
}

FConsoleManager& FConsoleManager::Get()
{
    TOptional<FConsoleManager>& ConsoleManager = GetConsoleManagerInstance();
    return ConsoleManager.GetValue();
}

void FConsoleManager::RegisterCommand(const FString& Name, IConsoleCommand* Command)
{
    if (!RegisterObject(Name, Command))
    {
        LOG_WARNING("ConsoleCommand '%s' is already registered", Name.CStr());
    }
}

void FConsoleManager::RegisterVariable(const FString& Name, IConsoleVariable* Variable)
{
    if (!RegisterObject(Name, Variable))
    {
        LOG_WARNING("ConsoleVariable '%s' is already registered", Name.CStr());
    }
}

void FConsoleManager::UnregisterObject(const FString& Name)
{
    auto ExistingObject = ConsoleObjects.find(Name);
    if (ExistingObject == ConsoleObjects.end())
    {
        ConsoleObjects.erase(ExistingObject);
    }
}

bool FConsoleManager::IsConsoleObject(const FString& Name) const
{
    auto ExistingObject = ConsoleObjects.find(Name);
    return ExistingObject != ConsoleObjects.end();
}

IConsoleCommand* FConsoleManager::FindCommand(const FString& Name)
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

IConsoleVariable* FConsoleManager::FindVariable(const FString& Name)
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

void FConsoleManager::PrintMessage(const FString& Message, EConsoleSeverity Severity)
{
    ConsoleMessages.Emplace(Message, Severity);
}

void FConsoleManager::ClearHistory()
{
    History.Clear();
    ConsoleMessages.Clear();
}

void FConsoleManager::FindCandidates(const FStringView& CandidateName, TArray<TPair<IConsoleObject*, FString>>& OutCandidates)
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

void FConsoleManager::Execute(const FString& Command)
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

bool FConsoleManager::RegisterObject(const FString& Name, IConsoleObject* Object)
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

IConsoleObject* FConsoleManager::FindConsoleObject(const FString& Name)
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
