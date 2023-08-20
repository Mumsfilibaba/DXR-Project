#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandList.h"
#include "D3D12CommandAllocator.h"
#include "D3D12Descriptors.h"
#include "D3D12Fence.h"
#include "D3D12DescriptorCache.h"
#include "D3D12Buffer.h"
#include "D3D12ResourceViews.h"
#include "D3D12SamplerState.h"
#include "D3D12PipelineState.h"
#include "D3D12TimestampQuery.h"
#include "D3D12Texture.h"
#include "RHI/IRHICommandContext.h"
#include "Core/Containers/SharedRef.h"

struct FD3D12UploadAllocation
{
    ID3D12Resource* Resource = nullptr;
    uint8* Memory            = nullptr;
    uint64 ResourceOffset    = 0;
};


class FD3D12GPUResourceUploader : public FD3D12DeviceChild
{
public:
    FD3D12GPUResourceUploader(FD3D12Device* InDevice);
    ~FD3D12GPUResourceUploader() = default;

    bool Reserve(uint64 InSizeInBytes);
    void Reset();

    FD3D12UploadAllocation Allocate(uint64 SizeInBytes, uint64 Alignment);

    FORCEINLINE uint64 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

private:
    uint8* MappedMemory  = nullptr;

    uint64 SizeInBytes   = 0;
    uint64 OffsetInBytes = 0;

    TComPtr<ID3D12Resource> Resource;

    TArray<TComPtr<ID3D12Resource>> GarbageResources;
};


class FD3D12CommandBatch
{
public:
    FD3D12CommandBatch(FD3D12Device* InDevice);
    ~FD3D12CommandBatch() = default;

    bool Initialize(uint32 Index);

    bool Reset()
    {
        Resources.Clear();
        NativeResources.Clear();
        DxResources.Clear();

        GpuResourceUploader.Reset();

        OnlineResourceDescriptorHeap->Reset();
        OnlineSamplerDescriptorHeap->Reset();

        return true;
    }

    FORCEINLINE void AddInUseResource(IRefCounted* InResource)
    {
        if (InResource)
        {
            Resources.Emplace(MakeSharedRef<IRefCounted>(InResource));
        }
    }

    FORCEINLINE void AddInUseResource(FD3D12Resource* InResource)
    {
        if (InResource)
        {
            DxResources.Emplace(MakeSharedRef<FD3D12Resource>(InResource));
        }
    }

    FORCEINLINE void AddInUseResource(const TComPtr<ID3D12Resource>& InResource)
    {
        if (InResource)
        {
            NativeResources.Emplace(InResource);
        }
    }

    FORCEINLINE FD3D12GPUResourceUploader& GetGpuResourceUploader()
    {
        return GpuResourceUploader;
    }

    FORCEINLINE FD3D12OnlineDescriptorManager* GetResourceDescriptorManager() const
    {
        return OnlineResourceDescriptorHeap.Get();
    }

    FORCEINLINE FD3D12OnlineDescriptorManager* GetSamplerDescriptorManager() const
    {
        return OnlineSamplerDescriptorHeap.Get();
    }

    FD3D12Device*                    Device = nullptr;
    
    uint64                           AssignedFenceValue = 0;

    FD3D12GPUResourceUploader        GpuResourceUploader;

    FD3D12OnlineDescriptorManagerRef OnlineResourceDescriptorHeap;
    FD3D12OnlineDescriptorManagerRef OnlineSamplerDescriptorHeap;

    TArray<FD3D12ResourceRef>        DxResources;
    TArray<TSharedRef<IRefCounted>>  Resources;

    TArray<TComPtr<ID3D12Resource>>  NativeResources;
};


class FD3D12ResourceBarrierBatcher
{
public:
    FD3D12ResourceBarrierBatcher()
        : Barriers()
    {
    }

    void AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState);
    void AddUnorderedAccessBarrier(ID3D12Resource* Resource);

    void FlushBarriers(FD3D12CommandList& CommandList)
    {
        if (!Barriers.IsEmpty())
        {
            CommandList.ResourceBarrier(Barriers.Data(), Barriers.Size());
            Barriers.Clear();
        }
    }

    FORCEINLINE const D3D12_RESOURCE_BARRIER* GetBarriers() const { return Barriers.Data(); }

    FORCEINLINE uint32 GetNumBarriers() const { return Barriers.Size(); }

private:
    TArray<D3D12_RESOURCE_BARRIER> Barriers;
};


