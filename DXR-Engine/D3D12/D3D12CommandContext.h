#pragma once
#include "RHICore/IRHICommandContext.h"

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

struct SD3D12UploadAllocation
{
    uint8* MappedPtr = nullptr;
    uint64 ResourceOffset = 0;
};

class CD3D12GPUResourceUploader : public CD3D12DeviceChild
{
public:
    CD3D12GPUResourceUploader( CD3D12Device* InDevice );
    ~CD3D12GPUResourceUploader() = default;

    bool Reserve( uint32 InSizeInBytes );

    void Reset();

    SD3D12UploadAllocation LinearAllocate( uint32 SizeInBytes );

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

class CD3D12CommandBatch
{
public:
    CD3D12CommandBatch( CD3D12Device* InDevice );
    ~CD3D12CommandBatch() = default;

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

    FORCEINLINE void AddInUseResource( CRHIResource* InResource )
    {
        if ( InResource )
        {
            Resources.Emplace( MakeSharedRef<CRHIResource>( InResource ) );
        }
    }

    FORCEINLINE void AddInUseResource( CD3D12Resource* InResource )
    {
        if ( InResource )
        {
            DxResources.Emplace( MakeSharedRef<CD3D12Resource>( InResource ) );
        }
    }

    FORCEINLINE void AddInUseResource( const TComPtr<ID3D12Resource>& InResource )
    {
        if ( InResource )
        {
            NativeResources.Emplace( InResource );
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

    TSharedRef<CD3D12OnlineDescriptorHeap> OnlineRayTracingResourceDescriptorHeap;
    TSharedRef<CD3D12OnlineDescriptorHeap> OnlineRayTracingSamplerDescriptorHeap;

    TArray<TSharedRef<CD3D12Resource>> DxResources;
    TArray<TSharedRef<CRHIResource>>  Resources;
    TArray<TComPtr<ID3D12Resource>>   NativeResources;
};

class CD3D12ResourceBarrierBatcher
{
public:
    CD3D12ResourceBarrierBatcher() = default;
    ~CD3D12ResourceBarrierBatcher() = default;

    void AddTransitionBarrier( ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState );

    void AddUnorderedAccessBarrier( ID3D12Resource* Resource )
    {
        Assert( Resource != nullptr );

        D3D12_RESOURCE_BARRIER Barrier;
        CMemory::Memzero( &Barrier );

        Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        Barrier.UAV.pResource = Resource;

        Barriers.Emplace( Barrier );
    }

    void FlushBarriers( CD3D12CommandList& CmdList )
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

class CD3D12CommandContext : public IRHICommandContext, public CD3D12DeviceChild
{
public:
    CD3D12CommandContext( CD3D12Device* InDevice );
    ~CD3D12CommandContext();

    bool Init();

    virtual void Begin() override final;
    virtual void End()   override final;

    virtual void BeginTimeStamp( CGPUProfiler* Profiler, uint32 Index ) override final;
    virtual void EndTimeStamp( CGPUProfiler* Profiler, uint32 Index ) override final;

    virtual void ClearRenderTargetView( CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor ) override final;
    virtual void ClearDepthStencilView( CRHIDepthStencilView* DepthStencilView, const SDepthStencilF& ClearValue ) override final;
    virtual void ClearUnorderedAccessViewFloat( CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor ) override final;

    virtual void SetShadingRate( EShadingRate ShadingRate ) override final;
    virtual void SetShadingRateImage( CRHITexture2D* ShadingImage ) override final;

    virtual void BeginRenderPass() override final;
    virtual void EndRenderPass() override final;

    virtual void SetViewport( float Width, float Height, float MinDepth, float MaxDepth, float x, float y ) override final;
    virtual void SetScissorRect( float Width, float Height, float x, float y ) override final;

    virtual void SetBlendFactor( const SColorF& Color ) override final;

    virtual void SetRenderTargets( CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView ) override final;

    virtual void SetVertexBuffers( CRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot ) override final;
    virtual void SetIndexBuffer( CRHIIndexBuffer* IndexBuffer ) override final;

    virtual void SetPrimitiveTopology( EPrimitiveTopology PrimitveTopologyType ) override final;

    virtual void SetGraphicsPipelineState( class CRHIGraphicsPipelineState* PipelineState ) override final;
    virtual void SetComputePipelineState( class CRHIComputePipelineState* PipelineState ) override final;

    virtual void Set32BitShaderConstants( CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants ) override final;

    virtual void SetShaderResourceView( CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex ) override final;
    virtual void SetShaderResourceViews( CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex ) override final;

    virtual void SetUnorderedAccessView( CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex ) override final;
    virtual void SetUnorderedAccessViews( CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex ) override final;

    virtual void SetConstantBuffer( CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex ) override final;
    virtual void SetConstantBuffers( CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex ) override final;

    virtual void SetSamplerState( CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex ) override final;
    virtual void SetSamplerStates( CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex ) override final;

    virtual void UpdateBuffer( CRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData ) override final;
    virtual void UpdateTexture2D( CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData ) override final;

    virtual void ResolveTexture( CRHITexture* Destination, CRHITexture* Source ) override final;

    virtual void CopyBuffer( CRHIBuffer* Destination, CRHIBuffer* Source, const SCopyBufferInfo& CopyInfo ) override final;
    virtual void CopyTexture( CRHITexture* Destination, CRHITexture* Source ) override final;
    virtual void CopyTextureRegion( CRHITexture* Destination, CRHITexture* Source, const SCopyTextureInfo& CopyTextureInfo ) override final;

    virtual void DiscardResource( class CRHIResource* Resource ) override final;

    virtual void BuildRayTracingGeometry( CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool Update ) override final;
    virtual void BuildRayTracingScene( CRHIRayTracingScene* RayTracingScene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool Update ) override final;

    virtual void SetRayTracingBindings(
        CRHIRayTracingScene* RayTracingScene,
        CRHIRayTracingPipelineState* PipelineState,
        const SRayTracingShaderResources* GlobalResource,
        const SRayTracingShaderResources* RayGenLocalResources,
        const SRayTracingShaderResources* MissLocalResources,
        const SRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources ) override final;

    virtual void GenerateMips( CRHITexture* Texture ) override final;

    virtual void TransitionTexture( CRHITexture* Texture, EResourceState BeforeState, EResourceState AfterState ) override final;
    virtual void TransitionBuffer( CRHIBuffer* Buffer, EResourceState BeforeState, EResourceState AfterState ) override final;

    virtual void UnorderedAccessTextureBarrier( CRHITexture* Texture ) override final;
    virtual void UnorderedAccessBufferBarrier( CRHIBuffer* Buffer ) override final;

    virtual void Draw( uint32 VertexCount, uint32 StartVertexLocation ) override final;
    virtual void DrawIndexed( uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation ) override final;
    virtual void DrawInstanced( uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation ) override final;
    virtual void DrawIndexedInstanced( uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation ) override final;

    virtual void Dispatch( uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ ) override final;

    virtual void DispatchRays( CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth ) override final;

    virtual void ClearState() override final;
    virtual void Flush() override final;

    virtual void InsertMarker( const CString& Message ) override final;

    virtual void BeginExternalCapture() override final;
    virtual void EndExternalCapture()   override final;

    void UpdateBuffer( CD3D12Resource* Resource, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData );

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
        uint32 MaxValue = NMath::Max<int32>( (int32)CmdBatches.Size() - 1, 0 );
        return NMath::Min<uint32>( NextCmdBatch - 1, MaxValue );
    }

    FORCEINLINE void UnorderedAccessBarrier( CD3D12Resource* Resource )
    {
        BarrierBatcher.AddUnorderedAccessBarrier( Resource->GetResource() );
    }

    FORCEINLINE void TransitionResource( CD3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState )
    {
        BarrierBatcher.AddTransitionBarrier( Resource->GetResource(), BeforeState, AfterState );
    }

    FORCEINLINE void FlushResourceBarriers()
    {
        BarrierBatcher.FlushBarriers( CmdList );
    }

    FORCEINLINE void DiscardResource( CD3D12Resource* Resource )
    {
        CmdBatch->AddInUseResource( Resource );
    }

private:
    void InternalClearState();

    CD3D12CommandList  CmdList;
    D3D12FenceHandle        Fence;
    CD3D12CommandQueue CmdQueue;

    uint64 FenceValue = 0;
    uint32 NextCmdBatch = 0;

    TArray<CD3D12CommandBatch> CmdBatches;
    CD3D12CommandBatch* CmdBatch = nullptr;

    TArray<TSharedRef<CD3D12GPUProfiler>> ResolveProfilers;

    TSharedRef<CD3D12ComputePipelineState> GenerateMipsTex2D_PSO;
    TSharedRef<CD3D12ComputePipelineState> GenerateMipsTexCube_PSO;

    TSharedRef<CD3D12GraphicsPipelineState> CurrentGraphicsPipelineState;
    TSharedRef<CD3D12ComputePipelineState>  CurrentComputePipelineState;
    TSharedRef<CD3D12RootSignature>         CurrentRootSignature;

    D3D12_PRIMITIVE_TOPOLOGY CurrentPrimitiveTolpology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    CD3D12ShaderConstantsCache   ShaderConstantsCache;
    CD3D12DescriptorCache        DescriptorCache;
    CD3D12ResourceBarrierBatcher BarrierBatcher;

    bool IsReady = false;
    bool IsCapturing = false;
};