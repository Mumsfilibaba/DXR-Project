#include "RHIModule.h"
#include "RHICoreInterface.h"
#include "RHICommandList.h"
#include "RHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(FDefaultModule, RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Globals

RHI_API FRHICoreInterface* GRHIInterface = nullptr;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RHI Functions

static FRHIModule* LoadNullRHI()
{
    return FModuleInterface::Get().LoadModule<FRHIModule>("NullRHI");
}

bool RHIInitialize(ERHIInstanceType InRenderApi)
{
    // Load Selected RHI
    FRHIModule* RHIModule = nullptr;
    if (InRenderApi == ERHIInstanceType::D3D12)
    {
        RHIModule = FModuleInterface::Get().LoadModule<FRHIModule>("D3D12RHI");
    }
	else if (InRenderApi == ERHIInstanceType::Metal)
	{
		RHIModule = FModuleInterface::Get().LoadModule<FRHIModule>("MetalRHI");
	}
    else if (InRenderApi == ERHIInstanceType::Null)
    {
        RHIModule = LoadNullRHI();
    }

    if (!RHIModule)
    {
        LOG_WARNING("[InitRHI] Failed to load RHI, trying to load NullRHI");

        RHIModule = LoadNullRHI();
        if (!RHIModule)
        {
            LOG_ERROR("[InitRHI] Failed to load RHI and fallback, the application has to terminate");
            return false;
        }
    }

    // TODO: This should be in EngineConfig and/or CCommandLine
    const bool bEnableDebug =
#if ENABLE_API_DEBUGGING
        true;
#else
        false;
#endif

    FRHICoreInterface* RHICoreInterface = RHIModule->CreateInterface();
    if (!(RHICoreInterface && RHICoreInterface->Initialize(bEnableDebug)))
    {
        LOG_ERROR("[InitRHI] Failed to init RHIInterface, the application has to terminate");
        return false;
    }

    GRHIInterface = RHICoreInterface;

    // Initialize the CommandListExecutor
    if (!GRHICommandExecutor.Initialize())
    {
        return false;
    }

    // Set the context to the command queue
    IRHICommandContext* CmdContext = RHIGetDefaultCommandContext();
    GRHICommandExecutor.SetContext(CmdContext);

    return true;
}

void RHIRelease()
{
    GRHICommandExecutor.Release();

    if (GRHIInterface)
    {
        GRHIInterface->Destroy();
        GRHIInterface = nullptr;
    }
}
