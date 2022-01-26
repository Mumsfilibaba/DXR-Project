#pragma once
#include "RHI/IRHICommandContext.h"

#include "Core/Containers/SharedRef.h"

#include "D3D12DeviceChild.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12DescriptorCache.h"
#include "D3D12RHIBuffer.h"
#include "D3D12RHIViews.h"
#include "D3D12RHISamplerState.h"
#include "D3D12RHIPipelineState.h"
#include "D3D12RHITimestampQuery.h"

struct SD3D12UploadAllocation
{
    uint8* MappedPtr = nullptr;
    uint64 ResourceOffset = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12GPUResourceUploader

class CD3D12GPUResourceUploader : public CD3D12DeviceChild
{
public:
    CD3D12GPUResourceUploader(CD3D12Device* InDevice);
    ~CD3D12GPUResourceUploader() = default;

    bool Reserve(uint32 InSizeInBytes);

    void Reset();

    SD3D12UploadAllocation LinearAllocate(uint32 SizeInBytes);

    FORCEINLINE ID3D12Resource* GetGpuResource() const
    {
        return Resource.Get();
    }

    FORCEINLINE uint32 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

private:
    uint8* MappedMemory = nullptr;

    uint32 SizeInBytes = 0;
    uint32 OffsetInBytes = 0;

    TComPtr<ID3D12Resource> Resource;

    TArray<TComPtr<ID3D12Resource>> GarbageResources;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12CommandBatch

class CD3D12CommandBatch
{
public:
    CD3D12CommandBatch(CD3D12Device* InDevice);
    ~CD3D12CommandBatch() = default;

    bool Init();

    bool Reset()
    {
        if (CmdAllocator.Reset())
        {
            Resources.Clear();
            NativeResources.Clear();
            DxResources.Clear();

            GpuResourceUploader.Reset();

            OnlineResourceDescriptorHeap->Reset();
            OnlineSamplerDescriptorHeap->Reset();

            return true;
        }
        else
        {
            return false;
        }
    }

    FORCEINLINE void AddInUseResource(CRHIResource* InResource)
    {
        if (InResource)
        {
            Resources.Emplace(MakeSharedRef<CRHIResource>(InResource));
        }
    }

    FORCEINLINE void AddInUseResource(CD3D12Resource* InResource)
    {
        if (InResource)
        {
            DxResources.Emplace(MakeSharedRef<CD3D12Resource>(InResource));
        }
    }

    FORCEINLINE void AddInUseResource(const TComPtr<ID3D12Resource>& InResource)
    {
        if (InResource)
        {
            NativeResources.Emplace(InResource);
        }
    }

    FORCEINLINE CD3D12GPUResourceUploader& GetGpuResourceUploader()
    {
        return GpuResourceUploader;
    }

    FORCEINLINE CD3D12CommandAllocator& GetCommandAllocator()
    {
        return CmdAllocator;
    }

    FORCEINLINE CD3D12OnlineDescriptorHeap* GetOnlineResourceDescriptorHeap() const
    {
        return OnlineResourceDescriptorHeap.Get();
    }

    FORCEINLINE CD3D12OnlineDescriptorHeap* GetOnlineSamplerDescriptorHeap() const
    {
        return OnlineSamplerDescriptorHeap.Get();
    }

    CD3D12Device* Device = nullptr;

    CD3D12CommandAllocator    CmdAllocator;
    CD3D12GPUResourceUploader GpuResourceUploader;

    TSharedRef<CD3D12OnlineDescriptorHeap> OnlineResourceDescriptorHeap;
    TSharedRef<CD3D12OnlineDescriptorHeap> OnlineSamplerDescriptorHeap;

    TArray<TSharedRef<CD3D12Resource>> DxResources;
    TArray<TSharedRef<CRHIResource>>  Resources;
    TArray<TComPtr<ID3D12Resource>>   NativeResources;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12ResourceBarrierBatcher

class CD3D12ResourceBarrierBatcher
{
public:
    CD3D12ResourceBarrierBatcher() = default;
    ~CD3D12ResourceBarrierBatcher() = default;

    void AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState);

    void AddUnorderedAccessBarrier(ID3D12Resource* Resource)
    {
        Assert(Resource != nullptr);

        D3D12_RESOURCE_BARRIER Barrier;
        CMemory::Memzero(&Barrier);

        Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        Barrier.UAV.pResource = Resource;

        Barriers.Emplace(Barrier);
    }

    void FlushBarriers(CD3D12CommandList& CmdList)
    {
        if (!Barriers.IsEmpty())
        {
            CmdList.ResourceBarrier(Barriers.Data(), Barriers.Size());
            Barriers.Clear();
        }
    }

    FORCEINLINE const D3D12_RESOURCE_BARRIER* GetBarriers() const
    {
        return Barriers.Data();
    }

