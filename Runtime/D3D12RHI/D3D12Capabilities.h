#pragma once
#include "D3D12RHI/D3D12Core.h"

// -------------------------------------------------------------------------------------------
// D3D12 Feature Support
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// General Capability Flags
// -------------------------------------------------------------------------------------------

extern D3D12RHI_API bool GD3D12ForceBinding;
extern D3D12RHI_API bool GD3D12SupportPipelineCache;
extern D3D12RHI_API bool GD3D12SupportPipelineStream;
extern D3D12RHI_API bool GD3D12SupportTightAlignment;
extern D3D12RHI_API bool GD3D12SupportGPUUploadHeaps;
extern D3D12RHI_API bool GD3D12SupportDynamicDepthBias;
extern D3D12RHI_API bool GD3D12SupportsBindless;
extern D3D12RHI_API bool GD3D12SupportEnhancedBarriers;

// -------------------------------------------------------------------------------------------
// Ray Tracing feature support (backend-native mirrors of the agnostic RHI::bSupports* flags)
// -------------------------------------------------------------------------------------------

extern D3D12RHI_API bool GD3D12SupportsInlineRayTracing;
extern D3D12RHI_API bool GD3D12SupportsOpacityMicromap;
extern D3D12RHI_API bool GD3D12SupportsShaderExecutionReordering;
extern D3D12RHI_API bool GD3D12ShaderExecutionReorderingActuallyReorders;
extern D3D12RHI_API bool GD3D12SupportsRayTracingPipelineAdditions;
extern D3D12RHI_API bool GD3D12SupportsClustersAndPTLAS;
extern D3D12RHI_API bool GD3D12SupportsIndirectAccelerationStructureOperations;
extern D3D12RHI_API bool GD3D12SupportsIndirectRayDispatch;

// -------------------------------------------------------------------------------------------
// Core Feature Tiers
// -------------------------------------------------------------------------------------------

extern D3D12RHI_API D3D12_RESOURCE_BINDING_TIER              GD3D12ResourceBindingTier;
extern D3D12RHI_API D3D12_RESOURCE_HEAP_TIER                 GD3D12ResourceHeapTier;
extern D3D12RHI_API D3D12_RAYTRACING_TIER                    GD3D12RayTracingTier;
extern D3D12RHI_API D3D12_VARIABLE_SHADING_RATE_TIER         GD3D12VariableRateShadingTier;
extern D3D12RHI_API D3D12_MESH_SHADER_TIER                   GD3D12MeshShaderTier;
extern D3D12RHI_API D3D12_SAMPLER_FEEDBACK_TIER              GD3D12SamplerFeedbackTier;
extern D3D12RHI_API D3D12_VIEW_INSTANCING_TIER               GD3D12ViewInstancingTier;
extern D3D12RHI_API D3D12_CONSERVATIVE_RASTERIZATION_TIER    GD3D12ConservativeRasterizationTier;
extern D3D12RHI_API D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER GD3D12ProgrammableSamplePositionsTier;
extern D3D12RHI_API D3D12_WORK_GRAPHS_TIER                   GD3D12WorkGraphsTier;
extern D3D12RHI_API D3D12_EXECUTE_INDIRECT_TIER              GD3D12ExecuteIndirectTier;
extern D3D12RHI_API D3D12_TILED_RESOURCES_TIER               GD3D12TiledResourcesTier;
extern D3D12RHI_API D3D12_WAVE_MMA_TIER                      GD3D12WaveMMATier;
extern D3D12RHI_API D3D_ROOT_SIGNATURE_VERSION               GD3D12RootSignatureVersion;
extern D3D12RHI_API D3D_SHADER_MODEL                         GD3D12HighestShaderModel;

// -------------------------------------------------------------------------------------------
// Boolean Capability Flags
// -------------------------------------------------------------------------------------------

extern D3D12RHI_API bool GD3D12RasterizerOrderViewsSupported;
extern D3D12RHI_API bool GD3D12TypedUAVLoadAdditionalFormats;
extern D3D12RHI_API bool GD3D12DepthBoundsTestSupported;
extern D3D12RHI_API bool GD3D12IsArchitectureUMA;
extern D3D12RHI_API bool GD3D12IsArchitectureCacheCoherentUMA;
extern D3D12RHI_API bool GD3D12PSSpecifiedStencilRefSupported;
extern D3D12RHI_API bool GD3D12WaveOpsSupported;
extern D3D12RHI_API bool GD3D12Int64ShaderOpsSupported;
extern D3D12RHI_API bool GD3D12BarycentricsSupported;
extern D3D12RHI_API bool GD3D12Native16BitShaderOpsSupported;
extern D3D12RHI_API bool GD3D12AtomicInt64OnTypedResourceSupported;
extern D3D12RHI_API bool GD3D12AtomicInt64OnGroupSharedSupported;
extern D3D12RHI_API bool GD3D12DerivativesInMeshAndAmpShadersSupported;
extern D3D12RHI_API bool GD3D12AtomicInt64OnDescriptorHeapResourceSupported;

// -------------------------------------------------------------------------------------------
// Wave / Lane counts
// -------------------------------------------------------------------------------------------

extern D3D12RHI_API uint32 GD3D12WaveLaneCountMin;
extern D3D12RHI_API uint32 GD3D12WaveLaneCountMax;
extern D3D12RHI_API uint32 GD3D12TotalLaneCount;

// -------------------------------------------------------------------------------------------
// Descriptor / Heap Limits
// -------------------------------------------------------------------------------------------

extern D3D12RHI_API uint32 GD3D12MaxSamplerDescriptorHeapSize;
extern D3D12RHI_API uint32 GD3D12MaxResourceDescriptorHeapSize;

// -------------------------------------------------------------------------------------------
// GPU Virtual Address / Command Capabilities
// -------------------------------------------------------------------------------------------

extern D3D12RHI_API uint32                           GD3D12VirtualAddressBitsPerResource;
extern D3D12RHI_API uint32                           GD3D12VirtualAddressBitsPerProcess;
extern D3D12RHI_API D3D12_COMMAND_LIST_SUPPORT_FLAGS GD3D12WriteBufferImmediateSupportFlags;

// -------------------------------------------------------------------------------------------
// D3D12 Capabilitiy Logging
// -------------------------------------------------------------------------------------------

extern D3D12RHI_API void DumpD3D12Capabilities();
extern D3D12RHI_API void DumpD3D12RayTracingCapabilities();
