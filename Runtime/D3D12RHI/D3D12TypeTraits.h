#pragma once
#include "Core/Templates/TypeTraits.h"
#include "RHI/RHIResources.h"

// Forward declarations - actual definitions included where needed
class FD3D12BufferRHI;
class FD3D12TextureRHI;
struct FD3D12QueryRHI;
class FD3D12ShaderResourceViewRHI;
class FD3D12UnorderedAccessViewRHI;
class FD3D12SamplerStateRHI;
class FD3D12FenceRHI;
class FD3D12SwapChainRHI;
class FD3D12SceneAccelerationStructureRHI;
class FD3D12GeometryAccelerationStructureRHI;
class FD3D12GraphicsPipelineStateRHI;
class FD3D12ComputePipelineStateRHI;
class FD3D12RayTracingPipelineStateRHI;
class FD3D12InputLayoutRHI;
class FD3D12RasterizerStateRHI;
class FD3D12DepthStencilStateRHI;
class FD3D12BlendStateRHI;
class FD3D12VertexShaderRHI;
class FD3D12HullShaderRHI;
class FD3D12DomainShaderRHI;
class FD3D12GeometryShaderRHI;
class FD3D12PixelShaderRHI;
class FD3D12ComputeShaderRHI;
class FD3D12RayGenShaderRHI;
class FD3D12RayAnyHitShaderRHI;
class FD3D12RayClosestHitShaderRHI;
class FD3D12RayMissShaderRHI;

template<typename T>
struct TD3D12RHIResourceType
{
};

// Buffer
template<>
struct TD3D12RHIResourceType<FRHIBuffer>
{
	typedef FD3D12BufferRHI Type;
};

// Query
template<>
struct TD3D12RHIResourceType<FRHIQuery>
{
	typedef FD3D12QueryRHI Type;
};

// Shader Resource View
template<>
struct TD3D12RHIResourceType<FRHIShaderResourceView>
{
	typedef FD3D12ShaderResourceViewRHI Type;
};

// Unordered Access View
template<>
struct TD3D12RHIResourceType<FRHIUnorderedAccessView>
{
	typedef FD3D12UnorderedAccessViewRHI Type;
};

// Sampler State
template<>
struct TD3D12RHIResourceType<FRHISamplerState>
{
	typedef FD3D12SamplerStateRHI Type;
};

// GPU Fence
template<>
struct TD3D12RHIResourceType<FRHIFence>
{
	typedef FD3D12FenceRHI Type;
};

// Swap Chain
template<>
struct TD3D12RHIResourceType<FRHISwapChain>
{
	typedef FD3D12SwapChainRHI Type;
};

// Ray Tracing Scene
template<>
struct TD3D12RHIResourceType<FRHISceneAccelerationStructure>
{
	typedef FD3D12SceneAccelerationStructureRHI Type;
};

// Ray Tracing Geometry
template<>
struct TD3D12RHIResourceType<FRHIGeometryAccelerationStructure>
{
	typedef FD3D12GeometryAccelerationStructureRHI Type;
};

// Graphics Pipeline State
template<>
struct TD3D12RHIResourceType<FRHIGraphicsPipelineState>
{
	typedef FD3D12GraphicsPipelineStateRHI Type;
};

// Compute Pipeline State
template<>
struct TD3D12RHIResourceType<FRHIComputePipelineState>
{
	typedef FD3D12ComputePipelineStateRHI Type;
};

// Ray Tracing Pipeline State
template<>
struct TD3D12RHIResourceType<FRHIRayTracingPipelineState>
{
	typedef FD3D12RayTracingPipelineStateRHI Type;
};

// Input Layout
template<>
struct TD3D12RHIResourceType<FRHIInputLayout>
{
	typedef FD3D12InputLayoutRHI Type;
};

// Rasterizer State
template<>
struct TD3D12RHIResourceType<FRHIRasterizerState>
{
	typedef FD3D12RasterizerStateRHI Type;
};

// Depth Stencil State
template<>
struct TD3D12RHIResourceType<FRHIDepthStencilState>
{
	typedef FD3D12DepthStencilStateRHI Type;
};

// Blend State
template<>
struct TD3D12RHIResourceType<FRHIBlendState>
{
	typedef FD3D12BlendStateRHI Type;
};

// Shaders
template<>
struct TD3D12RHIResourceType<FRHIVertexShader>
{
	typedef FD3D12VertexShaderRHI Type;
};

template<>
struct TD3D12RHIResourceType<FRHIHullShader>
{
	typedef FD3D12HullShaderRHI Type;
};

template<>
struct TD3D12RHIResourceType<FRHIDomainShader>
{
	typedef FD3D12DomainShaderRHI Type;
};

template<>
struct TD3D12RHIResourceType<FRHIGeometryShader>
{
	typedef FD3D12GeometryShaderRHI Type;
};

template<>
struct TD3D12RHIResourceType<FRHIPixelShader>
{
	typedef FD3D12PixelShaderRHI Type;
};

template<>
struct TD3D12RHIResourceType<FRHIComputeShader>
{
	typedef FD3D12ComputeShaderRHI Type;
};

template<>
struct TD3D12RHIResourceType<FRHIRayGenShader>
{
	typedef FD3D12RayGenShaderRHI Type;
};

template<>
struct TD3D12RHIResourceType<FRHIRayAnyHitShader>
{
	typedef FD3D12RayAnyHitShaderRHI Type;
};

template<>
struct TD3D12RHIResourceType<FRHIRayClosestHitShader>
{
	typedef FD3D12RayClosestHitShaderRHI Type;
};

template<>
struct TD3D12RHIResourceType<FRHIRayMissShader>
{
	typedef FD3D12RayMissShaderRHI Type;
};
