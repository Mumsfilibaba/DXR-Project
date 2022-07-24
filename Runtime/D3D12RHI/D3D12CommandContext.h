#pragma once
#include "RHI/IRHICommandContext.h"

#include "Core/Containers/SharedRef.h"

#include "D3D12DeviceChild.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12UploadAllocation

struct FD3D12UploadAllocation
{
    uint8* MappedPtr      = nullptr;
    uint64 ResourceOffset = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12GPUResourceUploader

class FD3D12GPUResourceUploader : public FD3D12DeviceChild
{
public:
    FD3D12GPUResourceUploader(FD3D12Device* InDevice);
    ~FD3D12GPUResourceUploader() = default;

    bool Reserve(uint32 InSizeInBytes);

    void Reset();

    FD3D12UploadAllocation LinearAllocate(uint32 SizeInBytes);

    FORCEINLINE ID3D12Resource* GetGpuResource() const
    {
        return Resource.Get();
    }

    FORCEINLINE uint32 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

private:
    uint8* MappedMemory  = nullptr;

    uint32 SizeInBytes   = 0;
    uint32 OffsetInBytes = 0;

    TComPtr<ID3D12Resource> Resource;

    TArray<TComPtr<ID3D12Resource>> GarbageResources;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandBatch

class FD3D12CommandBatch
{
public:
    FD3D12CommandBatch(FD3D12Device* InDevice);
    ~FD3D12CommandBatch() = default;

    bool Initialize(uint32 Index);

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

    FORCEINLINE FD3D12CommandAllocator& GetCommandAllocator()
    {
        return CmdAllocator;
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

    FD3D12CommandAllocator           CmdAllocator;
    FD3D12GPUResourceUploader        GpuResourceUploader;

    FD3D12OnlineDescriptorManagerRef OnlineResourceDescriptorHeap;
    FD3D12OnlineDescriptorManagerRef OnlineSamplerDescriptorHeap;

    TArray<FD3D12ResourceRef>        DxResources;
    TArray<TSharedRef<IRefCounted>>  Resources;

    TArray<TComPtr<ID3D12Resource>>  NativeResources;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ResourceBarrierBatcher

class FD3D12ResourceBarrierBatcher
{
public:
    
    FD3D12ResourceBarrierBatcher()
        : Barriers()
    { }

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandContextState

struct FD3D12CommandContextState : FNonCopyAndNonMovable
{
    FD3D12CommandContextState(FD3D12Device* InDevice);
    ~FD3D12CommandContextState() = default;

    bool Initialize();

    void ApplyGraphics(FD3D12CommandList& CommandList, FD3D12CommandBatch* Batch);
    void ApplyCompute(FD3D12CommandList& CommandList, FD3D12CommandBatch* Batch);

    void ClearGraphics();
    void ClearCompute();

    void SetVertexBuffer(FD3D12VertexBuffer* VertexBuffer, uint32 Slot);
    void SetIndexBuffer(FD3D12IndexBuffer* IndexBuffer);

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

        FD3D12TextureRef               ShadingRateTexture;
        D3D12_SHADING_RATE             ShadingRate = D3D12_SHADING_RATE_1X1;

        FD3D12RenderTargetViewCache    RTCache;
        FD3D12DepthStencilView*        DepthStencil;

        FD3D12IndexBufferCache         IBCache;
        FD3D12VertexBufferCache        VBCache;

        D3D12_PRIMITIVE_TOPOLOGY       PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    
        D3D12_VIEWPORT                 Viewports[D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                         NumViewports;

        D3D12_RECT                     ScissorRects[D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                         NumScissor;

        TStaticArray<float, 4>         BlendFactor;

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandContext

class FD3D12CommandContext : public IRHICommandContext, public FD3D12DeviceChild
{
private:

    friend class FD3D12CoreInterface;

    FD3D12CommandContext(FD3D12Device* InDevice);
    ~FD3D12CommandContext();

    static FD3D12CommandContext* CreateD3D12CommandContext(FD3D12Device* InDevice);

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // IRHICommandContext Interface

    virtual void StartContext()  override final;
    virtual void FinishContext() override final;

    virtual void BeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index) override final;
    virtual void EndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)   override final;

    virtual void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const TStaticArray<float, 4>& ClearColor)         override final;
    virtual void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)                 override final;
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor) override final;

    virtual void BeginRenderPass(const FRHIRenderPassInitializer& RenderPassInitializer) override final;
    virtual void EndRenderPass() override final;

    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) override final;
    virtual void SetScissorRect(float Width, float Height, float x, float y)                              override final;

    virtual void SetBlendFactor(const TStaticArray<float, 4>& Color) override final;

