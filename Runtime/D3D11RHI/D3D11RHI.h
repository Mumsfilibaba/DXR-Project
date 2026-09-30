#pragma once
#include "RHI/RHI.h"
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"
#include "D3D11RHI/D3D11CommandContext.h"
#include "D3D11RHI/D3D11SamplerState.h"
#include "D3D11RHI/D3D11TypeTraits.h"

class FD3D11Adapter;
class FD3D11Device;

struct D3D11RHI_API FD3D11ModuleRHI final : public FRHIModule
{
    virtual FRHIDevice* CreateDevice() override final;
};

class D3D11RHI_API FD3D11DeviceRHI : public FRHIDevice
{
public:
    static FORCEINLINE FD3D11DeviceRHI* Get()
    {
        CHECK(D3D11DeviceRHI != nullptr);
        return D3D11DeviceRHI;
    }

    template<typename TRHIType>
    static FORCEINLINE typename TAddPointer<typename TD3D11RHIResourceType<TRHIType>::Type>::Type ResourceCast(TRHIType* Resource)
    {
        return static_cast<typename TAddPointer<typename TD3D11RHIResourceType<TRHIType>::Type>::Type>(Resource);
    }

    template<typename TRHIType>
    static FORCEINLINE typename TAddPointer<const typename TD3D11RHIResourceType<TRHIType>::Type>::Type ResourceCast(const TRHIType* Resource)
    {
        return static_cast<typename TAddPointer<const typename TD3D11RHIResourceType<TRHIType>::Type>::Type>(Resource);
    }

public:
    FD3D11DeviceRHI();
    ~FD3D11DeviceRHI();

    bool Initialize();

    // FRHIDevice interface
    virtual void BeginFrame() override final;
    virtual void EndFrame()   override final;

