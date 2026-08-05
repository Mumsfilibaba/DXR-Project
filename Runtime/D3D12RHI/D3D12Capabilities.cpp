#include "Core/Misc/ConsoleManager.h"
#include "D3D12RHI/D3D12Capabilities.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12SwapChain.h"

static TAutoConsoleVariable<int32> CVarResourceBindingTierOverride(
    "D3D12RHI.ResourceBindingTierOverride",
    "Override the resource binding tier (0=no override, 1=Tier1, 2=Tier2, 3=Tier3)",
    0);

static TAutoConsoleVariable<bool> CVarEnableTightAlignment(
    "D3D12RHI.EnableTightAlignment",
    "Enable tight alignment if supported by the device",
    true);

static FAutoConsoleCommand CCmdD3D12DumpRayTracingCapsCommand(
    "D3D12RHI.DumpRayTracingCaps",
    "Logs the backend-native D3D12 ray-tracing capability table (GD3D12Supports* RT globals)",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        DumpD3D12RayTracingCapabilities();
    }));

static FAutoConsoleCommand CCmdD3D12DumpCapsCommand(
    "D3D12RHI.DumpCaps",
    "Logs the backend-native D3D12 capability table (all GD3D12* globals, including ray tracing)",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        DumpD3D12Capabilities();
    }));

// -------------------------------------------------------------------------------------------
// D3D12 Feature Support
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// General Capability Flags
// -------------------------------------------------------------------------------------------

D3D12RHI_API bool GD3D12ForceBinding            = false;
D3D12RHI_API bool GD3D12SupportPipelineCache    = false;
D3D12RHI_API bool GD3D12SupportPipelineStream   = false;
D3D12RHI_API bool GD3D12SupportTightAlignment   = false;
D3D12RHI_API bool GD3D12SupportGPUUploadHeaps   = false;
D3D12RHI_API bool GD3D12SupportDynamicDepthBias = false;
D3D12RHI_API bool GD3D12SupportsBindless        = false;
D3D12RHI_API bool GD3D12SupportEnhancedBarriers = false;

// -------------------------------------------------------------------------------------------
// Ray Tracing feature support (backend-native mirrors of the agnostic RHI::bSupports* flags)
// -------------------------------------------------------------------------------------------

D3D12RHI_API bool GD3D12SupportsInlineRayTracing                        = false;
D3D12RHI_API bool GD3D12SupportsOpacityMicromap                         = false;
D3D12RHI_API bool GD3D12SupportsShaderExecutionReordering               = false;
D3D12RHI_API bool GD3D12ShaderExecutionReorderingActuallyReorders       = false;
D3D12RHI_API bool GD3D12SupportsRayTracingPipelineAdditions             = false;
D3D12RHI_API bool GD3D12SupportsClustersAndPTLAS                        = false;
D3D12RHI_API bool GD3D12SupportsIndirectAccelerationStructureOperations = false;
D3D12RHI_API bool GD3D12SupportsIndirectRayDispatch                     = false;

// -------------------------------------------------------------------------------------------
// Core Feature Tiers
// -------------------------------------------------------------------------------------------

D3D12RHI_API D3D12_RESOURCE_BINDING_TIER              GD3D12ResourceBindingTier             = D3D12_RESOURCE_BINDING_TIER_1;
D3D12RHI_API D3D12_RESOURCE_HEAP_TIER                 GD3D12ResourceHeapTier                = D3D12_RESOURCE_HEAP_TIER_1;
D3D12RHI_API D3D12_RAYTRACING_TIER                    GD3D12RayTracingTier                  = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_VARIABLE_SHADING_RATE_TIER         GD3D12VariableRateShadingTier         = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_MESH_SHADER_TIER                   GD3D12MeshShaderTier                  = D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_SAMPLER_FEEDBACK_TIER              GD3D12SamplerFeedbackTier             = D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_VIEW_INSTANCING_TIER               GD3D12ViewInstancingTier              = D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_CONSERVATIVE_RASTERIZATION_TIER    GD3D12ConservativeRasterizationTier   = D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER GD3D12ProgrammableSamplePositionsTier = D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_WORK_GRAPHS_TIER                   GD3D12WorkGraphsTier                  = D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_EXECUTE_INDIRECT_TIER              GD3D12ExecuteIndirectTier             = D3D12_EXECUTE_INDIRECT_TIER_1_0;
D3D12RHI_API D3D12_TILED_RESOURCES_TIER               GD3D12TiledResourcesTier              = D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_WAVE_MMA_TIER                      GD3D12WaveMMATier                     = D3D12_WAVE_MMA_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D_ROOT_SIGNATURE_VERSION               GD3D12RootSignatureVersion            = D3D_ROOT_SIGNATURE_VERSION_1_0;
D3D12RHI_API D3D_SHADER_MODEL                         GD3D12HighestShaderModel              = D3D_SHADER_MODEL_6_0;

// -------------------------------------------------------------------------------------------
// Boolean Capability Flags
// -------------------------------------------------------------------------------------------

D3D12RHI_API bool GD3D12RasterizerOrderViewsSupported                = false;
D3D12RHI_API bool GD3D12TypedUAVLoadAdditionalFormats                = false;
D3D12RHI_API bool GD3D12DepthBoundsTestSupported                     = false;
D3D12RHI_API bool GD3D12IsArchitectureUMA                            = false;
D3D12RHI_API bool GD3D12IsArchitectureCacheCoherentUMA               = false;
D3D12RHI_API bool GD3D12PSSpecifiedStencilRefSupported               = false;
D3D12RHI_API bool GD3D12WaveOpsSupported                             = false;
D3D12RHI_API bool GD3D12Int64ShaderOpsSupported                      = false;
D3D12RHI_API bool GD3D12BarycentricsSupported                        = false;
D3D12RHI_API bool GD3D12Native16BitShaderOpsSupported                = false;
D3D12RHI_API bool GD3D12AtomicInt64OnTypedResourceSupported          = false;
D3D12RHI_API bool GD3D12AtomicInt64OnGroupSharedSupported            = false;
D3D12RHI_API bool GD3D12DerivativesInMeshAndAmpShadersSupported      = false;
D3D12RHI_API bool GD3D12AtomicInt64OnDescriptorHeapResourceSupported = false;

