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
    virtual void EndFrame()   override final { }

    virtual FRHITexture* CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final
    {
        return new FNullTextureRHI(InTextureDesc);
    }

    virtual FRHIBuffer* CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData) override final
    {
        return new FNullBufferRHI(InBufferDesc);
    }

    virtual FRHISamplerState* CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc) override final
    {
        return new FNullSamplerStateRHI(InSamplerDesc);
    }

    virtual class FRHISwapChain* CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc) override final
    {
        return new FNullSwapChainRHI(InSwapChainDesc);
    }

    virtual FRHISceneAccelerationStructure* CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc) override final
    {
        return new FNullRayTracingSceneRHI(InSceneDesc);
    }

    virtual FRHIGeometryAccelerationStructure* CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc) override final
    {
        return new FNullRayTracingGeometryRHI(InGeometryDesc);
    }

    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHIShaderResourceViewDesc& InDesc)
    {
        if (InDesc.IsBufferSRV())
        {
            return new FNullShaderResourceViewRHI(InDesc.BufferSRV.Buffer);
        }
        else if (InDesc.IsTextureSRV())
        {
            return new FNullShaderResourceViewRHI(InDesc.TextureSRV.Texture);
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
            return new FNullUnorderedAccessViewRHI(InDesc.BufferUAV.Buffer);
        }
        else if (InDesc.IsTextureUAV())
        {
            return new FNullUnorderedAccessViewRHI(InDesc.TextureUAV.Texture);
        }
        else
        {
            return nullptr;
        }
    }

    virtual FRHIRenderTargetView* CreateRenderTargetView(const FRHIRenderTargetViewDesc& InDesc) override final
    {
        return new FNullRenderTargetViewRHI(InDesc.Texture);
    }

    virtual FRHIDepthStencilView* CreateDepthStencilView(const FRHIDepthStencilViewDesc& InDesc) override final
    {
        return new FNullDepthStencilViewRHI(InDesc.Texture);
    }

    virtual class FRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullComputeShaderRHI();
    }

    virtual class FRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullVertexShaderRHI();
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
        return new FNullPixelShaderRHI();
    }

    virtual class FRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRayGenShaderRHI();
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
        return new FNullDepthStencilStateRHI(InDesc);
    }

    virtual class FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) override final
    {
        return new FNullRasterizerStateRHI(InDesc);
    }

    virtual class FRHIBlendState* CreateBlendState(const FRHIBlendStateDesc& InDesc) override final
    {
        return new FNullBlendStateRHI(InDesc);
    }

    virtual class FRHIInputLayout* CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements) override final
    {
        return new FNullInputLayoutRHI(InInputElements);
    }

    virtual class FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) override final
    {
        return new FNullGraphicsPipelineStateRHI();
    }
    
    virtual class FRHIComputePipelineState* CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) override final
    {
        return new FNullComputePipelineStateRHI();
    }
    
    virtual class FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final
    {
        return new FNullRayTracingPipelineStateRHI();
    }

    virtual class FRHIQuery* CreateQuery(EQueryType InQueryType) override final
    {
        return new FNullQueryRHI(InQueryType);
    }

    virtual FRHIFence* CreateFence() override final
    {
        return new FNullFenceRHI();
    }

    virtual struct IRHICommandContext* ObtainCommandContext() override final
    {
        return CommandContext;
    }

    virtual bool QueryUAVFormatSupport(EFormat Format) const override final
    {
        return true;
    }

    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const override final
    {
        OutMemoryInfo = {};
        return false;
    }

    virtual bool GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode) override final
    {
        OutResult = 0;
        return true;
    }

    virtual bool GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode) override final
    {
        OutResult = {};
        return true;
    }

    virtual void EnqueueResourceDeletion(FRHIResource* Resource) override final
    {
        delete Resource;
    }

    virtual void* GetRHINativeAdapter() override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeDevice() override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeDirectCommandQueue() override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeComputeCommandQueue() override final
    {
        return nullptr;
    }

    virtual void* GetRHINativeCopyCommandQueue() override final
    {
        return nullptr;
    }

    virtual FString GetAdapterName() const override final
    {
        return FString("NullRHI Adapter");
    }

private:
    FNullRHICommandContext* CommandContext;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
