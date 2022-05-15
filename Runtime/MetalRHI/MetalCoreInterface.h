#pragma once
#include "MetalBuffer.h"
#include "MetalTexture.h"
#include "MetalViews.h"
#include "MetalSamplerState.h"
#include "MetalViewport.h"
#include "MetalShader.h"
#include "MetalCommandContext.h"
#include "MetalTimestampQuery.h"
#include "MetalPipelineState.h"
#include "MetalRayTracing.h"
#include "MetalDeviceContext.h"

#include "RHI/RHICoreInterface.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalCoreInterface

class CMetalCoreInterface final : public CRHICoreInterface
{
private:

	CMetalCoreInterface();
	~CMetalCoreInterface();

public:

	static CMetalCoreInterface* CreateMetalCoreInterface();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHICoreInterface Interface

	virtual bool Initialize(bool bEnableDebug) override final;

    virtual CRHITexture2D*        RHICreateTexture2D(const CRHITexture2DInitializer& Initializer) override final;
    virtual CRHITexture2DArray*   RHICreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer) override final;
    virtual CRHITextureCube*      RHICreateTextureCube(const CRHITextureCubeInitializer& Initializer) override final;
    virtual CRHITextureCubeArray* RHICreateTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer) override final;
    virtual CRHITexture3D*        RHICreateTexture3D(const CRHITexture3DInitializer& Initializer) override final;

    virtual CRHISamplerState*     RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer) override final;

    virtual CRHIVertexBuffer*     RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer) override final;
    virtual CRHIIndexBuffer*      RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer) override final;
    virtual CRHIGenericBuffer*    RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer) override final;
    virtual CRHIConstantBuffer*   RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer) override final;

    virtual CRHIRayTracingScene*     RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer) override final;
    virtual CRHIRayTracingGeometry*  RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer) override final;

    virtual CRHIShaderResourceView*  RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer) override final;
    virtual CRHIShaderResourceView*  RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer) override final;

    virtual CRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer) override final;
    virtual CRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer) override final;

    virtual CRHIComputeShader*       RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final;

    virtual CRHIVertexShader*        RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIHullShader*          RHICreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIDomainShader*        RHICreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIGeometryShader*      RHICreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIMeshShader*          RHICreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIPixelShader*         RHICreatePixelShader(const TArray<uint8>& ShaderCode) override final;

    virtual CRHIRayGenShader*        RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIRayAnyHitShader*     RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIRayMissShader*       RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final;
    
    virtual CRHIDepthStencilState*   RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer) override final;
    virtual CRHIRasterizerState*     RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer) override final;
    virtual CRHIBlendState*          RHICreateBlendState(const CRHIBlendStateInitializer& Initializer) override final;
    virtual CRHIVertexInputLayout*   RHICreateVertexInputLayout(const CRHIVertexInputLayoutInitializer& Initializer) override final;

    virtual CRHIGraphicsPipelineState*   RHICreateGraphicsPipelineState(const CRHIGraphicsPipelineStateInitializer& Initializer) override final;
    virtual CRHIComputePipelineState*    RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer) override final;
    virtual CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const CRHIRayTracingPipelineStateInitializer& Initializer) override final;

    virtual CRHITimestampQuery* RHICreateTimestampQuery() override final;

    virtual CRHIViewport*       RHICreateViewport(const CRHIViewportInitializer& Initializer) override final;

    virtual IRHICommandContext* RHIGetDefaultCommandContext() override final;

    virtual String GetAdapterName() const override final;

    virtual void RHIQueryRayTracingSupport(SRayTracingSupport& OutSupport) const override final;

    virtual void RHIQueryShadingRateSupport(SShadingRateSupport& OutSupport) const override final;

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final;

public:
    
    CMetalDeviceContext* GetDeviceContext() const { return DeviceContext; }
    
private:
    
    template<typename MetalTextureType, typename InitializerType>
    MetalTextureType* CreateTexture(const InitializerType& Initializer);
    
    template<typename MetalBufferType, typename InitializerType>
    MetalBufferType* CreateBuffer(const InitializerType& Initializer);
    
	CMetalDeviceContext*  DeviceContext;
    CMetalCommandContext* CommandContext;
};

#pragma clang diagnostic pop
