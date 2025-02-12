#include "Core/Templates/CString.h"
#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanInstance.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/Platform/PlatformVulkan.h"

static TAutoConsoleVariable<bool> CVarVulkanVerboseLogging(
    "VulkanRHI.VerboseLogging",
    "Enables more logging within VulkanRHI",
    true);

static TAutoConsoleVariable<bool> CVarBreakOnValidationError(
    "VulkanRHI.BreakOnValidationError",
    "Enables breakpoints when the validation-layer encounters an error",
    true);

DISABLE_UNREFERENCED_VARIABLE_WARNING

VKAPI_ATTR VkBool32 VKAPI_CALL DebugLayerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData)
{
    if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LOG_ERROR("[Vulkan Validation layer] %s", CallbackData->pMessage);
        
        if (CVarBreakOnValidationError.GetValue())
        {
            DEBUG_BREAK();
        }
    }
    else if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOG_WARNING("[Vulkan Validation layer] %s", CallbackData->pMessage);
    }

    return VK_FALSE;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

FVulkanInstance::FVulkanInstance()
    : DriverHandle(nullptr)
    , Instance(VK_NULL_HANDLE)
    , DebugMessenger()
    , ExtensionNames()
    , LayerNames()
{
}

FVulkanInstance::~FVulkanInstance()
{
    Release();
}

bool FVulkanInstance::Initialize(const FVulkanInstanceCreateInfo& InstanceDesc)
{
    DriverHandle = FPlatformVulkan::LoadVulkanLibrary();
    if (!DriverHandle)
    {
        VULKAN_ERROR("Failed to load Vulkan library");
        return false;
    }

    vkGetInstanceProcAddr = FPlatformLibrary::LoadSymbol<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr", DriverHandle);
    if (!vkGetInstanceProcAddr)
    {
        VULKAN_ERROR("Failed to load vkGetInstanceProcAddr");
        return false;
    }

    VULKAN_LOAD_INSTANCE_FUNCTION(Instance, CreateInstance);
    VULKAN_LOAD_INSTANCE_FUNCTION(Instance, EnumerateInstanceLayerProperties);
    VULKAN_LOAD_INSTANCE_FUNCTION(Instance, EnumerateInstanceExtensionProperties);

    VkResult Result = VK_SUCCESS;

    // Instance Layers
    uint32 LayerPropertiesCount = 0;
    Result = vkEnumerateInstanceLayerProperties(&LayerPropertiesCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve Instance LayerProperties Count");
        return false;
    }

    TArray<VkLayerProperties> LayerProperties(LayerPropertiesCount);
    Result = vkEnumerateInstanceLayerProperties(&LayerPropertiesCount, LayerProperties.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve Instance LayerProperties");
        return false;
    }

    // Instance Extensions
    uint32 ExtensionPropertiesCount = 0;
    Result = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionPropertiesCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve Instance ExtensionProperties Count");
        return false;
    }

    TArray<VkExtensionProperties> ExtensionProperties(ExtensionPropertiesCount);
    Result = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionPropertiesCount, ExtensionProperties.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve Instance ExtensionProperties");
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
                LOG_INFO("    %s", ExtensionProperty.extensionName);
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
                LOG_INFO("    %s: %s", LayerProperty.layerName, LayerProperty.description);
            }
        }
        else
        {
            VULKAN_INFO("No available Instance Layers");
        }
    }

    // Verify Layers
    TArray<const CHAR*> EnabledLayerNames;
    for (const VkLayerProperties& LayerProperty : LayerProperties)
    {
        const auto CompareLayer = [=](const CHAR* Other) -> bool
        {
            return FCString::Strcmp(LayerProperty.layerName, Other) == 0;
        };

        if (InstanceDesc.RequiredLayerNames.ContainsWithPredicate(CompareLayer) || InstanceDesc.OptionalLayerNames.ContainsWithPredicate(CompareLayer))
        {
            EnabledLayerNames.Add(LayerProperty.layerName);
            LayerNames.Emplace(LayerProperty.layerName);
        }
    }

    for (const CHAR* LayerName : InstanceDesc.RequiredLayerNames)
    {
        const auto CompareLayer = [=](const CHAR* Other) -> bool
        {
            return FCString::Strcmp(LayerName, Other) == 0;
        };
        
        if (!EnabledLayerNames.ContainsWithPredicate(CompareLayer))
        {
            VULKAN_ERROR("Instance layer '%s' could not be enabled", LayerName);
            return false;
        }
    }

    // Verify Extensions
    TArray<const CHAR*> EnabledExtensionNames;
    for (const VkExtensionProperties& ExtensionProperty : ExtensionProperties)
    {
        const auto CompareExtension = [=](const CHAR* Other) -> bool
        {
            return FCString::Strcmp(ExtensionProperty.extensionName, Other) == 0;
        };

        if (InstanceDesc.RequiredExtensionNames.ContainsWithPredicate(CompareExtension) || InstanceDesc.OptionalExtensionNames.ContainsWithPredicate(CompareExtension))
        {
            EnabledExtensionNames.Add(ExtensionProperty.extensionName);
            ExtensionNames.Emplace(ExtensionProperty.extensionName);
        }
    }

    for (const CHAR* ExtensionName : InstanceDesc.RequiredExtensionNames)
    {
        const auto CompareExtension = [=](const CHAR* Other) -> bool
        {
            return FCString::Strcmp(ExtensionName, Other) == 0;
        };
        
        if (!EnabledExtensionNames.ContainsWithPredicate(CompareExtension))
        {
            VULKAN_ERROR("Instance layer '%s' could not be enabled", ExtensionName);
            return false;
        }
    }

    if (bVerboseLogging)
    {
        if (!EnabledLayerNames.IsEmpty())
        {
            VULKAN_INFO("Enabled Instance Layers:");

            for (const CHAR* LayerName : EnabledLayerNames)
            {
                LOG_INFO("    %s", LayerName);
            }
        }

        if (!EnabledExtensionNames.IsEmpty())
        {
            VULKAN_INFO("Enabled Instance Extensions:");

            for (const CHAR* ExtensionName : EnabledExtensionNames)
            {
                LOG_INFO("    %s", ExtensionName);
            }
        }
    }

    VkApplicationInfo ApplicationInfo;
    FMemory::Memzero(&ApplicationInfo);

    ApplicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ApplicationInfo.pNext              = nullptr;
    ApplicationInfo.apiVersion         = VK_API_VERSION_1_2;
    ApplicationInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    ApplicationInfo.pApplicationName   = "DXR-Project";
    ApplicationInfo.pEngineName        = "DXR-Engine";
    ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo InstanceCreateInfo;
    FMemory::Memzero(&InstanceCreateInfo);

    InstanceCreateInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.flags                    = 0;
    InstanceCreateInfo.pApplicationInfo         = &ApplicationInfo;
    InstanceCreateInfo.enabledExtensionCount    = EnabledExtensionNames.Size();
    InstanceCreateInfo.ppEnabledExtensionNames  = EnabledExtensionNames.Data();
    InstanceCreateInfo.enabledLayerCount        = EnabledLayerNames.Size();
    InstanceCreateInfo.ppEnabledLayerNames      = EnabledLayerNames.Data();

