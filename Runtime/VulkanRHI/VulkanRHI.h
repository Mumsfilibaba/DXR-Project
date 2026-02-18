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
#include "VulkanRHI/VulkanTypeTraits.h"

struct VULKANRHI_API FVulkanRHIModule final : public FRHIModule
{
    virtual FRHI* CreateRHI() override final;
};

class VULKANRHI_API FVulkanRHI : public FRHI
{
public:

    /**
     * @brief Converts an EResourceAccess state to Vulkan pipeline stage flags, checking runtime device features.
     * @param ResourceState The resource access state to convert
     * @return VkPipelineStageFlags2 containing the appropriate pipeline stages based on enabled device features
     *
     * This function checks runtime device features to build the correct pipeline stage flags.
     */
    static VkPipelineStageFlags2 ResourceStateToPipelineStageFlags(EResourceAccess ResourceState);

    /**
     * @brief Converts an EResourceAccess state to Vulkan access flags, checking runtime device features.
     * @param ResourceState The resource access state to convert
     * @return VkAccessFlags2 containing the appropriate access flags based on enabled device features
     *
     * This function checks runtime device features to return the correct access flags, ensuring they match 
     * the pipeline stage flags returned by ResourceStateToPipelineStageFlags.
     */
    static VkAccessFlags2 ResourceStateToAccessFlags(EResourceAccess ResourceState);

    /**
     * @brief Converts an EResourceAccess state to Vulkan image layout, checking runtime device features.
     * @param ResourceState The resource access state to convert
     * @return VkImageLayout containing the appropriate image layout based on enabled device features
     *
     * This function checks runtime device features to return the correct image layout, ensuring layouts 
     * are only used when the required features are enabled.
     */
    static VkImageLayout ResourceStateToImageLayout(EResourceAccess ResourceState);

    /**
     * @brief Casts an RHI texture to its Vulkan implementation type, handling back buffer logic.
     * @param Texture The RHI texture pointer to cast
     * @return Pointer to the Vulkan texture, or nullptr if Texture is nullptr
     *
     * This function handles special cases for presentable textures (back buffers). For regular textures,
     * it performs a simple cast. For back buffers, it returns the back buffer texture directly.
     */
    static FVulkanTexture* ResourceCast(FRHITexture* Texture);

    /**
     * @brief Casts an RHI texture to its Vulkan implementation type with command context, handling back buffer logic.
     * @param InCommandContext The Vulkan command context used to resolve the current back buffer
     * @param Texture The RHI texture pointer to cast
     * @return Pointer to the Vulkan texture, or nullptr if Texture is nullptr
     *
     * This function handles special cases for presentable textures (back buffers) by resolving
     * to the current back buffer texture using the provided command context. For regular textures,
     * it performs a simple cast.
     */
    static FVulkanTexture* ResourceCast(FVulkanCommandContext* InCommandContext, FRHITexture* Texture);

    /**
     * @brief Casts an RHI resource to its Vulkan implementation type using type traits.
     * @param Resource The RHI resource pointer to cast
     * @return Pointer to the Vulkan implementation type, or nullptr if Resource is nullptr
     *
     * This template function automatically deduces the Vulkan type from the RHI type using TVulkanRHIResourceType.
     * Works for all RHI resource types except Texture (which has special overloads for back buffer handling).
     */
    template<typename TRHIType>
    static FORCEINLINE typename TAddPointer<typename TVulkanRHIResourceType<TRHIType>::Type>::Type ResourceCast(TRHIType* Resource)
    {
        return static_cast<typename TAddPointer<typename TVulkanRHIResourceType<TRHIType>::Type>::Type>(Resource);
    }

    static FORCEINLINE FVulkanRHI* Get()
    {
        CHECK(VulkanRHI != nullptr);
        return VulkanRHI;
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

    template<typename... ArgTypes>
    void DeferDeletion(ArgTypes&&... Args)
    {
        TScopedLock Lock(DeletionQueueCS);
        DeletionQueue.Emplace(Forward<ArgTypes>(Args)...);
    }

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

    static FVulkanRHI* VulkanRHI;
};
