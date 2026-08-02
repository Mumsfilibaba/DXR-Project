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

static TAutoConsoleVariable<String> CVarType(
    "RHI.Type", 
    "Selects the RHI Layer to use",
    "Unknown");

static FRHIModule* LoadNullRHI()
{
    return FModuleManager::Get().LoadModule<FRHIModule>("NullRHI");
}

static ERHIType GetRHITypeFromConfig()
{
    const String RHITypeString = CVarType->GetString();
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


RHI_API void RHI::DumpCapabilities()
{
    const auto YesNo = [](bool bBoolean) -> const char*
    {
        return bBoolean ? "Yes" : "No";
    };

    LOG_INFO("[RHI] ---------------------------------- Device Feature Support ----------------------------------");

    // -------------------------------------------------------------------------------------------
    // Shader / Pipeline Features
    // -------------------------------------------------------------------------------------------
    LOG_INFO("[RHI] Max Shader Model                          : %s", ToString(RHI::MaxShaderModel));
    LOG_INFO("[RHI] Geometry Shaders                          : %s", YesNo(RHI::bSupportsGeometryShaders));
    LOG_INFO("[RHI] SV_RenderTargetArrayIndex from VS         : %s", YesNo(RHI::bSupportRenderTargetArrayIndexFromVertexShader));
    LOG_INFO("[RHI] Bindless                                  : %s", YesNo(RHI::bSupportsBindless));

    // -------------------------------------------------------------------------------------------
    // View Instancing
    // -------------------------------------------------------------------------------------------
    LOG_INFO("[RHI] View Instancing                           : %s", YesNo(RHI::bSupportsViewInstancing));
    LOG_INFO("[RHI]   Max View Instances                      : %u", RHI::MaxViewInstanceCount);

    // -------------------------------------------------------------------------------------------
    // Hardware Ray Tracing
    // -------------------------------------------------------------------------------------------
    LOG_INFO("[RHI] Ray Tracing                               : %s", YesNo(RHI::bSupportsRayTracing));
    LOG_INFO("[RHI]   Tier                                    : %s", ToString(RHI::RayTracingTier));
    LOG_INFO("[RHI]   Max Recursion Depth                     : %u", RHI::RayTracingMaxRecursionDepth);
    
    RHI::DumpRayTracingCapabilities();
    
    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading (VRS)
    // -------------------------------------------------------------------------------------------
    LOG_INFO("[RHI] Variable Rate Shading                     : %s", YesNo(RHI::bSupportsVRS));
    LOG_INFO("[RHI]   Tier                                    : %s", ToString(RHI::ShadingRateTier)); 
    LOG_INFO("[RHI]   Shading Rate Image Tile Size            : %u", RHI::ShadingRateImageTileSize);
    
    // -------------------------------------------------------------------------------------------
    // Draw Indirect
    // -------------------------------------------------------------------------------------------
    LOG_INFO("[RHI] DrawIndirect                              : %s", YesNo(RHI::bSupportsDrawIndirect));
    LOG_INFO("[RHI]   DrawIndirectCount                       : %s", YesNo(RHI::bSupportsDrawIndirectCount));
    LOG_INFO("[RHI]   DispatchIndirect                        : %s", YesNo(RHI::bSupportsDispatchIndirect));
    LOG_INFO("[RHI]   DispatchMeshIndirect                    : %s", YesNo(RHI::bSupportsDispatchMeshIndirect));
    LOG_INFO("[RHI]   DispatchMeshIndirectCount               : %s", YesNo(RHI::bSupportsDispatchMeshIndirectCount));
    LOG_INFO("[RHI]   Max Draw Commands Per Indirect Call     : %u", RHI::MaxDrawIndirectCommandCount);
    LOG_INFO("[RHI]   Max Mesh Commands Per Indirect Call     : %u", RHI::MaxDispatchMeshIndirectCommandCount);

    // -------------------------------------------------------------------------------------------
    // Texture / Image Limits
    // -------------------------------------------------------------------------------------------
    LOG_INFO("[RHI] Texture / Image Limits:");

    LOG_INFO("[RHI]   Texture1D:  MaxWidth                    : %u", RHI::MaxTexture1DSize);
    LOG_INFO("[RHI]               MaxArrayLayers              : %u", RHI::MaxTexture1DArrayLayers);
    
    LOG_INFO("[RHI]   Texture2D:  MaxSize (W/H)               : %u", RHI::MaxTexture2DSize);
    LOG_INFO("[RHI]               MaxArrayLayers              : %u", RHI::MaxTexture2DArrayLayers);
    
    LOG_INFO("[RHI]   Texture3D:  MaxWidth                    : %u", RHI::MaxTexture3DWidth);
    LOG_INFO("[RHI]               MaxHeight                   : %u", RHI::MaxTexture3DHeight);
    LOG_INFO("[RHI]               MaxDepth                    : %u", RHI::MaxTexture3DDepth);
    
    LOG_INFO("[RHI]   CubeTexure: MaxFaceResolution           : %u", RHI::MaxCubeTextureSize);
    LOG_INFO("[RHI]               MaxCubeArrayCount           : %u", RHI::MaxCubeArrayCount);

    // -------------------------------------------------------------------------------------------
    // Buffer / Memory Limits
    // -------------------------------------------------------------------------------------------
    LOG_INFO("[RHI] Buffer / Memory Limits:");
    
    LOG_INFO("[RHI]   MaxBufferSize                           : %llu", static_cast<uint64>(RHI::MaxBufferSize)); 
    
    LOG_INFO("[RHI]   ConstantBuffer:   MaxSize               : %u", RHI::MaxConstantBufferSize); 
    
    LOG_INFO("[RHI]   StorageBuffer:    MaxSize               : %llu", static_cast<uint64>(RHI::MaxStorageBufferSize));
    
    LOG_INFO("[RHI]   StructuredBuffer: MinStride             : %u", RHI::StructuredBufferMinStride);
    LOG_INFO("[RHI]                     MaxStride             : %u", RHI::StructuredBufferMaxStride);

    LOG_INFO("[RHI]   RawBuffer:        RequiredAlignment     : %u", RHI::RawBufferRequiredAlignment);

    LOG_INFO("[RHI] --------------------------------------------------------------------------------------------");
}


RHI_API FRHIDevice* RHI::Device = nullptr;

bool RHI::Initialize()
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

    FRHIDevice* LocalRHI = RHIModule->CreateDevice();
    if (!LocalRHI)
    {
        LOG_ERROR("[RHIInitialize] Failed to create RHIInterface, the application has to terminate");
        return false;
    }

    // Create the validation interface if chosen
    if (CVarEnableValidation.GetValue())
    {
        FRHIValidationDevice* ValidationDevice = new FRHIValidationDevice(LocalRHI);
        LocalRHI = ValidationDevice;
    }

    Device = LocalRHI;

    // Initialize the CommandListExecutor
    if (!FRHICommandListExecutor::Initialize())
    {
        return false;
    }

    // Log features of the loaded device and RHI
    DumpCapabilities();
    return true;
}

void RHI::Release()
{
    // The RHI-implementation might need the executor in the destructor so we flush before we delete it
    if (FRHICommandListExecutor::IsInitialized())
    {
        FRHICommandListExecutor::Get().FlushDeletedResources();
    }

    SAFE_DELETE(Device);

    FRHICommandListExecutor::Release();
}