// -------------------------------------------------------------------------------------------
// Wave / Lane counts
// -------------------------------------------------------------------------------------------

D3D12RHI_API uint32 GD3D12WaveLaneCountMin = 0;
D3D12RHI_API uint32 GD3D12WaveLaneCountMax = 0;
D3D12RHI_API uint32 GD3D12TotalLaneCount   = 0;

// -------------------------------------------------------------------------------------------
// Descriptor / Heap Limits
// -------------------------------------------------------------------------------------------

D3D12RHI_API uint32 GD3D12MaxSamplerDescriptorHeapSize  = 0;
D3D12RHI_API uint32 GD3D12MaxResourceDescriptorHeapSize = 0;

// -------------------------------------------------------------------------------------------
// GPU Virtual Address / Command Capabilities
// -------------------------------------------------------------------------------------------

D3D12RHI_API uint32                           GD3D12VirtualAddressBitsPerResource    = 0;
D3D12RHI_API uint32                           GD3D12VirtualAddressBitsPerProcess     = 0;
D3D12RHI_API D3D12_COMMAND_LIST_SUPPORT_FLAGS GD3D12WriteBufferImmediateSupportFlags = D3D12_COMMAND_LIST_SUPPORT_FLAG_NONE;

D3D12RHI_API void DumpD3D12RayTracingCapabilities()
{
    const auto YesNo = [](bool bValue) -> const CHAR*
    {
        return bValue ? "Yes" : "No";
    };

    LOG_INFO("[D3D12RHI] ------------------------- D3D12 Ray Tracing Capabilities (native) -------------------------");
    LOG_INFO("[D3D12RHI]   D3D12_RAYTRACING_TIER                 : %d", static_cast<int32>(GD3D12RayTracingTier));
    LOG_INFO("[D3D12RHI]   Inline Ray Tracing (RayQuery)         : %s", YesNo(GD3D12SupportsInlineRayTracing));
    LOG_INFO("[D3D12RHI]   Opacity Micromap                      : %s", YesNo(GD3D12SupportsOpacityMicromap));
    LOG_INFO("[D3D12RHI]   Shader Execution Reordering           : %s", YesNo(GD3D12SupportsShaderExecutionReordering));
    LOG_INFO("[D3D12RHI]   Shader Execution Reordering (Reorders): %s", YesNo(GD3D12ShaderExecutionReorderingActuallyReorders));
    LOG_INFO("[D3D12RHI]   Pipeline Additions (AddToStateObject) : %s", YesNo(GD3D12SupportsRayTracingPipelineAdditions));
    LOG_INFO("[D3D12RHI]   Clusters + Partitioned Scene (PTLAS)  : %s", YesNo(GD3D12SupportsClustersAndPTLAS));
    LOG_INFO("[D3D12RHI]   Indirect AS Operations                : %s", YesNo(GD3D12SupportsIndirectAccelerationStructureOperations));
    LOG_INFO("[D3D12RHI]   Indirect Ray Dispatch                 : %s", YesNo(GD3D12SupportsIndirectRayDispatch));
    LOG_INFO("[D3D12RHI] -----------------------------------------------------------------------------------------");
}

