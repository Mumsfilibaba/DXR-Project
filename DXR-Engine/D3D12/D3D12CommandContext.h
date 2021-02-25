#pragma once
#include "RenderLayer/ICommandContext.h"

#include "Core/Ref.h"

#include "D3D12DeviceChild.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12Buffer.h"
#include "D3D12Views.h"
#include "D3D12SamplerState.h"
#include "D3D12PipelineState.h"
#include "D3D12DescriptorCache.h"

struct D3D12UploadAllocation
{
    Byte*  MappedPtr      = nullptr;
    UInt64 ResourceOffset = 0;
};

class D3D12GPUResourceUploader : public D3D12DeviceChild
{
public:
    D3D12GPUResourceUploader(D3D12Device* InDevice);
    ~D3D12GPUResourceUploader() = default;

    Bool Reserve(UInt32 InSizeInBytes);
    void Reset();

    D3D12UploadAllocation LinearAllocate(UInt32 SizeInBytes);

    FORCEINLINE ID3D12Resource* GetGpuResource() const
    {
        return Resource.Get();
    }

    FORCEINLINE UInt32 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

private:
    Byte*  MappedMemory  = nullptr;
    UInt32 SizeInBytes   = 0;
    UInt32 OffsetInBytes = 0;
    TComPtr<ID3D12Resource> Resource;
    TArray<TComPtr<ID3D12Resource>> GarbageResources;
};

class D3D12CommandBatch
{
public:
    D3D12CommandBatch(D3D12Device* InDevice);
    ~D3D12CommandBatch() = default;

    Bool Init();

    Bool Reset()
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

    void AddInUseResource(Resource* InResource)
    {
        if (InResource)
        {
            Resources.EmplaceBack(MakeSharedRef<Resource>(InResource));
        }
    }

    void AddInUseResource(D3D12Resource* InResource)
    {
        if (InResource)
        {
            DxResources.EmplaceBack(MakeSharedRef<D3D12Resource>(InResource));
        }
    }

    void AddInUseResource(const TComPtr<ID3D12Resource>& InResource)
    {
        if (InResource)
        {
            NativeResources.EmplaceBack(InResource);
        }
    }

    FORCEINLINE D3D12GPUResourceUploader& GetGpuResourceUploader()
    {
        return GpuResourceUploader;
    }

    FORCEINLINE D3D12CommandAllocatorHandle& GetCommandAllocator()
    {
        return CmdAllocator;
    }

    FORCEINLINE D3D12OnlineDescriptorHeap* GetOnlineResourceDescriptorHeap() const
    {
        return OnlineResourceDescriptorHeap.Get();
    }

    FORCEINLINE D3D12OnlineDescriptorHeap* GetOnlineSamplerDescriptorHeap() const
    {
        return OnlineSamplerDescriptorHeap.Get();
    }

    D3D12Device* Device = nullptr;
    
    D3D12CommandAllocatorHandle CmdAllocator;
    D3D12GPUResourceUploader    GpuResourceUploader;
    
    TRef<D3D12OnlineDescriptorHeap> OnlineResourceDescriptorHeap;
    TRef<D3D12OnlineDescriptorHeap> OnlineSamplerDescriptorHeap;
    
    TRef<D3D12OnlineDescriptorHeap> OnlineRayTracingResourceDescriptorHeap;
    TRef<D3D12OnlineDescriptorHeap> OnlineRayTracingSamplerDescriptorHeap;
    
    TArray<TRef<D3D12Resource>>     DxResources;
    TArray<TRef<Resource>>          Resources;
    TArray<TComPtr<ID3D12Resource>> NativeResources;
};

class D3D12ResourceBarrierBatcher
{
public:
    D3D12ResourceBarrierBatcher()  = default;
    ~D3D12ResourceBarrierBatcher() = default;

    void AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState);

    void AddUnorderedAccessBarrier(ID3D12Resource* Resource)
    {
        Assert(Resource != nullptr);

        D3D12_RESOURCE_BARRIER Barrier;
        Memory::Memzero(&Barrier);

        Barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        Barrier.UAV.pResource = Resource;

        Barriers.EmplaceBack(Barrier);
    }

    void FlushBarriers(D3D12CommandListHandle& CmdList)
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

    FORCEINLINE UInt32 GetNumBarriers() const
    {
        return Barriers.Size();
    }