    virtual void SetVertexBuffers(FRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot) override final;
    virtual void SetIndexBuffer(FRHIIndexBuffer* IndexBuffer)                                                    override final;

    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) override final;

    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState)   override final;

    virtual void Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) override final;

    virtual void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)                                        override final;
    virtual void SetShaderResourceViews(FRHIShader* Shader, FRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex) override final;

    virtual void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)                                          override final;
    virtual void SetUnorderedAccessViews(FRHIShader* Shader, FRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex) override final;

    virtual void SetConstantBuffer(FRHIShader* Shader, FRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)                                     override final;
    virtual void SetConstantBuffers(FRHIShader* Shader, FRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex) override final;

    virtual void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)                                   override final;
    virtual void SetSamplerStates(FRHIShader* Shader, FRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex) override final;

    virtual void UpdateBuffer(FRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)           override final;
    virtual void UpdateTexture2D(FRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData) override final;

    virtual void ResolveTexture(FRHITexture* Destination, FRHITexture* Source) override final;

    virtual void CopyBuffer(FRHIBuffer* Destination, FRHIBuffer* Source, const FRHICopyBufferInfo& CopyInfo)                  override final;
    virtual void CopyTexture(FRHITexture* Destination, FRHITexture* Source)                                                   override final;
    virtual void CopyTextureRegion(FRHITexture* Destination, FRHITexture* Source, const FRHICopyTextureInfo& CopyTextureInfo) override final;

    virtual void DestroyResource(class IRefCounted* Resource) override final;
    virtual void DiscardContents(class FRHITexture* Texture)  override final;

    virtual void BuildRayTracingGeometry(FRHIRayTracingGeometry* Geometry, FRHIVertexBuffer* VertexBuffer, FRHIIndexBuffer* IndexBuffer, bool bUpdate)       override final;
    virtual void BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate) override final;

     /** @brief: Sets the resources used by the ray tracing pipeline NOTE: temporary and will soon be refactored */
    virtual void SetRayTracingBindings(
        FRHIRayTracingScene* RayTracingScene,
        FRHIRayTracingPipelineState* PipelineState,
        const FRayTracingShaderResources* GlobalResource,
        const FRayTracingShaderResources* RayGenLocalResources,
        const FRayTracingShaderResources* MissLocalResources,
        const FRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources) override final;

    virtual void GenerateMips(FRHITexture* Texture) override final;

    virtual void TransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) override final;
    virtual void TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)    override final;

    virtual void UnorderedAccessTextureBarrier(FRHITexture* Texture) override final;
    virtual void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)    override final;

    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final;
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)                                                                         override final;
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)                                 override final;
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;

    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;

    virtual void DispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) override final;

    virtual void PresentViewport(FRHIViewport* Viewport, bool bVerticalSync) override final;

    virtual void ClearState() override final;

    virtual void Flush() override final;

    virtual void InsertMarker(const FStringView& Message) override final;

    virtual void BeginExternalCapture() override final;
    virtual void EndExternalCapture()   override final;

    virtual void* GetRHIBaseCommandList() override final { return reinterpret_cast<void*>(&CommandList); }

public:
    void StartCommandList();
    void EndCommandList();

    void UpdateBuffer(FD3D12Resource* Resource, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData);

    FORCEINLINE uint32 GetCurrentEpochValue() const
    {
        uint32 MaxValue = NMath::Max<int32>((int32)CmdBatches.Size() - 1, 0);
        return NMath::Min<uint32>(NextCmdBatch - 1, MaxValue);
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

    FORCEINLINE FD3D12CommandQueue& GetQueue()       { return CommandQueue; }
    FORCEINLINE FD3D12CommandList&  GetCommandList() { return CommandList; }

    FORCEINLINE void FlushResourceBarriers() { BarrierBatcher.FlushBarriers(CommandList); }

    FORCEINLINE void DestroyResource(FD3D12Resource* Resource) { CmdBatch->AddInUseResource(Resource); }

private:
    bool Initialize();

    void InternalClearState();

    // TODO: Look into if this is the best way
    FCriticalSection                CommandContextCS;

    FD3D12CommandList               CommandList;
    FD3D12Fence                     Fence;
    FD3D12CommandQueue              CommandQueue;

    FD3D12CommandContextState       State;
    FD3D12ResourceBarrierBatcher    BarrierBatcher;

    uint64                          FenceValue   = 0;
    uint32                          NextCmdBatch = 0;

    TArray<FD3D12CommandBatch>      CmdBatches;
    FD3D12CommandBatch*             CmdBatch = nullptr;

    TArray<FD3D12TimestampQueryRef> ResolveQueries;

};