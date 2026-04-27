#pragma once
#include "Core/Templates/TypeTraits.h"
#include "RHI/RHIResources.h"

class FMetalBufferRHI;
class FMetalTextureRHI;
struct FMetalQueryRHI;
class FMetalShaderResourceViewRHI;
class FMetalUnorderedAccessViewRHI;
class FMetalRenderTargetViewRHI;
class FMetalDepthStencilViewRHI;
class FMetalSamplerStateRHI;
class FMetalFenceRHI;
class FMetalSwapChainRHI;
class FMetalSceneAccelerationStructureRHI;
class FMetalGeometryAccelerationStructureRHI;
class FMetalGraphicsPipelineStateRHI;
class FMetalComputePipelineStateRHI;
class FMetalRayTracingPipelineStateRHI;
class FMetalInputLayoutRHI;
class FMetalRasterizerStateRHI;
class FMetalDepthStencilStateRHI;
class FMetalBlendStateRHI;
class FMetalVertexShaderRHI;
class FMetalHullShaderRHI;
class FMetalDomainShaderRHI;
class FMetalGeometryShaderRHI;
class FMetalPixelShaderRHI;
class FMetalComputeShaderRHI;
class FMetalRayGenShaderRHI;
class FMetalRayAnyHitShaderRHI;
class FMetalRayClosestHitShaderRHI;
class FMetalRayMissShaderRHI;

template<typename T>
struct TMetalRHIResourceType
{
};

template<> struct TMetalRHIResourceType<FRHIBuffer>
{
    typedef FMetalBufferRHI Type;
};

template<> struct TMetalRHIResourceType<FRHITexture>
{
    typedef FMetalTextureRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIQuery>
{
    typedef FMetalQueryRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIShaderResourceView>
{
    typedef FMetalShaderResourceViewRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIUnorderedAccessView>
{
    typedef FMetalUnorderedAccessViewRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIRenderTargetView>
{
    typedef FMetalRenderTargetViewRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIDepthStencilView>
{
    typedef FMetalDepthStencilViewRHI Type;
};

template<> struct TMetalRHIResourceType<FRHISamplerState>
{
    typedef FMetalSamplerStateRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIFence>
{
    typedef FMetalFenceRHI Type;
};

template<> struct TMetalRHIResourceType<FRHISwapChain>
{
    typedef FMetalSwapChainRHI Type;
};

template<> struct TMetalRHIResourceType<FRHISceneAccelerationStructure>
{
    typedef FMetalSceneAccelerationStructureRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIGeometryAccelerationStructure>
{
    typedef FMetalGeometryAccelerationStructureRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIGraphicsPipelineState>
{
    typedef FMetalGraphicsPipelineStateRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIComputePipelineState>
{
    typedef FMetalComputePipelineStateRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIRayTracingPipelineState>
{
    typedef FMetalRayTracingPipelineStateRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIInputLayout>
{
    typedef FMetalInputLayoutRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIRasterizerState>
{
    typedef FMetalRasterizerStateRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIDepthStencilState>
{
    typedef FMetalDepthStencilStateRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIBlendState>
{
    typedef FMetalBlendStateRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIVertexShader>
{
    typedef FMetalVertexShaderRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIHullShader>
{
    typedef FMetalHullShaderRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIDomainShader>
{
    typedef FMetalDomainShaderRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIGeometryShader>
{
    typedef FMetalGeometryShaderRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIPixelShader>
{
    typedef FMetalPixelShaderRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIComputeShader>
{
    typedef FMetalComputeShaderRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIRayGenShader>
{
    typedef FMetalRayGenShaderRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIRayAnyHitShader>
{
    typedef FMetalRayAnyHitShaderRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIRayClosestHitShader>
{
    typedef FMetalRayClosestHitShaderRHI Type;
};

template<> struct TMetalRHIResourceType<FRHIRayMissShader>
{
    typedef FMetalRayMissShaderRHI Type;
};
