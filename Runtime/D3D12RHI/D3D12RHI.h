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

    /**
     * @brief Casts an RHI texture to its D3D12 implementation type, handling back buffer logic.
     * @param Texture The RHI texture pointer to cast
     * @return Pointer to the D3D12 texture, or nullptr if Texture is nullptr
     *
     * This function handles special cases for presentable textures (back buffers) by resolving
     * to the current back buffer texture. For regular textures, it performs a simple cast.
     */
    static FD3D12TextureRHI* ResourceCast(FRHITexture* Texture);

    /**
     * @brief Casts an RHI resource to its D3D12 implementation type using type traits.
     * @param Resource The RHI resource pointer to cast
     * @return Pointer to the D3D12 implementation type, or nullptr if Resource is nullptr
     *
     * This template function automatically deduces the D3D12 type from the RHI type using TD3D12RHIResourceType.
     * Works for all RHI resource types except Texture (which has a special overload for back buffer handling).
     */
    template<typename TRHIType>
    static FORCEINLINE typename TAddPointer<typename TD3D12RHIResourceType<TRHIType>::Type>::Type ResourceCast(TRHIType* Resource)
    {
        return static_cast<typename TAddPointer<typename TD3D12RHIResourceType<TRHIType>::Type>::Type>(Resource);
    }

    static FORCEINLINE FD3D12RHI* Get()
    {
        CHECK(D3D12RHI != nullptr);
        return D3D12RHI; 
    }

public:
    FD3D12RHI();
    ~FD3D12RHI();

    bool Initialize();

    // FRHI Interface
    virtual void BeginFrame() override final { }
    virtual void EndFrame() override final { }

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

    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const override final;
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
    void SubmitCommands(FD3D12CommandSubmission* CommandSubmission, bool bFlushDeletionQueue);

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
    
    typedef TMap<FRHISamplerStateDesc, FD3D12SamplerStateRHIRef>  FSamplerStateMap;
    typedef TQueue<FD3D12CommandSubmission*, EQueueType::MPSC> FCommandSubmissionQueue;

    FD3D12Adapter*               Adapter;
    FD3D12Device*                Device;
    FD3D12CommandContext*        DirectCommandContext;
    TArray<FD3D12DeferredObject> DeletionQueue;
    FCriticalSection             DeletionQueueCS;
    FCommandSubmissionQueue      PendingSubmissions;
    FSamplerStateMap             SamplerStateMap;
    FCriticalSection             SamplerStateMapCS;

    static FD3D12RHI* D3D12RHI;
};
