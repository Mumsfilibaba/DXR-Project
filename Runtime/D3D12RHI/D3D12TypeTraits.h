#pragma once
#include "Core/Templates/TypeTraits.h"
#include "RHI/RHIResources.h"

class FRHIShaderBindingTable;
class FRHIOpacityMicromap;

class FD3D12BufferRHI;
class FD3D12TextureRHI;
struct FD3D12QueryRHI;
class FD3D12ShaderResourceViewRHI;
class FD3D12UnorderedAccessViewRHI;
class FD3D12RenderTargetViewRHI;
class FD3D12DepthStencilViewRHI;
class FD3D12SamplerStateRHI;
class FD3D12FenceRHI;
class FD3D12SwapChainRHI;
class FD3D12SceneAccelerationStructureRHI;
class FD3D12GeometryAccelerationStructureRHI;
class FD3D12ShaderBindingTable;
class FD3D12OpacityMicromapRHI;
class FD3D12GraphicsPipelineStateRHI;
class FD3D12ComputePipelineStateRHI;
class FD3D12MeshletPipelineStateRHI;
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
class FD3D12MeshShaderRHI;
class FD3D12AmplificationShaderRHI;
class FD3D12ComputeShaderRHI;
class FD3D12RayGenShaderRHI;
class FD3D12RayAnyHitShaderRHI;
class FD3D12RayClosestHitShaderRHI;
class FD3D12RayMissShaderRHI;

template<typename T>
struct TD3D12RHIResourceType
{
};

template<> struct TD3D12RHIResourceType<FRHIBuffer>
{
    typedef FD3D12BufferRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIQuery>
{
    typedef FD3D12QueryRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIShaderResourceView>
{
    typedef FD3D12ShaderResourceViewRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIUnorderedAccessView>
{
    typedef FD3D12UnorderedAccessViewRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIRenderTargetView>
{
    typedef FD3D12RenderTargetViewRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIDepthStencilView>
{
    typedef FD3D12DepthStencilViewRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHISamplerState>
{
    typedef FD3D12SamplerStateRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIFence>
{
    typedef FD3D12FenceRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHISwapChain>
{
    typedef FD3D12SwapChainRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHISceneAccelerationStructure>
{
    typedef FD3D12SceneAccelerationStructureRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIGeometryAccelerationStructure>
{
    typedef FD3D12GeometryAccelerationStructureRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIShaderBindingTable>
{
    typedef FD3D12ShaderBindingTable Type;
};

template<> struct TD3D12RHIResourceType<FRHIOpacityMicromap>
{
    typedef FD3D12OpacityMicromapRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIGraphicsPipelineState>
{
    typedef FD3D12GraphicsPipelineStateRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIComputePipelineState>
{
    typedef FD3D12ComputePipelineStateRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIMeshletPipelineState>
{
    typedef FD3D12MeshletPipelineStateRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayTracingPipelineState>
{
    typedef FD3D12RayTracingPipelineStateRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIInputLayout>
{
    typedef FD3D12InputLayoutRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIRasterizerState>
{
    typedef FD3D12RasterizerStateRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIDepthStencilState>
{
    typedef FD3D12DepthStencilStateRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIBlendState>
{
    typedef FD3D12BlendStateRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIVertexShader>
{
    typedef FD3D12VertexShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIHullShader>
{
    typedef FD3D12HullShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIDomainShader>
{
    typedef FD3D12DomainShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIGeometryShader>
{
    typedef FD3D12GeometryShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIPixelShader>
{
    typedef FD3D12PixelShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIMeshShader>
{
    typedef FD3D12MeshShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIAmplificationShader>
{
    typedef FD3D12AmplificationShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIComputeShader>
{
    typedef FD3D12ComputeShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayGenShader>
{
    typedef FD3D12RayGenShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayAnyHitShader>
{
    typedef FD3D12RayAnyHitShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayClosestHitShader>
{
    typedef FD3D12RayClosestHitShaderRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayMissShader>
{
    typedef FD3D12RayMissShaderRHI Type;
};