private:
    TArray<D3D12_RESOURCE_BARRIER> Barriers;
};

class D3D12CommandContext : public ICommandContext, public D3D12DeviceChild
{
public:
    D3D12CommandContext(D3D12Device* InDevice);
    ~D3D12CommandContext();

    Bool Init();

    D3D12CommandQueueHandle& GetQueue()      { return CmdQueue; }
    D3D12CommandListHandle& GetCommandList() { return CmdList; }
    
    void UpdateBuffer(D3D12Resource* Resource, UInt64 OffsetInBytes, UInt64 SizeInBytes, const Void* SourceData);

    void UnorderedAccessBarrier(D3D12Resource* Resource)
    {
        BarrierBatcher.AddUnorderedAccessBarrier(Resource->GetResource());
    }

    void TransitionResource(D3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
    {
        BarrierBatcher.AddTransitionBarrier(Resource->GetResource(), BeforeState, AfterState);
    }

    void FlushResourceBarriers()
    {
        BarrierBatcher.FlushBarriers(CmdList);
    }

    void DiscardResource(D3D12Resource* Resource)
    {
        CmdBatch->AddInUseResource(Resource);
    }

public:
    virtual void Begin() override final;
    virtual void End()   override final;

    virtual void ClearRenderTargetView(RenderTargetView* RenderTargetView, const ColorF& ClearColor) override final;
    virtual void ClearDepthStencilView(DepthStencilView* DepthStencilView, const DepthStencilF& ClearValue) override final;

    // TODO: Use ColorF
    virtual void ClearUnorderedAccessViewFloat(UnorderedAccessView* UnorderedAccessView, const Float ClearColor[4]) override final;

    virtual void SetShadingRate(EShadingRate ShadingRate) override final;
    virtual void SetShadingRateImage(Texture2D* ShadingImage) override final;

    virtual void BeginRenderPass() override final;
    virtual void EndRenderPass()   override final;

    virtual void SetViewport(Float Width, Float Height, Float MinDepth, Float MaxDepth, Float x, Float y) override final;
    virtual void SetScissorRect(Float Width, Float Height, Float x, Float y) override final;

    virtual void SetBlendFactor(const ColorF& Color) override final;

    virtual void SetRenderTargets(RenderTargetView* const * RenderTargetViews, UInt32 RenderTargetCount, DepthStencilView* DepthStencilView) override final;

    virtual void SetVertexBuffers(VertexBuffer* const * VertexBuffers, UInt32 BufferCount, UInt32 BufferSlot) override final;
    virtual void SetIndexBuffer(IndexBuffer* IndexBuffer) override final;

    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) override final;

    virtual void SetGraphicsPipelineState(class GraphicsPipelineState* PipelineState) override final;
    virtual void SetComputePipelineState(class ComputePipelineState* PipelineState) override final;

    virtual void Set32BitShaderConstants(Shader* Shader, const Void* Shader32BitConstants, UInt32 Num32BitConstants) override final;

    virtual void SetShaderResourceView(Shader* Shader, ShaderResourceView* ShaderResourceView, UInt32 ParameterIndex) override final;
    virtual void SetShaderResourceViews(Shader* Shader, ShaderResourceView* const* ShaderResourceView, UInt32 NumShaderResourceViews, UInt32 ParameterIndex) override final;

    virtual void SetUnorderedAccessView(Shader* Shader, UnorderedAccessView* UnorderedAccessView, UInt32 ParameterIndex) override final;
    virtual void SetUnorderedAccessViews(Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, UInt32 NumUnorderedAccessViews, UInt32 ParameterIndex) override final;

    virtual void SetConstantBuffer(Shader* Shader, ConstantBuffer* ConstantBuffer, UInt32 ParameterIndex) override final;
    virtual void SetConstantBuffers(Shader* Shader, ConstantBuffer* const* ConstantBuffers, UInt32 NumConstantBuffers, UInt32 ParameterIndex) override final;

    virtual void SetSamplerState(Shader* Shader, SamplerState* SamplerState, UInt32 ParameterIndex) override final;
    virtual void SetSamplerStates(Shader* Shader, SamplerState* const* SamplerStates, UInt32 NumSamplerStates, UInt32 ParameterIndex) override final;

