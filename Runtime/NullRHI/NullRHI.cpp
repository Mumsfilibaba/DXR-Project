#include "NullRHI.h"

IMPLEMENT_ENGINE_MODULE(FNullRHIModule, NullRHI);

FRHI* FNullRHIModule::CreateRHI()
{
    return new FNullRHI();
}