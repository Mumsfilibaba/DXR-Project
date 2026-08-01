#pragma once
#include "VulkanRHI/VulkanCore.h"

// -------------------------------------------------------------------------------------------
// Vulkan Device Feature Support
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool   GVulkanForceBinding;
extern VULKANRHI_API bool   GVulkanAllowNullDescriptors;
extern VULKANRHI_API bool   GVulkanAllowGeometryShaders;
extern VULKANRHI_API bool   GVulkanAllowResetCommandBuffers;
extern VULKANRHI_API bool   GVulkanRobustBufferAccessEnabled;
extern VULKANRHI_API bool   GVulkanGPUAssistedValidationEnabled;
extern VULKANRHI_API bool   GVulkanSupportsDepthClip;
extern VULKANRHI_API bool   GVulkanSupportsDepthClamp;
extern VULKANRHI_API bool   GVulkanSupportsNullDescriptors;
extern VULKANRHI_API bool   GVulkanSupportsRobustness2;
extern VULKANRHI_API bool   GVulkanSupportsConservativeRasterization;
extern VULKANRHI_API float  GVulkanMaxExtraPrimitiveOverestimationSize;
extern VULKANRHI_API bool   GVulkanSupportsPipelineCacheControl;
extern VULKANRHI_API bool   GVulkanSupportsDynamicRendering;
extern VULKANRHI_API bool   GVulkanSupportsSynchronization2;
extern VULKANRHI_API bool   GVulkanSupportsMaintenance4;
extern VULKANRHI_API bool   GVulkanSupportsMultiviews;
extern VULKANRHI_API bool   GVulkanSupportsBindless;
extern VULKANRHI_API bool   GVulkanSupportsMutableDescriptorType;
extern VULKANRHI_API bool   GVulkanSupportsDepthBoundsTest;
extern VULKANRHI_API bool   GVulkanSupportsSparseBinding;
extern VULKANRHI_API bool   GVulkanSupportsSparseResidency2D;
extern VULKANRHI_API bool   GVulkanSupportsSparseResidency3D;
extern VULKANRHI_API bool   GVulkanSupportsSparseResidencyAliased;
extern VULKANRHI_API bool   GVulkanSupportsGeometryShader;
extern VULKANRHI_API bool   GVulkanSupportsTessellation;
extern VULKANRHI_API bool   GVulkanSupportsImageCubeArray;
extern VULKANRHI_API uint32 GVulkanMaxMultiviewViewCount;
extern VULKANRHI_API uint32 GVulkanMaxDrawIndirectCount;

// -------------------------------------------------------------------------------------------
// Programmable sample positions (VK_EXT_sample_locations)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool GVulkanSupportsSampleLocations;

// -------------------------------------------------------------------------------------------
// Programmable sample positions (VK_EXT_fragment_shader_interlock)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool GVulkanSupportsFragmentShaderInterlock;

// -------------------------------------------------------------------------------------------
// Shader Language Capabilities
//
// Inputs to the derived HLSL shader model, and the per-feature answer to "may a shader use this".
// A shader model is a language level rather than a feature bundle, so these stay separate from it.
// -------------------------------------------------------------------------------------------

/** Subgroup ops covering everything the HLSL wave intrinsics lower to, in the stages the engine uses */
extern VULKANRHI_API bool GVulkanSupportsWaveOperations;

/** VK_NV_shader_subgroup_partitioned, what DXC lowers the SM 6.5 WaveMatch/WaveMultiPrefix* family to */
extern VULKANRHI_API bool GVulkanSupportsPartitionedWaveOperations;

/** 16-bit arithmetic plus 16-bit storage access: the equivalent of D3D12's Native16BitShaderOpsSupported */
extern VULKANRHI_API bool GVulkanSupportsNative16BitOps;

/** 64-bit integer arithmetic in shaders: the equivalent of D3D12's Int64ShaderOps */
extern VULKANRHI_API bool GVulkanSupportsInt64ShaderOps;

/** 64-bit integer atomics on buffers and groupshared memory, added by SM 6.6 */
extern VULKANRHI_API bool GVulkanSupportsInt64Atomics;

/** VK_KHR_shader_maximal_reconvergence, the mandatory dependency of VK_KHR_shader_quad_control */
extern VULKANRHI_API bool GVulkanSupportsMaximalReconvergence;

