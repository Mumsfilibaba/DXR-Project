#pragma once
#include "D3D12Device.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandContext.h"
#include "D3D12Texture.h"
#include "D3D12SamplerState.h"
#include "D3D12Shader.h"
#include "D3D12RayTracing.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Containers/Map.h"
#include "CoreApplication/Windows/WindowsWindow.h"
#include "RHI/RHI.h"

class FD3D12CommandContext;

struct D3D12RHI_API FD3D12RHIModule final : public FRHIModule
{
    virtual class FRHI* CreateRHI() override final;
};

class D3D12RHI_API FD3D12RHI : public FRHI
{
public:
    static FD3D12RHI* GetRHI() 
    {
        CHECK(GD3D12RHI != nullptr);
        return GD3D12RHI; 
    }

    FD3D12RHI();
    ~FD3D12RHI();

    virtual bool Initialize() override final;

    virtual void RHIBeginFrame() override final { }
    virtual void RHIEndFrame() override final { }

    virtual FRHITexture* RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) override final;
    virtual FRHIViewport* RHICreateViewport(const FRHIViewportInfo& InViewportInfo) override final;
    virtual FRHIQuery* RHICreateQuery(EQueryType InQueryType) override final;
    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& InDesc) override final;
    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc) override final;
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVDesc& InDesc) override final;
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVDesc& InDesc)  override final;
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& InDesc) override final;
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
    virtual bool RHIGetQueryResult(FRHIQuery* Query, uint64& OutResult) override final;
    virtual void RHIEnqueueResourceDeletion(FRHIResource* Resource) override final;
    virtual FString RHIGetAdapterName() const override final;

    virtual IRHICommandContext* RHIObtainCommandContext() override final;
    virtual void* RHIGetAdapter() override final;
    virtual void* RHIGetDevice() override final;
    virtual void* RHIGetDirectCommandQueue() override final;
    virtual void* RHIGetComputeCommandQueue() override final;
    virtual void* RHIGetCopyCommandQueue() override final;

    template<typename... ArgTypes>
    void DeferDeletion(ArgTypes&&... Args)
    {
        TScopedLock Lock(DeletionQueueCS);
        DeletionQueue.Emplace(Forward<ArgTypes>(Args)...);
    }
    
    void ProcessPendingCommands();
    void SubmitCommands(FD3D12CommandPayload* CommandPayload, bool bFlushDeletionQueue);

    FD3D12ComputePipelineStateRef GetGenerateMipsPipelineTexure2D()   const { return GenerateMipsTex2D_PSO; }
    FD3D12ComputePipelineStateRef GetGenerateMipsPipelineTexureCube() const { return GenerateMipsTexCube_PSO; }
    
    FD3D12Adapter* GetAdapter() const
    {
        return Adapter;
    }

    FD3D12Device* GetDevice() const
    {
        return Device;
    }

    FD3D12CommandContext* ObtainCommandContext()
    {
        return DirectCommandContext;
    }

private:
    typedef TMap<FRHISamplerStateInfo, FD3D12SamplerStateRef> FSamplerStateMap;
    typedef TQueue<FD3D12CommandPayload*, EQueueType::MPSC>   FCommandPayloadQueue;

    FD3D12Adapter*                Adapter;
    FD3D12Device*                 Device;
    FD3D12CommandContext*         DirectCommandContext;
    TArray<FD3D12DeferredObject>  DeletionQueue;
    FCriticalSection              DeletionQueueCS;
    FD3D12ComputePipelineStateRef GenerateMipsTex2D_PSO;
    FD3D12ComputePipelineStateRef GenerateMipsTexCube_PSO;
    FCommandPayloadQueue          PendingSubmissions;
    FSamplerStateMap              SamplerStateMap;
    FCriticalSection              SamplerStateMapCS;

    static FD3D12RHI* GD3D12RHI;
};