#if VK_KHR_portability_enumeration
    if (IsExtensionEnabled(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
        InstanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
#endif
    
    FVulkanStructureHelper InstanceCreateHelper(InstanceCreateInfo);

    bool bEnableDebugLayer = false;
    if (IConsoleVariable* CVarEnableDebugLayer = FConsoleManager::Get().FindConsoleVariable("RHI.EnableDebugLayer"))
    {
        bEnableDebugLayer = CVarEnableDebugLayer->GetBool();
    }

#if VK_EXT_debug_utils
    VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo;
    FMemory::Memzero(&DebugMessengerCreateInfo);

    if (bEnableDebugLayer)
    {
        DebugMessengerCreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        DebugMessengerCreateInfo.flags           = 0;
        DebugMessengerCreateInfo.pNext           = nullptr;
        DebugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        DebugMessengerCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        DebugMessengerCreateInfo.pfnUserCallback = DebugLayerCallback;
        DebugMessengerCreateInfo.pUserData       = nullptr;

        InstanceCreateHelper.AddNext(DebugMessengerCreateInfo);
    }
#endif

    Result = vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Instance");
        return false;
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    // Load functions that require the instance to be created

    VULKAN_LOAD_INSTANCE_FUNCTION(Instance, DestroyInstance);

    // Initialize DebugUtils extension helper
    if (!FVulkanDebugUtilsEXT::Initialize(this))
    {
        return false;
    }
    
#if VK_EXT_debug_utils
    if (FVulkanDebugUtilsEXT::IsEnabled() && bEnableDebugLayer)
    {
        Result = vkCreateDebugUtilsMessengerEXT(Instance, &DebugMessengerCreateInfo, nullptr, &DebugMessenger);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("Failed to create DebugMessenger");
            return false;
        }
    }
#endif

    return true;
}

void FVulkanInstance::Release()
{
#if VK_EXT_debug_utils
    if (VULKAN_CHECK_HANDLE(DebugMessenger))
    {
        vkDestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
        DebugMessenger = VK_NULL_HANDLE;
    }
#endif

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
