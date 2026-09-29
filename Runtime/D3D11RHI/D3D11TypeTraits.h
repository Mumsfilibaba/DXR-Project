#pragma once
#include "Core/Templates/TypeTraits.h"
#include "RHI/RHIResources.h"

class FD3D11BufferRHI;
class FD3D11TextureRHI;
struct FD3D11QueryRHI;
class FD3D11ShaderResourceViewRHI;
class FD3D11UnorderedAccessViewRHI;
class FD3D11RenderTargetViewRHI;
class FD3D11DepthStencilViewRHI;
class FD3D11SamplerStateRHI;
class FD3D11FenceRHI;
class FD3D11SwapChainRHI;
class FD3D11GraphicsPipelineStateRHI;
class FD3D11ComputePipelineStateRHI;
class FD3D11InputLayoutRHI;
class FD3D11RasterizerStateRHI;
class FD3D11DepthStencilStateRHI;
class FD3D11BlendStateRHI;
class FD3D11VertexShaderRHI;
class FD3D11HullShaderRHI;
class FD3D11DomainShaderRHI;
class FD3D11GeometryShaderRHI;
class FD3D11PixelShaderRHI;
class FD3D11ComputeShaderRHI;

template<typename T>
struct TD3D11RHIResourceType
{
};

template<> struct TD3D11RHIResourceType<FRHIBuffer>
{
    typedef FD3D11BufferRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHITexture>
{
    typedef FD3D11TextureRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIQuery>
{
    typedef FD3D11QueryRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIShaderResourceView>
{
    typedef FD3D11ShaderResourceViewRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIUnorderedAccessView>
{
    typedef FD3D11UnorderedAccessViewRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIRenderTargetView>
{
    typedef FD3D11RenderTargetViewRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIDepthStencilView>
{
    typedef FD3D11DepthStencilViewRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHISamplerState>
{
    typedef FD3D11SamplerStateRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIFence>
{
    typedef FD3D11FenceRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHISwapChain>
{
    typedef FD3D11SwapChainRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIGraphicsPipelineState>
{
    typedef FD3D11GraphicsPipelineStateRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIComputePipelineState>
{
    typedef FD3D11ComputePipelineStateRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIInputLayout>
{
    typedef FD3D11InputLayoutRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIRasterizerState>
{
    typedef FD3D11RasterizerStateRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIDepthStencilState>
{
    typedef FD3D11DepthStencilStateRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIBlendState>
{
    typedef FD3D11BlendStateRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIVertexShader>
{
    typedef FD3D11VertexShaderRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIHullShader>
{
    typedef FD3D11HullShaderRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIDomainShader>
{
    typedef FD3D11DomainShaderRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIGeometryShader>
{
    typedef FD3D11GeometryShaderRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIPixelShader>
{
    typedef FD3D11PixelShaderRHI Type;
};

template<> struct TD3D11RHIResourceType<FRHIComputeShader>
{
    typedef FD3D11ComputeShaderRHI Type;
};
