#include "RHIInterface.h"
#include "RHICommandList.h"
#include "RHIShaderCompiler.h"

#include "Core/Misc/EngineConfig.h"
#include "Core/Misc/ConsoleManager.h"

IMPLEMENT_ENGINE_MODULE(FRHIInterfaceModule, RHI);

TAutoConsoleVariable<bool> CVarEnableDebugLayer(
    "RHI.EnableDebugLayer", 
    "Enables the DebugLayer for the RHI",
    false);

TAutoConsoleVariable<FString> CVarType(
    "RHI.Type", 
    "Selects the RHI Layer to use",
    "Unknown");

RHI_API FRHIInterface* GRHIInterface = nullptr;

static FRHIInterfaceModule* LoadNullRHI()
{
    return FModuleManager::Get().LoadModule<FRHIInterfaceModule>("NullRHI");
}

static ERHIInstanceType GetRHITypeFromConfig()
{
    const FString RHITypeString = CVarType->GetString();
    if (RHITypeString == "D3D12")
    {
#if PLATFORM_WINDOWS
        return ERHIInstanceType::D3D12;
#else
        LOG_ERROR("D3D12RHI Is not supported on this platform");
#endif
    }
    else if (RHITypeString == "Metal")
    {
#if PLATFORM_MACOS
        return ERHIInstanceType::Metal;
#else
        LOG_ERROR("MetalRHI Is not supported on this platform");
#endif
    }
    else if (RHITypeString == "Null")
    {
        return ERHIInstanceType::Null;
    }

    return ERHIInstanceType::Unknown;
}

static ERHIInstanceType GetRHIType()
{
    ERHIInstanceType InstanceType = GetRHITypeFromConfig();
    if (InstanceType == ERHIInstanceType::Unknown)
    {
#if PLATFORM_WINDOWS
        return ERHIInstanceType::D3D12;
#elif PLATFORM_MACOS
        return ERHIInstanceType::Metal;
#else
        return ERHIInstanceType::Null;
#endif
    }

    return InstanceType;
}


bool RHIInitialize()
{
    // Select RHI
    ERHIInstanceType InstanceType = GetRHIType();

    // Load Selected RHI
    FRHIInterfaceModule* RHIModule = nullptr;
    if (InstanceType == ERHIInstanceType::D3D12)
    {
        RHIModule = FModuleManager::Get().LoadModule<FRHIInterfaceModule>("D3D12RHI");
    }
    else if (InstanceType == ERHIInstanceType::Metal)
    {
        RHIModule = FModuleManager::Get().LoadModule<FRHIInterfaceModule>("MetalRHI");
    }
    else if (InstanceType == ERHIInstanceType::Null)
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
        LOG_ERROR("[RHIInitialize] Failed to �nitialize RHIInterface, the application has to terminate");
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
