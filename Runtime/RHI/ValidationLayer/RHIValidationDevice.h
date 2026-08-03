#pragma once
#include "Core/Containers/Map.h"
#include "RHI/RHIDevice.h"

class FRHIValidationCommandContext;

class RHI_API FRHIValidationDevice : public FRHIDevice
{
public:
    FRHIValidationDevice(FRHIDevice* InRealRHI);
    ~FRHIValidationDevice();

    virtual void BeginFrame() override final;
    virtual void EndFrame()   override final;

    virtual FRHITexture*                               CreateTexture(const FRHITextureDesc& InTextureDesc, ERHIResourceState InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer*                                CreateBuffer(const FRHIBufferDesc& InBufferDesc, ERHIResourceState InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState*                          CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc) override final;
    virtual FRHISwapChain*                             CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc) override final;
    virtual FRHISceneAccelerationStructure*            CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc) override final;
    virtual FRHIGeometryAccelerationStructure*         CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc) override final;
    virtual FRHIShaderResourceView*                    CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView*                   CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView*                   CreateSamplerFeedbackUnorderedAccessView(FRHITexture* InFeedbackTexture, FRHITexture* InTargetedTexture) override final;
    virtual FRHIRenderTargetView*                      CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc) override final;
    virtual FRHIDepthStencilView*                      CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc) override final;
    virtual FRHIComputeShader*                         CreateComputeShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIVertexShader*                          CreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIHullShader*                            CreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDomainShader*                          CreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIGeometryShader*                        CreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIMeshShader*                            CreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIAmplificationShader*                   CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIPixelShader*                           CreatePixelShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayGenShader*                          CreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayAnyHitShader*                       CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayClosestHitShader*                   CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayMissShader*                         CreateRayMissShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDepthStencilState*                     CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) override final;
    virtual FRHIRasterizerState*                       CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) override final;
    virtual FRHIBlendState*                            CreateBlendState(const FRHIBlendStateDesc& InDesc) override final;
    virtual FRHIInputLayout*                           CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements) override final;
    virtual FRHIGraphicsPipelineState*                 CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) override final;
    virtual FRHIComputePipelineState*                  CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) override final;
    virtual FRHIMeshletPipelineState*                  CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& InDesc) override final;
    virtual FRHIRayTracingPipelineState*               CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final;
    virtual FRHIClusterAccelerationStructure*          CreateClusterAccelerationStructure(const FRHIClusterAccelerationStructureDesc& InDesc) override final;
    virtual FRHIClusterTemplate*                       CreateClusterTemplate(const FRHIClusterTemplateDesc& InDesc) override final;
    virtual FRHIPartitionedSceneAccelerationStructure* CreatePartitionedSceneAccelerationStructure(const FRHIRayTracingAccelerationStructurePartitionedSceneInputs& InInputs) override final;
    virtual FRHIOpacityMicromap*                       CreateOpacityMicromap(const FRHIOpacityMicromapDesc& InDesc) override final;
    virtual FRHIShaderBindingTable*                    CreateShaderBindingTable(const FRHIShaderBindingTableDesc& InDesc) override final;
    virtual FRHIQuery*                                 CreateQuery(EQueryType InQueryType) override final;
    virtual FRHIFence*                                 CreateFence() override final;

    virtual void                           GetRayTracingAccelerationStructureOperationPrebuildInfo(const FRHIRayTracingAccelerationStructureOperationInputs& InInputs, FRHIRayTracingAccelerationStructurePrebuildInfo& OutInfo) override final;
    virtual FRHIRayTracingShaderIdentifier GetRayTracingShaderIdentifier(FRHIRayTracingPipelineState* InPipeline, const String& InExportName) override final;
    virtual bool                           IsAccelerationStructureSerializationHeaderValid(const FRHIAccelerationStructureSerializationHeader& InHeader) override final;

    virtual IRHICommandContext* ObtainCommandContext() override final;

    virtual bool QueryUAVFormatSupport(EFormat Format) const override final;
    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const override final;

    virtual bool GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode) override final;
    virtual bool GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode) override final;

    virtual void EnqueueResourceDeletion(FRHIResource* Resource) override final;

    virtual void* GetRHINativeAdapter()             override final;
    virtual void* GetRHINativeDevice()              override final;
    virtual void* GetRHINativeDirectCommandQueue()  override final;
    virtual void* GetRHINativeComputeCommandQueue() override final;
    virtual void* GetRHINativeCopyCommandQueue()    override final;

    virtual String GetAdapterName() const override final;

    virtual ERHIType GetRHIType() const override final;

private:
    FRHIDevice* Device;
    TMap<IRHICommandContext*, FRHIValidationCommandContext*> RealContextToValidationContextMap;
};
