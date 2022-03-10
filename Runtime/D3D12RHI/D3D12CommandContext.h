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
#include "D3D12Buffer.h"
#include "D3D12Views.h"
#include "D3D12SamplerState.h"
#include "D3D12PipelineState.h"
#include "D3D12TimestampQuery.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class CD3D12CommandContext> CD3D12CommandContextRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SD3D12UploadAllocation

struct SD3D12UploadAllocation
{
    uint8* MappedPtr = nullptr;
    uint64 ResourceOffset = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12GPUResourceUploader

class CD3D12GPUResourceUploader : public CD3D12DeviceObject
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

    bool Initialize();

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

    FORCEINLINE void AddInUseResource(CRHIObject* InResource)
    {
        if (InResource)
        {
            Resources.Emplace(MakeSharedRef<CRHIObject>(InResource));
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
    TArray<TSharedRef<CRHIObject>>     Resources;

    TArray<TComPtr<ID3D12Resource>> NativeResources;
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

        Barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
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
// CD3D12CommandContext

class CD3D12CommandContext : public IRHICommandContext, public CD3D12DeviceObject
{
public:

    static CD3D12CommandContext* CreateContext(CD3D12Device* InDevice);

    virtual void Begin() override final;
    virtual void End() override final;

    virtual void BeginTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index) override final;
    virtual void EndTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index) override final;

    virtual void ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor) override final;
    virtual void ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const SDepthStencil& ClearValue) override final;
    virtual void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor) override final;

    virtual void SetShadingRate(ERHIShadingRate ShadingRate) override final;
    virtual void SetShadingRateImage(CRHITexture2D* ShadingImage) override final;

    // TODO: Implement RenderPasses (For Vulkan)
    virtual void BeginRenderPass() override final;
    virtual void EndRenderPass() override final;

    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) override final;
    virtual void SetScissorRect(float Width, float Height, float x, float y) override final;

    virtual void SetBlendFactor(const SColorF& Color) override final;

    virtual void SetRenderTargets(CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView) override final;

    virtual void SetVertexBuffers(CRHIBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot) override final;
    virtual void SetIndexBuffer(CRHIBuffer* IndexBuffer) override final;

    virtual void SetPrimitiveTopology(ERHIPrimitiveTopology PrimitveTopologyType) override final;

    virtual void SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState) override final;
    virtual void SetComputePipelineState(class CRHIComputePipelineState* PipelineState) override final;

    virtual void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) override final;

    virtual void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) override final;
    virtual void SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex) override final;

    virtual void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) override final;
    virtual void SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex) override final;

    virtual void SetConstantBuffer(CRHIShader* Shader, CRHIBuffer* ConstantBuffer, uint32 ParameterIndex) override final;
    virtual void SetConstantBuffers(CRHIShader* Shader, CRHIBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex) override final;

    virtual void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex) override final;
    virtual void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex) override final;

    virtual void UpdateBuffer(CRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData) override final;
    virtual void UpdateTexture2D(CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) override final;

    virtual void ResolveTexture(CRHITexture* Destination, CRHITexture* Source) override final;

    virtual void CopyBuffer(CRHIBuffer* Destination, CRHIBuffer* Source, const SRHICopyBufferInfo& CopyInfo) override final;
    virtual void CopyTexture(CRHITexture* Destination, CRHITexture* Source) override final;
    virtual void CopyTextureRegion(CRHITexture* Destination, CRHITexture* Source, const SRHICopyTextureInfo& CopyTextureInfo) override final;

    virtual void DestroyResource(class CRHIObject* Resource) override final;
    virtual void DiscardContents(class CRHIResource* Resource) override final;

    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIBuffer* VertexBuffer, CRHIBuffer* IndexBuffer, bool bUpdate) override final;
    virtual void BuildRayTracingScene(CRHIRayTracingScene* RayTracingScene, const SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate) override final;

    /* Sets the resources used by the ray tracing pipeline NOTE: temporary and will soon be refactored */
    virtual void SetRayTracingBindings(
        CRHIRayTracingScene* RayTracingScene,
        CRHIRayTracingPipelineState* PipelineState,
        const SRayTracingShaderResources* GlobalResource,
        const SRayTracingShaderResources* RayGenLocalResources,
        const SRayTracingShaderResources* MissLocalResources,
        const SRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources) override final;

    virtual void GenerateMips(CRHITexture* Texture) override final;

    virtual void TransitionTexture(CRHITexture* Texture, ERHIResourceState BeforeState, ERHIResourceState AfterState) override final;
    virtual void TransitionBuffer(CRHIBuffer* Buffer, ERHIResourceState BeforeState, ERHIResourceState AfterState) override final;

    virtual void UnorderedAccessTextureBarrier(CRHITexture* Texture) override final;
    virtual void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer) override final;

    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final;
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final;
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;

    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;

    virtual void DispatchRays(CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) override final;

    virtual void ClearState() override final;

    virtual void Flush() override final;

    virtual void InsertMarker(const String& Message) override final;

    virtual void BeginExternalCapture() override final;
    virtual void EndExternalCapture() override final;

