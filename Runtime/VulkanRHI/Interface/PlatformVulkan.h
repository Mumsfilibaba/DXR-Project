#pragma once
#include "VulkanCore.h"

#include "Core/Containers/Array.h"
#include "Core/Modules/Platform/PlatformLibrary.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CPlatformVulkan

class CPlatformVulkan
{
public:

    static FORCEINLINE TArray<const char*> GetOptionalInstanceExtentions()
    {
        return
        {
        #if VK_KHR_get_physical_device_properties2
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        #endif
        };
    }
    
    static FORCEINLINE TArray<const char*> GetOptionalDeviceExtentions()
    {
        return
        {
            // This extension must be enabled on platforms that has it available
            "VK_KHR_portability_subset",
            
        #if VK_KHR_get_memory_requirements2
            VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        #endif
        #if VK_KHR_maintenance1
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        #endif
        #if VK_KHR_maintenance2
            VK_KHR_MAINTENANCE2_EXTENSION_NAME,
        #endif
        #if VK_KHR_maintenance3
            VK_KHR_MAINTENANCE3_EXTENSION_NAME,
        #endif
        #if VK_KHR_maintenance4
            VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
        #endif
        #if VK_EXT_descriptor_indexing
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        #endif
        #if VK_KHR_buffer_device_address
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        #endif
        #if VK_KHR_deferred_host_operations
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        #endif
        #if VK_KHR_pipeline_library
            VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        #endif
        #if VK_KHR_timeline_semaphore
            VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
        #endif
        #if VK_KHR_shader_draw_parameters
            VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
        #endif
        #if VK_NV_mesh_shader
            VK_NV_MESH_SHADER_EXTENSION_NAME,
        #endif
        #if VK_EXT_memory_budget
            VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
        #endif
        #if VK_KHR_push_descriptor
            VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        #endif
        #if VK_KHR_ray_query
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
        #endif
        #if VK_KHR_ray_tracing_pipeline
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        #endif
        #if VK_KHR_acceleration_structure
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        #endif
        #if VK_KHR_dedicated_allocation
            VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
        #endif
        };
    }
    
    static FORCEINLINE TArray<const char*> GetRequiredInstanceExtensions() { return TArray<const char*>(); }
    static FORCEINLINE TArray<const char*> GetRequiredInstanceLayers()     { return TArray<const char*>(); }

    static FORCEINLINE TArray<const char*> GetRequiredDeviceExtensions() { return TArray<const char*>(); }
    static FORCEINLINE TArray<const char*> GetRequiredDeviceLayers()     { return TArray<const char*>(); }

    static FORCEINLINE DynamicLibraryHandle LoadVulkanLibrary() { return 0; }

#if VK_KHR_surface
    static FORCEINLINE VkResult CreateSurface(VkInstance Instance, void* InWindowHandle, VkSurfaceKHR* OutSurface) { return VK_ERROR_UNKNOWN; }
#endif
};
