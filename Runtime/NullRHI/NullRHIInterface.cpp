#include "NullRHI.h"
#include "NullRHIInterface.h"

IMPLEMENT_ENGINE_MODULE(FNullRHIInterfaceModule, NullRHI);

FRHIInterface* FNullRHIInterfaceModule::CreateInterface()
{
    return dbg_new FNullRHIInterface();
}