D3D12RHI_API void DumpD3D12Capabilities()
{
    const auto YesNo = [](bool bValue) -> const CHAR*
    {
        return bValue ? "Yes" : "No";
    };

    LOG_INFO("[D3D12RHI] ------------------------- D3D12 Capabilities (native) -------------------------");
    LOG_INFO("[D3D12RHI]   Force Binding                         : %s", YesNo(GD3D12ForceBinding));
    LOG_INFO("[D3D12RHI]   Bindless                              : %s", YesNo(GD3D12SupportsBindless));
    LOG_INFO("[D3D12RHI]   Enhanced Barriers                     : %s", YesNo(GD3D12SupportEnhancedBarriers));
    LOG_INFO("[D3D12RHI]   GPU Upload Heaps                      : %s", YesNo(GD3D12SupportGPUUploadHeaps));
    LOG_INFO("[D3D12RHI]   Tight Alignment                       : %s", YesNo(GD3D12SupportTightAlignment));
    LOG_INFO("[D3D12RHI]   Dynamic Depth Bias                    : %s", YesNo(GD3D12SupportDynamicDepthBias));
    LOG_INFO("[D3D12RHI]   Pipeline Cache                        : %s", YesNo(GD3D12SupportPipelineCache));
    LOG_INFO("[D3D12RHI]   Pipeline Stream                       : %s", YesNo(GD3D12SupportPipelineStream));

    LOG_INFO("[D3D12RHI]   Resource Binding Tier                 : %d", static_cast<int32>(GD3D12ResourceBindingTier));
    LOG_INFO("[D3D12RHI]   Resource Heap Tier                    : %d", static_cast<int32>(GD3D12ResourceHeapTier));
    LOG_INFO("[D3D12RHI]   Variable Rate Shading Tier            : %d", static_cast<int32>(GD3D12VariableRateShadingTier));
    LOG_INFO("[D3D12RHI]   Mesh Shader Tier                      : %d", static_cast<int32>(GD3D12MeshShaderTier));
    LOG_INFO("[D3D12RHI]   Sampler Feedback Tier                 : %d", static_cast<int32>(GD3D12SamplerFeedbackTier));
    LOG_INFO("[D3D12RHI]   View Instancing Tier                  : %d", static_cast<int32>(GD3D12ViewInstancingTier));
    LOG_INFO("[D3D12RHI]   Conservative Rasterization Tier       : %d", static_cast<int32>(GD3D12ConservativeRasterizationTier));
    LOG_INFO("[D3D12RHI]   Programmable Sample Positions Tier    : %d", static_cast<int32>(GD3D12ProgrammableSamplePositionsTier));
    LOG_INFO("[D3D12RHI]   Work Graphs Tier                      : %d", static_cast<int32>(GD3D12WorkGraphsTier));
    LOG_INFO("[D3D12RHI]   Execute Indirect Tier                 : %d", static_cast<int32>(GD3D12ExecuteIndirectTier));
    LOG_INFO("[D3D12RHI]   Tiled Resources Tier                  : %d", static_cast<int32>(GD3D12TiledResourcesTier));
    LOG_INFO("[D3D12RHI]   Wave MMA Tier                         : %d", static_cast<int32>(GD3D12WaveMMATier));
    LOG_INFO("[D3D12RHI]   Root Signature Version                : %d", static_cast<int32>(GD3D12RootSignatureVersion));
    LOG_INFO("[D3D12RHI]   Highest Shader Model                  : 0x%X", static_cast<int32>(GD3D12HighestShaderModel));

    LOG_INFO("[D3D12RHI]   Rasterizer Ordered Views              : %s", YesNo(GD3D12RasterizerOrderViewsSupported));
    LOG_INFO("[D3D12RHI]   Typed UAV Load Additional Formats     : %s", YesNo(GD3D12TypedUAVLoadAdditionalFormats));
    LOG_INFO("[D3D12RHI]   Depth Bounds Test                     : %s", YesNo(GD3D12DepthBoundsTestSupported));
    LOG_INFO("[D3D12RHI]   Architecture UMA                      : %s", YesNo(GD3D12IsArchitectureUMA));
    LOG_INFO("[D3D12RHI]   Architecture Cache-Coherent UMA       : %s", YesNo(GD3D12IsArchitectureCacheCoherentUMA));
    LOG_INFO("[D3D12RHI]   PS-Specified Stencil Ref              : %s", YesNo(GD3D12PSSpecifiedStencilRefSupported));
    LOG_INFO("[D3D12RHI]   Wave Ops                              : %s", YesNo(GD3D12WaveOpsSupported));
    LOG_INFO("[D3D12RHI]   Int64 Shader Ops                      : %s", YesNo(GD3D12Int64ShaderOpsSupported));
    LOG_INFO("[D3D12RHI]   Barycentrics                          : %s", YesNo(GD3D12BarycentricsSupported));
    LOG_INFO("[D3D12RHI]   Native 16-bit Shader Ops              : %s", YesNo(GD3D12Native16BitShaderOpsSupported));
    LOG_INFO("[D3D12RHI]   Atomic Int64 (Typed Resource)         : %s", YesNo(GD3D12AtomicInt64OnTypedResourceSupported));
    LOG_INFO("[D3D12RHI]   Atomic Int64 (Group Shared)           : %s", YesNo(GD3D12AtomicInt64OnGroupSharedSupported));
    LOG_INFO("[D3D12RHI]   Derivatives in Mesh/Amp Shaders       : %s", YesNo(GD3D12DerivativesInMeshAndAmpShadersSupported));
    LOG_INFO("[D3D12RHI]   Atomic Int64 (Descriptor Heap Rsrc)   : %s", YesNo(GD3D12AtomicInt64OnDescriptorHeapResourceSupported));

    LOG_INFO("[D3D12RHI]   Wave Lanes (min / max / total)        : %u / %u / %u", GD3D12WaveLaneCountMin, GD3D12WaveLaneCountMax, GD3D12TotalLaneCount);
    LOG_INFO("[D3D12RHI]   Max Resource Descriptor Heap Size     : %u", GD3D12MaxResourceDescriptorHeapSize);
    LOG_INFO("[D3D12RHI]   Max Sampler Descriptor Heap Size      : %u", GD3D12MaxSamplerDescriptorHeapSize);
    LOG_INFO("[D3D12RHI]   VA Bits (per resource / per process)  : %u / %u", GD3D12VirtualAddressBitsPerResource, GD3D12VirtualAddressBitsPerProcess);
    LOG_INFO("[D3D12RHI]   Write Buffer Immediate Support Flags  : 0x%X", static_cast<int32>(GD3D12WriteBufferImmediateSupportFlags));
    LOG_INFO("[D3D12RHI] -----------------------------------------------------------------------------");

    DumpD3D12RayTracingCapabilities();
}

