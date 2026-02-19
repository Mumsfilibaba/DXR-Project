#pragma once
#include "Core/Templates/TypeTraits.h"
#include "RHI/RHIResources.h"

// Forward declarations - actual definitions included where needed
class FVulkanBufferRHI;
class FVulkanTextureRHI;
struct FVulkanQueryRHI;
class FVulkanShaderResourceViewRHI;
class FVulkanUnorderedAccessViewRHI;
class FVulkanSamplerStateRHI;
class FVulkanFenceRHI;
class FVulkanSwapChainRHI;
class FVulkanSceneAccelerationStructureRHI;
class FVulkanGeometryAccelerationStructureRHI;
class FVulkanGraphicsPipelineStateRHI;
class FVulkanComputePipelineStateRHI;
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
class FVulkanComputeShaderRHI;
class FVulkanRayGenShaderRHI;
class FVulkanRayAnyHitShaderRHI;
class FVulkanRayClosestHitShaderRHI;
class FVulkanRayMissShaderRHI;

template<typename T>
struct TVulkanRHIResourceType
{
};

// Buffer
template<>
struct TVulkanRHIResourceType<FRHIBuffer>
{
	typedef FVulkanBufferRHI Type;
};

// Query
template<>
struct TVulkanRHIResourceType<FRHIQuery>
{
	typedef FVulkanQueryRHI Type;
};

// Shader Resource View
template<>
struct TVulkanRHIResourceType<FRHIShaderResourceView>
{
	typedef FVulkanShaderResourceViewRHI Type;
};

// Unordered Access View
template<>
struct TVulkanRHIResourceType<FRHIUnorderedAccessView>
{
	typedef FVulkanUnorderedAccessViewRHI Type;
};

// Sampler State
template<>
struct TVulkanRHIResourceType<FRHISamplerState>
{
	typedef FVulkanSamplerStateRHI Type;
};

// GPU Fence
template<>
struct TVulkanRHIResourceType<FRHIFence>
{
	typedef FVulkanFenceRHI Type;
};

// Swap Chain
template<>
struct TVulkanRHIResourceType<FRHISwapChain>
{
	typedef FVulkanSwapChainRHI Type;
};

// Ray Tracing Scene
template<>
struct TVulkanRHIResourceType<FRHISceneAccelerationStructure>
{
	typedef FVulkanSceneAccelerationStructureRHI Type;
};

// Ray Tracing Geometry
template<>
struct TVulkanRHIResourceType<FRHIGeometryAccelerationStructure>
{
	typedef FVulkanGeometryAccelerationStructureRHI Type;
};

// Graphics Pipeline State
template<>
struct TVulkanRHIResourceType<FRHIGraphicsPipelineState>
{
	typedef FVulkanGraphicsPipelineStateRHI Type;
};

// Compute Pipeline State
template<>
struct TVulkanRHIResourceType<FRHIComputePipelineState>
{
	typedef FVulkanComputePipelineStateRHI Type;
};

// Ray Tracing Pipeline State
template<>
struct TVulkanRHIResourceType<FRHIRayTracingPipelineState>
{
	typedef FVulkanRayTracingPipelineStateRHI Type;
};

// Input Layout
template<>
struct TVulkanRHIResourceType<FRHIInputLayout>
{
	typedef FVulkanInputLayoutRHI Type;
};

// Rasterizer State
template<>
struct TVulkanRHIResourceType<FRHIRasterizerState>
{
	typedef FVulkanRasterizerStateRHI Type;
};

// Depth Stencil State
template<>
struct TVulkanRHIResourceType<FRHIDepthStencilState>
{
	typedef FVulkanDepthStencilStateRHI Type;
};

// Blend State
template<>
struct TVulkanRHIResourceType<FRHIBlendState>
{
	typedef FVulkanBlendStateRHI Type;
};

// Shaders
template<>
struct TVulkanRHIResourceType<FRHIVertexShader>
{
	typedef FVulkanVertexShaderRHI Type;
};

template<>
struct TVulkanRHIResourceType<FRHIHullShader>
{
	typedef FVulkanHullShaderRHI Type;
};

template<>
struct TVulkanRHIResourceType<FRHIDomainShader>
{
	typedef FVulkanDomainShaderRHI Type;
};

template<>
struct TVulkanRHIResourceType<FRHIGeometryShader>
{
	typedef FVulkanGeometryShaderRHI Type;
};

template<>
struct TVulkanRHIResourceType<FRHIPixelShader>
{
	typedef FVulkanPixelShaderRHI Type;
};

template<>
struct TVulkanRHIResourceType<FRHIComputeShader>
{
	typedef FVulkanComputeShaderRHI Type;
};

template<>
struct TVulkanRHIResourceType<FRHIRayGenShader>
{
	typedef FVulkanRayGenShaderRHI Type;
};

template<>
struct TVulkanRHIResourceType<FRHIRayAnyHitShader>
{
	typedef FVulkanRayAnyHitShaderRHI Type;
};

template<>
struct TVulkanRHIResourceType<FRHIRayClosestHitShader>
{
	typedef FVulkanRayClosestHitShaderRHI Type;
};

template<>
struct TVulkanRHIResourceType<FRHIRayMissShader>
{
	typedef FVulkanRayMissShaderRHI Type;
};
