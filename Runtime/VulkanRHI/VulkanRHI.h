#pragma once
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanCommandContext.h"
#include "VulkanQueue.h"
#include "VulkanDeletionQueue.h"
#include "RHI/RHI.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Queue.h"

struct VULKANRHI_API FVulkanRHIModule final : public FRHIModule
{
    virtual class FRHI* CreateRHI() override final;
};

class VULKANRHI_API FVulkanRHI : public FRHI
{
public:
    FVulkanRHI();
    ~FVulkanRHI();

    static FVulkanRHI* GetRHI() 
    {
        CHECK(GVulkanRHI != nullptr);
        return GVulkanRHI; 
    }

    virtual bool Initialize() override final;

    virtual void RHIBeginFrame() override final;
    virtual void RHIEndFrame() override final;

    virtual FRHITexture* RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) override final;
    virtual FRHIViewport* RHICreateViewport(const FRHIViewportInfo& InViewportInfo) override final;
    virtual FRHIQuery* RHICreateQuery(EQueryType InQueryType) override final;
    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& InDesc) override final;
    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc) override final;
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVDesc& InDesc) override final;
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVDesc& InDesc) override final;
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
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer) override final;

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final;
    virtual bool RHIGetQueryResult(FRHIQuery* Query, uint64& OutResult) override final;
    virtual void RHIEnqueueResourceDeletion(FRHIResource* Resource) override final;
    virtual FString RHIGetAdapterName() const override final;

    virtual IRHICommandContext* RHIObtainCommandContext() override final
    {
        return GraphicsCommandContext;
    }

    virtual void* RHIGetAdapter() override final 
    {
        CHECK(PhysicalDevice != nullptr);
        return reinterpret_cast<void*>(PhysicalDevice->GetVkPhysicalDevice());
    }

    virtual void* RHIGetDevice() override final
    {
        CHECK(Device != nullptr);
        return reinterpret_cast<void*>(Device->GetVkDevice());
    }

    virtual void* RHIGetDirectCommandQueue() override final
    {
        CHECK(GraphicsQueue != nullptr);
        return reinterpret_cast<void*>(GraphicsQueue->GetVkQueue());
    }

    virtual void* RHIGetComputeCommandQueue() override final
    {
        // TODO: Finish
        return nullptr;
    }

    virtual void* RHIGetCopyCommandQueue() override final
    {
        // TODO: Finish
        return nullptr;
    }

    template<typename... ArgTypes>
    void DeferDeletion(ArgTypes&&... Args)
    {
        TScopedLock Lock(DeletionQueueCS);
        DeletionQueue.Emplace(Forward<ArgTypes>(Args)...);
    }

    void ProcessPendingCommands();
    void SubmitCommands(FVulkanCommandPayload* CommandPayload, bool bFlushDeletionQueue);

    FVulkanInstance* GetInstance()
    {
        return &Instance;
    }

    FVulkanPhysicalDevice* GetAdapter() const
    {
        return PhysicalDevice;
    }

    FVulkanDevice* GetDevice() const
    {
        return Device;
    }

    FVulkanCommandContext* ObtainCommandContext()
    {
        return GraphicsCommandContext;
    }

private:
    FVulkanInstance        Instance;
    FVulkanPhysicalDevice* PhysicalDevice;
    FVulkanDevice*         Device;
    FVulkanQueue*          GraphicsQueue;
    FVulkanCommandContext* GraphicsCommandContext;

    TArray<FVulkanDeferredObject> DeletionQueue;
    FCriticalSection              DeletionQueueCS;

    TMap<FRHISamplerStateInfo, TSharedRef<FVulkanSamplerState>> SamplerStateMap;
    FCriticalSection SamplerStateMapCS;

    TQueue<FVulkanCommandPayload*, EQueueType::MPSC> PendingSubmissions;

    static FVulkanRHI* GVulkanRHI;
};
