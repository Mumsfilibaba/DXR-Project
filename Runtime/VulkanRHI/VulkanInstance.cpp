#include "Core/Templates/CString.h"
#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanInstance.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanExtensions.h"
#include "VulkanRHI/Platform/VulkanPlatform.h"

static TAutoConsoleVariable<bool> CVarVulkanVerboseLogging(
    TEXT("VulkanRHI.VerboseLogging"),
    TEXT("Enables more logging within VulkanRHI"),
    true);

#if VULKAN_ENABLE_GPU_VALIDATION
static TAutoConsoleVariable<bool> CVarVulkanEnableGPUAssistedValidation(
    TEXT("VulkanRHI.EnableGPUAssistedValidation"),
    TEXT("Enable GPU-Assisted Validation to detect shader-level out-of-bounds buffer accesses at runtime. Requires the debug layer."),
    false);

static TAutoConsoleVariable<bool> CVarVulkanEnableSynchronizationValidation(
    TEXT("VulkanRHI.EnableSynchronizationValidation"),
    TEXT("Also enable Khronos synchronization validation. Only takes effect together with VulkanRHI.EnableGPUAssistedValidation."),
    false);
#endif

FVulkanInstance::FVulkanInstance()
    : DriverHandle(nullptr)
    , Instance(VK_NULL_HANDLE)
    , ExtensionNames()
    , LayerNames()
{
}

FVulkanInstance::~FVulkanInstance()
{
    Release();
}

