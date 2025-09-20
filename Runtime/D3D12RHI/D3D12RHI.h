#pragma once
#include "Core/Platform/CriticalSection.h"
#include "Core/Containers/Map.h"
#include "CoreApplication/Windows/WindowsWindow.h"
#include "RHI/RHI.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12SamplerState.h"
#include "D3D12RHI/D3D12Shader.h"
#include "D3D12RHI/D3D12RayTracing.h"

class FD3D12CommandContext;

struct D3D12RHI_API FD3D12RHIModule final : public FRHIModule
{
    virtual FRHI* CreateRHI() override final;
};

class D3D12RHI_API FD3D12RHI : public FRHI
{
public:
    static FD3D12RHI* Get() 
    {
        CHECK(GD3D12RHI != nullptr);
        return GD3D12RHI; 
    }

public:
    FD3D12RHI();
    ~FD3D12RHI();

    bool Initialize();

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
    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHIBufferSRVInfo& InInfo)  override final;
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

    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryStats) const override final;
    virtual bool QueryUAVFormatSupport(EFormat Format) const override final;
    virtual bool GetQueryResult(FRHIQuery* Query, uint64& OutResult) override final;
    virtual void EnqueueResourceDeletion(FRHIResource* Resource) override final;
    virtual FString GetAdapterName() const override final;

    virtual IRHICommandContext* ObtainCommandContext() override final;

    virtual void* GetNativeAdapter() override final;
    virtual void* GetNativeDevice() override final;
    virtual void* GetNativeDirectCommandQueue() override final;
    virtual void* GetNativeComputeCommandQueue() override final;
    virtual void* GetNativeCopyCommandQueue() override final;

    template<typename... ArgTypes>
    void DeferDeletion(ArgTypes&&... Args)
    {
        TScopedLock Lock(DeletionQueueCS);
        DeletionQueue.Emplace(Forward<ArgTypes>(Args)...);
    }
    
    void ProcessPendingCommands();
    void SubmitCommands(FD3D12CommandPayload* CommandPayload, bool bFlushDeletionQueue);

    FD3D12Adapter* GetAdapter() const
    {
        return Adapter;
    }

    FD3D12Device* GetDevice() const
    {
        return Device;
    }

    FD3D12CommandContext* ObtainD3D12CommandContext()
    {
        return DirectCommandContext;
    }

private:
    typedef TMap<FRHISamplerStateInfo, FD3D12SamplerStateRef> FSamplerStateMap;
    typedef TQueue<FD3D12CommandPayload*, EQueueType::MPSC>   FCommandPayloadQueue;

    FD3D12Adapter*               Adapter;
    FD3D12Device*                Device;
    FD3D12CommandContext*        DirectCommandContext;
    TArray<FD3D12DeferredObject> DeletionQueue;
    FCriticalSection             DeletionQueueCS;
    FCommandPayloadQueue         PendingSubmissions;
    FSamplerStateMap             SamplerStateMap;
    FCriticalSection             SamplerStateMapCS;

    static FD3D12RHI* GD3D12RHI;
};
