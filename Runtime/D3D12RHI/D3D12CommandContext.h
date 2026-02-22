#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Map.h"
#include "RHI/IRHICommandContext.h"
#include "D3D12RHI/D3D12Fence.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Query.h"
#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12CommandContextState.h"
#include "D3D12RHI/D3D12ResourceState.h"

class FD3D12BarrierBatcher
{
public:
    void AddTransitionBarrier(FD3D12Resource* InResource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    void AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, uint32 SubresourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    void AddUnorderedAccessBarrier(FD3D12Resource* InResource);
    void AddUnorderedAccessBarrier(ID3D12Resource* Resource);
    void FlushBarriers(FD3D12CommandList& CommandList);

    bool HasPendingBarriers() const 
    {
        return !Barriers.IsEmpty();
    }

private:
    TArray<D3D12_RESOURCE_BARRIER> Barriers;
};

class FD3D12CommandContext : public IRHICommandContext, public FD3D12DeviceChild
{
public:
    FD3D12CommandContext(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12CommandContext();

    // IRHICommandContext Interface
    virtual void BeginFrame() override final;
    virtual void EndFrame() override final;

    virtual void StartContext() override final;
    virtual void FinishContext() override final;

    virtual void BeginQuery(FRHIQuery* Query) override final;
    virtual void EndQuery(FRHIQuery* Query) override final;
    virtual void QueryTimestamp(FRHIQuery* Query) override final;
    virtual void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) override final;
    virtual void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil) override final;
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) override final;
    virtual void BeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo) override final;
    virtual void EndRenderPass() override final { }
    virtual void SetViewport(const FViewportRegion& ViewportRegion) override final;
    virtual void SetScissorRect(const FScissorRegion& ScissorRegion) override final;
    virtual void SetBlendFactor(const FVector4& Color) override final;
    virtual void SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) override final;
    virtual void SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) override final;
    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState) override final;
    virtual void SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants) override final;
    virtual void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) override final;
    virtual void SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex) override final;
    virtual void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) override final;
    virtual void SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex) override final;
    virtual void SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex) override final;
    virtual void SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex) override final;
    virtual void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex) override final;
    virtual void SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex) override final;
    virtual void UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) override final;
    virtual void UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) override final;
    virtual void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc) override final;
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc) override final;
    virtual void CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) override final;
    virtual void WriteFence(FRHIGpuFence* Fence) override final;
    virtual void DiscardContents(class FRHITexture* Texture) override final;
    virtual void BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo) override final;
    virtual void BuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo) override final;
    virtual void SetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources) override final;
    virtual void TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition) override final;
    virtual void TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) override final;
    virtual void RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState) override final;
    virtual void RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState) override final;
    virtual void UnorderedAccessTextureBarrier(FRHITexture* Texture) override final;
    virtual void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer) override final;
    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final;
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final;
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;
    virtual void DispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth) override final;
    virtual void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) override final;
    virtual void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height) override final;
    virtual void InsertMarker(const FStringView& Message) override final;

    virtual void ClearState() override final;
    virtual void Flush() override final;

    virtual void BeginExternalCapture() override final;
    virtual void EndExternalCapture() override final;

    virtual void* GetNativeCommandList() override final 
    { 
        return reinterpret_cast<void*>(&CommandList);
    }

    bool Initialize();
    void ObtainCommandList();
    void FinishCommandList(bool bFlushAllocator);
    void SplitCommandList(bool bFlushAllocator, bool bWaitForQueue);
    void SplitCommandListAndResetState(bool bFlushAllocator, bool bWaitForQueue);
    
    void UpdateBuffer(FD3D12Resource* Resource, const FBufferRegion& BufferRegion, const void* SourceData);

    FD3D12CommandList& GetCommandList() 
    {
        CHECK(CommandList != nullptr);
        return *CommandList; 
    }

    FD3D12Commands& GetCommands()
    {
        CHECK(Commands != nullptr);
        return *Commands;
    }

    FD3D12BarrierBatcher& GetBarrierBatcher()
    {
        return ResourceBarrierBatcher;
    }

    ED3D12CommandQueueType GetQueueType() const
    {
        return QueueType;
    }

    bool IsRecording() const
    {
        return bIsRecording;
    }

    bool NeedsCommandList() const
    {
        return CommandList == nullptr;
    }

private:
    void ConditionalSplitCommandList();

    FD3D12ResourceState& RetrievePendingResourceState(FD3D12Resource* Resource);
    void AddPendingBarrier(FD3D12Resource* Resource, D3D12_RESOURCE_STATES DesiredState, uint32 Subresource);

    FD3D12CommandList*                         CommandList;
    FD3D12CommandAllocator*                    CommandAllocator;
    FD3D12Commands*                            Commands;
    FD3D12CommandContextState                  ContextState;
    FD3D12QueryAllocator                       TimingQueryAllocator;
    FD3D12QueryAllocator                       OcclusionQueryAllocator;
    FD3D12BarrierBatcher                    ResourceBarrierBatcher;
    TArray<FD3D12PendingBarrier>               PendingBarriers;
    TMap<FD3D12Resource*, FD3D12ResourceState> PendingResourceStates;
    ED3D12CommandQueueType                     QueueType;
    bool                                       bIsCapturing : 1; // Keeps track of any programmatic captures currently being done
    bool                                       bIsRecording : 1; // Keeps track of the recording state of the context. I.e has StartContext been called

    // TODO: The whole CommandContext should only be used from one thread at a time
    FCriticalSection          CommandContextCS;
};
