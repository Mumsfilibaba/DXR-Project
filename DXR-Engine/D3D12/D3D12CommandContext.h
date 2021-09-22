#pragma once
#include "RenderLayer/ICommandContext.h"

#include "Core/Containers/SharedRef.h"

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
#include "D3D12GPUProfiler.h"

struct D3D12UploadAllocation
{
    uint8* MappedPtr = nullptr;
    uint64 ResourceOffset = 0;
};

class D3D12GPUResourceUploader : public D3D12DeviceChild
{
public:
    D3D12GPUResourceUploader( D3D12Device* InDevice );
    ~D3D12GPUResourceUploader() = default;

    bool Reserve( uint32 InSizeInBytes );

    void Reset();

    D3D12UploadAllocation LinearAllocate( uint32 SizeInBytes );

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

class D3D12CommandBatch
{
public:
    D3D12CommandBatch( D3D12Device* InDevice );
    ~D3D12CommandBatch() = default;

    bool Init();

    bool Reset()
    {
        if ( CmdAllocator.Reset() )
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

    void AddInUseResource( Resource* InResource )
    {
        if ( InResource )
        {
            Resources.Emplace( MakeSharedRef<Resource>( InResource ) );
        }
    }

    void AddInUseResource( D3D12Resource* InResource )
    {
        if ( InResource )
        {
            DxResources.Emplace( MakeSharedRef<D3D12Resource>( InResource ) );
        }
    }

    void AddInUseResource( const TComPtr<ID3D12Resource>& InResource )
    {
        if ( InResource )
        {
            NativeResources.Emplace( InResource );
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

    TSharedRef<D3D12OnlineDescriptorHeap> OnlineResourceDescriptorHeap;
    TSharedRef<D3D12OnlineDescriptorHeap> OnlineSamplerDescriptorHeap;

    TSharedRef<D3D12OnlineDescriptorHeap> OnlineRayTracingResourceDescriptorHeap;
    TSharedRef<D3D12OnlineDescriptorHeap> OnlineRayTracingSamplerDescriptorHeap;

    TArray<TSharedRef<D3D12Resource>>     DxResources;
    TArray<TSharedRef<Resource>>          Resources;
    TArray<TComPtr<ID3D12Resource>> NativeResources;
};

class D3D12ResourceBarrierBatcher
{
public:
    D3D12ResourceBarrierBatcher() = default;
    ~D3D12ResourceBarrierBatcher() = default;

    void AddTransitionBarrier( ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState );

    void AddUnorderedAccessBarrier( ID3D12Resource* Resource )
    {
        Assert( Resource != nullptr );

        D3D12_RESOURCE_BARRIER Barrier;
        Memory::Memzero( &Barrier );

        Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        Barrier.UAV.pResource = Resource;

        Barriers.Emplace( Barrier );
    }

    void FlushBarriers( D3D12CommandListHandle& CmdList )
    {
        if ( !Barriers.IsEmpty() )
        {
            CmdList.ResourceBarrier( Barriers.Data(), Barriers.Size() );
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

class D3D12CommandContext : public ICommandContext, public D3D12DeviceChild
{
public:
    D3D12CommandContext( D3D12Device* InDevice );
    ~D3D12CommandContext();

    bool Init();

    D3D12CommandQueueHandle& GetQueue()
    {
        return CmdQueue;
    }

    D3D12CommandListHandle& GetCommandList()
    {
        return CmdList;
    }

    uint32 GetCurrentEpochValue() const
    {
        uint32 MaxValue = NMath::Max<int32>( (int32)CmdBatches.Size() - 1, 0 );
        return NMath::Min<uint32>( NextCmdBatch - 1, MaxValue );
    }

    void UpdateBuffer( D3D12Resource* Resource, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData );

    void UnorderedAccessBarrier( D3D12Resource* Resource )
    {
        BarrierBatcher.AddUnorderedAccessBarrier( Resource->GetResource() );
    }

    void TransitionResource( D3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState )
    {
        BarrierBatcher.AddTransitionBarrier( Resource->GetResource(), BeforeState, AfterState );
    }

    void FlushResourceBarriers()
    {
        BarrierBatcher.FlushBarriers( CmdList );
    }

    void DiscardResource( D3D12Resource* Resource )
    {
        CmdBatch->AddInUseResource( Resource );
    }

public:
    virtual void Begin() override final;
    virtual void End()   override final;

    virtual void BeginTimeStamp( GPUProfiler* Profiler, uint32 Index ) override final;
    virtual void EndTimeStamp( GPUProfiler* Profiler, uint32 Index ) override final;

    virtual void ClearRenderTargetView( RenderTargetView* RenderTargetView, const ColorF& ClearColor ) override final;
    virtual void ClearDepthStencilView( DepthStencilView* DepthStencilView, const DepthStencilF& ClearValue ) override final;
    virtual void ClearUnorderedAccessViewFloat( UnorderedAccessView* UnorderedAccessView, const ColorF& ClearColor ) override final;

    virtual void SetShadingRate( EShadingRate ShadingRate ) override final;
    virtual void SetShadingRateImage( Texture2D* ShadingImage ) override final;

    virtual void BeginRenderPass() override final;
    virtual void EndRenderPass()   override final;

    virtual void SetViewport( float Width, float Height, float MinDepth, float MaxDepth, float x, float y ) override final;
    virtual void SetScissorRect( float Width, float Height, float x, float y ) override final;

    virtual void SetBlendFactor( const ColorF& Color ) override final;

    virtual void SetRenderTargets( RenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, DepthStencilView* DepthStencilView ) override final;

    virtual void SetVertexBuffers( VertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot ) override final;
    virtual void SetIndexBuffer( IndexBuffer* IndexBuffer ) override final;

    virtual void SetPrimitiveTopology( EPrimitiveTopology PrimitveTopologyType ) override final;

    virtual void SetGraphicsPipelineState( class GraphicsPipelineState* PipelineState ) override final;
    virtual void SetComputePipelineState( class ComputePipelineState* PipelineState ) override final;

    virtual void Set32BitShaderConstants( Shader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants ) override final;

    virtual void SetShaderResourceView( Shader* Shader, ShaderResourceView* ShaderResourceView, uint32 ParameterIndex ) override final;
    virtual void SetShaderResourceViews( Shader* Shader, ShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex ) override final;

    virtual void SetUnorderedAccessView( Shader* Shader, UnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex ) override final;
    virtual void SetUnorderedAccessViews( Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex ) override final;

    virtual void SetConstantBuffer( Shader* Shader, ConstantBuffer* ConstantBuffer, uint32 ParameterIndex ) override final;
    virtual void SetConstantBuffers( Shader* Shader, ConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex ) override final;

    virtual void SetSamplerState( Shader* Shader, SamplerState* SamplerState, uint32 ParameterIndex ) override final;
    virtual void SetSamplerStates( Shader* Shader, SamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex ) override final;

    virtual void UpdateBuffer( Buffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData ) override final;
    virtual void UpdateTexture2D( Texture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData ) override final;

    virtual void ResolveTexture( Texture* Destination, Texture* Source ) override final;

    virtual void CopyBuffer( Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo ) override final;
    virtual void CopyTexture( Texture* Destination, Texture* Source ) override final;
    virtual void CopyTextureRegion( Texture* Destination, Texture* Source, const CopyTextureInfo& CopyTextureInfo ) override final;

    virtual void DiscardResource( class Resource* Resource ) override final;

    virtual void BuildRayTracingGeometry( RayTracingGeometry* Geometry, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer, bool Update ) override final;
    virtual void BuildRayTracingScene( RayTracingScene* RayTracingScene, const RayTracingGeometryInstance* Instances, uint32 NumInstances, bool Update ) override final;

    virtual void SetRayTracingBindings(
        RayTracingScene* RayTracingScene,
        RayTracingPipelineState* PipelineState,
        const RayTracingShaderResources* GlobalResource,
        const RayTracingShaderResources* RayGenLocalResources,
        const RayTracingShaderResources* MissLocalResources,
        const RayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources ) override final;

    virtual void GenerateMips( Texture* Texture ) override final;

    virtual void TransitionTexture( Texture* Texture, EResourceState BeforeState, EResourceState AfterState ) override final;
    virtual void TransitionBuffer( Buffer* Buffer, EResourceState BeforeState, EResourceState AfterState ) override final;

    virtual void UnorderedAccessTextureBarrier( Texture* Texture ) override final;
    virtual void UnorderedAccessBufferBarrier( Buffer* Buffer ) override final;

    virtual void Draw( uint32 VertexCount, uint32 StartVertexLocation ) override final;
    virtual void DrawIndexed( uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation ) override final;
    virtual void DrawInstanced( uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation ) override final;

    virtual void DrawIndexedInstanced(
        uint32 IndexCountPerInstance,
        uint32 InstanceCount,
        uint32 StartIndexLocation,
        uint32 BaseVertexLocation,
        uint32 StartInstanceLocation ) override final;

    virtual void Dispatch( uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ ) override final;

    virtual void DispatchRays(
        RayTracingScene* InScene,
        RayTracingPipelineState* InPipelineState,
        uint32 InWidth,
        uint32 InHeight,
        uint32 InDepth ) override final;

    virtual void ClearState() override final;
    virtual void Flush() override final;

    virtual void InsertMarker( const std::string& Message ) override final;

    virtual void BeginExternalCapture() override final;
    virtual void EndExternalCapture()   override final;

private:
    void InternalClearState();

    D3D12CommandListHandle  CmdList;
    D3D12FenceHandle        Fence;
    D3D12CommandQueueHandle CmdQueue;

    uint64 FenceValue = 0;
    uint32 NextCmdBatch = 0;

    TArray<D3D12CommandBatch> CmdBatches;
    D3D12CommandBatch* CmdBatch = nullptr;

    TArray<TSharedRef<D3D12GPUProfiler>> ResolveProfilers;

    TSharedRef<D3D12ComputePipelineState> GenerateMipsTex2D_PSO;
    TSharedRef<D3D12ComputePipelineState> GenerateMipsTexCube_PSO;

    TSharedRef<D3D12GraphicsPipelineState> CurrentGraphicsPipelineState;
    TSharedRef<D3D12ComputePipelineState>  CurrentComputePipelineState;
    TSharedRef<D3D12RootSignature>         CurrentRootSignature;

    D3D12ShaderConstantsCache   ShaderConstantsCache;
    D3D12DescriptorCache        DescriptorCache;
    D3D12ResourceBarrierBatcher BarrierBatcher;

    bool IsReady = false;
    bool IsCapturing = false;
};