    FORCEINLINE uint32 GetNumBarriers() const
    {
        return Barriers.Size();
    }

private:
    TArray<D3D12_RESOURCE_BARRIER> Barriers;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHICommandContext

class CD3D12RHICommandContext : public IRHICommandContext, public CD3D12DeviceChild
{
public:

    /* Create and initialize a new CommandContext */
    static CD3D12RHICommandContext* Make(CD3D12Device* InDevice);

    /* Start recording commands with this context */
    virtual void Begin() override final;
    /* Stop recording commands with this context */
    virtual void End() override final;

    /* Begins the timestamp with the specified index in the timestamp-query */
    virtual void BeginTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index) override final;
    /* Ends the timestamp with the specified index in the timestamp-query */
    virtual void EndTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index) override final;

    /* Clears a RenderTargetView with a specific color */
    virtual void ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor) override final;
    /* Clears a DepthStencilView with a depth and stencil value */
    virtual void ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const SDepthStencil& ClearValue) override final;
    /* Clears a UnorderedAccessView with a specific color */
    virtual void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor) override final;

    /* Sets the ShadingRate for the fullscreen */
    virtual void SetShadingRate(EShadingRate ShadingRate) override final;
    /* Set the ShadingRate image that should be used */
    virtual void SetShadingRateImage(CRHITexture2D* ShadingImage) override final;

    // TODO: Implement RenderPasses (For Vulkan)
    virtual void BeginRenderPass() override final;
    virtual void EndRenderPass() override final;

    /* Set the current viewport settings */
    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) override final;
    /* Set the current scissor settings */
    virtual void SetScissorRect(float Width, float Height, float x, float y) override final;

    /* Set the BlendFactor color */
    virtual void SetBlendFactor(const SColorF& Color) override final;

    /* Sets all the RenderTargetViews and the DepthStencilView that should be used, nullptr is valid if the view should not be set */
    virtual void SetRenderTargets(CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView) override final;

    /* Set the VertexBuffers */
    virtual void SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot) override final;
    /* Set the IndexBuffer */
    virtual void SetIndexBuffer(CRHIIndexBuffer* IndexBuffer) override final;

    /* Set the primitive topology */
    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) override final;

    /* Sets the current graphics PipelineState */
    virtual void SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState) override final;
    /* Sets the current compute PipelineState */
    virtual void SetComputePipelineState(class CRHIComputePipelineState* PipelineState) override final;

    /* Set shader constants */
    virtual void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) override final;

    /* Sets a single ShaderResourceView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) override final;
    /* Sets multiple ShaderResourceViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex) override final;

    /* Sets a single UnorderedAccessView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) override final;
    /* Sets multiple UnorderedAccessViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex) override final;

    /* Sets a single ConstantBuffer to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex) override final;
    /* Sets multiple ConstantBuffers to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex) override final;

    /* Sets a single SamplerState to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex) override final;
    /* Sets multiple SamplerStates to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader, which can be queried from the shader-object */
    virtual void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex) override final;

    /* Updates the contents of a Buffer */
    virtual void UpdateBuffer(CRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) override final;
    /* Updates the contents of a Texture2D */
    virtual void UpdateTexture2D(CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) override final;

    /* Resolves a multi-sampled texture, must have the same sizes and compatible formats */
    virtual void ResolveTexture(CRHITexture* Destination, CRHITexture* Source) override final;

    /* Copies the contents from one buffer to another */
    virtual void CopyBuffer(CRHIBuffer* Destination, CRHIBuffer* Source, const SCopyBufferInfo& CopyInfo) override final;
    /* Copies the entire contents of one texture to another, which require the size and formats to be the same */
    virtual void CopyTexture(CRHITexture* Destination, CRHITexture* Source) override final;
    /* Copies the region of one texture to another */
    virtual void CopyTextureRegion(CRHITexture* Destination, CRHITexture* Source, const SCopyTextureInfo& CopyTextureInfo) override final;

    /* Discards a resource, this can be used to not having to deal with resource life time, the resource will be destroyed when the underlying command list is completed */
    virtual void DestroyResource(class CRHIResource* Resource) override final;
    /* Signal the driver that the contents can be discarded */
    virtual void DiscardResource(class CRHIMemoryResource* Resource) override final;

    /* Builds the Bottom Level Acceleration Structure for ray tracing */
    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool bUpdate) override final;
    /* Builds the Top Level Acceleration Structure for ray tracing */
    virtual void BuildRayTracingScene(CRHIRayTracingScene* RayTracingScene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate) override final;

    /* Sets the resources used by the ray tracing pipeline NOTE: temporary and will soon be refactored */
    virtual void SetRayTracingBindings(
        CRHIRayTracingScene* RayTracingScene,
        CRHIRayTracingPipelineState* PipelineState,
        const SRayTracingShaderResources* GlobalResource,
        const SRayTracingShaderResources* RayGenLocalResources,
        const SRayTracingShaderResources* MissLocalResources,
        const SRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources) override final;

    /* Generate mip-levels for a texture. Works with Texture2D and TextureCubes */
    virtual void GenerateMips(CRHITexture* Texture) override final;

    /* Transition the ResourceState of a texture resource */
    virtual void TransitionTexture(CRHITexture* Texture, EResourceState BeforeState, EResourceState AfterState) override final;
    /* Transition the ResourceState of a buffer resource */
    virtual void TransitionBuffer(CRHIBuffer* Buffer, EResourceState BeforeState, EResourceState AfterState) override final;

    /* Add a UnorderedAccessBarrier for a texture resource, which should be issued before reading of a resource in UnorderedAccessState */
    virtual void UnorderedAccessTextureBarrier(CRHITexture* Texture) override final;
    /* Add a UnorderedAccessBarrier for a buffer resource, which should be issued before reading of a resource in UnorderedAccessState */
    virtual void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer) override final;

    /* Issue a draw-call */
    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final;
    /* Issue a draw-call for drawing with an IndexBuffer */
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final;
    /* Issue a draw-call for drawing instanced */
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final;
    /* Issue a draw-call for drawing instanced with an IndexBuffer */
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;

    /* Issues a compute dispatch */
    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;

    /* Issues a ray generation dispatch */
    virtual void DispatchRays(CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) override final;

    /* Clears the state of the context, clearing all bound references currently bound */
    virtual void ClearState() override final;

    /* Waits for all current execution on the GPU to finish */
    virtual void Flush() override final;

    /* Inserts a marker on the GPU timeline */
    virtual void InsertMarker(const CString& Message) override final;

    /* Begins a PIX capture event, currently only available on D3D12 */
    virtual void BeginExternalCapture() override final;
    /* Ends a PIX capture event, currently only available on D3D12 */
    virtual void EndExternalCapture() override final;