void FD3D12Device::QueryDeviceFeatureSupport()
{
    // -------------------------------------------------------------------------------------------
    // Baseline defaults
    // -------------------------------------------------------------------------------------------

    GD3D12SupportTightAlignment            = false;
    GD3D12SupportGPUUploadHeaps            = false;
    GD3D12SupportDynamicDepthBias          = false;
    GD3D12SupportsBindless                 = false;
    GD3D12SupportEnhancedBarriers          = false;

    GD3D12SupportsInlineRayTracing                        = false;
    GD3D12SupportsOpacityMicromap                         = false;
    GD3D12SupportsShaderExecutionReordering               = false;
    GD3D12ShaderExecutionReorderingActuallyReorders       = false;
    GD3D12SupportsRayTracingPipelineAdditions             = false;
    GD3D12SupportsClustersAndPTLAS                        = false;
    GD3D12SupportsIndirectAccelerationStructureOperations = false;
    GD3D12SupportsIndirectRayDispatch                     = false;

    GD3D12ResourceBindingTier              = D3D12_RESOURCE_BINDING_TIER_1;
    GD3D12ResourceHeapTier                 = D3D12_RESOURCE_HEAP_TIER_1;
    GD3D12RayTracingTier                   = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    GD3D12VariableRateShadingTier          = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
    GD3D12MeshShaderTier                   = D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
    GD3D12SamplerFeedbackTier              = D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED;
    GD3D12ViewInstancingTier               = D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
    GD3D12ConservativeRasterizationTier    = D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED;
    GD3D12ProgrammableSamplePositionsTier  = D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED;
    GD3D12WorkGraphsTier                   = D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED;
    GD3D12ExecuteIndirectTier              = D3D12_EXECUTE_INDIRECT_TIER_1_0;
    GD3D12TiledResourcesTier               = D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED;
    GD3D12RootSignatureVersion             = D3D_ROOT_SIGNATURE_VERSION_1_0;
    GD3D12HighestShaderModel               = D3D_SHADER_MODEL_6_0;

    GD3D12RasterizerOrderViewsSupported    = false;
    GD3D12TypedUAVLoadAdditionalFormats    = false;
    GD3D12DepthBoundsTestSupported         = false;
    GD3D12IsArchitectureUMA                = false;
    GD3D12IsArchitectureCacheCoherentUMA   = false;

    GD3D12MaxSamplerDescriptorHeapSize     = 0;
    GD3D12MaxResourceDescriptorHeapSize    = 0;

    GD3D12VirtualAddressBitsPerProcess     = 0;
    GD3D12VirtualAddressBitsPerResource    = 0;
    GD3D12WriteBufferImmediateSupportFlags = D3D12_COMMAND_LIST_SUPPORT_FLAG_NONE;

    // -------------------------------------------------------------------------------------------
    // Heap Tier, Binding Tier, Conservative Rasterization, Typed UAV loads, ROVs, Tiled Resources
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS Features = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &Features, sizeof(Features));
        if (SUCCEEDED(hr))
        {
            GD3D12ResourceBindingTier            = Features.ResourceBindingTier;
            GD3D12ResourceHeapTier               = Features.ResourceHeapTier;
            GD3D12ConservativeRasterizationTier  = Features.ConservativeRasterizationTier;
            GD3D12RasterizerOrderViewsSupported  = !!Features.ROVsSupported;
            GD3D12TypedUAVLoadAdditionalFormats  = !!Features.TypedUAVLoadAdditionalFormats;
            GD3D12TiledResourcesTier             = Features.TiledResourcesTier;
            GD3D12PSSpecifiedStencilRefSupported = !!Features.PSSpecifiedStencilRefSupported;

            const int32 BindingTierOverride = CVarResourceBindingTierOverride.GetValue();
            if (BindingTierOverride >= 1 && BindingTierOverride <= 3)
            {
                GD3D12ResourceBindingTier = static_cast<D3D12_RESOURCE_BINDING_TIER>(BindingTierOverride);
                D3D12_WARNING("[FD3D12Device] ResourceBinding Tier OVERRIDDEN to: %d", GD3D12ResourceBindingTier);
            }

            D3D12_INFO("[FD3D12Device] ResourceBinding Tier: %d", GD3D12ResourceBindingTier);
            D3D12_INFO("[FD3D12Device] ResourceHeap Tier: %d", GD3D12ResourceHeapTier);
            D3D12_INFO("[FD3D12Device] ConservativeRasterization Tier: %d", GD3D12ConservativeRasterizationTier);
            D3D12_INFO("[FD3D12Device] TypedUAVLoadAdditionalFormats: %s", GD3D12TypedUAVLoadAdditionalFormats ? "true" : "false");
            D3D12_INFO("[FD3D12Device] ROVsSupported: %s", GD3D12RasterizerOrderViewsSupported ? "true" : "false");
            D3D12_INFO("[FD3D12Device] TiledResources Tier: %d", GD3D12TiledResourcesTier);
            D3D12_INFO("[FD3D12Device] PSSpecifiedStencilRefSupported: %s", GD3D12PSSpecifiedStencilRefSupported ? "true" : "false");
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Wave Ops, Int64 ops, Lane counts
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS1 Features1 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &Features1, sizeof(Features1));
        if (SUCCEEDED(hr))
        {
            GD3D12WaveOpsSupported        = !!Features1.WaveOps;
            GD3D12Int64ShaderOpsSupported = !!Features1.Int64ShaderOps;
            GD3D12WaveLaneCountMin        = Features1.WaveLaneCountMin;
            GD3D12WaveLaneCountMax        = Features1.WaveLaneCountMax;
            GD3D12TotalLaneCount          = Features1.TotalLaneCount;

            D3D12_INFO("[FD3D12Device] WaveOps Supported: %s", GD3D12WaveOpsSupported ? "true" : "false");
            D3D12_INFO("[FD3D12Device] Int64ShaderOps Supported: %s", GD3D12Int64ShaderOpsSupported ? "true" : "false");
            D3D12_INFO("[FD3D12Device] Wave Lane Count Min/Max: %u / %u", GD3D12WaveLaneCountMin, GD3D12WaveLaneCountMax);
            D3D12_INFO("[FD3D12Device] Total Lane Count: %u", GD3D12TotalLaneCount);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS1 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Depth bounds test & programmable sample positions
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS2 Features2 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &Features2, sizeof(Features2));
        if (SUCCEEDED(hr))
        {
            GD3D12DepthBoundsTestSupported        = !!Features2.DepthBoundsTestSupported;
            GD3D12ProgrammableSamplePositionsTier = Features2.ProgrammableSamplePositionsTier;

            D3D12_INFO("[FD3D12Device] DepthBoundsTestSupported: %s", GD3D12DepthBoundsTestSupported ? "true" : "false");
            D3D12_INFO("[FD3D12Device] ProgrammableSamplePositions Tier: %d", GD3D12ProgrammableSamplePositionsTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS2 query failed (hr=0x%08X). Using defaults.", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // View Instancing & Write buffer immediate flags
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS3 Features3 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &Features3, sizeof(Features3));
        if (SUCCEEDED(hr))
        {
            GD3D12ViewInstancingTier               = Features3.ViewInstancingTier;
            GD3D12WriteBufferImmediateSupportFlags = Features3.WriteBufferImmediateSupportFlags;
            GD3D12BarycentricsSupported            = !!Features3.BarycentricsSupported;

            D3D12_INFO("[FD3D12Device] BarycentricsSupported: %s", GD3D12BarycentricsSupported ? "true" : "false");
            D3D12_INFO("[FD3D12Device] ViewInstancing Tier: %d", GD3D12ViewInstancingTier);
            D3D12_INFO("[FD3D12Device] WriteBufferImmediate SupportFlags: 0x%X", static_cast<uint32>(GD3D12WriteBufferImmediateSupportFlags));
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_OPTIONS3 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Native 16-bit shader ops
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS4 Features4 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &Features4, sizeof(Features4));
        if (SUCCEEDED(hr))
        {
            GD3D12Native16BitShaderOpsSupported = !!Features4.Native16BitShaderOpsSupported;
            D3D12_INFO("[FD3D12Device] Native16BitShaderOps Supported: %s", GD3D12Native16BitShaderOpsSupported ? "true" : "false");
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS4 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Ray Tracing (DXR)
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 Features5 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &Features5, sizeof(Features5));
        if (SUCCEEDED(hr))
        {
            GD3D12RayTracingTier = Features5.RaytracingTier;
            D3D12_INFO("[FD3D12Device] RayTracing Tier: %d", GD3D12RayTracingTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_OPTIONS5 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 Features6 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &Features6, sizeof(Features6));
        if (SUCCEEDED(hr))
        {
            GD3D12VariableRateShadingTier = Features6.VariableShadingRateTier;
            D3D12_INFO("[FD3D12Device] VariableRateShading Tier: %d", GD3D12VariableRateShadingTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS6 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Mesh Shaders & Sampler Feedback
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 Features7 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &Features7, sizeof(Features7));
        if (SUCCEEDED(hr))
        {
            GD3D12MeshShaderTier      = Features7.MeshShaderTier;
            GD3D12SamplerFeedbackTier = Features7.SamplerFeedbackTier;

            D3D12_INFO("[FD3D12Device] MeshShader Tier: %d", GD3D12MeshShaderTier);
            D3D12_INFO("[FD3D12Device] SamplerFeedback Tier: %d", GD3D12SamplerFeedbackTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS7 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Atomic int64 on typed/groupshared, Mesh/Amplification derivatives, Wave MMA
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS9 Features9 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &Features9, sizeof(Features9));
        if (SUCCEEDED(hr))
        {
            GD3D12AtomicInt64OnTypedResourceSupported     = !!Features9.AtomicInt64OnTypedResourceSupported;
            GD3D12AtomicInt64OnGroupSharedSupported       = !!Features9.AtomicInt64OnGroupSharedSupported;
            GD3D12DerivativesInMeshAndAmpShadersSupported = !!Features9.DerivativesInMeshAndAmplificationShadersSupported;
            GD3D12WaveMMATier                             = Features9.WaveMMATier;

            D3D12_INFO("[FD3D12Device] AtomicInt64OnTypedResource Supported: %s", GD3D12AtomicInt64OnTypedResourceSupported ? "true" : "false");
            D3D12_INFO("[FD3D12Device] AtomicInt64OnGroupShared Supported: %s", GD3D12AtomicInt64OnGroupSharedSupported ? "true" : "false");
            D3D12_INFO("[FD3D12Device] DerivativesInMeshAndAmpShaders Supported: %s", GD3D12DerivativesInMeshAndAmpShadersSupported ? "true" : "false");
            D3D12_INFO("[FD3D12Device] WaveMMA Tier: %d", GD3D12WaveMMATier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS9 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Atomic int64 on descriptor-heap resources
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS11 Features11 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &Features11, sizeof(Features11));
        if (SUCCEEDED(hr))
        {
            GD3D12AtomicInt64OnDescriptorHeapResourceSupported = !!Features11.AtomicInt64OnDescriptorHeapResourceSupported;
            D3D12_INFO("[FD3D12Device] AtomicInt64OnDescriptorHeapResource Supported: %s", GD3D12AtomicInt64OnDescriptorHeapResourceSupported ? "true" : "false");
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS11 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Enhanced Barriers
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS12 Features12 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &Features12, sizeof(Features12));
        if (SUCCEEDED(hr))
        {
            GD3D12SupportEnhancedBarriers = !!Features12.EnhancedBarriersSupported;
            D3D12_INFO("[FD3D12Device] Enhanced Barriers Supported: %s", GD3D12SupportEnhancedBarriers ? "true" : "false");
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS12 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // GPU Upload Heaps
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS16 Features16 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &Features16, sizeof(Features16));
        if (SUCCEEDED(hr))
        {
            GD3D12SupportGPUUploadHeaps = !!Features16.GPUUploadHeapSupported;
            D3D12_INFO("[FD3D12Device] GPU Upload Heaps Supported: %s", GD3D12SupportGPUUploadHeaps ? "true" : "false");

        #if D3D12_ENABLE_DYNAMIC_DEPTH_BIAS && D3D12_USE_ID3D12COMMANDLIST_9
            GD3D12SupportDynamicDepthBias = !!Features16.DynamicDepthBiasSupported;
            D3D12_INFO("[FD3D12Device] Dynamic Depth Bias Supported: %s", GD3D12SupportDynamicDepthBias ? "true" : "false");
        #endif
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS16 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Descriptor heap sizes
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS19 Features19 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS19, &Features19, sizeof(Features19));
        if (SUCCEEDED(hr))
        {
            GD3D12MaxSamplerDescriptorHeapSize  = Features19.MaxSamplerDescriptorHeapSizeWithStaticSamplers;
            GD3D12MaxResourceDescriptorHeapSize = Features19.MaxViewDescriptorHeapSize;

            D3D12_INFO("[FD3D12Device] Max Sampler Descriptor-Heap Size: %u", GD3D12MaxSamplerDescriptorHeapSize);
            D3D12_INFO("[FD3D12Device] Max Resource Descriptor-Heap Size: %u", GD3D12MaxResourceDescriptorHeapSize);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS19 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Tight Alignment Support
    // -------------------------------------------------------------------------------------------

#if D3D12_USE_TIGHT_ALIGNMENT
    {
        D3D12_FEATURE_DATA_TIGHT_ALIGNMENT TightAlignment = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_TIGHT_ALIGNMENT, &TightAlignment, sizeof(TightAlignment));
        if (SUCCEEDED(hr))
        {
            const bool bDeviceSupports = (TightAlignment.SupportTier >= D3D12_TIGHT_ALIGNMENT_TIER_1);
            GD3D12SupportTightAlignment = bDeviceSupports && CVarEnableTightAlignment.GetValue();
            if (bDeviceSupports && !GD3D12SupportTightAlignment)
            {
                D3D12_INFO("[FD3D12Device] Tight Alignment Support: disabled by CVar (device supports it)");
            }
            else
            {
                D3D12_INFO("[FD3D12Device] Tight Alignment Support: %s", GD3D12SupportTightAlignment ? "true" : "false");
            }
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_TIGHT_ALIGNMENT query failed (hr=0x%08X)", hr);
        }
    }
#endif

    // -------------------------------------------------------------------------------------------
    // Root Signature Version
    // -------------------------------------------------------------------------------------------

    {
    #if D3D12_USE_VERSIONED_ROOT_SIGNATURES
        D3D12_FEATURE_DATA_ROOT_SIGNATURE RootSignature = { D3D_ROOT_SIGNATURE_VERSION_1_2 };
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &RootSignature, sizeof(RootSignature));
        if (FAILED(hr))
        {
            RootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
            hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &RootSignature, sizeof(RootSignature));
        }
    #else
        D3D12_FEATURE_DATA_ROOT_SIGNATURE RootSignature = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &RootSignature, sizeof(RootSignature));
    #endif

        if (SUCCEEDED(hr))
        {
            GD3D12RootSignatureVersion = RootSignature.HighestVersion;

            const CHAR* VersionString = "1.0";
            if (GD3D12RootSignatureVersion == D3D_ROOT_SIGNATURE_VERSION_1_1)
            {
                VersionString = "1.1";
            }
        #if D3D12_USE_VERSIONED_ROOT_SIGNATURES
            else if (GD3D12RootSignatureVersion == D3D_ROOT_SIGNATURE_VERSION_1_2)
            {
                VersionString = "1.2";
            }
        #endif

            D3D12_INFO("[FD3D12Device] RootSignature Version Supported: %s", VersionString);
        }
        else
        {
            GD3D12RootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_ROOT_SIGNATURE query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Highest Shader Model
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_SHADER_MODEL ShaderModel = {};
        ShaderModel.HighestShaderModel = D3D_HIGHEST_SHADER_MODEL;
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &ShaderModel, sizeof(ShaderModel));
        if (SUCCEEDED(hr))
        {
            GD3D12HighestShaderModel = ShaderModel.HighestShaderModel;
            D3D12_INFO("[FD3D12Device] Highest Shader Model Supported: 0x%X", GD3D12HighestShaderModel);
        }
        else
        {
            GD3D12HighestShaderModel = D3D_SHADER_MODEL_6_0;
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_SHADER_MODEL query failed (hr=0x%08X). Using SM 6.0 baseline.", hr);
        }

        GD3D12SupportsBindless = (GD3D12HighestShaderModel >= D3D_SHADER_MODEL_6_6) && (GD3D12ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3);
        D3D12_INFO("[FD3D12Device] Supports Bindless: %s", GD3D12SupportsBindless ? "true" : "false");
    }

    // -------------------------------------------------------------------------------------------
    // VirtualAddress
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT VirtualAddress = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &VirtualAddress, sizeof(VirtualAddress));
        if (SUCCEEDED(hr))
        {
            GD3D12VirtualAddressBitsPerProcess  = VirtualAddress.MaxGPUVirtualAddressBitsPerProcess;
            GD3D12VirtualAddressBitsPerResource = VirtualAddress.MaxGPUVirtualAddressBitsPerResource;

            D3D12_INFO("[FD3D12Device] VirtualAddress Bits/Process Supported: %u", GD3D12VirtualAddressBitsPerProcess);
            D3D12_INFO("[FD3D12Device] VirtualAddress Bits/Resource Supported: %u", GD3D12VirtualAddressBitsPerResource);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Architecture (UMA / CacheCoherentUMA)
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_ARCHITECTURE1 Architecture = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &Architecture, sizeof(Architecture));
        if (SUCCEEDED(hr))
        {
            GD3D12IsArchitectureUMA              = !!Architecture.UMA;
            GD3D12IsArchitectureCacheCoherentUMA = !!Architecture.CacheCoherentUMA;

            D3D12_INFO("[FD3D12Device] UMA Architecture: %s", GD3D12IsArchitectureUMA ? "true" : "false");
            D3D12_INFO("[FD3D12Device] CacheCoherentUMA Architecture: %s", GD3D12IsArchitectureCacheCoherentUMA ? "true" : "false");
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_ARCHITECTURE1 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // WorkGraphs & ExecuteIndirect
    // -------------------------------------------------------------------------------------------
    
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS21 Features21 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &Features21, sizeof(Features21));
        if (SUCCEEDED(hr))
        {
            GD3D12WorkGraphsTier      = Features21.WorkGraphsTier;
            GD3D12ExecuteIndirectTier = Features21.ExecuteIndirectTier;

            D3D12_INFO("[FD3D12Device] WorkGraphs Tier: %d", GD3D12WorkGraphsTier);
            D3D12_INFO("[FD3D12Device] ExecuteIndirect Tier: %d", GD3D12ExecuteIndirectTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS21 query failed (hr=0x%08X)", hr);
        }
    }

    return;
}

