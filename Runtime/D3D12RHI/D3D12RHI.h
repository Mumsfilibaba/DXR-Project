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
    FD3D12RHI();
    ~FD3D12RHI();
    
    static FD3D12RHI* GetRHI() 
    {
        CHECK(GD3D12RHI != nullptr);
        return GD3D12RHI; 
    }

    virtual bool Initialize() override final;

    virtual void RHIBeginFrame() override final { }
    virtual void RHIEndFrame() override final { }

    virtual FRHITexture* RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) override final;
    virtual FRHIViewport* RHICreateViewport(const FRHIViewportInfo& InViewportInfo) override final;
    virtual FRHIQuery* RHICreateQuery() override final;
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
    virtual FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& InInitializer) override final;
    virtual FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer) override final;
    virtual FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer) override final;
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final;

    virtual IRHICommandContext* RHIObtainCommandContext() override final { return DirectContext; }

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final;

    virtual FString RHIGetAdapterName() const override final 
    { 
        CHECK(Adapter != nullptr);
        return Adapter->GetDescription(); 
    }

    virtual void* RHIGetAdapter() override final 
    {
        CHECK(Adapter != nullptr);
        return reinterpret_cast<void*>(Adapter->GetDXGIAdapter());
    }

    virtual void* RHIGetDevice() override final
    {
        CHECK(Device != nullptr);
        return reinterpret_cast<void*>(Device->GetD3D12Device());
    }

    virtual void* RHIGetDirectCommandQueue() override final
    {
        CHECK(Device != nullptr);
        return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Direct));
    }

    virtual void* RHIGetComputeCommandQueue() override final
    {
        CHECK(Device != nullptr);
        return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Compute));
    }

    virtual void* RHIGetCopyCommandQueue() override final
    {
        CHECK(Device != nullptr);
        return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Copy));
    }

    template<typename... ArgTypes>
    void DeferDeletion(ArgTypes&&... Args)
    {
        TScopedLock Lock(DeletionQueueCS);
        DeletionQueue.Emplace(Forward<ArgTypes>(Args)...);
    }

    void EnqueueResourceDeletion(FRHIResource* Resource);
    
    void ProcessPendingCommands();
    void SubmitCommands(FD3D12CommandPayload* CommandPayload, bool bFlushDeletionQueue);

    FD3D12OfflineDescriptorHeap* GetResourceOfflineDescriptorHeap()     const { return ResourceOfflineDescriptorHeap; }
    FD3D12OfflineDescriptorHeap* GetRenderTargetOfflineDescriptorHeap() const { return RenderTargetOfflineDescriptorHeap; }
    FD3D12OfflineDescriptorHeap* GetDepthStencilOfflineDescriptorHeap() const { return DepthStencilOfflineDescriptorHeap; }
    FD3D12OfflineDescriptorHeap* GetSamplerOfflineDescriptorHeap()      const { return SamplerOfflineDescriptorHeap; }

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
        return DirectContext;
    }

private:
    FD3D12Adapter*        Adapter;
    FD3D12Device*         Device;
    FD3D12CommandContext* DirectContext;

    FD3D12OfflineDescriptorHeap*  ResourceOfflineDescriptorHeap;
    FD3D12OfflineDescriptorHeap*  RenderTargetOfflineDescriptorHeap;
    FD3D12OfflineDescriptorHeap*  DepthStencilOfflineDescriptorHeap;
    FD3D12OfflineDescriptorHeap*  SamplerOfflineDescriptorHeap;

    TArray<FD3D12DeferredObject>  DeletionQueue;
    FCriticalSection              DeletionQueueCS;

    TMap<FRHISamplerStateInfo, FD3D12SamplerStateRef> SamplerStateMap;
    FCriticalSection                                  SamplerStateMapCS;

    FD3D12ComputePipelineStateRef GenerateMipsTex2D_PSO;
    FD3D12ComputePipelineStateRef GenerateMipsTexCube_PSO;

    TQueue<FD3D12CommandPayload*, EQueueType::MPSC> PendingSubmissions;

    static FD3D12RHI* GD3D12RHI;
};
