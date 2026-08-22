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

    static TAutoConsoleVariable<int32>  IntVariable("Test.Console.Int", "An int variable for the console tests", 42);
    static TAutoConsoleVariable<float>  FloatVariable("Test.Console.Float", "A float variable for the console tests", 1.5f);
    static TAutoConsoleVariable<bool>   BoolVariable("Test.Console.Bool", "A bool variable for the console tests", true);
    static TAutoConsoleVariable<String> StringVariable("Test.Console.String", "A string variable for the console tests", String("Value"));
}

void RegisterConsoleCrowdVariables()
{
    static bool bIsRegistered = false;
    if (bIsRegistered)
    {
        return;
    }

    bIsRegistered = true;

    static TArray<String> Names;
    static TArray<String> HelpStrings;

    Names.Reserve(GNumConsoleCrowdVariables);
    HelpStrings.Reserve(GNumConsoleCrowdVariables);

    static const CHAR* Subsystems[] = { "Renderer", "Scene", "Texture", "Shadow", "Bindless" };
    static const CHAR* Settings[]   = { "Enable", "MaxCount", "Bias", "Quality", "DebugName", "Threshold", "Scale", "Timeout" };

    for (int32 Index = 0; Index < GNumConsoleCrowdVariables; ++Index)
    {
        const CHAR* Subsystem = Subsystems[Index % static_cast<int32>(ARRAY_COUNT(Subsystems))];
        const CHAR* Setting   = Settings[Index % static_cast<int32>(ARRAY_COUNT(Settings))];

        Names.Emplace(String::Printf("%s.%s.%s%d", GConsoleCrowdPrefix, Subsystem, Setting, Index));
        HelpStrings.Emplace(String::Printf("Controls the %s %s of the %s subsystem", Setting, "behavior", Subsystem));

        const CHAR* Name = *Names.Last();
        const CHAR* Help = *HelpStrings.Last();

        switch (Index % 4)
        {
            case 0:  FConsoleManager::Get().RegisterVariable(Name, Help, Index * 16, EConsoleVariableFlags::Default);              break;
            case 1:  FConsoleManager::Get().RegisterVariable(Name, Help, static_cast<float>(Index) * 0.5f, EConsoleVariableFlags::Default); break;
            case 2:  FConsoleManager::Get().RegisterVariable(Name, Help, (Index % 8) == 2, EConsoleVariableFlags::Default);        break;
            default: FConsoleManager::Get().RegisterVariable(Name, Help, "DefaultValue", EConsoleVariableFlags::Default);          break;
        }
    }
}
