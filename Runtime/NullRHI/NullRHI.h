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
    FNullRHI()
        : FRHI(ERHIType::Null)
        , CommandContext(new FNullRHICommandContext())
    {
    }

    ~FNullRHI()
    {
        SAFE_DELETE(CommandContext);
    }

    virtual bool Initialize() override final
    { 
        return true; 
    }

    virtual void RHIBeginFrame() override final { }
    virtual void RHIEndFrame() override final { }

    virtual FRHITexture* RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final
    {
        return new FNullRHITexture(InTextureInfo);
    }

    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData) override final
    {
        return new FNullRHIBuffer(InBufferInfo);
    }

    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) override final
    {
        return new FNullRHISamplerState(InSamplerInfo);
    }

    virtual class FRHIViewport* RHICreateViewport(const FRHIViewportInfo& InViewportInfo) override final
    {
        return new FNullRHIViewport(InViewportInfo);
    }

    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo) override final
    {
        return new FNullRHIRayTracingScene(InSceneInfo);
    }

    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo) override final
    {
        return new FNullRHIRayTracingGeometry(InGeometryInfo);
    }

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInfo& InInfo) override final
    {
        return new FNullRHIShaderResourceView(InInfo.Texture);
    }

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInfo& InInfo) override final
    {
        return new FNullRHIShaderResourceView(InInfo.Buffer);
    }

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInfo& InInfo) override final
    {
        return new FNullRHIUnorderedAccessView(InInfo.Texture);
    }

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInfo& InInfo) override final
    {
        return new FNullRHIUnorderedAccessView(InInfo.Buffer);
    }

    virtual class FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIComputeShader();
    }

    virtual class FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIVertexShader();
    }

    virtual class FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIPixelShader();
    }

    virtual class FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIRayGenShader();
    }

    virtual class FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return new TNullRHIShader<FRHIRayAnyHitShader>();
    }

    virtual class FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return new TNullRHIShader<FRHIRayClosestHitShader>();
    }

    virtual class FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final
    {
        return new TNullRHIShader<FRHIRayMissShader>();
    }

    virtual class FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer) override final
    {
        return new FNullRHIDepthStencilState(InInitializer);
    }

    virtual class FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer) override final
    {
        return new FNullRHIRasterizerState(InInitializer);
    }

    virtual class FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& InInitializer) override final
    {
        return new FNullRHIBlendState(InInitializer);
    }

    virtual class FRHIVertexLayout* RHICreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList) override final
    {
        return new FNullRHIVertexLayout(InInitializerList);
    }

    virtual class FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer) override final
    {
        return new FNullRHIGraphicsPipelineState();
    }

    virtual class FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer) override final
    {
        return new FNullRHIComputePipelineState();
    }

    virtual class FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer) override final
    {
        return new FNullRHIRayTracingPipelineState();
    }

    virtual bool RHIGetQueryResult(FRHIQuery* Query, uint64& OutResult) override final
    {
        OutResult = 0;
        return true;
    }

    virtual void RHIEnqueueResourceDeletion(FRHIResource* Resource) override final
    {
        delete Resource;
    }

    virtual class FRHIQuery* RHICreateQuery(EQueryType InQueryType) override final
    {
        return new FNullRHIQuery(InQueryType);
    }

    virtual struct IRHICommandContext* RHIObtainCommandContext() override final
    {
        return CommandContext;
    }

    virtual FString RHIGetAdapterName() const override final
    {
        return FString("NullRHI Adapter");
    }

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final
    {
        return true;
    }

private:
    FNullRHICommandContext* CommandContext;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
