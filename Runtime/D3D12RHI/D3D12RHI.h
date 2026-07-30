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
#include "D3D12RHI/RayTracing/D3D12RayTracing.h"
#include "D3D12RHI/D3D12TypeTraits.h"

class FD3D12CommandContext;

struct D3D12RHI_API FD3D12ModuleRHI final : public FRHIModule
{
    virtual FRHIDevice* CreateDevice() override final;
};

class D3D12RHI_API FD3D12DeviceRHI : public FRHIDevice
{
public:
    static FORCEINLINE FD3D12DeviceRHI* Get()
    {
        CHECK(GD3D12DeviceRHI != nullptr);
        return GD3D12DeviceRHI;
    }

    template<typename... ArgTypes>
    static void DeferDeletion(ArgTypes&&... Args)
    {
        Get()->DeferDeletionInternal(Forward<ArgTypes>(Args)...);
    }

    // Drains the deferred-deletion queue until empty (flushing RHI deletes between passes).
    static void FlushDeferredDeletions();

    static FD3D12TextureRHI*                   ResourceCast(FRHITexture* Texture);
    static const FD3D12TextureRHI*             ResourceCast(const FRHITexture* Texture);
    
    static FD3D12UnorderedAccessViewRHI*       ResourceCast(FRHIUnorderedAccessView* UnorderedAccessView);
    static const FD3D12UnorderedAccessViewRHI* ResourceCast(const FRHIUnorderedAccessView* UnorderedAccessView);

    static FD3D12RenderTargetViewRHI*          ResourceCast(FRHIRenderTargetView* RenderTargetView);
    static const FD3D12RenderTargetViewRHI*    ResourceCast(const FRHIRenderTargetView* RenderTargetView);

    template<typename TRHIType>
    static FORCEINLINE typename TAddPointer<typename TD3D12RHIResourceType<TRHIType>::Type>::Type ResourceCast(TRHIType* Resource)
    {
        return static_cast<typename TAddPointer<typename TD3D12RHIResourceType<TRHIType>::Type>::Type>(Resource);
    }

    template<typename TRHIType>
    static FORCEINLINE typename TAddPointer<const typename TD3D12RHIResourceType<TRHIType>::Type>::Type ResourceCast(const TRHIType* Resource)
    {
        return static_cast<typename TAddPointer<const typename TD3D12RHIResourceType<TRHIType>::Type>::Type>(Resource);
    }

public:
    FD3D12DeviceRHI();
    ~FD3D12DeviceRHI();

    bool Initialize();

    void BeginFrame(FD3D12CommandContext* InCommandContext);
    void EndFrame(FD3D12CommandContext* InCommandContext);
    
    // FRHIDevice interface
    virtual void BeginFrame() override final;
    virtual void EndFrame()   override final;

    virtual FRHITexture*                               CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer*                                CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState*                          CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc) override final;
    virtual FRHISwapChain*                             CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc) override final;
    virtual FRHIQuery*                                 CreateQuery(EQueryType InQueryType) override final;
    virtual FRHIFence*                                 CreateFence() override final;
    virtual FRHISceneAccelerationStructure*            CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc) override final;
    virtual FRHIGeometryAccelerationStructure*         CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc) override final;
    virtual FRHIOpacityMicromap*                       CreateOpacityMicromap(const FRHIOpacityMicromapDesc& InDesc) override final;
    virtual FRHIShaderResourceView*                    CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView*                   CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc) override final;
    virtual FRHIRenderTargetView*                      CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc) override final;
    virtual FRHIDepthStencilView*                      CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc) override final;
    virtual FRHIComputeShader*                         CreateComputeShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIVertexShader*                          CreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIHullShader*                            CreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDomainShader*                          CreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIGeometryShader*                        CreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIPixelShader*                           CreatePixelShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIMeshShader*                            CreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIAmplificationShader*                   CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
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
    virtual FRHIShaderBindingTable*                    CreateShaderBindingTable(const FRHIShaderBindingTableDesc& InDesc) override final;
    virtual FRHIClusterAccelerationStructure*          CreateClusterAccelerationStructure(const FRHIClusterAccelerationStructureDesc&) override final { return nullptr; }
    virtual FRHIClusterTemplate*                       CreateClusterTemplate(const FRHIClusterTemplateDesc&) override final { return nullptr; }
    virtual FRHIPartitionedSceneAccelerationStructure* CreatePartitionedSceneAccelerationStructure(const FRHIRayTracingAccelerationStructurePartitionedSceneInputs&) override final { return nullptr; }
    
    virtual FRHIRayTracingShaderIdentifier GetRayTracingShaderIdentifier(FRHIRayTracingPipelineState* InPipeline, const String& InExportName) override final;
    virtual void                           GetRayTracingAccelerationStructureOperationPrebuildInfo(const FRHIRayTracingAccelerationStructureOperationInputs&, FRHIRayTracingAccelerationStructurePrebuildInfo& OutInfo) override final { OutInfo = FRHIRayTracingAccelerationStructurePrebuildInfo(); }

    virtual bool IsAccelerationStructureSerializationHeaderValid(const FRHIAccelerationStructureSerializationHeader& InHeader) override final;

    virtual IRHICommandContext* ObtainCommandContext() override final;

    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const override final;
    virtual bool QueryUAVFormatSupport(EFormat Format) const override final;

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

    static FD3D12DeviceRHI* GD3D12DeviceRHI;
};
