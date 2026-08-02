#pragma once
#include "NullRHIResources.h"
#include "NullRHIShader.h"
#include "NullRHICommandContext.h"
#include "RHI/RHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct NULLRHI_API FNullModuleRHI final : public FRHIModule
{
    virtual FRHIDevice* CreateDevice() override final;
};

class NULLRHI_API FNullDeviceRHI final : public FRHIDevice
{
public:
    FNullDeviceRHI();
    ~FNullDeviceRHI();

    virtual void BeginFrame() override final { }
    virtual void EndFrame()   override final { }

    virtual FRHITexture* CreateTexture(const FRHITextureDesc& InTextureDesc, ERHIResourceState InInitialState, const IRHITextureData* InInitialData) override final
    {
        return new FNullTextureRHI(InTextureDesc);
    }

    virtual FRHIBuffer* CreateBuffer(const FRHIBufferDesc& InBufferDesc, ERHIResourceState InInitialState, const void* InInitialData) override final
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

    virtual FRHIShaderResourceView* CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc) override final
    {
        return new FNullShaderResourceViewRHI(InResource, InDesc);
    }

    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc) override final
    {
        return new FNullUnorderedAccessViewRHI(InResource, InDesc);
    }

    virtual FRHIRenderTargetView* CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc) override final
    {
        return new FNullRenderTargetViewRHI(InResource, InDesc);
    }

    virtual FRHIDepthStencilView* CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc) override final
    {
        return new FNullDepthStencilViewRHI(InResource, InDesc);
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
        return new FNullHullShaderRHI();
    }

    virtual class FRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullDomainShaderRHI();
    }

    virtual class FRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullGeometryShaderRHI();
    }

    virtual class FRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullMeshShaderRHI();
    }

    virtual class FRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullAmplificationShaderRHI();
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

    virtual class FRHIMeshletPipelineState* CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& InDesc) override final
    {
        return new FNullMeshletPipelineStateRHI();
    }
    
    virtual class FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final
    {
        return new FNullRayTracingPipelineStateRHI();
    }

    virtual FRHIShaderBindingTable* CreateShaderBindingTable(const FRHIShaderBindingTableDesc& InDesc) override final { return new FNullShaderBindingTableRHI(InDesc); }
    virtual FRHIRayTracingShaderIdentifier GetRayTracingShaderIdentifier(FRHIRayTracingPipelineState*, const String&) override final { return FRHIRayTracingShaderIdentifier(); }
    virtual FRHIOpacityMicromap* CreateOpacityMicromap(const FRHIOpacityMicromapDesc&) override final { return nullptr; }
    virtual FRHIClusterAccelerationStructure* CreateClusterAccelerationStructure(const FRHIClusterAccelerationStructureDesc&) override final { return nullptr; }
    virtual FRHIClusterTemplate* CreateClusterTemplate(const FRHIClusterTemplateDesc&) override final { return nullptr; }
    virtual FRHIPartitionedSceneAccelerationStructure* CreatePartitionedSceneAccelerationStructure(const FRHIRayTracingAccelerationStructurePartitionedSceneInputs&) override final { return nullptr; }
    virtual void GetRayTracingAccelerationStructureOperationPrebuildInfo(const FRHIRayTracingAccelerationStructureOperationInputs&, FRHIRayTracingAccelerationStructurePrebuildInfo& OutInfo) override final { OutInfo = FRHIRayTracingAccelerationStructurePrebuildInfo(); }
    virtual bool IsAccelerationStructureSerializationHeaderValid(const FRHIAccelerationStructureSerializationHeader&) override final { return false; }

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
        OutMemoryInfo.MemoryType   = MemoryType;
        OutMemoryInfo.MemoryUsage  = 0;
        OutMemoryInfo.MemoryBudget = (MemoryType == EVideoMemoryType::Local)
            ? uint64(8) * 1024ull * 1024ull * 1024ull   // 8 GiB local "VRAM"
            : uint64(16) * 1024ull * 1024ull * 1024ull; // 16 GiB non-local "system"
        return true;
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

    virtual String GetAdapterName() const override final
    {
        return String("NullRHI Adapter");
    }

    virtual ERHIType GetRHIType() const override final;

private:
    FNullRHICommandContext* CommandContext;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
