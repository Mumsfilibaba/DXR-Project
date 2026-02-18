#pragma once
#include "Core/Templates/TypeTraits.h"
#include "RHI/RHIResources.h"

// Forward declarations - actual definitions included where needed
class FVulkanBuffer;
class FVulkanTexture;
struct FVulkanQuery;
class FVulkanShaderResourceView;
class FVulkanUnorderedAccessView;
class FVulkanSamplerState;
class FVulkanGpuFence;
class FVulkanSwapChain;
class FVulkanRayTracingScene;
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

// Buffer
template<>
struct TVulkanRHIResourceType<FRHIBuffer>
{
	typedef FVulkanBuffer Type;
};

// Query
template<>
struct TVulkanRHIResourceType<FRHIQuery>
{
	typedef FVulkanQuery Type;
};

// Shader Resource View
template<>
struct TVulkanRHIResourceType<FRHIShaderResourceView>
{
	typedef FVulkanShaderResourceView Type;
};

// Unordered Access View
template<>
struct TVulkanRHIResourceType<FRHIUnorderedAccessView>
{
	typedef FVulkanUnorderedAccessView Type;
};

// Sampler State
template<>
struct TVulkanRHIResourceType<FRHISamplerState>
{
	typedef FVulkanSamplerState Type;
};

// GPU Fence
template<>
struct TVulkanRHIResourceType<FRHIGpuFence>
{
	typedef FVulkanGpuFence Type;
};

// Swap Chain
template<>
struct TVulkanRHIResourceType<FRHISwapChain>
{
	typedef FVulkanSwapChain Type;
};

// Ray Tracing Scene
template<>
struct TVulkanRHIResourceType<FRHIRayTracingScene>
{
	typedef FVulkanRayTracingScene Type;
};

// Ray Tracing Geometry
template<>
struct TVulkanRHIResourceType<FRHIRayTracingGeometry>
{
	typedef FVulkanRayTracingGeometry Type;
};

// Graphics Pipeline State
template<>
struct TVulkanRHIResourceType<FRHIGraphicsPipelineState>
{
	typedef FVulkanGraphicsPipelineState Type;
};

// Compute Pipeline State
template<>
struct TVulkanRHIResourceType<FRHIComputePipelineState>
{
	typedef FVulkanComputePipelineState Type;
};

// Ray Tracing Pipeline State
template<>
struct TVulkanRHIResourceType<FRHIRayTracingPipelineState>
{
	typedef FVulkanRayTracingPipelineState Type;
};

// Input Layout
template<>
struct TVulkanRHIResourceType<FRHIInputLayout>
{
	typedef FVulkanInputLayout Type;
};

// Rasterizer State
template<>
struct TVulkanRHIResourceType<FRHIRasterizerState>
{
	typedef FVulkanRasterizerState Type;
};

// Depth Stencil State
template<>
struct TVulkanRHIResourceType<FRHIDepthStencilState>
{
	typedef FVulkanDepthStencilState Type;
};

// Blend State
template<>
struct TVulkanRHIResourceType<FRHIBlendState>
{
	typedef FVulkanBlendState Type;
};

// Shaders
template<>
struct TVulkanRHIResourceType<FRHIVertexShader>
{
	typedef FVulkanVertexShader Type;
};

template<>
struct TVulkanRHIResourceType<FRHIHullShader>
{
	typedef FVulkanHullShader Type;
};

template<>
struct TVulkanRHIResourceType<FRHIDomainShader>
{
	typedef FVulkanDomainShader Type;
};

template<>
struct TVulkanRHIResourceType<FRHIGeometryShader>
{
	typedef FVulkanGeometryShader Type;
};

template<>
struct TVulkanRHIResourceType<FRHIPixelShader>
{
	typedef FVulkanPixelShader Type;
};

template<>
struct TVulkanRHIResourceType<FRHIComputeShader>
{
	typedef FVulkanComputeShader Type;
};

template<>
struct TVulkanRHIResourceType<FRHIRayGenShader>
{
	typedef FVulkanRayGenShader Type;
};

template<>
struct TVulkanRHIResourceType<FRHIRayAnyHitShader>
{
	typedef FVulkanRayAnyHitShader Type;
};

template<>
struct TVulkanRHIResourceType<FRHIRayClosestHitShader>
{
	typedef FVulkanRayClosestHitShader Type;
};

template<>
struct TVulkanRHIResourceType<FRHIRayMissShader>
{
	typedef FVulkanRayMissShader Type;
};
