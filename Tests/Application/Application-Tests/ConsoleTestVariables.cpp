#include "ConsoleTestVariables.h"

#include <Core/CoreDefines.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Misc/ConsoleManager.h>

void RegisterConsoleTestVariables()
{
    static bool bIsRegistered = false;
    if (bIsRegistered)
    {
        return;
    }

    bIsRegistered = true;

    static TAutoConsoleVariable<int32>   IntVariable(TEXT("Test.Console.Int"), TEXT("An int variable for the console tests"), 42);
    static TAutoConsoleVariable<float>   FloatVariable(TEXT("Test.Console.Float"), TEXT("A float variable for the console tests"), 1.5f);
    static TAutoConsoleVariable<bool>    BoolVariable(TEXT("Test.Console.Bool"), TEXT("A bool variable for the console tests"), true);
    static TAutoConsoleVariable<FString> StringVariable(TEXT("Test.Console.String"), TEXT("A string variable for the console tests"), FString(TEXT("Value")));
}

void RegisterConsoleCrowdVariables()
{
    static bool bIsRegistered = false;
    if (bIsRegistered)
    {
        return;
    }

    bIsRegistered = true;

    static TArray<FString> Names;
    static TArray<FString> HelpStrings;

    Names.Reserve(GNumConsoleCrowdVariables);
    HelpStrings.Reserve(GNumConsoleCrowdVariables);

    static const CHAR_T* Subsystems[] = { TEXT("Renderer"), TEXT("Scene"), TEXT("Texture"), TEXT("Shadow"), TEXT("Bindless") };
    static const CHAR_T* Settings[]   = { TEXT("Enable"), TEXT("MaxCount"), TEXT("Bias"), TEXT("Quality"), TEXT("DebugName"), TEXT("Threshold"), TEXT("Scale"), TEXT("Timeout") };

    for (int32 Index = 0; Index < GNumConsoleCrowdVariables; ++Index)
    {
        const CHAR_T* Subsystem = Subsystems[Index % static_cast<int32>(ARRAY_COUNT(Subsystems))];
        const CHAR_T* Setting   = Settings[Index % static_cast<int32>(ARRAY_COUNT(Settings))];

        Names.Emplace(FString::Printf(TEXT("%s.%s.%s%d"), GConsoleCrowdPrefix, Subsystem, Setting, Index));
        HelpStrings.Emplace(FString::Printf(TEXT("Controls the %s %s of the %s subsystem"), Setting, TEXT("behavior"), Subsystem));

        const CHAR_T* Name = *Names.Last();
        const CHAR_T* Help = *HelpStrings.Last();

        switch (Index % 4)
        {
            case 0:  FConsoleManager::Get().RegisterVariable(Name, Help, Index * 16, EConsoleVariableFlags::Default);              break;
            case 1:  FConsoleManager::Get().RegisterVariable(Name, Help, static_cast<float>(Index) * 0.5f, EConsoleVariableFlags::Default); break;
            case 2:  FConsoleManager::Get().RegisterVariable(Name, Help, (Index % 8) == 2, EConsoleVariableFlags::Default);        break;
            default: FConsoleManager::Get().RegisterVariable(Name, Help, TEXT("DefaultValue"), EConsoleVariableFlags::Default);          break;
        }
    }
}
