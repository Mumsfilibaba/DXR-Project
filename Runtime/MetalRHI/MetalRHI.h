#pragma once
#include "RHI/RHI.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/MetalSamplerState.h"
#include "MetalRHI/MetalSwapChain.h"
#include "MetalRHI/MetalShader.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalQuery.h"
#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalRayTracing.h"
#include "MetalRHI/MetalDeviceContext.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FMetalRHIModule final : public FRHIModule
{
    virtual class FRHI* CreateRHI() override final;
};

class FMetalRHI final : public FRHI
{
public:
    static FMetalRHI* Get() 
    {
        CHECK(GMetalRHI != nullptr);
        return GMetalRHI; 
    }

public:
    FMetalRHI();
    ~FMetalRHI();

    bool Initialize();

public:

    // FRHI Interface
    virtual void BeginFrame() override final { }
    virtual void EndFrame() override final { }

    virtual FRHITexture* CreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer* CreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState* CreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) override final;
    virtual FRHISwapChain* CreateSwapChain(const FRHISwapChainInfo& InSwapChainInfo) override final;
    virtual FRHIQuery* CreateQuery(EQueryType InQueryType) override final;
    virtual FRHIRayTracingScene* CreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo) override final;
    virtual FRHIRayTracingGeometry* CreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo) override final;
    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHITextureSRVInfo& InInfo) override final;
    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHIBufferSRVInfo& InInfo) override final;
    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(const FRHITextureUAVInfo& InInfo) override final;
    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(const FRHIBufferUAVInfo& InInfo) override final;
    virtual FRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIHullShader* CreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIPixelShader* CreatePixelShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayAnyHitShader* CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayMissShader* CreateRayMissShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDepthStencilState* CreateDepthStencilState(const FRHIDepthStencilStateInfo& InInfo) override final;
    virtual FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateInfo& InInfo) override final;
    virtual FRHIBlendState* CreateBlendState(const FRHIBlendStateInfo& InInfo) override final;
    virtual FRHIInputLayout* CreateInputLayout(const TArray<FRHIInputElementInfo>& InInputElements) override final;
    virtual FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer) override final;
    virtual FRHIComputePipelineState* CreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer) override final;
    virtual FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer) override final;

    virtual bool QueryUAVFormatSupport(EFormat Format) const override final;
    
    virtual bool GetQueryResult(FRHIQuery* Query, uint64& OutResult) override final
    {
        OutResult = 0;
        return true;
    }

    virtual void EnqueueResourceDeletion(FRHIResource* Resource) override final
    {
        // delete Resource;
    }
    
    virtual IRHICommandContext* ObtainCommandContext() override final
    {
        return CommandContext;
    }

    virtual FString GetAdapterName() const override final 
    {
        // TODO: Finish
        return FString(); 
    }

    virtual void* GetNativeAdapter() override final 
    {
        // TODO: Finish
        return nullptr;
    }

    virtual void* GetNativeDevice() override final
    {
        CHECK(DeviceContext != nullptr);
        return reinterpret_cast<void*>(DeviceContext->GetMTLDevice());
    }

    virtual void* GetNativeDirectCommandQueue() override final
    {
        CHECK(DeviceContext != nullptr);
        return reinterpret_cast<void*>(DeviceContext->GetMTLCommandQueue());
    }

    virtual void* GetNativeComputeCommandQueue() override final
    {
        // TODO: Finish
        return nullptr;
    }

    virtual void* GetNativeCopyCommandQueue() override final
    {
        // TODO: Finish
        return nullptr;
    }

public:
    FMetalDeviceContext* GetMetalDeviceContext() const
    {
        return DeviceContext;
    }
    
private:
    FMetalDeviceContext*  DeviceContext;
    FMetalCommandContext* CommandContext;

    static FMetalRHI* GMetalRHI;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
