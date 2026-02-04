#pragma once
#include "NullRHIResources.h"
#include "NullRHIShader.h"
#include "NullRHICommandContext.h"
#include "RHI/RHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct NULLRHI_API FNullRHIModule final : public FRHIModule
{
    virtual FRHI* CreateRHI() override final;
};

class NULLRHI_API FNullRHI final : public FRHI
{
public:
    FNullRHI();
    ~FNullRHI();

    virtual void BeginFrame() override final { }
    virtual void EndFrame() override final { }

    virtual FRHITexture* CreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final
    {
        return new FNullRHITexture(InTextureInfo);
    }

    virtual FRHIBuffer* CreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData) override final
    {
        return new FNullRHIBuffer(InBufferInfo);
    }

    virtual FRHISamplerState* CreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) override final
    {
        return new FNullRHISamplerState(InSamplerInfo);
    }

    virtual class FRHISwapChain* CreateSwapChain(const FRHISwapChainInfo& InSwapChainInfo) override final
    {
        return new FNullRHISwapChain(InSwapChainInfo);
    }

    virtual FRHIRayTracingScene* CreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo) override final
    {
        return new FNullRHIRayTracingScene(InSceneInfo);
    }

    virtual FRHIRayTracingGeometry* CreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo) override final
    {
        return new FNullRHIRayTracingGeometry(InGeometryInfo);
    }

    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHIShaderResourceViewInfo& InInfo)
    {
        if (InInfo.IsBufferSRV())
        {
            return new FNullRHIShaderResourceView(InInfo.BufferSRV.Buffer);
        }
        else if (InInfo.IsTextureSRV())
        {
            return new FNullRHIShaderResourceView(InInfo.TextureSRV.Texture);
        }
        else
        {
            return nullptr;
        }
    }

    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(const FRHIUnorderedAccessViewInfo& InInfo)
    {
        if (InInfo.IsBufferUAV())
        {
            return new FNullRHIUnorderedAccessView(InInfo.BufferUAV.Buffer);
        }
        else if (InInfo.IsTextureUAV())
        {
            return new FNullRHIUnorderedAccessView(InInfo.TextureUAV.Texture);
        }
        else
        {
            return nullptr;
        }
    }

    virtual class FRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIComputeShader();
    }

    virtual class FRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIVertexShader();
    }

    virtual class FRHIHullShader* CreateHullShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIPixelShader* CreatePixelShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIPixelShader();
    }

    virtual class FRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIRayGenShader();
    }

    virtual class FRHIRayAnyHitShader* CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return new TNullRHIShader<FRHIRayAnyHitShader>();
    }

    virtual class FRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return new TNullRHIShader<FRHIRayClosestHitShader>();
    }

    virtual class FRHIRayMissShader* CreateRayMissShader(const TArray<uint8>& ShaderCode) override final
    {
        return new TNullRHIShader<FRHIRayMissShader>();
    }

    virtual class FRHIDepthStencilState* CreateDepthStencilState(const FRHIDepthStencilStateInfo& InInfo) override final
    {
        return new FNullRHIDepthStencilState(InInfo);
    }

    virtual class FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateInfo& InInfo) override final
    {
        return new FNullRHIRasterizerState(InInfo);
    }

    virtual class FRHIBlendState* CreateBlendState(const FRHIBlendStateInfo& InInfo) override final
    {
        return new FNullRHIBlendState(InInfo);
    }

    virtual class FRHIInputLayout* CreateInputLayout(const TArray<FRHIInputElementInfo>& InInputElements) override final
    {
        return new FNullRHIInputLayout(InInputElements);
    }

    virtual class FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInfo& InInfo) override final
    {
        return new FNullRHIGraphicsPipelineState();
    }

    virtual class FRHIComputePipelineState* CreateComputePipelineState(const FRHIComputePipelineStateInfo& InInfo) override final
    {
        return new FNullRHIComputePipelineState();
    }

    virtual class FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer) override final
    {
        return new FNullRHIRayTracingPipelineState();
    }

    virtual bool GetQueryResult(FRHIQuery* Query, uint64& OutResult) override final
    {
        OutResult = 0;
        return true;
    }

    virtual void EnqueueResourceDeletion(FRHIResource* Resource) override final
    {
        delete Resource;
    }

    virtual class FRHIQuery* CreateQuery(EQueryType InQueryType) override final
    {
        return new FNullRHIQuery(InQueryType);
    }

    virtual FRHIGpuFence* CreateFence() override final
    {
        return new FNullRHIGpuFence();
    }

    virtual struct IRHICommandContext* ObtainCommandContext() override final
    {
        return CommandContext;
    }

    virtual FString GetAdapterName() const override final
    {
        return FString("NullRHI Adapter");
    }

    virtual bool QueryUAVFormatSupport(EFormat Format) const override final
    {
        return true;
    }

private:
    FNullRHICommandContext* CommandContext;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
