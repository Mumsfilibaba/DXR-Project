#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Queue.h"
#include "RHI/RHI.h"
#include "VulkanRHI/VulkanInstance.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanDeletionQueue.h"

struct VULKANRHI_API FVulkanRHIModule final : public FRHIModule
{
    virtual FRHI* CreateRHI() override final;
};

class VULKANRHI_API FVulkanRHI : public FRHI
{
public:
    static FVulkanRHI* Get()
    {
        CHECK(GVulkanRHI != nullptr);
        return GVulkanRHI;
    }

    template<typename... ArgTypes>
    static void DeferDeletion(ArgTypes&&... Args)
    {
        Get()->DeferDeletionInternal(Forward<ArgTypes>(Args)...);
    }

public:
    FVulkanRHI();
    ~FVulkanRHI();

    bool Initialize();

    // FRHI Interface
    virtual void BeginFrame() override final;
    virtual void EndFrame() override final;

    virtual FRHITexture* CreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer* CreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState* CreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) override final;
    virtual FRHISwapChain* CreateSwapChain(const FRHISwapChainInfo& InSwapChainInfo) override final;
    virtual FRHIQuery* CreateQuery(EQueryType InQueryType) override final;
    virtual FRHIGpuFence* CreateFence() override final;
    virtual FRHIRayTracingScene* CreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo) override final;
    virtual FRHIRayTracingGeometry* CreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo) override final;
    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHIShaderResourceViewInfo& InInfo) override final;
    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(const FRHIUnorderedAccessViewInfo& InInfo) override final;
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
    virtual FRHIDepthStencilState* CreateDepthStencilState(const FRHIDepthStencilStateInfo& InInfo) override final;
    virtual FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateInfo& InInfo) override final;
    virtual FRHIBlendState* CreateBlendState(const FRHIBlendStateInfo& InInfo) override final;
    virtual FRHIInputLayout* CreateInputLayout(const TArray<FRHIInputElementInfo>& InInputElements) override final;
    virtual FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInfo& InInfo) override final;
    virtual FRHIComputePipelineState* CreateComputePipelineState(const FRHIComputePipelineStateInfo& InInfo) override final;
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

    void ProcessPendingCommandSubmissions();
    void SubmitCommands(FVulkanCommandSubmission* CommandSubmission, bool bFlushDeletionQueue);

    FVulkanInstance* GetInstance()
    {
        return &Instance;
    }

    FVulkanPhysicalDevice* GetPhysicalDevice() const
    {
        return PhysicalDevice;
    }

    FVulkanDevice* GetDevice() const
    {
        return Device;
    }

    FVulkanCommandContext* ObtainVulkanCommandContext()
    {
        return GraphicsCommandContext;
    }

private:
    template<typename... ArgTypes>
    void DeferDeletionInternal(ArgTypes&&... Args)
    {
        TScopedLock Lock(DeletionQueueCS);
        DeletionQueue.Emplace(Forward<ArgTypes>(Args)...);
    }

    typedef TQueue<FVulkanCommandSubmission*, EQueueType::MPSC>         FCommandSubmissionQueue;
    typedef TMap<FRHISamplerStateInfo, TSharedRef<FVulkanSamplerState>> FSamplerStateMap;

    FVulkanInstance               Instance;
    FVulkanPhysicalDevice*        PhysicalDevice;
    FVulkanDevice*                Device;
    FVulkanQueue*                 GraphicsQueue;
    FVulkanCommandContext*        GraphicsCommandContext;
    TArray<FVulkanDeferredObject> DeletionQueue;
    FCriticalSection              DeletionQueueCS;
    FSamplerStateMap              SamplerStateMap;
    FCriticalSection              SamplerStateMapCS;
    FCommandSubmissionQueue       PendingSubmissions;

    static FVulkanRHI* GVulkanRHI;
};
