#include "ConsoleTestVariables.h"

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
