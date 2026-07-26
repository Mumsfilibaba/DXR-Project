#pragma once
#include "Core/Templates/TypeTraits.h"
#include "RHI/RHIResources.h"

class FVulkanBufferRHI;
class FVulkanTextureRHI;
struct FVulkanQueryRHI;
class FVulkanShaderResourceViewRHI;
class FVulkanUnorderedAccessViewRHI;
class FVulkanRenderTargetViewRHI;
class FVulkanDepthStencilViewRHI;
class FVulkanSamplerStateRHI;
class FVulkanFenceRHI;
class FVulkanSwapChainRHI;
class FVulkanAccelerationStructure;
class FVulkanGeometryAccelerationStructureRHI;
class FVulkanSceneAccelerationStructureRHI;
class FVulkanOpacityMicromap;
class FVulkanClusterAccelerationStructureRHI;
class FVulkanClusterTemplateRHI;
class FVulkanPartitionedSceneAccelerationStructureRHI;
class FVulkanShaderBindingTable;
class FVulkanGraphicsPipelineStateRHI;
class FVulkanComputePipelineStateRHI;
class FVulkanMeshletPipelineStateRHI;
class FVulkanRayTracingPipelineStateRHI;
class FVulkanInputLayoutRHI;
class FVulkanRasterizerStateRHI;
class FVulkanDepthStencilStateRHI;
class FVulkanBlendStateRHI;
class FVulkanVertexShaderRHI;
class FVulkanHullShaderRHI;
class FVulkanDomainShaderRHI;
class FVulkanGeometryShaderRHI;
class FVulkanPixelShaderRHI;
class FVulkanMeshShaderRHI;
class FVulkanAmplificationShaderRHI;
class FVulkanComputeShaderRHI;
class FVulkanRayGenShaderRHI;
class FVulkanRayAnyHitShaderRHI;
class FVulkanRayClosestHitShaderRHI;
class FVulkanRayMissShaderRHI;

template<typename T>
struct TVulkanRHIResourceType
{
};

template<> struct TVulkanRHIResourceType<FRHIBuffer>
{
    typedef FVulkanBufferRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIQuery>
{
    typedef FVulkanQueryRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIShaderResourceView>
{
    typedef FVulkanShaderResourceViewRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIUnorderedAccessView>
{
    typedef FVulkanUnorderedAccessViewRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIRenderTargetView>
{
    typedef FVulkanRenderTargetViewRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIDepthStencilView>
{
    typedef FVulkanDepthStencilViewRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHISamplerState>
{
    typedef FVulkanSamplerStateRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIFence>
{
    typedef FVulkanFenceRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHISwapChain>
{
    typedef FVulkanSwapChainRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayTracingAccelerationStructure>
{
    typedef FVulkanAccelerationStructure Type;
};

template<> struct TVulkanRHIResourceType<FRHIGeometryAccelerationStructure>
{
    typedef FVulkanGeometryAccelerationStructureRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHISceneAccelerationStructure>
{
    typedef FVulkanSceneAccelerationStructureRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIShaderBindingTable>
{
    typedef FVulkanShaderBindingTable Type;
};

template<> struct TVulkanRHIResourceType<FRHIOpacityMicromap>
{
    typedef FVulkanOpacityMicromap Type;
};

template<> struct TVulkanRHIResourceType<FRHIClusterAccelerationStructure>
{
    typedef FVulkanClusterAccelerationStructureRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIClusterTemplate>
{
    typedef FVulkanClusterTemplateRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIPartitionedSceneAccelerationStructure>
{
    typedef FVulkanPartitionedSceneAccelerationStructureRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIGraphicsPipelineState>
{
    typedef FVulkanGraphicsPipelineStateRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIComputePipelineState>
{
    typedef FVulkanComputePipelineStateRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIMeshletPipelineState>
{
    typedef FVulkanMeshletPipelineStateRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayTracingPipelineState>
{
    typedef FVulkanRayTracingPipelineStateRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIInputLayout>
{
    typedef FVulkanInputLayoutRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIRasterizerState>
{
    typedef FVulkanRasterizerStateRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIDepthStencilState>
{
    typedef FVulkanDepthStencilStateRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIBlendState>
{
    typedef FVulkanBlendStateRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIVertexShader>
{
    typedef FVulkanVertexShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIHullShader>
{
    typedef FVulkanHullShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIDomainShader>
{
    typedef FVulkanDomainShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIGeometryShader>
{
    typedef FVulkanGeometryShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIPixelShader>
{
    typedef FVulkanPixelShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIMeshShader>
{
    typedef FVulkanMeshShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIAmplificationShader>
{
    typedef FVulkanAmplificationShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIComputeShader>
{
    typedef FVulkanComputeShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayGenShader>
{
    typedef FVulkanRayGenShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayAnyHitShader>
{
    typedef FVulkanRayAnyHitShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayClosestHitShader>
{
    typedef FVulkanRayClosestHitShaderRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayMissShader>
{
    typedef FVulkanRayMissShaderRHI Type;
};
