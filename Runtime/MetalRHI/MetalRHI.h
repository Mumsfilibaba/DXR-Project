#pragma once
#include "MetalBuffer.h"
#include "MetalTexture.h"
#include "MetalViews.h"
#include "MetalSamplerState.h"
#include "MetalViewport.h"
#include "MetalShader.h"
#include "MetalCommandContext.h"
#include "MetalQuery.h"
#include "MetalPipelineState.h"
#include "MetalRayTracing.h"
#include "MetalDeviceContext.h"
#include "RHI/RHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FMetalRHIModule final : public FRHIModule
{
    virtual class FRHI* CreateRHI() override final;
};

class FMetalRHI final : public FRHI
{
public:
    static FMetalRHI* GetRHI() 
    {
        CHECK(GMetalRHI != nullptr);
        return GMetalRHI; 
    }

public:
    FMetalRHI();
    ~FMetalRHI();

public:

    // FRHI Interface
    virtual bool Initialize() override final;

    virtual void RHIBeginFrame() override final { }
    virtual void RHIEndFrame() override final { }

    virtual FRHITexture* RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) override final;
    virtual FRHIViewport* RHICreateViewport(const FRHIViewportInfo& InViewportInfo) override final;
    virtual FRHIQuery* RHICreateQuery(EQueryType InQueryType) override final;
    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo) override final;
    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo) override final;
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInfo& InInfo) override final;
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInfo& InInfo) override final;
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInfo& InInfo) override final;
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInfo& InInfo) override final;
    virtual FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer) override final;
    virtual FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer) override final;
    virtual FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& InInitializer) override final;
    virtual FRHIVertexLayout* RHICreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList) override final;
    virtual FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer) override final;
    virtual FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer) override final;
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer) override final;

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final;
    
    virtual bool RHIGetQueryResult(FRHIQuery* Query, uint64& OutResult) override final
    {
        OutResult = 0;
        return true;
    }

    virtual void RHIEnqueueResourceDeletion(FRHIResource* Resource) override final
    {
        // delete Resource;
    }
    
    virtual IRHICommandContext* RHIObtainCommandContext() override final
    {
        return CommandContext;
    }

    virtual FString RHIGetAdapterName() const override final 
    {
        // TODO: Finish
        return FString(); 
    }

    virtual void* RHIGetAdapter() override final 
    {
        // TODO: Finish
        return nullptr;
    }

    virtual void* RHIGetDevice() override final
    {
        CHECK(DeviceContext != nullptr);
        return reinterpret_cast<void*>(DeviceContext->GetMTLDevice());
    }

    virtual void* RHIGetDirectCommandQueue() override final
    {
        CHECK(DeviceContext != nullptr);
        return reinterpret_cast<void*>(DeviceContext->GetMTLCommandQueue());
    }

    virtual void* RHIGetComputeCommandQueue() override final
    {
        // TODO: Finish
        return nullptr;
    }

    virtual void* RHIGetCopyCommandQueue() override final
    {
        // TODO: Finish
        return nullptr;
    }

public:
    FMetalDeviceContext* GetDeviceContext() const
    {
        return DeviceContext;
    }
    
private:
    FMetalDeviceContext*  DeviceContext;
    FMetalCommandContext* CommandContext;

    static FMetalRHI* GMetalRHI;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
