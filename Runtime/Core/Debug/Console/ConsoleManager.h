#pragma once
#include "ConsoleInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FConsoleManager

class CORE_API FConsoleManager 
    : public FConsoleInterface
{
public:
    FConsoleManager()  = default;
    ~FConsoleManager() = default;

    virtual void RegisterCommand(const FString& Name, IConsoleCommand* Command)    override final;
    virtual void RegisterVariable(const FString& Name, IConsoleVariable* Variable) override final;

    virtual void UnregisterObject(const FString& Name) override final;

    virtual bool IsConsoleObject(const FString& Name) const override final;

    virtual IConsoleCommand*  FindCommand(const FString& Name)  override final;
    virtual IConsoleVariable* FindVariable(const FString& Name) override final;

    virtual void FindCandidates(const FStringView& CandidateName, TArray<TPair<IConsoleObject*, FString>>& OutCandidates) override final;

    virtual void PrintMessage(const FString& Message, EConsoleSeverity Severity) override final;

    virtual void ClearHistory() override final;

    virtual void Execute(const FString& Command) override final;

    virtual const TArray<TPair<FString, EConsoleSeverity>>& GetMessages() const override final
    { 
        return ConsoleMessages;
    }

    virtual const TArray<FString>& GetHistory() const override final
    { 
        return History; 
    }

private:
    bool RegisterObject(const FString& Name, IConsoleObject* Variable);

    IConsoleObject* FindConsoleObject(const FString& Name);

private:
    TMap<FString, IConsoleObject*, FStringHasher> ConsoleObjects;

    TArray<TPair<FString, EConsoleSeverity>> ConsoleMessages;

    TArray<FString> History;
    int32           HistoryLength = 50;
};