public:
    void UpdateBuffer(CD3D12Resource* Resource, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData);

    FORCEINLINE CD3D12CommandQueue& GetQueue()
    {
        return CmdQueue;
    }

    FORCEINLINE CD3D12CommandList& GetCommandList()
    {
        return CmdList;
    }

    FORCEINLINE uint32 GetCurrentEpochValue() const
    {
        uint32 MaxValue = NMath::Max<int32>((int32)CmdBatches.Size() - 1, 0);
        return NMath::Min<uint32>(NextCmdBatch - 1, MaxValue);
    }

    FORCEINLINE void UnorderedAccessBarrier(CD3D12Resource* Resource)
    {
        D3D12_ERROR(Resource != nullptr, "UnorderedAccessBarrier cannot be called with a nullptr resource");
        BarrierBatcher.AddUnorderedAccessBarrier(Resource->GetResource());
    }

    FORCEINLINE void TransitionResource(CD3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
    {
        D3D12_ERROR(Resource != nullptr, "TransitionResource cannot be called with a nullptr resource");
        BarrierBatcher.AddTransitionBarrier(Resource->GetResource(), BeforeState, AfterState);
    }

    FORCEINLINE void FlushResourceBarriers()
    {
        BarrierBatcher.FlushBarriers(CmdList);
    }

    FORCEINLINE void DestroyResource(CD3D12Resource* Resource)
    {
        CmdBatch->AddInUseResource(Resource);
    }

private:

    CD3D12RHICommandContext(CD3D12Device* InDevice);
    ~CD3D12RHICommandContext();

    bool Init();

    void InternalClearState();

    CD3D12CommandList  CmdList;
    CD3D12Fence        Fence;
    CD3D12CommandQueue CmdQueue;

    uint64 FenceValue = 0;
    uint32 NextCmdBatch = 0;

    TArray<CD3D12CommandBatch> CmdBatches;
    CD3D12CommandBatch* CmdBatch = nullptr;

    TArray<TSharedRef<CD3D12RHITimestampQuery>> ResolveProfilers;

    TSharedRef<CD3D12RHIGraphicsPipelineState> CurrentGraphicsPipelineState;
    TSharedRef<CD3D12RHIComputePipelineState>  CurrentComputePipelineState;

    TSharedRef<CD3D12RootSignature> CurrentRootSignature;

    D3D12_PRIMITIVE_TOPOLOGY CurrentPrimitiveTolpology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    CD3D12ShaderConstantsCache   ShaderConstantsCache;
    CD3D12DescriptorCache        DescriptorCache;
    CD3D12ResourceBarrierBatcher BarrierBatcher;

    bool bIsReady = false;
    bool bIsCapturing = false;
};