bool FD3D12DeviceRHI::InitializeDeviceFeatureSupport()
{
    // -------------------------------------------------------------------------------------------
    // Baseline Defaults
    // -------------------------------------------------------------------------------------------

    RHI::DefaultSwapChainFormat = GetD3D12DefaultBackBufferFormat();

    RHI::bSupportsGeometryShaders                       = true; // Geometry Shaders are always supported
    RHI::bSupportRenderTargetArrayIndexFromVertexShader = false;
    RHI::MaxShaderModel                                 = EShaderModel::Unknown;

    RHI::bSupportsViewInstancing     = false;
    RHI::MaxViewInstanceCount        = 1;

    RHI::bSupportsRayTracing         = false;
    RHI::RayTracingTier              = ERayTracingTier::NotSupported;
    RHI::RayTracingMaxRecursionDepth = 0;

    RHI::bSupportsVRS                = false;
    RHI::ShadingRateTier             = EShadingRateTier::NotSupported;
    RHI::ShadingRateImageTileSize    = 0;

    RHI::bSupportsSamplerFeedback    = false;
    RHI::SamplerFeedbackTier         = ESamplerFeedbackTier::NotSupported;

    RHI::bSupportsDrawIndirect               = true;
    RHI::bSupportsDrawIndirectCount          = true;
    RHI::bSupportsDispatchIndirect           = true;
    RHI::bSupportsDispatchMeshIndirect       = GD3D12MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
    RHI::bSupportsDispatchMeshIndirectCount  = RHI::bSupportsDispatchMeshIndirect;
    RHI::MaxDrawIndirectCommandCount         = uint32(~0u);
    RHI::MaxDispatchMeshIndirectCommandCount = uint32(~0u);

    // -------------------------------------------------------------------------------------------
    // Texture / image limits (canonical D3D12 defines)
    // -------------------------------------------------------------------------------------------

    RHI::MaxTexture1DSize        = D3D12_REQ_TEXTURE1D_U_DIMENSION;
    RHI::MaxTexture1DArrayLayers = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
    RHI::MaxTexture2DSize        = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    RHI::MaxTexture2DArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    RHI::MaxTexture3DWidth       = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    RHI::MaxTexture3DHeight      = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    RHI::MaxTexture3DDepth       = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    RHI::MaxCubeTextureSize      = D3D12_REQ_TEXTURECUBE_DIMENSION;
    RHI::MaxCubeArrayCount       = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / RHI_NUM_CUBE_FACES;

    // -------------------------------------------------------------------------------------------
    // Buffer / memory limits
    // -------------------------------------------------------------------------------------------

    RHI::MaxBufferSize              = uint64(~0ull);
    RHI::MaxConstantBufferSize      = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
    RHI::MaxStorageBufferSize       = uint64(~0ull);
    RHI::StructuredBufferMinStride  = 0;
    RHI::StructuredBufferMaxStride  = uint32(~0u);
    RHI::RawBufferRequiredAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;

    // -------------------------------------------------------------------------------------------
    // Shader Model
    // -------------------------------------------------------------------------------------------

    RHI::MaxShaderModel = ConvertShaderModel(GD3D12HighestShaderModel);

    // -------------------------------------------------------------------------------------------
    // SV_RenderTargetArrayIndex from VS
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS Features = {};
        if (SUCCEEDED(GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &Features, sizeof(Features))))
        {
            RHI::bSupportRenderTargetArrayIndexFromVertexShader = !!Features.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
        }
    }

    // -------------------------------------------------------------------------------------------
    // Ray Tracing (DXR)
    // -------------------------------------------------------------------------------------------

    GD3D12SupportsInlineRayTracing                        = false;
    GD3D12SupportsOpacityMicromap                         = false;
    GD3D12SupportsShaderExecutionReordering               = false;
    GD3D12ShaderExecutionReorderingActuallyReorders       = false;
    GD3D12SupportsRayTracingPipelineAdditions             = false;
    GD3D12SupportsClustersAndPTLAS                        = false;
    GD3D12SupportsIndirectAccelerationStructureOperations = false;
    GD3D12SupportsIndirectRayDispatch                     = false;

    RHI::RayTracingMaxTrianglesPerCluster      = 0;
    RHI::RayTracingMaxVerticesPerCluster       = 0;
    RHI::RayTracingMaxPartitionedInstanceCount = 0;

    if (GD3D12RayTracingTier >= D3D12_RAYTRACING_TIER_1_0)
    {
        RHI::bSupportsRayTracing                    = true;
        RHI::RayTracingMaxRecursionDepth            = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
        RHI::bSupportsShaderBindingTableDescriptors = true;
        RHI::bSupportsToolsVisualization            = true;

        GD3D12SupportsRayTracingPipelineAdditions = GetDevice()->GetD3D12Device7() != nullptr;

        if (GD3D12RayTracingTier >= D3D12_RAYTRACING_TIER_1_1)
        {
            RHI::RayTracingTier            = ERayTracingTier::Tier1_1;
            GD3D12SupportsInlineRayTracing = true;
        }
        else
        {
            RHI::RayTracingTier = ERayTracingTier::Tier1;
        }

    #if D3D12_ENABLE_INDIRECT_RAY_DISPATCH
        GD3D12SupportsIndirectRayDispatch = true;
    #endif

    #if D3D12_ENABLE_OPACITY_MICROMAPS || D3D12_ENABLE_SHADER_EXECUTION_REORDERING
        if (GD3D12RayTracingTier >= D3D12_RAYTRACING_TIER_1_2)
        {
            RHI::RayTracingTier = ERayTracingTier::Tier1_2;
        
        #if D3D12_ENABLE_OPACITY_MICROMAPS
            GD3D12SupportsOpacityMicromap = true;
        #endif

        #if D3D12_ENABLE_SHADER_EXECUTION_REORDERING
            GD3D12SupportsShaderExecutionReordering = (GD3D12HighestShaderModel >= D3D_SHADER_MODEL_6_9);

            if (!GD3D12SupportsShaderExecutionReordering)
            {
                D3D12_INFO("[FD3D12DeviceRHI] Shader Execution Reordering: disabled, requires Shader Model 6.9 (device reports 0x%X)", static_cast<int32>(GD3D12HighestShaderModel));
            }

            #if D3D12_SUPPORT_OPTIONS22
                if (GD3D12SupportsShaderExecutionReordering)
                {
                    D3D12_FEATURE_DATA_D3D12_OPTIONS22 Features22 = {};
                    if (SUCCEEDED(GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS22, &Features22, sizeof(Features22))))
                    {
                        GD3D12ShaderExecutionReorderingActuallyReorders = Features22.ShaderExecutionReorderingActuallyReorders;
                    }
                }
            #endif
        #endif
        }
    #endif

    #if D3D12_ENABLE_CLUSTERS_AND_PTLAS
        if (GD3D12RayTracingTier >= D3D12_RAYTRACING_TIER_2_0)
        {
            RHI::RayTracingTier                   = ERayTracingTier::Tier2_0;
            RHI::RayTracingMaxTrianglesPerCluster = D3D12_RTAS_MAX_TRIANGLES_PER_CLUSTER;
            RHI::RayTracingMaxVerticesPerCluster  = D3D12_RTAS_MAX_VERTICES_PER_CLUSTER;
            
            GD3D12SupportsClustersAndPTLAS = true;

        #if D3D12_ENABLE_INDIRECT_RTAS_OPERATIONS
            GD3D12SupportsIndirectAccelerationStructureOperations = true;
        #endif
        }
    #endif
    }
    else
    {
        RHI::bSupportsRayTracing         = false;
        RHI::RayTracingMaxRecursionDepth = 0;
        RHI::RayTracingTier              = ERayTracingTier::NotSupported;
    }

    RHI::bSupportsInlineRayTracing                                 = GD3D12SupportsInlineRayTracing;
    RHI::bSupportsOpacityMicromap                                  = GD3D12SupportsOpacityMicromap;
    RHI::bSupportsShaderExecutionReordering                        = GD3D12SupportsShaderExecutionReordering;
    RHI::bShaderExecutionReorderingActuallyReorders                = GD3D12ShaderExecutionReorderingActuallyReorders;
    RHI::bSupportsRayTracingPipelineAdditions                      = GD3D12SupportsRayTracingPipelineAdditions;
    RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure = GD3D12SupportsClustersAndPTLAS;
    RHI::bSupportsIndirectAccelerationStructureOperations          = GD3D12SupportsIndirectAccelerationStructureOperations;
    RHI::bSupportsDispatchRaysIndirect                             = GD3D12SupportsIndirectRayDispatch;

    // Log the backend-native capability view alongside the agnostic RHI table.
    DumpD3D12Capabilities();

    // -------------------------------------------------------------------------------------------
    // View Instancing
    // -------------------------------------------------------------------------------------------

    if (GD3D12ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED)
    {
        RHI::bSupportsViewInstancing = true;
        RHI::MaxViewInstanceCount    = D3D12_MAX_VIEW_INSTANCE_COUNT;
    }
    else
    {
        RHI::bSupportsViewInstancing = false;
        RHI::MaxViewInstanceCount    = 1;
    }

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading (VRS)
    // -------------------------------------------------------------------------------------------

    switch (GD3D12VariableRateShadingTier)
    {
    default:
    case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED:
        RHI::ShadingRateTier          = EShadingRateTier::NotSupported;
        RHI::bSupportsVRS             = false;
        RHI::ShadingRateImageTileSize = 0;
        break;

    case D3D12_VARIABLE_SHADING_RATE_TIER_1:
        RHI::ShadingRateTier = EShadingRateTier::Tier1;
        RHI::bSupportsVRS    = true;
        break;

    case D3D12_VARIABLE_SHADING_RATE_TIER_2:
        RHI::ShadingRateTier = EShadingRateTier::Tier2;
        RHI::bSupportsVRS    = true;
        break;
    }

    if (RHI::bSupportsVRS)
    {
        // Tile size is not cached
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 Features6 = {};
        if (SUCCEEDED(GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &Features6, sizeof(Features6))))
        {
            RHI::ShadingRateImageTileSize = Features6.ShadingRateImageTileSize;
        }
        else
        {
            RHI::ShadingRateImageTileSize = 0;
        }
    }

    // -------------------------------------------------------------------------------------------
    // Sampler Feedback
    // -------------------------------------------------------------------------------------------

