#include "RHIInterface.h"
#include "RHICommandList.h"
#include "RHIShaderCompiler.h"

#include "Core/Debug/Console/ConsoleInterface.h"

IMPLEMENT_ENGINE_MODULE(FRHIInterfaceModule, RHI);

TAutoConsoleVariable<bool> CVarEnableDebugLayer("RHI.EnableDebugLayer", false);

RHI_API FRHIInterface* GRHIInterface = nullptr;

static FRHIInterfaceModule* LoadNullRHI()
{
    return FModuleManager::Get().LoadModule<FRHIInterfaceModule>("NullRHI");
}

bool RHIInitialize(ERHIInstanceType InRenderApi)
{
    // Load Selected RHI
    FRHIInterfaceModule* RHIModule = nullptr;
    if (InRenderApi == ERHIInstanceType::D3D12)
    {
        RHIModule = FModuleManager::Get().LoadModule<FRHIInterfaceModule>("D3D12RHI");
    }
    else if (InRenderApi == ERHIInstanceType::Metal)
    {
        RHIModule = FModuleManager::Get().LoadModule<FRHIInterfaceModule>("MetalRHI");
    }
    else if (InRenderApi == ERHIInstanceType::Null)
    {
        RHIModule = LoadNullRHI();
    }

    if (!RHIModule)
    {
        LOG_WARNING("[RHIInitialize] Failed to load RHI, trying to load NullRHI");

        RHIModule = LoadNullRHI();
        if (!RHIModule)
        {
            LOG_ERROR("[RHIInitialize] Failed to load RHI and fallback, the application has to terminate");
            return false;
        }
    }

    FRHIInterface* RHIInterface = RHIModule->CreateInterface();
    if (!RHIInterface)
    {
        LOG_ERROR("[RHIInitialize] Failed to create RHIInterface, the application has to terminate");
        return false;
    }

    if (!RHIInterface->Initialize())
    {
        LOG_ERROR("[RHIInitialize] Failed to Ïnitialize RHIInterface, the application has to terminate");
        return false;
    }

    GRHIInterface = RHIInterface;

    // Initialize the CommandListExecutor
    if (!GRHICommandExecutor.Initialize())
    {
        return false;
    }

    // Set the context to the command queue
    IRHICommandContext* CommandContext = RHIGetDefaultCommandContext();
    GRHICommandExecutor.SetContext(CommandContext);
    return true;
}

void RHIRelease()
{
    GRHICommandExecutor.Release();

    if (GRHIInterface)
    {
        delete GRHIInterface;
        GRHIInterface = nullptr;
    }
}
