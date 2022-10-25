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

#include "RHI/RHIInterface.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

class FMetalInterface final
    : public FRHIInterface
{
public:
    FMetalInterface();
    ~FMetalInterface();

    virtual bool Initialize() override final;

    virtual FRHITexture*               RHICreateTexture2D(const FRHITexture2DInitializer& Initializer)               override final;
    virtual FRHITexture*          RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer)     override final;
    virtual FRHITextureCube*             RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer)           override final;
    virtual FRHITextureCubeArray*        RHICreateTextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer) override final;
    virtual FRHITexture3D*               RHICreateTexture3D(const FRHITexture3DInitializer& Initializer)               override final;

    virtual FRHISamplerState*            RHICreateSamplerState(const FRHISamplerStateInitializer& Initializer) override final;

    virtual FRHIVertexBuffer*            RHICreateBuffer(const FRHIVertexBufferInitializer& Initializer)     override final;
    virtual FRHIIndexBuffer*             RHICreateBuffer(const FRHIIndexBufferInitializer& Initializer)       override final;
    virtual FRHIGenericBuffer*           RHICreateBuffer(const FRHIGenericBufferInitializer& Initializer)   override final;
    virtual FRHIConstantBuffer*          RHICreateBuffer(const FRHIConstantBufferInitializer& Initializer) override final;

    virtual FRHIRayTracingScene*         RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& Initializer)       override final;
    virtual FRHIRayTracingGeometry*      RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& Initializer) override final;

    virtual FRHIShaderResourceView*      RHICreateShaderResourceView(const FRHITextureSRVDesc& Initializer) override final;
    virtual FRHIShaderResourceView*      RHICreateShaderResourceView(const FRHIBufferSRVDesc& Initializer)  override final;

    virtual FRHIUnorderedAccessView*     RHICreateUnorderedAccessView(const FRHITextureUAVDesc& Initializer) override final;
    virtual FRHIUnorderedAccessView*     RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& Initializer)  override final;

    virtual FRHIComputeShader*           RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIVertexShader*            RHICreateVertexShader(const TArray<uint8>& ShaderCode)        override final;
    virtual FRHIHullShader*              RHICreateHullShader(const TArray<uint8>& ShaderCode)          override final;
    virtual FRHIDomainShader*            RHICreateDomainShader(const TArray<uint8>& ShaderCode)        override final;
    virtual FRHIGeometryShader*          RHICreateGeometryShader(const TArray<uint8>& ShaderCode)      override final;
    virtual FRHIMeshShader*              RHICreateMeshShader(const TArray<uint8>& ShaderCode)          override final;
    virtual FRHIAmplificationShader*     RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIPixelShader*             RHICreatePixelShader(const TArray<uint8>& ShaderCode)         override final;

    virtual FRHIRayGenShader*            RHICreateRayGenShader(const TArray<uint8>& ShaderCode)        override final;
    virtual FRHIRayAnyHitShader*         RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)     override final;
    virtual FRHIRayClosestHitShader*     RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayMissShader*           RHICreateRayMissShader(const TArray<uint8>& ShaderCode)       override final;
    
    virtual FRHIDepthStencilState*       RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& Initializer) override final;
    virtual FRHIRasterizerState*         RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer)     override final;
    virtual FRHIBlendState*              RHICreateBlendState(const FRHIBlendStateInitializer& Initializer)               override final;
    virtual FRHIVertexInputLayout*       RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer) override final;

    virtual FRHIGraphicsPipelineState*   RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer)     override final;
    virtual FRHIComputePipelineState*    RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer)       override final;
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer) override final;

    virtual FRHITimestampQuery*          RHICreateTimestampQuery() override final;

    virtual FRHIViewport*                RHICreateViewport(const FRHIViewportInitializer& Initializer) override final;

    virtual IRHICommandContext*          RHIGetDefaultCommandContext() override final;

    virtual FString GetAdapterDescription() const override final;

    virtual void RHIQueryRayTracingSupport(FRayTracingSupport& OutSupport)   const override final;
    virtual void RHIQueryShadingRateSupport(FShadingRateSupport& OutSupport) const override final;
    virtual bool RHIQueryUAVFormatSupport(EFormat Format)                    const override final;

public:
    FMetalDeviceContext* GetDeviceContext() const { return DeviceContext; }
    
private:
    template<typename MetalTextureType, typename InitializerType>
    MetalTextureType* CreateTexture(const InitializerType& Initializer);
    
    template<typename MetalBufferType, typename InitializerType, const uint32 BufferAlignment = kBufferAlignment>
    MetalBufferType* CreateBuffer(const InitializerType& Initializer);
    
    FMetalDeviceContext*  DeviceContext;
    FMetalCommandContext* CommandContext;
};

#pragma clang diagnostic pop