#if D3D12_USE_SAMPLER_FEEDBACK
    if (GetDevice()->GetD3D12Device8() != nullptr)
    {
        switch (GD3D12SamplerFeedbackTier)
        {
        case D3D12_SAMPLER_FEEDBACK_TIER_0_9:
            RHI::SamplerFeedbackTier = ESamplerFeedbackTier::Tier0_9;
            break;

        case D3D12_SAMPLER_FEEDBACK_TIER_1_0:
            RHI::SamplerFeedbackTier = ESamplerFeedbackTier::Tier1_0;
            break;

        default:
            RHI::SamplerFeedbackTier = ESamplerFeedbackTier::NotSupported;
            break;
        }
    }
#endif

    RHI::bSupportsSamplerFeedback = RHI::SamplerFeedbackTier != ESamplerFeedbackTier::NotSupported;

    // -------------------------------------------------------------------------------------------
    // Programmable Sample Positions
    // -------------------------------------------------------------------------------------------

#if D3D12_USE_ID3D12COMMANDLIST_1
    switch (GD3D12ProgrammableSamplePositionsTier)
    {
    default:
    case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED:
        RHI::SamplePositionsTier = ESamplePositionsTier::NotSupported;
        break;

    case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_1:
        RHI::SamplePositionsTier         = ESamplePositionsTier::Tier1;
        RHI::MaxSamplePositionGridWidth  = 1;
        RHI::MaxSamplePositionGridHeight = 1;
        break;

    case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_2:
        RHI::SamplePositionsTier         = ESamplePositionsTier::Tier2;
        RHI::MaxSamplePositionGridWidth  = 2;
        RHI::MaxSamplePositionGridHeight = 2;
        break;
    }

    RHI::bSupportsProgrammableSamplePositions = RHI::SamplePositionsTier != ESamplePositionsTier::NotSupported;
    if (RHI::bSupportsProgrammableSamplePositions)
    {
        // D3D12 accepts 1, 2, 4, 8 and 16 samples per pixel at every tier.
        RHI::SupportedSamplePositionSampleCounts = 1u | 2u | 4u | 8u | 16u;
    }
#endif

    RHI::bSupportsDynamicDepthBias           = GD3D12SupportDynamicDepthBias;
    RHI::bSupportsDepthBoundsTest            = GD3D12DepthBoundsTestSupported;
    RHI::bSupportsStreamOutput               = true;
    RHI::bSupportsTimestampQueries           = true;
    RHI::bSupportsPipelineStatisticsQueries  = true;
    RHI::bSupportsGPUTimestampBubblesRemoval = true;

    return true;
}
