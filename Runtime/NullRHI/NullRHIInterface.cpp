#include "NullRHIInterface.h"

IMPLEMENT_ENGINE_MODULE(FNullRHIInterfaceModule, NullRHI);

FRHIInterface* FNullRHIInterfaceModule::CreateInterface()
{
    return new FNullRHIInterface();
}