    virtual void UpdateBuffer(Buffer* Destination, UInt64 OffsetInBytes, UInt64 SizeInBytes, const Void* SourceData) override final;
    virtual void UpdateTexture2D(Texture2D* Destination, UInt32 Width, UInt32 Height, UInt32 MipLevel, const Void* SourceData) override final;

    virtual void ResolveTexture(Texture* Destination, Texture* Source) override final;
    
    virtual void CopyBuffer(Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo) override final;
    virtual void CopyTexture(Texture* Destination, Texture* Source) override final;
    virtual void CopyTextureRegion(Texture* Destination, Texture* Source, const CopyTextureInfo& CopyTextureInfo) override final;

    virtual void DiscardResource(class Resource* Resource) override final;

    virtual void BuildRayTracingGeometry(RayTracingGeometry* Geometry, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer, Bool Update) override final;
    virtual void BuildRayTracingScene(RayTracingScene* RayTracingScene, const RayTracingGeometryInstance* Instances, UInt32 NumInstances, Bool Update) override final;

    virtual void SetRayTracingBindings(
        RayTracingScene* RayTracingScene,
        RayTracingPipelineState* PipelineState,
        const RayTracingShaderResources* GlobalResource,
        const RayTracingShaderResources* RayGenLocalResources,
        const RayTracingShaderResources* MissLocalResources,
        const RayTracingShaderResources* HitGroupResources, UInt32 NumHitGroupResources) override final;

    virtual void GenerateMips(Texture* Texture) override final;

    virtual void TransitionTexture(Texture* Texture, EResourceState BeforeState, EResourceState AfterState) override final;
    virtual void TransitionBuffer(Buffer* Buffer, EResourceState BeforeState, EResourceState AfterState) override final;

    virtual void UnorderedAccessTextureBarrier(Texture* Texture) override final;
    virtual void UnorderedAccessBufferBarrier(Buffer* Buffer) override final;

    virtual void Draw(UInt32 VertexCount, UInt32 StartVertexLocation) override final;
    virtual void DrawIndexed(UInt32 IndexCount, UInt32 StartIndexLocation, UInt32 BaseVertexLocation) override final;
    virtual void DrawInstanced(UInt32 VertexCountPerInstance, UInt32 InstanceCount, UInt32 StartVertexLocation, UInt32 StartInstanceLocation) override final;
    
    virtual void DrawIndexedInstanced(
        UInt32 IndexCountPerInstance, 
        UInt32 InstanceCount, 
        UInt32 StartIndexLocation, 
        UInt32 BaseVertexLocation, 
        UInt32 StartInstanceLocation) override final;

    virtual void Dispatch(UInt32 WorkGroupsX, UInt32 WorkGroupsY, UInt32 WorkGroupsZ) override final;
    
    virtual void DispatchRays(
        RayTracingScene* InScene,
        RayTracingPipelineState* InPipelineState,
        UInt32 InWidth,
        UInt32 InHeight,
        UInt32 InDepth) override final;

    virtual void ClearState() override final;
    virtual void Flush() override final;

    virtual void InsertMarker(const std::string& Message) override final;

    virtual void BeginExternalCapture() override final;
    virtual void EndExternalCapture()   override final;

private:
    void InternalClearState();

    D3D12CommandListHandle  CmdList;
    D3D12FenceHandle        Fence;
    D3D12CommandQueueHandle CmdQueue;

    UInt64 FenceValue   = 0;
    UInt32 NextCmdBatch = 0;

    TArray<D3D12CommandBatch> CmdBatches;
    D3D12CommandBatch*        CmdBatch = nullptr;

    TRef<D3D12ComputePipelineState> GenerateMipsTex2D_PSO;
    TRef<D3D12ComputePipelineState> GenerateMipsTexCube_PSO;

    TRef<D3D12GraphicsPipelineState> CurrentGraphicsPipelineState;
    TRef<D3D12ComputePipelineState>  CurrentComputePipelineState;
    TRef<D3D12RootSignature>         CurrentRootSignature;

    D3D12ShaderConstantsCache   ShaderConstantsCache;
    D3D12DescriptorCache        DescriptorCache;
    D3D12ResourceBarrierBatcher BarrierBatcher;

    Bool IsReady     = false;
    Bool IsCapturing = false;
};