struct FD3D12CommandContextState : public FD3D12DeviceChild, public FNonCopyAndNonMovable
{
    FD3D12CommandContextState(FD3D12Device* InDevice);
    ~FD3D12CommandContextState() = default;

    bool Initialize();

    void ApplyGraphics(FD3D12CommandList& CommandList, FD3D12CommandBatch* Batch);
    void ApplyCompute(FD3D12CommandList& CommandList, FD3D12CommandBatch* Batch);

    void ClearGraphics();
    void ClearCompute();

    void SetVertexBuffer(FD3D12Buffer* VertexBuffer, uint32 Slot);
    void SetIndexBuffer(FD3D12Buffer* IndexBuffer, DXGI_FORMAT IndexFormat);

    void ClearAll()
    {
        ClearGraphics();
        ClearCompute();

        DescriptorCache.Clear();
        ShaderConstantsCache.Reset();

        bIsReady            = false;
        bIsCapturing        = false;
        bIsRenderPassActive = false;
        bBindRootSignature  = true;
    }

    struct
    {
        FD3D12GraphicsPipelineStateRef PipelineState;

        FD3D12TextureRef            ShadingRateTexture;
        D3D12_SHADING_RATE          ShadingRate = D3D12_SHADING_RATE_1X1;

        FD3D12RenderTargetViewCache RTCache;
        FD3D12DepthStencilView*     DepthStencil;

        FD3D12IndexBufferCache      IBCache;
        FD3D12VertexBufferCache     VBCache;

        D3D12_PRIMITIVE_TOPOLOGY    PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    
        D3D12_VIEWPORT              Viewports[D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                      NumViewports;

        D3D12_RECT                  ScissorRects[D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                      NumScissor;

        FVector4                    BlendFactor;

        bool bBindRenderTargets     : 1;
        bool bBindBlendFactor       : 1;
        bool bBindPrimitiveTopology : 1;
        bool bBindPipeline          : 1;
        bool bBindVertexBuffers     : 1;
        bool bBindIndexBuffer       : 1;
        bool bBindScissorRects      : 1;
        bool bBindViewports         : 1;
    } Graphics;

    struct 
    {
        FD3D12ComputePipelineStateRef PipelineState;
        
        bool bBindPipeline : 1;
    } Compute;

    FD3D12ShaderConstantsCache ShaderConstantsCache;
    FD3D12DescriptorCache      DescriptorCache;

    bool bIsReady            : 1;
    bool bIsCapturing        : 1;
    bool bIsRenderPassActive : 1;
    bool bBindRootSignature  : 1;
};

class FD3D12CommandContext : public IRHICommandContext, public FD3D12DeviceChild
{
public:
    FD3D12CommandContext(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12CommandContext();

    virtual void RHIStartContext() override final;
    
    virtual void RHIFinishContext() override final;

    virtual void RHIBeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index) override final;

    virtual void RHIEndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index) override final;

    virtual void RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) override final;
    