bool FVulkanInstance::Initialize(FVulkanInstanceCreateInfo& CreateInfo)
{
    DriverHandle = VulkanPlatform::LoadVulkanLibrary();
    if (!DriverHandle)
    {
        VULKAN_ERROR_CRITICAL("Failed to load Vulkan library");
        return false;
    }

    vkGetInstanceProcAddr = FPlatformLibrary::LoadSymbol<PFN_vkGetInstanceProcAddr>(TEXT("vkGetInstanceProcAddr"), DriverHandle);
    if (!vkGetInstanceProcAddr)
    {
        VULKAN_ERROR_CRITICAL("Failed to load vkGetInstanceProcAddr");
        return false;
    }

    if (!VulkanLoader::LoadGlobalFunctions())
    {
        return false;
    }

    VkResult Result = VK_SUCCESS;

    // Instance Layers
    uint32 LayerPropertiesCount = 0;
    Result = vkEnumerateInstanceLayerProperties(&LayerPropertiesCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve Instance LayerProperties Count");
        return false;
    }

    TArray<VkLayerProperties> LayerProperties(LayerPropertiesCount);
    Result = vkEnumerateInstanceLayerProperties(&LayerPropertiesCount, LayerProperties.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve Instance LayerProperties");
        return false;
    }

    // Instance Extensions
    uint32 ExtensionPropertiesCount = 0;
    Result = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionPropertiesCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve Instance ExtensionProperties Count");
        return false;
    }

    TArray<VkExtensionProperties> ExtensionProperties(ExtensionPropertiesCount);
    Result = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionPropertiesCount, ExtensionProperties.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve Instance ExtensionProperties");
        return false;
    }

    const bool bVerboseLogging = CVarVulkanVerboseLogging.GetValue();
    if (bVerboseLogging)
    {
        if (!ExtensionProperties.IsEmpty())
        {
            VULKAN_INFO("Available Instance Extensions:");
            for (const VkExtensionProperties& ExtensionProperty : ExtensionProperties)
            {
                LOG_INFO("    %s", StringCast<CHAR_T>(ExtensionProperty.extensionName).Get());
            }
        }
        else
        {
            VULKAN_INFO("No available Instance Extensions");
        }

        if (!LayerProperties.IsEmpty())
        {
            VULKAN_INFO("Available Instance Layers:");
            for (const VkLayerProperties& LayerProperty : LayerProperties)
            {
                LOG_INFO("    %s: %s", StringCast<CHAR_T>(LayerProperty.layerName).Get(), StringCast<CHAR_T>(LayerProperty.description).Get());
            }
        }
        else
        {
            VULKAN_INFO("No available Instance Layers");
        }
    }

    // Verify Layers
    TArray<const ANSICHAR*> EnabledLayerNames;
    for (const VkLayerProperties& LayerProperty : LayerProperties)
    {
        const auto MatchLayer = [=](const ANSICHAR* Other) -> bool { return FPlatformString::Strcmp(LayerProperty.layerName, Other) == 0; };

        if (CreateInfo.RequiredLayerNames.ContainsWithPredicate(MatchLayer) || CreateInfo.OptionalLayerNames.ContainsWithPredicate(MatchLayer))
        {
            EnabledLayerNames.Add(LayerProperty.layerName);
            LayerNames.Emplace(LayerProperty.layerName);
        }
    }

    for (const ANSICHAR* LayerName : CreateInfo.RequiredLayerNames)
    {
        const auto MatchLayer = [=](const ANSICHAR* Other) -> bool { return FPlatformString::Strcmp(LayerName, Other) == 0; };

        if (!EnabledLayerNames.ContainsWithPredicate(MatchLayer))
        {
            VULKAN_ERROR_CRITICAL("Instance layer '%s' could not be enabled", StringCast<CHAR_T>(LayerName).Get());
            return false;
        }
    }

    for (const ANSICHAR* LayerName : EnabledLayerNames)
    {
        uint32 LayerExtensionCount = 0;
        Result = vkEnumerateInstanceExtensionProperties(LayerName, &LayerExtensionCount, nullptr);
        if (VULKAN_FAILED(Result) || LayerExtensionCount == 0)
        {
            continue;
        }

        TArray<VkExtensionProperties> LayerExtensions(LayerExtensionCount);
        Result = vkEnumerateInstanceExtensionProperties(LayerName, &LayerExtensionCount, LayerExtensions.Data());
        if (VULKAN_FAILED(Result))
        {
            continue;
        }

        for (const VkExtensionProperties& LayerExtension : LayerExtensions)
        {
            const auto MatchExtension = [&](const VkExtensionProperties& Other) -> bool { return FPlatformString::Strcmp(LayerExtension.extensionName, Other.extensionName) == 0; };
            if (!ExtensionProperties.ContainsWithPredicate(MatchExtension))
            {
                ExtensionProperties.Add(LayerExtension);
            }
        }
    }

    // Resolve instance extensions
    TArray<const ANSICHAR*> EnabledExtensionNames;
    for (const TUniquePtr<FVulkanInstanceExtension>& Extension : CreateInfo.Extensions)
    {
        Extension->SetEnabled(false);

        if (!Extension->ShouldEnable())
        {
            continue;
        }

        for (const VkExtensionProperties& Property : ExtensionProperties)
        {
            if (FPlatformString::Strcmp(Extension->GetExtensionName(), Property.extensionName) == 0)
            {
                Extension->SetEnabled(true);
                EnabledExtensionNames.Add(Property.extensionName);
                ExtensionNames.Emplace(Property.extensionName);
                break;
            }
        }

        if (!Extension->IsEnabled() && Extension->IsRequired())
        {
            VULKAN_ERROR_CRITICAL("Required instance extension '%s' is not available", StringCast<CHAR_T>(Extension->GetExtensionName()).Get());
            return false;
        }
    }

    if (bVerboseLogging)
    {
        if (!EnabledLayerNames.IsEmpty())
        {
            VULKAN_INFO("Enabled Instance Layers:");

            for (const ANSICHAR* LayerName : EnabledLayerNames)
            {
                LOG_INFO("    %s", StringCast<CHAR_T>(LayerName).Get());
            }
        }

        if (!EnabledExtensionNames.IsEmpty())
        {
            VULKAN_INFO("Enabled Instance Extensions:");

            for (const ANSICHAR* ExtensionName : EnabledExtensionNames)
            {
                LOG_INFO("    %s", StringCast<CHAR_T>(ExtensionName).Get());
            }
        }
    }

    VkApplicationInfo ApplicationInfo = {};
    ApplicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ApplicationInfo.pNext              = nullptr;
    ApplicationInfo.apiVersion         = VULKAN_TARGET_API_VERSION;
    ApplicationInfo.pApplicationName   = "DXR-Project";
    ApplicationInfo.pEngineName        = "DXR-Engine";
    ApplicationInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo InstanceCreateInfo = {};
    InstanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.flags                   = 0;
    InstanceCreateInfo.pApplicationInfo        = &ApplicationInfo;
    InstanceCreateInfo.enabledExtensionCount   = EnabledExtensionNames.Size();
    InstanceCreateInfo.ppEnabledExtensionNames = EnabledExtensionNames.Data();
    InstanceCreateInfo.enabledLayerCount       = EnabledLayerNames.Size();
    InstanceCreateInfo.ppEnabledLayerNames     = EnabledLayerNames.Data();

    for (const TUniquePtr<FVulkanInstanceExtension>& Extension : CreateInfo.Extensions)
    {
        if (Extension->IsEnabled())
        {
            Extension->PrepareInstanceCreateInfo(InstanceCreateInfo);
        }
    }

    Result = vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Instance");
        return false;
    }

    return true;
}

void FVulkanInstance::Release()
{
    if (VULKAN_CHECK_HANDLE(Instance))
    {
        vkDestroyInstance(Instance, nullptr);
        Instance = VK_NULL_HANDLE;
    }

    if (DriverHandle)
    {
        FPlatformLibrary::FreeDynamicLib(DriverHandle);
        DriverHandle = nullptr;
    }
}
