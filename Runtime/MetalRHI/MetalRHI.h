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
    virtual FRHIViewport* CreateViewport(const FRHIViewportInfo& InViewportInfo) override final;
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
    virtual FRHIDepthStencilState* CreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer) override final;
    virtual FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer) override final;
    virtual FRHIBlendState* CreateBlendState(const FRHIBlendStateInitializer& InInitializer) override final;
    virtual FRHIVertexLayout* CreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList) override final;
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
