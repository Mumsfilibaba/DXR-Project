#include "ConsoleInterface.h"
#include "ConsoleManager.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FConsoleInterface

static auto& GetConsoleManagerInstance()
{
    static TOptional<FConsoleManager> GInstance(InPlace);
    return GInstance;
}

FConsoleInterface& FConsoleInterface::Get()
{
    auto& ConsoleManager = GetConsoleManagerInstance();
    return ConsoleManager.GetValue();
}