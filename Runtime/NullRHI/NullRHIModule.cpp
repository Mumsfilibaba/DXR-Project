#include "NullRHIModule.h"
#include "NullRHICoreInterface.h"

IMPLEMENT_ENGINE_MODULE(FNullRHIModule, NullRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIModule

FRHICoreInterface* FNullRHIModule::CreateInterface()
{
    return dbg_new FNullRHICoreInterface();
}