    virtual void RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil) override final;
    
    virtual void RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) override final;

    virtual void RHIBeginRenderPass(const FRHIRenderPassDesc& RenderPassInitializer) override final;
    
    virtual void RHIEndRenderPass() override final;

    virtual void RHISetViewport(const FRHIViewportRegion& ViewportRegion) override final;

    virtual void RHISetScissorRect(const FRHIScissorRegion& ScissorRegion) override final;

    virtual void RHISetBlendFactor(const FVector4& Color) override final;

    virtual void RHISetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) override final;
    
    virtual void RHISetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) override final;

    virtual void RHISetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) override final;

    virtual void RHISetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    
    virtual void RHISetComputePipelineState(class FRHIComputePipelineState* PipelineState) override final;

    virtual void RHISet32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) override final;

    virtual void RHISetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) override final;
    
    virtual void RHISetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex) override final;

    virtual void RHISetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) override final;
    
    virtual void RHISetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex) override final;

    virtual void RHISetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex) override final;
    
    virtual void RHISetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex) override final;

    virtual void RHISetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex) override final;
    
    virtual void RHISetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex) override final;

    virtual void RHIUpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) override final;
    
    virtual void RHIUpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) override final;

    virtual void RHIResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final;

    virtual void RHICopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc) override final;
    
    virtual void RHICopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    
    virtual void RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc) override final;

    virtual void RHIDestroyResource(class IRefCounted* Resource) override final;

    virtual void RHIDiscardContents(class FRHITexture* Texture) override final;

    virtual void RHIBuildRayTracingGeometry(
        FRHIRayTracingGeometry* RayTracingGeometry,
        FRHIBuffer*             VertexBuffer,
        uint32                  NumVertices,
        FRHIBuffer*             IndexBuffer,
        uint32                  NumIndices,
        EIndexFormat            IndexFormat,
        bool                    bUpdate) override final;
    
    virtual void RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate) override final;

    virtual void RHISetRayTracingBindings(
        FRHIRayTracingScene*              RayTracingScene,
        FRHIRayTracingPipelineState*      PipelineState,
        const FRayTracingShaderResources* GlobalResource,
        const FRayTracingShaderResources* RayGenLocalResources,
        const FRayTracingShaderResources* MissLocalResources,
        const FRayTracingShaderResources* HitGroupResources,
        uint32                            NumHitGroupResources) override final;

    virtual void RHIGenerateMips(FRHITexture* Texture) override final;

    virtual void RHITransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) override final;

    virtual void RHITransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) override final;

    virtual void RHIUnorderedAccessTextureBarrier(FRHITexture* Texture) override final;
    
    virtual void RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer) override final;

    virtual void RHIDraw(uint32 VertexCount, uint32 StartVertexLocation) override final;

    virtual void RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final;
    
    virtual void RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final;
    
    virtual void RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;

    virtual void RHIDispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;

    virtual void RHIDispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) override final;

    virtual void RHIPresentViewport(FRHIViewport* Viewport, bool bVerticalSync) override final;

    virtual void RHIClearState() override final;

    virtual void RHIFlush() override final;

    virtual void RHIInsertMarker(const FStringView& Message) override final;

    virtual void RHIBeginExternalCapture() override final;

    virtual void RHIEndExternalCapture() override final;

    virtual void* RHIGetNativeCommandList() override final 
    { 
        return reinterpret_cast<void*>(&CommandList);
    }

public:
    bool Initialize();
    
    void ObtainCommandList();

    void FinishCommandList();

    void UpdateBuffer(FD3D12Resource* Resource, const FBufferRegion& BufferRegion, const void* SourceData);

    FORCEINLINE FD3D12CommandList& GetCommandList() 
    {
        CHECK(CommandList != nullptr);
        return *CommandList; 
    }

    FORCEINLINE FD3D12CommandAllocatorManager& GetCommandAllocatorManager()
    {
        return CommandAllocatorManager;
    }

    FORCEINLINE FD3D12CommandAllocator& GetCommandAllocator()
    {
        CHECK(CommandAllocator != nullptr);
        return *CommandAllocator;
    }
    
    FORCEINLINE uint32 GetCurrentBachIndex() const
    {
        CHECK(int32(NextCmdBatch) < CmdBatches.Size());
        return FMath::Max<int32>(int32(NextCmdBatch) - 1, 0);
    }

    FORCEINLINE void UnorderedAccessBarrier(FD3D12Resource* Resource)
    {
        D3D12_ERROR_COND(Resource != nullptr, "UnorderedAccessBarrier cannot be called with a nullptr resource");
        BarrierBatcher.AddUnorderedAccessBarrier(Resource->GetD3D12Resource());
    }

    FORCEINLINE void TransitionResource(FD3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
    {
        D3D12_ERROR_COND(Resource != nullptr, "TransitionResource cannot be called with a nullptr resource");
        BarrierBatcher.AddTransitionBarrier(Resource->GetD3D12Resource(), BeforeState, AfterState);
    }

    FORCEINLINE void DestroyResource(FD3D12Resource* Resource) 
    { 
        CmdBatch->AddInUseResource(Resource);
    }

    FORCEINLINE void FlushResourceBarriers() 
    { 
        BarrierBatcher.FlushBarriers(*CommandList);
    }

private:
    ED3D12CommandQueueType          QueueType;

    FD3D12CommandListRef            CommandList;
    FD3D12CommandAllocatorRef       CommandAllocator;
    FD3D12CommandAllocatorManager   CommandAllocatorManager;
    FD3D12CommandContextState       State;

    // TODO: The whole commandcontext should only be used from one thread at a time
    FCriticalSection                CommandContextCS;

    TArray<FD3D12TimestampQueryRef> ResolveQueries;
    
    // TODO: Refactor all below
    FD3D12ResourceBarrierBatcher    BarrierBatcher;

    uint32                          NextCmdBatch = 0;

    TArray<FD3D12CommandBatch>      CmdBatches;
    FD3D12CommandBatch*             CmdBatch = nullptr;
};
