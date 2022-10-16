#include "NullRHIModule.h"
#include "NullRHIInterface.h"

IMPLEMENT_ENGINE_MODULE(FNullRHIModule, NullRHI);

FRHIInterface* FNullRHIModule::CreateInterface()
{
    return dbg_new FNullRHIInterface();
}