    virtual FRHITexture*               CreateTexture(const FRHITextureDesc& InTextureDesc, ERHIResourceState InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer*                CreateBuffer(const FRHIBufferDesc& InBufferDesc, ERHIResourceState InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState*          CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc) override final;
    virtual FRHISwapChain*             CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc) override final;
    virtual FRHIQuery*                 CreateQuery(EQueryType InQueryType) override final;
    virtual FRHIFence*                 CreateFence() override final;
    virtual FRHIShaderResourceView*    CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView*   CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc) override final;
    virtual FRHIRenderTargetView*      CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc) override final;
    virtual FRHIDepthStencilView*      CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc) override final;
    virtual FRHIComputeShader*         CreateComputeShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIVertexShader*          CreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIHullShader*            CreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDomainShader*          CreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIGeometryShader*        CreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIPixelShader*           CreatePixelShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDepthStencilState*     CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) override final;
    virtual FRHIRasterizerState*       CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) override final;
    virtual FRHIBlendState*            CreateBlendState(const FRHIBlendStateDesc& InDesc) override final;
    virtual FRHIInputLayout*           CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements) override final;
    virtual FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) override final;
    virtual FRHIComputePipelineState*  CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) override final;

    virtual IRHICommandContext* ObtainCommandContext() override final;

    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const override final;
    virtual bool QueryUAVFormatSupport(EFormat Format) const override final;
    virtual bool QuerySupportedSampleCounts(EFormat Format, uint32& OutSampleCounts) const override final;

    virtual bool GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode) override final;
    virtual bool GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode) override final;

    virtual void EnqueueResourceDeletion(FRHIResource* Resource) override final;

    virtual void* GetRHINativeAdapter() override final;
    virtual void* GetRHINativeDevice()  override final;

    virtual String   GetAdapterName() const override final;
    virtual ERHIType GetRHIType()     const override final { return ERHIType::D3D11; }

    FD3D11Adapter* GetAdapter() const
    {
        return Adapter;
    }

    FD3D11Device* GetDevice() const
    {
        return Device;
    }

    FD3D11CommandContext* GetCommandContext() const
    {
        return CommandContext;
    }

    // -------------------------------------------------------------------------------------------
    // FRHIDevice interface that D3D11 can never support
    // -------------------------------------------------------------------------------------------

    virtual FRHIMeshShader*            CreateMeshShader(const TArray<uint8>&) override final { return nullptr; }
    virtual FRHIAmplificationShader*   CreateAmplificationShader(const TArray<uint8>&) override final { return nullptr; }
    virtual FRHIRayGenShader*          CreateRayGenShader(const TArray<uint8>&) override final { return nullptr; }
    virtual FRHIRayAnyHitShader*       CreateRayAnyHitShader(const TArray<uint8>&) override final { return nullptr; }
    virtual FRHIRayClosestHitShader*   CreateRayClosestHitShader(const TArray<uint8>&) override final { return nullptr; }
    virtual FRHIRayMissShader*         CreateRayMissShader(const TArray<uint8>&) override final { return nullptr; }
    virtual FRHIRayIntersectionShader* CreateRayIntersectionShader(const TArray<uint8>&) override final { return nullptr; }
    virtual FRHIRayCallableShader*     CreateRayCallableShader(const TArray<uint8>&) override final { return nullptr; }

    virtual FRHIMeshletPipelineState* CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc&) override final
    {
        return nullptr;
    }

    virtual FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc&) override final
    {
        return nullptr;
    }

    virtual FRHIShaderBindingTable* CreateShaderBindingTable(const FRHIShaderBindingTableDesc&) override final
    {
        return nullptr;
    }

    virtual FRHISceneAccelerationStructure* CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc&) override final
    {
        return nullptr;
    }

    virtual FRHIGeometryAccelerationStructure* CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc&) override final
    {
        return nullptr;
    }

    virtual FRHIOpacityMicromap* CreateOpacityMicromap(const FRHIOpacityMicromapDesc&) override final
    {
        return nullptr;
    }

    virtual FRHIClusterAccelerationStructure* CreateClusterAccelerationStructure(const FRHIClusterAccelerationStructureDesc&) override final
    {
        return nullptr;
    }

    virtual FRHIClusterTemplate* CreateClusterTemplate(const FRHIClusterTemplateDesc&) override final
    {
        return nullptr;
    }

    virtual FRHIPartitionedSceneAccelerationStructure* CreatePartitionedSceneAccelerationStructure(const FRHIRayTracingAccelerationStructurePartitionedSceneInputs&) override final
    {
        return nullptr;
    }

    virtual FRHIUnorderedAccessView* CreateSamplerFeedbackUnorderedAccessView(FRHITexture*, FRHITexture*) override final
    {
        return nullptr;
    }

    virtual FRHIRayTracingShaderIdentifier GetRayTracingShaderIdentifier(FRHIRayTracingPipelineState*, const String&) override final
    {
        return FRHIRayTracingShaderIdentifier();
    }

    virtual void GetRayTracingAccelerationStructureOperationPrebuildInfo(const FRHIRayTracingAccelerationStructureOperationInputs&, FRHIRayTracingAccelerationStructurePrebuildInfo& OutInfo) override final
    {
        OutInfo = FRHIRayTracingAccelerationStructurePrebuildInfo();
    }

    virtual bool IsAccelerationStructureSerializationHeaderValid(const FRHIAccelerationStructureSerializationHeader&) override final
    {
        return false;
    }

    // D3D11 records into the immediate context and exposes no command queues
    virtual void* GetRHINativeDirectCommandQueue()  override final { return nullptr; }
    virtual void* GetRHINativeComputeCommandQueue() override final { return nullptr; }
    virtual void* GetRHINativeCopyCommandQueue()    override final { return nullptr; }

private:
    bool InitializeDeviceFeatureSupport();

    typedef TMap<FRHISamplerStateDesc, FD3D11SamplerStateRHIRef> FSamplerStateMap;

    FD3D11Adapter*        Adapter;
    FD3D11Device*         Device;
    FD3D11CommandContext* CommandContext;
    uint64                FrameNumber;
    FSamplerStateMap      SamplerStateMap;
    FCriticalSection      SamplerStateMapCS;

    static FD3D11DeviceRHI* D3D11DeviceRHI;
};