/** VK_KHR_shader_quad_control, for the SM 6.7 QuadAny/QuadAll intrinsics */
extern VULKANRHI_API bool GVulkanSupportsQuadControl;

/** Packed dot-product intrinsics added by SM 6.4 (dot4add_u8packed, dot4add_i8packed, dot2add) */
extern VULKANRHI_API bool GVulkanSupportsIntegerDotProduct;

/** VK_KHR_fragment_shader_barycentric, for SM 6.1 SV_Barycentrics */
extern VULKANRHI_API bool GVulkanSupportsFragmentBarycentric;

extern VULKANRHI_API uint32 GVulkanSubgroupSupportedOperations;
extern VULKANRHI_API uint32 GVulkanSubgroupSupportedStages;
extern VULKANRHI_API uint32 GVulkanSubgroupSize;

/** Highest HLSL shader model this device is equivalent to, mirrored into RHI::MaxShaderModel */
extern VULKANRHI_API EShaderModel GVulkanShaderModel;

// -------------------------------------------------------------------------------------------
// Ray Tracing (VK_KHR_ray_tracing_pipeline, VK_KHR_ray_query)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool GVulkanSupportsRayTracingPipeline; 
extern VULKANRHI_API bool GVulkanSupportsRayQuery;
extern VULKANRHI_API bool GVulkanSupportsAccelerationStructures;
extern VULKANRHI_API bool GVulkanSupportsOpacityMicromap;
extern VULKANRHI_API bool GVulkanSupportsShaderExecutionReordering;
extern VULKANRHI_API bool GVulkanShaderExecutionReorderingActuallyReorders;
extern VULKANRHI_API bool GVulkanSupportsClustersAndPTLAS;
extern VULKANRHI_API bool GVulkanSupportsIndirectAccelerationStructureOperations;
extern VULKANRHI_API bool GVulkanSupportsIndirectRayDispatch;

// -------------------------------------------------------------------------------------------
// Variable Rate Shading (VK_KHR_fragment_shading_rate)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool   GVulkanSupportsFragmentShadingRate;
extern VULKANRHI_API uint32 GVulkanShadingRateTileSize;

// -------------------------------------------------------------------------------------------
// Mesh shaders (VK_EXT_mesh_shader)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool   GVulkanSupportsMeshShaders;
extern VULKANRHI_API bool   GVulkanSupportsTaskShaders;
extern VULKANRHI_API bool   GVulkanSupportsMeshShaderMultiview;
extern VULKANRHI_API bool   GVulkanSupportsMeshShaderQueries;
extern VULKANRHI_API bool   GVulkanSupportsMeshShaderPrimitiveFragmentShadingRate;
extern VULKANRHI_API uint32 GVulkanMaxMeshOutputVertices;
extern VULKANRHI_API uint32 GVulkanMaxMeshWorkGroupInvocations;
extern VULKANRHI_API uint32 GVulkanMaxTaskWorkGroupInvocations;

// -------------------------------------------------------------------------------------------
// Transform Feedback / Stream Output (VK_EXT_transform_feedback)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool   GVulkanSupportsTransformFeedback;

// -------------------------------------------------------------------------------------------
// Dynamic Rendering (VK_KHR_dynamic_rendering / Vulkan 1.3)
// -------------------------------------------------------------------------------------------

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
extern VULKANRHI_API bool   GVulkanUseDynamicRendering;
#else
inline constexpr bool       GVulkanUseDynamicRendering = true;
#endif

// -------------------------------------------------------------------------------------------
// Descriptor / Heap Limits
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API uint32 GVulkanMaxDescriptorSetSamplers;
extern VULKANRHI_API uint32 GVulkanMaxDescriptorSetSampledImages;
extern VULKANRHI_API uint32 GVulkanMaxDescriptorSetStorageImages;
extern VULKANRHI_API uint32 GVulkanMaxDescriptorSetUniformBuffers;
extern VULKANRHI_API uint32 GVulkanMaxDescriptorSetStorageBuffers;

// -------------------------------------------------------------------------------------------
// Vulkan Capabilitiy Logging
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API void DumpVulkanCapabilities();
extern VULKANRHI_API void DumpVulkanRayTracingCapabilities();
