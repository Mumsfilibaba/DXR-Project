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
#include "D3D12RHI/D3D12TypeTraits.h"

class FD3D12CommandContext;

struct D3D12RHI_API FD3D12RHIModule final : public FRHIModule
{
    virtual FRHI* CreateRHI() override final;
};

class D3D12RHI_API FD3D12RHI : public FRHI
{
public:
    static FD3D12TextureRHI* ResourceCast(FRHITexture* Texture);

    template<typename TRHIType>
    static FORCEINLINE typename TAddPointer<typename TD3D12RHIResourceType<TRHIType>::Type>::Type ResourceCast(TRHIType* Resource)
    {
        return static_cast<typename TAddPointer<typename TD3D12RHIResourceType<TRHIType>::Type>::Type>(Resource);
    }

    static FORCEINLINE FD3D12RHI* Get()
    {
        CHECK(GD3D12RHI != nullptr);
        return GD3D12RHI;
    }

    template<typename... ArgTypes>
    static void DeferDeletion(ArgTypes&&... Args)
    {
        Get()->DeferDeletionInternal(Forward<ArgTypes>(Args)...);
    }

public:
    FD3D12RHI();
    ~FD3D12RHI();

    bool Initialize();

    void BeginFrame(FD3D12CommandContext* InCommandContext);
    
    // FRHI Interface
    virtual void BeginFrame() override final;
    virtual void EndFrame() override final;

    virtual FRHITexture* CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer* CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState* CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc) override final;
    virtual FRHISwapChain* CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc) override final;
    virtual FRHIQuery* CreateQuery(EQueryType InQueryType) override final;
    virtual FRHIFence* CreateFence() override final;
    virtual FRHISceneAccelerationStructure* CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc) override final;
    virtual FRHIGeometryAccelerationStructure* CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc) override final;
    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHIShaderResourceViewDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(const FRHIUnorderedAccessViewDesc& InDesc) override final;
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
    virtual FRHIDepthStencilState* CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) override final;
    virtual FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) override final;
    virtual FRHIBlendState* CreateBlendState(const FRHIBlendStateDesc& InDesc) override final;
    virtual FRHIInputLayout* CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements) override final;
    virtual FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) override final;
    virtual FRHIComputePipelineState* CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) override final;
    virtual FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final;

    virtual IRHICommandContext* ObtainCommandContext() override final;

    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const override final;
    virtual bool QueryUAVFormatSupport(EFormat Format) const override final;

    virtual bool GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode) override final;
    virtual bool GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode) override final;

    virtual void EnqueueResourceDeletion(FRHIResource* Resource) override final;
    
    virtual void* GetNativeAdapter() override final;
    virtual void* GetNativeDevice() override final;
    virtual void* GetNativeDirectCommandQueue() override final;
    virtual void* GetNativeComputeCommandQueue() override final;
    virtual void* GetNativeCopyCommandQueue() override final;
    
    virtual FString GetAdapterName() const override final;
    
    void FlushDeletionQueue(FD3D12Commands* Commands);
    void FlushCompletedSubmissions();

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
    bool InitializeDeviceFeatureSupport();
    void TickCoreProgression();

    template<typename... ArgTypes>
    void DeferDeletionInternal(ArgTypes&&... Args)
    {
        TScopedLock Lock(DeferredObjectsCS);
        DeferredObjects.Emplace(Forward<ArgTypes>(Args)...);
    }
    
    typedef TMap<FRHISamplerStateDesc, FD3D12SamplerStateRHIRef> FSamplerStateMap;

    FD3D12Adapter*               Adapter;
    FD3D12Device*                Device;
    FD3D12CommandContext*        DirectCommandContext;
    TArray<FD3D12DeferredObject> DeferredObjects;
    FCriticalSection             DeferredObjectsCS;
    FSamplerStateMap             SamplerStateMap;
    FCriticalSection             SamplerStateMapCS;

    static FD3D12RHI* GD3D12RHI;
};
