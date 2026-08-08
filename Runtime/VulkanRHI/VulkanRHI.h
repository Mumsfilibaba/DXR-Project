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
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanTypeTraits.h"

struct VULKANRHI_API FVulkanModuleRHI final : public FRHIModule
{
    virtual FRHIDevice* CreateDevice() override final;
};

class VULKANRHI_API FVulkanDeviceRHI : public FRHIDevice
{
public:
    static FORCEINLINE FVulkanDeviceRHI* Get()
    {
        CHECK(VulkanDeviceRHI != nullptr);
        return VulkanDeviceRHI;
    }

    template<typename... ArgTypes>
    static void DeferDeletion(ArgTypes&&... Args)
    {
        Get()->DeferDeletionInternal(Forward<ArgTypes>(Args)...);
    }

    // Convert ERHIResourceState to Vulkan access flags
    static VkAccessFlags2KHR ResourceStateToAccessFlags(ERHIResourceState ResourceState);
    
    // Convert ERHIResourceState to Vulkan image layout
    static VkImageLayout ResourceStateToImageLayout(ERHIResourceState ResourceState);
    
    // Convert ERHIResourceState to Vulkan pipeline-stage flags
    static VkPipelineStageFlags2KHR ResourceStateToPipelineStageFlags(ERHIResourceState ResourceState);

    static FVulkanTextureRHI*       ResourceCast(FRHITexture* Texture);
    static const FVulkanTextureRHI* ResourceCast(const FRHITexture* Texture);
    
    static FVulkanUnorderedAccessViewRHI*       ResourceCast(FRHIUnorderedAccessView* UnorderedAccessView);
    static const FVulkanUnorderedAccessViewRHI* ResourceCast(const FRHIUnorderedAccessView* UnorderedAccessView);
    
    static FVulkanRenderTargetViewRHI*       ResourceCast(FRHIRenderTargetView* RenderTargetView);
    static const FVulkanRenderTargetViewRHI* ResourceCast(const FRHIRenderTargetView* RenderTargetView);

    static FVulkanAccelerationStructure*       ResourceCast(FRHIRayTracingAccelerationStructure* AccelerationStructure);
    static const FVulkanAccelerationStructure* ResourceCast(const FRHIRayTracingAccelerationStructure* AccelerationStructure);

    template<typename TRHIType>
    static FORCEINLINE typename TAddPointer<typename TVulkanRHIResourceType<TRHIType>::Type>::Type ResourceCast(TRHIType* Resource)
    {
        return static_cast<typename TAddPointer<typename TVulkanRHIResourceType<TRHIType>::Type>::Type>(Resource);
    }

    template<typename TRHIType>
    static FORCEINLINE typename TAddPointer<const typename TVulkanRHIResourceType<TRHIType>::Type>::Type ResourceCast(const TRHIType* Resource)
    {
        return static_cast<typename TAddPointer<const typename TVulkanRHIResourceType<TRHIType>::Type>::Type>(Resource);
    }

public:
    FVulkanDeviceRHI();
    ~FVulkanDeviceRHI();

    bool Initialize();

    // FRHIDevice interface
    virtual void BeginFrame() override final;
    virtual void EndFrame()   override final;

