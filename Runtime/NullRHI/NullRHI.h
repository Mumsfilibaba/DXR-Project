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

    virtual FRHITexture* CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final
    {
        FNullRHITexture* Texture = new FNullRHITexture(InTextureDesc);
        return Texture;
    }

    virtual FRHIBuffer* CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData) override final
    {
        FNullRHIBuffer* Buffer = new FNullRHIBuffer(InBufferDesc);
        return Buffer;
    }

    virtual FRHISamplerState* CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc) override final
    {
        return new FNullRHISamplerState(InSamplerDesc);
    }

    virtual class FRHISwapChain* CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc) override final
    {
        return new FNullRHISwapChain(InSwapChainDesc);
    }

    virtual FRHISceneAccelerationStructure* CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc) override final
    {
        return new FNullRHIRayTracingScene(InSceneDesc);
    }

    virtual FRHIGeometryAccelerationStructure* CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc) override final
    {
        return new FNullRHIRayTracingGeometry(InGeometryDesc);
    }

    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHIShaderResourceViewDesc& InDesc)
    {
        if (InDesc.IsBufferSRV())
        {
            return new FNullRHIShaderResourceView(InDesc.BufferSRV.Buffer);
        }
        else if (InDesc.IsTextureSRV())
        {
            return new FNullRHIShaderResourceView(InDesc.TextureSRV.Texture);
        }
        else
        {
            return nullptr;
        }
    }

    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(const FRHIUnorderedAccessViewDesc& InDesc)
    {
        if (InDesc.IsBufferUAV())
        {
            return new FNullRHIUnorderedAccessView(InDesc.BufferUAV.Buffer);
        }
        else if (InDesc.IsTextureUAV())
        {
            return new FNullRHIUnorderedAccessView(InDesc.TextureUAV.Texture);
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

    virtual class FRHIDepthStencilState* CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) override final
    {
        return new FNullRHIDepthStencilState(InDesc);
    }

    virtual class FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) override final
    {
        return new FNullRHIRasterizerState(InDesc);
    }

    virtual class FRHIBlendState* CreateBlendState(const FRHIBlendStateDesc& InDesc) override final
    {
        return new FNullRHIBlendState(InDesc);
    }

    virtual class FRHIInputLayout* CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements) override final
    {
        return new FNullRHIInputLayout(InInputElements);
    }

    virtual class FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) override final
    {
        return new FNullRHIGraphicsPipelineState();
    }

    virtual class FRHIComputePipelineState* CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) override final
    {
        return new FNullRHIComputePipelineState();
    }

    virtual class FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final
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

    virtual FRHIFence* CreateFence() override final
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
