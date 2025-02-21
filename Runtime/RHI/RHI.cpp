#include "Core/Misc/EngineConfig.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RHIValidation.h"

IMPLEMENT_ENGINE_MODULE(FRHIModule, RHI);

static TAutoConsoleVariable<bool> CVarEnableDebugLayer(
    "RHI.EnableDebugLayer", 
    "Enables the DebugLayer for the RHI",
    false);

static TAutoConsoleVariable<bool> CVarEnableValidation(
    "RHI.EnableValidation",
    "Enables the custom RHI-validation",
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
        return ERHIType::D3D12;
    }
    else if (RHITypeString.Equals("Vulkan", EStringCaseType::NoCase))
    {
        return ERHIType::Vulkan;
    }
    else if (RHITypeString.Equals("Metal", EStringCaseType::NoCase))
    {
        return ERHIType::Metal;
    }
    else if (RHITypeString.Equals("Null", EStringCaseType::NoCase))
    {
        return ERHIType::Null;
    }

    return ERHIType::Unknown;
}

static ERHIType GetPlatformDefaultRHI()
{
#if PLATFORM_WINDOWS
    return ERHIType::D3D12;
#elif PLATFORM_MACOS
    return ERHIType::Vulkan;
#else
    return ERHIType::Null;
#endif
}

static bool IsRHISupportedByPlatform(ERHIType RHIType)
{
    switch(RHIType)
    {
        case ERHIType::D3D12:
        {
        #if PLATFORM_WINDOWS
            return true;
        #else 
            return false;
        #endif
        }

        case ERHIType::Metal:
        {
        #if PLATFORM_MACOS
            return true;
        #else 
            return false;
        #endif
        }

        case ERHIType::Vulkan:
        case ERHIType::Null:
        {
            // Vulkan and NullRHI are valid on all platforms
            return true;
        }

        default:
        {
            // Invalid RHI type
            return false;
        }
    }
}

static ERHIType GetRHIType()
{
    ERHIType RHIType = GetRHITypeFromConfig();

    const bool bIsRHISupported = IsRHISupportedByPlatform(RHIType);
    if (RHIType == ERHIType::Unknown || !bIsRHISupported)
    {
        switch(RHIType)
        {
            case ERHIType::D3D12:
            {
                LOG_ERROR("D3D12RHI Is not supported on this platform, falling back to default RHI for the platform");
                break;
            }

            case ERHIType::Metal:
            {
                LOG_ERROR("MetalRHI Is not supported on this platform, falling back to default RHI for the platform");
                break;
            }

            case ERHIType::Vulkan:
            {
                LOG_ERROR("VulkanRHI Is not supported on this platform, falling back to default RHI for the platform");
                break;
            }

            case ERHIType::Null:
            {
                LOG_ERROR("NullRHI Is not supported on this platform, falling back to default RHI for the platform");
                break;
            }

            default:
            {
                LOG_ERROR("Trying to load unknown RHI-module, falling back to default RHI for the platform");
                break;
            }
        }

        return GetPlatformDefaultRHI();
    }

    return RHIType;
}

bool RHIInitialize()
{
    // Select RHI
    ERHIType RHIType = GetRHIType();

    // Load Selected RHI
    FRHIModule* RHIModule = nullptr;
    if (RHIType == ERHIType::D3D12)
    {
        RHIModule = FModuleManager::Get().LoadModule<FRHIModule>("D3D12RHI");
    }
    else if (RHIType == ERHIType::Vulkan)
    {
        RHIModule = FModuleManager::Get().LoadModule<FRHIModule>("VulkanRHI");
    }
    else if (RHIType == ERHIType::Metal)
    {
        RHIModule = FModuleManager::Get().LoadModule<FRHIModule>("MetalRHI");
    }
    else if (RHIType == ERHIType::Null)
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

    FRHI* LocalRHI = RHIModule->CreateRHI();
    if (!LocalRHI)
    {
        LOG_ERROR("[RHIInitialize] Failed to create RHIInterface, the application has to terminate");
        return false;
    }

    // Create the validation interface if chosen
    if (CVarEnableValidation.GetValue())
    {
        FRHIValidation* ValidationRHI = new FRHIValidation(LocalRHI);
        LocalRHI = ValidationRHI;
    }

    if (!LocalRHI->Initialize())
    {
        LOG_ERROR("[RHIInitialize] Failed to initialize RHIInterface, the application has to terminate");
        return false;
    }

    GRHI = LocalRHI;

    // Initialize the CommandListExecutor
    if (!FRHICommandListExecutor::Initialize())
    {
        return false;
    }

    return true;
}

void RHIRelease()
{
    // The RHI-implementation might need the executor in the destructor so we flush before we delete it
    FRHICommandListExecutor::Get().FlushDeletedResources();

    if (GRHI)
    {
        delete GRHI;
        GRHI = nullptr;
    }

    FRHICommandListExecutor::Release();
}