public:
    void UpdateBuffer(CD3D12Resource* Resource, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData);

    FORCEINLINE CD3D12CommandQueue& GetQueue()
    {
        return CommandQueue;
    }

    FORCEINLINE CD3D12CommandList& GetCommandList()
    {
        return CommandList;
    }

    FORCEINLINE uint32 GetCurrentEpochValue() const
    {
        uint32 MaxValue = NMath::Max<int32>((int32)CmdBatches.Size() - 1, 0);
        return NMath::Min<uint32>(NextCmdBatch - 1, MaxValue);
    }

    FORCEINLINE void UnorderedAccessBarrier(CD3D12Resource* Resource)
    {
        D3D12_ERROR(Resource != nullptr, "UnorderedAccessBarrier cannot be called with a nullptr resource");
        BarrierBatcher.AddUnorderedAccessBarrier(Resource->GetD3D12Resource());
    }

    FORCEINLINE void TransitionResource(CD3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
    {
        D3D12_ERROR(Resource != nullptr                              , "TransitionResource cannot be called with a nullptr resource");
        D3D12_ERROR(Resource->GetHeapType() != D3D12_HEAP_TYPE_UPLOAD, "Resources from Upload-heap cannot be transitioned");

        BarrierBatcher.AddTransitionBarrier(Resource->GetD3D12Resource(), BeforeState, AfterState);
    }

    FORCEINLINE void FlushResourceBarriers()
    {
        BarrierBatcher.FlushBarriers(CommandList);
    }

    FORCEINLINE void DestroyResource(CD3D12Resource* Resource)
    {
        CmdBatch->AddInUseResource(Resource);
    }

private:

    CD3D12CommandContext(CD3D12Device* InDevice);
    ~CD3D12CommandContext();

    bool Initialize();

    void InternalClearState();

    CD3D12CommandList  CommandList;
    CD3D12Fence        Fence;
    CD3D12CommandQueue CommandQueue;

    uint64 FenceValue   = 0;
    uint32 NextCmdBatch = 0;

    TArray<CD3D12CommandBatch> CmdBatches;
    CD3D12CommandBatch* CmdBatch = nullptr;

    TArray<TSharedRef<CD3D12TimestampQuery>> ResolveProfilers;

    TSharedRef<CD3D12RootSignature>         CurrentRootSignature;
    TSharedRef<CD3D12GraphicsPipelineState> CurrentGraphicsPipelineState;
    TSharedRef<CD3D12ComputePipelineState>  CurrentComputePipelineState;

    D3D12_PRIMITIVE_TOPOLOGY CurrentPrimitiveTolpology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    CD3D12ShaderConstantsCache   ShaderConstantsCache;
    CD3D12DescriptorCache        DescriptorCache;
    CD3D12ResourceBarrierBatcher BarrierBatcher;

    bool bIsReady = false;
    bool bIsCapturing = false;
};