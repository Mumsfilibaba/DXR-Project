#pragma once
#include "Core/Templates/TypeTraits.h"
#include "RHI/RHIResources.h"

class FVulkanBuffer;
class FVulkanTexture;
struct FVulkanQueryRHI;
class FVulkanShaderResourceView;
class FVulkanUnorderedAccessView;
class FVulkanSamplerState;
class FVulkanGpuFence;
class FVulkanSwapChain;
class FVulkanRayTracingGeometry;
class FVulkanGraphicsPipelineState;
class FVulkanComputePipelineState;
class FVulkanRayTracingPipelineState;
class FVulkanInputLayout;
class FVulkanRasterizerState;
class FVulkanDepthStencilState;
class FVulkanBlendState;
class FVulkanVertexShader;
class FVulkanHullShader;
class FVulkanDomainShader;
class FVulkanGeometryShader;
class FVulkanPixelShader;
class FVulkanComputeShader;
class FVulkanRayGenShader;
class FVulkanRayAnyHitShader;
class FVulkanRayClosestHitShader;
class FVulkanRayMissShader;

template<typename T>
struct TVulkanRHIResourceType
{
};

template<> struct TVulkanRHIResourceType<FRHIBuffer>
{
    typedef FVulkanBuffer Type;
};

template<> struct TVulkanRHIResourceType<FRHIQuery>
{
    typedef FVulkanQueryRHI Type;
};

template<> struct TVulkanRHIResourceType<FRHIShaderResourceView>
{
    typedef FVulkanShaderResourceView Type;
};

template<> struct TVulkanRHIResourceType<FRHIUnorderedAccessView>
{
    typedef FVulkanUnorderedAccessView Type;
};

template<> struct TVulkanRHIResourceType<FRHISamplerState>
{
    typedef FVulkanSamplerState Type;
};

template<> struct TVulkanRHIResourceType<FRHIGpuFence>
{
    typedef FVulkanGpuFence Type;
};

template<> struct TVulkanRHIResourceType<FRHISwapChain>
{
    typedef FVulkanSwapChain Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayTracingGeometry>
{
    typedef FVulkanRayTracingGeometry Type;
};

template<> struct TVulkanRHIResourceType<FRHIGraphicsPipelineState>
{
    typedef FVulkanGraphicsPipelineState Type;
};

template<> struct TVulkanRHIResourceType<FRHIComputePipelineState>
{
    typedef FVulkanComputePipelineState Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayTracingPipelineState>
{
    typedef FVulkanRayTracingPipelineState Type;
};

template<> struct TVulkanRHIResourceType<FRHIInputLayout>
{
    typedef FVulkanInputLayout Type;
};

template<> struct TVulkanRHIResourceType<FRHIRasterizerState>
{
    typedef FVulkanRasterizerState Type;
};

template<> struct TVulkanRHIResourceType<FRHIDepthStencilState>
{
    typedef FVulkanDepthStencilState Type;
};

template<> struct TVulkanRHIResourceType<FRHIBlendState>
{
    typedef FVulkanBlendState Type;
};

template<> struct TVulkanRHIResourceType<FRHIVertexShader>
{
    typedef FVulkanVertexShader Type;
};

template<> struct TVulkanRHIResourceType<FRHIHullShader>
{
    typedef FVulkanHullShader Type;
};

template<> struct TVulkanRHIResourceType<FRHIDomainShader>
{
    typedef FVulkanDomainShader Type;
};

template<> struct TVulkanRHIResourceType<FRHIGeometryShader>
{
    typedef FVulkanGeometryShader Type;
};

template<> struct TVulkanRHIResourceType<FRHIPixelShader>
{
    typedef FVulkanPixelShader Type;
};

template<> struct TVulkanRHIResourceType<FRHIComputeShader>
{
    typedef FVulkanComputeShader Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayGenShader>
{
    typedef FVulkanRayGenShader Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayAnyHitShader>
{
    typedef FVulkanRayAnyHitShader Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayClosestHitShader>
{
    typedef FVulkanRayClosestHitShader Type;
};

template<> struct TVulkanRHIResourceType<FRHIRayMissShader>
{
    typedef FVulkanRayMissShader Type;
};
