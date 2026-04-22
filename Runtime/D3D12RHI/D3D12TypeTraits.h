#pragma once
#include "Core/Templates/TypeTraits.h"
#include "RHI/RHIResources.h"

class FD3D12Buffer;
class FD3D12Texture;
struct FD3D12QueryRHI;
class FD3D12ShaderResourceView;
class FD3D12UnorderedAccessView;
class FD3D12SamplerState;
class FD3D12GpuFence;
class FD3D12SwapChain;
class FD3D12RayTracingScene;
class FD3D12RayTracingGeometry;
class FD3D12GraphicsPipelineState;
class FD3D12ComputePipelineState;
class FD3D12RayTracingPipelineState;
class FD3D12InputLayout;
class FD3D12RasterizerState;
class FD3D12DepthStencilState;
class FD3D12BlendState;
class FD3D12VertexShader;
class FD3D12HullShader;
class FD3D12DomainShader;
class FD3D12GeometryShader;
class FD3D12PixelShader;
class FD3D12ComputeShader;
class FD3D12RayGenShader;
class FD3D12RayAnyHitShader;
class FD3D12RayClosestHitShader;
class FD3D12RayMissShader;

template<typename T>
struct TD3D12RHIResourceType
{
};

template<> struct TD3D12RHIResourceType<FRHIBuffer>
{
    typedef FD3D12Buffer Type;
};

template<> struct TD3D12RHIResourceType<FRHIQuery>
{
    typedef FD3D12QueryRHI Type;
};

template<> struct TD3D12RHIResourceType<FRHIShaderResourceView>
{
    typedef FD3D12ShaderResourceView Type;
};

template<> struct TD3D12RHIResourceType<FRHIUnorderedAccessView>
{
    typedef FD3D12UnorderedAccessView Type;
};

template<> struct TD3D12RHIResourceType<FRHISamplerState>
{
    typedef FD3D12SamplerState Type;
};

template<> struct TD3D12RHIResourceType<FRHIGpuFence>
{
    typedef FD3D12GpuFence Type;
};

template<> struct TD3D12RHIResourceType<FRHISwapChain>
{
    typedef FD3D12SwapChain Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayTracingScene>
{
    typedef FD3D12RayTracingScene Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayTracingGeometry>
{
    typedef FD3D12RayTracingGeometry Type;
};

template<> struct TD3D12RHIResourceType<FRHIGraphicsPipelineState>
{
    typedef FD3D12GraphicsPipelineState Type;
};

template<> struct TD3D12RHIResourceType<FRHIComputePipelineState>
{
    typedef FD3D12ComputePipelineState Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayTracingPipelineState>
{
    typedef FD3D12RayTracingPipelineState Type;
};

template<> struct TD3D12RHIResourceType<FRHIInputLayout>
{
    typedef FD3D12InputLayout Type;
};

template<> struct TD3D12RHIResourceType<FRHIRasterizerState>
{
    typedef FD3D12RasterizerState Type;
};

template<> struct TD3D12RHIResourceType<FRHIDepthStencilState>
{
    typedef FD3D12DepthStencilState Type;
};

template<> struct TD3D12RHIResourceType<FRHIBlendState>
{
    typedef FD3D12BlendState Type;
};

template<> struct TD3D12RHIResourceType<FRHIVertexShader>
{
    typedef FD3D12VertexShader Type;
};

template<> struct TD3D12RHIResourceType<FRHIHullShader>
{
    typedef FD3D12HullShader Type;
};

template<> struct TD3D12RHIResourceType<FRHIDomainShader>
{
    typedef FD3D12DomainShader Type;
};

template<> struct TD3D12RHIResourceType<FRHIGeometryShader>
{
    typedef FD3D12GeometryShader Type;
};

template<> struct TD3D12RHIResourceType<FRHIPixelShader>
{
    typedef FD3D12PixelShader Type;
};

template<> struct TD3D12RHIResourceType<FRHIComputeShader>
{
    typedef FD3D12ComputeShader Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayGenShader>
{
    typedef FD3D12RayGenShader Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayAnyHitShader>
{
    typedef FD3D12RayAnyHitShader Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayClosestHitShader>
{
    typedef FD3D12RayClosestHitShader Type;
};

template<> struct TD3D12RHIResourceType<FRHIRayMissShader>
{
    typedef FD3D12RayMissShader Type;
};
