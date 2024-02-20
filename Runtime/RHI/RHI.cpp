#include "RHI.h"
#include "RHICommandList.h"
#include "ShaderCompiler.h"
#include "Core/Misc/EngineConfig.h"
#include "Core/Misc/ConsoleManager.h"

IMPLEMENT_ENGINE_MODULE(FRHIModule, RHI);

static TAutoConsoleVariable<bool> CVarEnableDebugLayer(
    "RHI.EnableDebugLayer", 
    "Enables the DebugLayer for the RHI",
    false);

static TAutoConsoleVariable<FString> CVarType(
    "RHI.Type", 
    "Selects the RHI Layer to use",
    "Unknown");

RHI_API FRHI* GRHI = nullptr;

static FRHIModule* LoadNullRHI()
{
    return FModuleManager::Get().LoadModule<FRHIModule>("NullRHI");
}

static ERHIType GetRHITypeFromConfig()
{
    const FString RHITypeString = CVarType->GetString();
    if (RHITypeString.Equals("D3D12", EStringCaseType::NoCase))
    {
#if PLATFORM_WINDOWS
        return ERHIType::D3D12;
#else
        LOG_ERROR("D3D12RHI Is not supported on this platform");
#endif
    }
    else if (RHITypeString.Equals("Vulkan", EStringCaseType::NoCase))
    {
        return ERHIType::Vulkan;
    }
    else if (RHITypeString.Equals("Metal", EStringCaseType::NoCase))
    {
#if PLATFORM_MACOS
        return ERHIType::Metal;
#else
        LOG_ERROR("MetalRHI Is not supported on this platform");
#endif
    }
    else if (RHITypeString.Equals("Null", EStringCaseType::NoCase))
    {
        return ERHIType::Null;
    }

    return ERHIType::Unknown;
}

static ERHIType GetRHIType()
{
    ERHIType InstanceType = GetRHITypeFromConfig();
    if (InstanceType == ERHIType::Unknown)
    {
#if PLATFORM_WINDOWS
        return ERHIType::D3D12;
#elif PLATFORM_MACOS
        return ERHIType::Metal;
#else
        return ERHIType::Null;
#endif
    }

    return InstanceType;
}


bool RHIInitialize()
{
    // Select RHI
    ERHIType InstanceType = GetRHIType();

    // Load Selected RHI
    FRHIModule* RHIModule = nullptr;
    if (InstanceType == ERHIType::D3D12)
    {
        RHIModule = FModuleManager::Get().LoadModule<FRHIModule>("D3D12RHI");
    }
    else if (InstanceType == ERHIType::Vulkan)
    {
        RHIModule = FModuleManager::Get().LoadModule<FRHIModule>("VulkanRHI");
    }
    else if (InstanceType == ERHIType::Metal)
    {
        RHIModule = FModuleManager::Get().LoadModule<FRHIModule>("MetalRHI");
    }
    else if (InstanceType == ERHIType::Null)
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

    FRHI* RHIInterface = RHIModule->CreateRHI();
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

    GRHI = RHIInterface;

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

    if (GRHI)
    {
        delete GRHI;
        GRHI = nullptr;
    }
}
