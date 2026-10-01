#pragma once
#include "RHI/IRHICommandContext.h"
#include "D3D11RHI/D3D11CommandContextState.h"
#include "D3D11RHI/D3D11DeviceChild.h"

class FD3D11CommandContext : public IRHICommandContext, public FD3D11DeviceChild
{
public:
    FD3D11CommandContext(FD3D11Device* InDevice);
    ~FD3D11CommandContext();

    bool Initialize();

    // IRHICommandContext Interface
    virtual void BeginFrame() override final;
    virtual void EndFrame() override final;

    virtual void StartContext() override final;
    virtual void FinishContext() override final;

    virtual void BeginQuery(FRHIQuery* Query) override final;
    virtual void EndQuery(FRHIQuery* Query) override final;
    virtual void QueryTimestamp(FRHIQuery* Query) override final;
    virtual void ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor) override final;
    virtual void ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil) override final;
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor) override final;
    virtual void ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4]) override final;
    virtual void BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc) override final;
    virtual void EndRenderPass() override final;
    virtual void SetViewport(const FViewportRegion& ViewportRegion) override final;
    virtual void SetScissorRect(const FScissorRegion& ScissorRegion) override final;
    virtual void SetBlendFactor(const Vector4& Color) override final;
    virtual void SetStencilRef(uint32 StencilRef) override final;
    virtual void SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias) override final;
    virtual void SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) override final;
    virtual void SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) override final;
    virtual void SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets) override final;
    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState) override final;
    virtual void SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants) override final;
    virtual void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex) override final;
    virtual void SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex) override final;
    virtual void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex) override final;
    virtual void SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex) override final;
    virtual void SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex) override final;
    virtual void SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex) override final;
    virtual void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex) override final;
    virtual void SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex) override final;
    virtual void UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) override final;
    virtual void UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) override final;
    virtual void UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch) override final;
    virtual void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc) override final;
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc) override final;
    virtual void CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) override final;
    virtual void CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice) override final;
    virtual void WriteFence(FRHIFence* Fence) override final;
    virtual void DiscardContents(class FRHITexture* Texture) override final;
    virtual void TransitionBarrier(TArrayView<const FRHITransitionBarrierDesc> TransitionDescs) override final;
    virtual void UnorderedAccessBarrier(TArrayView<const FRHIUnorderedAccessBarrierDesc> BarrierDescs) override final;
    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final;
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final;
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;
    virtual void DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) override final;
    virtual void DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) override final;
    virtual void DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset) override final;
    virtual void AcquireNextBackBuffer(FRHISwapChain* SwapChain) override final;
    virtual void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) override final;
    virtual void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace) override final;
    virtual void SetSwapChainHDRMetadata(FRHISwapChain* SwapChain, const FRHIHDRMetadata& Metadata) override final;
    virtual void PushEvent(const StringView& Name) override final;
    virtual void PopEvent() override final;

    virtual void ClearState() override final;
    virtual void Flush() override final;

    virtual void* GetRHINativeCommandList() override final;

    ID3D11DeviceContext* GetD3D11Context() const;

    FORCEINLINE FD3D11CommandContextState& GetContextState()
    {
        return ContextState;
    }

    // -------------------------------------------------------------------------------------------
    // IRHICommandContext interface that D3D11 can never support
    // -------------------------------------------------------------------------------------------

    virtual void SetDepthBounds(float, float) override final { }
    virtual void SetSamplePositions(const FRHISamplePositionsDesc&) override final { }
    virtual void SetMeshletPipelineState(class FRHIMeshletPipelineState*) override final { }
    virtual void TranscodeSamplerFeedback(FRHITexture*, uint32, FRHITexture*, uint32, ESamplerFeedbackTranscodeMode) override final { }
    virtual void DispatchMesh(uint32, uint32, uint32) override final { }
    virtual void DrawIndirectCount(FRHIBuffer*, uint64, FRHIBuffer*, uint64, uint32) override final { }
    virtual void DrawIndexedIndirectCount(FRHIBuffer*, uint64, FRHIBuffer*, uint64, uint32) override final { }
    virtual void DispatchMeshIndirect(FRHIBuffer*, uint64, uint32) override final { }
    virtual void DispatchMeshIndirectCount(FRHIBuffer*, uint64, FRHIBuffer*, uint64, uint32) override final { }
    virtual void BuildSceneAccelerationStructure(FRHISceneAccelerationStructure*, const FRHISceneAccelerationStructureBuildDesc&) override final { }
    virtual void BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure*, const FRHIGeometryAccelerationStructureBuildDesc&) override final { }
    virtual void WriteAccelerationStructurePostBuildInfo(FRHIBuffer*, uint64, EAccelerationStructurePostBuildInfoType, FRHIRayTracingAccelerationStructure* const*, uint32) override final { }
    virtual void CopyAccelerationStructure(FRHIRayTracingAccelerationStructure*, FRHIRayTracingAccelerationStructure*, EAccelerationStructureCopyMode) override final { }
    virtual void CompactAccelerationStructure(FRHIRayTracingAccelerationStructure*, uint64) override final { }
    virtual void SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure*, FRHIBuffer*, uint64) override final { }
    virtual void DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure*, FRHIBuffer*, uint64) override final { }
    virtual void SetHitRecordLocalShaderBindings(FRHIShaderBindingTable*, ERayTracingShaderRecordKind, uint32, const FRHIHitGroupLocalShaderBinding*, uint32) override final { }
    virtual void BuildShaderBindingTable(FRHIShaderBindingTable*) override final { }
    virtual void ResetShaderBindingTable(FRHIShaderBindingTable*) override final { }
    virtual void SetRayTracingPipelineState(FRHIRayTracingPipelineState*) override final { }
    virtual void DispatchRays(FRHIShaderBindingTable*, uint32, uint32, uint32) override final { }
    virtual void DispatchRaysIndirect(FRHIShaderBindingTable*, FRHIBuffer*, uint64) override final { }
    virtual void BuildOpacityMicromap(FRHIOpacityMicromap*, const FRHIOpacityMicromapBuildDesc&) override final { }
    virtual void ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc*, uint32) override final { }

private:
    FD3D11CommandContextState          ContextState;
    TComPtr<ID3DUserDefinedAnnotation> Annotation;
};
