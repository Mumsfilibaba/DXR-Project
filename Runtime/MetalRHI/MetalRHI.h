#pragma once
#include "RHI/RHI.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/MetalSamplerState.h"
#include "MetalRHI/MetalSwapChain.h"
#include "MetalRHI/MetalShader.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalQuery.h"
#include "MetalRHI/MetalFence.h"
#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalRayTracing.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalTypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FMetalRHIModule final : public FRHIModule
{
    virtual class FRHI* CreateRHI() override final;
};

class FMetalRHI : public FRHI
{
public:
    static FORCEINLINE FMetalRHI* Get()
    {
        CHECK(GMetalRHI != nullptr);
        return GMetalRHI;
    }

    template<typename TRHIType>
    static FORCEINLINE typename TAddPointer<typename TMetalRHIResourceType<TRHIType>::Type>::Type ResourceCast(TRHIType* Resource)
    {
        return static_cast<typename TAddPointer<typename TMetalRHIResourceType<TRHIType>::Type>::Type>(Resource);
    }

public:
    FMetalRHI();
    ~FMetalRHI();

    bool Initialize();

    // FRHI Interface
    virtual void BeginFrame() override final;
    virtual void EndFrame()   override final;

    virtual FRHITexture*                       CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer*                        CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState*                  CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc) override final;
    virtual FRHISwapChain*                     CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc) override final;
    virtual FRHIQuery*                         CreateQuery(EQueryType InQueryType) override final;
    virtual FRHIFence*                         CreateFence() override final;
    virtual FRHISceneAccelerationStructure*    CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc) override final;
    virtual FRHIGeometryAccelerationStructure* CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc) override final;
    virtual FRHIShaderResourceView*            CreateShaderResourceView(const FRHIShaderResourceViewDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView*           CreateUnorderedAccessView(const FRHIUnorderedAccessViewDesc& InDesc) override final;
    virtual FRHIRenderTargetView*              CreateRenderTargetView(const FRHIRenderTargetViewDesc& InDesc) override final;
    virtual FRHIDepthStencilView*              CreateDepthStencilView(const FRHIDepthStencilViewDesc& InDesc) override final;
    virtual FRHIComputeShader*                 CreateComputeShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIVertexShader*                  CreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIHullShader*                    CreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDomainShader*                  CreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIGeometryShader*                CreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIPixelShader*                   CreatePixelShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIMeshShader*                    CreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIAmplificationShader*           CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayGenShader*                  CreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayAnyHitShader*               CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayClosestHitShader*           CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayMissShader*                 CreateRayMissShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDepthStencilState*             CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) override final;
    virtual FRHIRasterizerState*               CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) override final;
    virtual FRHIBlendState*                    CreateBlendState(const FRHIBlendStateDesc& InDesc) override final;
    virtual FRHIInputLayout*                   CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements) override final;
    virtual FRHIGraphicsPipelineState*         CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) override final;
    virtual FRHIComputePipelineState*          CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) override final;
    virtual FRHIRayTracingPipelineState*       CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final;

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

    virtual FString GetAdapterName() const override final;

    FMetalDevice* GetMetalDevice() const
    {
        return Device;
    }

    FMetalCommandContext* ObtainMetalCommandContext()
    {
        return CommandContext;
    }

private:
    FMetalDevice*         Device;
    FMetalCommandContext* CommandContext;

    static FMetalRHI* GMetalRHI;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