    virtual FRHITexture*                               CreateTexture(const FRHITextureDesc& InTextureDesc, ERHIResourceState InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer*                                CreateBuffer(const FRHIBufferDesc& InBufferDesc, ERHIResourceState InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState*                          CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc) override final;
    virtual FRHISwapChain*                             CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc) override final;
    virtual FRHIQuery*                                 CreateQuery(EQueryType InQueryType) override final;
    virtual FRHIFence*                                 CreateFence() override final;
    virtual FRHISceneAccelerationStructure*            CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc) override final;
    virtual FRHIGeometryAccelerationStructure*         CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc) override final;
    virtual FRHIShaderResourceView*                    CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView*                   CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView*                   CreateSamplerFeedbackUnorderedAccessView(FRHITexture* InFeedbackTexture, FRHITexture* InTargetedTexture) override final;
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
    virtual FRHIRayIntersectionShader*                 CreateRayIntersectionShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayCallableShader*                     CreateRayCallableShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDepthStencilState*                     CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) override final;
    virtual FRHIRasterizerState*                       CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) override final;
    virtual FRHIBlendState*                            CreateBlendState(const FRHIBlendStateDesc& InDesc) override final;
    virtual FRHIInputLayout*                           CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements) override final;
    virtual FRHIGraphicsPipelineState*                 CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) override final;
    virtual FRHIComputePipelineState*                  CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) override final;
    virtual FRHIMeshletPipelineState*                  CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& InDesc) override final;
    virtual FRHIRayTracingPipelineState*               CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final;
    virtual FRHIShaderBindingTable*                    CreateShaderBindingTable(const FRHIShaderBindingTableDesc& InDesc) override final;
    virtual FRHIOpacityMicromap*                       CreateOpacityMicromap(const FRHIOpacityMicromapDesc& InDesc) override final;
    virtual FRHIClusterAccelerationStructure*          CreateClusterAccelerationStructure(const FRHIClusterAccelerationStructureDesc& InDesc) override final;
    virtual FRHIClusterTemplate*                       CreateClusterTemplate(const FRHIClusterTemplateDesc& InDesc) override final;
    virtual FRHIPartitionedSceneAccelerationStructure* CreatePartitionedSceneAccelerationStructure(const FRHIRayTracingAccelerationStructurePartitionedSceneInputs& InInputs) override final;
    
    virtual void                           GetRayTracingAccelerationStructureOperationPrebuildInfo(const FRHIRayTracingAccelerationStructureOperationInputs& InInputs, FRHIRayTracingAccelerationStructurePrebuildInfo& OutInfo) override final;
    virtual FRHIRayTracingShaderIdentifier GetRayTracingShaderIdentifier(FRHIRayTracingPipelineState* InPipeline, const String& InExportName) override final;
    virtual bool                           IsAccelerationStructureSerializationHeaderValid(const FRHIAccelerationStructureSerializationHeader& InHeader) override final;

    virtual IRHICommandContext* ObtainCommandContext() override final;

    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const override final;
    virtual bool QueryUAVFormatSupport(EFormat Format) const override final;
    virtual bool QuerySupportedSampleCounts(EFormat Format, uint32& OutSampleCounts) const override final;
    
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

    void NotifyCommandBufferOpened();
    void NotifyCommandBufferRetired(FVulkanCommands* Commands);

    void FlushCompletedSubmissions();

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

    FVulkanQueue* GetPresentQueue() const
    {
        return Device ? Device->GetPresentQueue() : nullptr;
    }

#if VULKAN_ENABLE_CRASH_MARKERS
    bool IsCrashMarkersEnabled() const 
    {
        return CrashMarkers != nullptr;
    }

    FVulkanCrashMarkers* GetCrashMarkers()
    {
        return CrashMarkers;
    }
#endif

private:

    template<typename... ArgTypes>
    void DeferDeletionInternal(ArgTypes&&... Args)
    {
        TScopedLock Lock(DeferredObjectsCS);
        DeferredObjects.Emplace(Forward<ArgTypes>(Args)...);
    }

    typedef TMap<FRHISamplerStateDesc, TSharedRef<FVulkanSamplerStateRHI>> FSamplerStateMap;

    FVulkanInstance               Instance;
#if VK_EXT_debug_utils
    VkDebugUtilsMessengerEXT     DebugMessenger;
#endif
    FVulkanPhysicalDevice*        PhysicalDevice;
    FVulkanDevice*                Device;
    FVulkanCommandContext*        GraphicsCommandContext;
    uint64                        FrameNumber;
    TArray<FVulkanDeferredObject> DeferredObjects;
    int32                         NumOpenCommandBuffers;
    FCriticalSection              DeferredObjectsCS;
    FSamplerStateMap              SamplerStateMap;
    FCriticalSection              SamplerStateMapCS;
#if VULKAN_ENABLE_CRASH_MARKERS
    FVulkanCrashMarkers*          CrashMarkers;
#endif

    static FVulkanDeviceRHI* VulkanDeviceRHI;
};
