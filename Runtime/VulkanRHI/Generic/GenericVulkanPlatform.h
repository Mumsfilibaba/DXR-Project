#pragma once
#include "VulkanCore.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/PlatformLibrary.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FGenericVulkanPlatform
{
    static FORCEINLINE TArray<const CHAR*> GetOptionalInstanceExtentions()
    {
        return
        {
        #if VK_KHR_get_physical_device_properties2
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        #endif
        #if VK_KHR_device_group_creation
            VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
        #endif
        };
    }
    
    static FORCEINLINE TArray<const CHAR*> GetOptionalDeviceExtentions()
    {
        return
        {
            // NOTE: This extension must be enabled on platforms that has it available
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
        #if VK_KHR_ray_tracing_maintenance1
            VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME,
        #endif
        #if VK_KHR_dedicated_allocation
            VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
        #endif
        #if VK_KHR_spirv_1_4
            VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        #endif
        #if VK_KHR_shader_float_controls
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
        #endif
        #if VK_KHR_device_group
            VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
        #endif
        #if VK_KHR_synchronization2
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        #endif
        #if VK_EXT_descriptor_indexing
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        #endif
        #if VK_EXT_memory_budget
            VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
        #endif
        #if VK_EXT_mesh_shader
            VK_EXT_MESH_SHADER_EXTENSION_NAME,
        #endif
        #if VK_EXT_descriptor_buffer
            VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
        #endif
        #if VK_NV_ray_tracing_invocation_reorder
            VK_NV_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME,
        #endif
        };
    }
    
    static FORCEINLINE TArray<const CHAR*> GetRequiredInstanceExtensions() { return TArray<const CHAR*>(); }

    static FORCEINLINE TArray<const CHAR*> GetRequiredInstanceLayers() { return TArray<const CHAR*>(); }

    static FORCEINLINE TArray<const CHAR*> GetRequiredDeviceExtensions() { return TArray<const CHAR*>(); }
    
    static FORCEINLINE TArray<const CHAR*> GetRequiredDeviceLayers() { return TArray<const CHAR*>(); }

    static FORCEINLINE void* LoadVulkanLibrary() { return 0; }

#if VK_KHR_surface
    static FORCEINLINE VkResult CreateSurface(VkInstance Instance, void* InWindowHandle, VkSurfaceKHR* OutSurface) { return VK_ERROR_UNKNOWN; }
#endif
};

ENABLE_UNREFERENCED_VARIABLE_WARNING