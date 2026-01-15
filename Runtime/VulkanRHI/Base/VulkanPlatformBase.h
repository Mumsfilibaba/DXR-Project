#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/PlatformLibrary.h"
#include "VulkanRHI/VulkanCore.h"

// Need the beta header for VK_KHR_portability_subset
#include <vulkan/vulkan_beta.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct VulkanPlatformBase
{
    static FORCEINLINE TArray<const CHAR*> GetOptionalInstanceExtensions()
    {
        return
        {
        #if VK_EXT_surface_maintenance1
            VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
        #endif
        #if VK_KHR_get_surface_capabilities2
            VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
        #endif
        };
    }
    
    static FORCEINLINE TArray<const CHAR*> GetOptionalDeviceExtensions()
    {
        return
        {
        #if VK_KHR_portability_subset
            VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, // NOTE: This extension must be enabled on platforms that has it available
        #endif
        #if VK_KHR_maintenance5
            VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
        #endif
        #if VK_KHR_maintenance6
            VK_KHR_MAINTENANCE_6_EXTENSION_NAME,
        #endif
		#if VK_KHR_maintenance7
            VK_KHR_MAINTENANCE_7_EXTENSION_NAME,
		#endif
		#if VK_KHR_maintenance8
			VK_KHR_MAINTENANCE_8_EXTENSION_NAME,
		#endif
		#if VK_KHR_maintenance9
			VK_KHR_MAINTENANCE_9_EXTENSION_NAME,
		#endif
        #if VK_KHR_deferred_host_operations
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        #endif
        #if VK_KHR_pipeline_library
            VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
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
        #if VK_KHR_fragment_shading_rate
            VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, 
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
        #if VK_EXT_depth_clip_enable
            VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,
        #endif
        #if VK_EXT_conservative_rasterization
            VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
        #endif
        #if VK_KHR_robustness2
            VK_KHR_ROBUSTNESS_2_EXTENSION_NAME,
        #endif
        #if VK_EXT_swapchain_maintenance1
        VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
        #endif
        #if VK_EXT_sample_locations
            VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME,
        #endif
        #if VK_EXT_fragment_shader_interlock
			VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME,
        #endif
        #if VK_NV_ray_tracing_invocation_reorder
            VK_NV_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME,
        #endif
        };
    }
    
    static FORCEINLINE TArray<const CHAR*> GetRequiredInstanceExtensions()
    {
        return TArray<const CHAR*>();
    }

    static FORCEINLINE TArray<const CHAR*> GetRequiredInstanceLayers()
    {
        return TArray<const CHAR*>();
    }

    static FORCEINLINE TArray<const CHAR*> GetRequiredDeviceExtensions()
    {
        return TArray<const CHAR*>();
    }
    
    static FORCEINLINE TArray<const CHAR*> GetRequiredDeviceLayers()
    {
        return TArray<const CHAR*>();
    }

    static FORCEINLINE void* LoadVulkanLibrary() { return nullptr; }

#if VK_KHR_surface
    static FORCEINLINE VkResult CreateSurface(VkInstance Instance, void* InWindowHandle, VkSurfaceKHR* OutSurface)
    {
        return VK_ERROR_UNKNOWN;
    }
#endif
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
