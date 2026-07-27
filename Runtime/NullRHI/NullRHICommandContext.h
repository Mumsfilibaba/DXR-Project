#pragma once
#include "NullRHIResources.h"
#include "RHI/IRHICommandContext.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FNullRHICommandContext final : public IRHICommandContext
{
    virtual void BeginFrame() override final { }
    virtual void EndFrame()   override final { }

    virtual void StartContext()  override final { }
    virtual void FinishContext() override final { }

    virtual void BeginQuery(FRHIQuery* Query) override final { }
    virtual void EndQuery(FRHIQuery* Query) override final { }
    virtual void QueryTimestamp(FRHIQuery* Query) override final { }
    virtual void ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor) override final { }
    virtual void ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil) override final { }
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor) override final { }
    virtual void ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4]) override final { }
    virtual void BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc) override final { }
    virtual void EndRenderPass() override final { }
    virtual void SetViewport(const FViewportRegion& ViewportRegion) override final { }
    virtual void SetScissorRect(const FScissorRegion& ScissorRegion) override final { }
    virtual void SetBlendFactor(const Vector4& Color) override final { }
    virtual void SetStencilRef(uint32 StencilRef) override final { }
    virtual void SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias) override final { }
    virtual void SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) override final { }
    virtual void SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) override final { }
    virtual void SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets) override final { }
    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final { }
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState) override final { }
    virtual void SetMeshletPipelineState(class FRHIMeshletPipelineState* PipelineState) override final { }
    virtual void SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants) override final { }
    virtual void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex) override final { }
    virtual void SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex) override final { }
    virtual void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex) override final { }
    virtual void SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex) override final { }
    virtual void SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex) override final { }
    virtual void SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex) override final { }
    virtual void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex) override final { }
    virtual void SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex) override final { }
    virtual void UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) override final { }
    virtual void UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& BufferRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) override final { }
    virtual void UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch) override final { }
    virtual void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final { }
    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc) override final { }
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) override final { }
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyTextureInfo) override final { }
    virtual void CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) override final { }
    virtual void CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice) override final { }
    virtual void WriteFence(FRHIFence* Fence) override final { }
    virtual void DiscardContents(class FRHITexture* Texture) override final { }
    virtual void BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc) override final { }
    virtual void BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc) override final { }
    virtual void TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition) override final { }
    virtual void TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) override final { }
    virtual void RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState) override final { }
    virtual void RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState) override final { }
    virtual void UnorderedAccessTextureBarrier(FRHITexture* Texture) override final { }
    virtual void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer) override final { }
    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final { }
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final { }
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final { }
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final { }
    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final { }
    virtual void DispatchMesh(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override final { }
    virtual void DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) override final { }
    virtual void DrawIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) override final { }
    virtual void DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) override final { }
    virtual void DrawIndexedIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) override final { }
    virtual void DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset) override final { }
    virtual void DispatchMeshIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) override final { }
    virtual void DispatchMeshIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) override final { }

    virtual void SetHitRecordLocalShaderBindings(FRHIShaderBindingTable* ShaderBindingTable, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings) override final { }
    virtual void BuildShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable) override final { }
    virtual void ResetShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable) override final { }

    virtual void SetRayTracingPipelineState(FRHIRayTracingPipelineState* PipelineState) override final { }
    virtual void DispatchRays(FRHIShaderBindingTable* ShaderBindingTable, uint32 Width, uint32 Height, uint32 Depth) override final { }
    virtual void DispatchRaysIndirect(FRHIShaderBindingTable* ShaderBindingTable, FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset) override final { }
    virtual void BuildOpacityMicromap(FRHIOpacityMicromap* OpacityMicromap, const FRHIOpacityMicromapBuildDesc& BuildDesc) override final { }
    virtual void ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* Operations, uint32 NumOperations) override final { }
    virtual void WriteAccelerationStructurePostBuildInfo(FRHIBuffer* DstBuffer, uint64 DstOffset, EAccelerationStructurePostBuildInfoType InfoType, FRHIRayTracingAccelerationStructure* const* Sources, uint32 NumSources) override final { }
    virtual void CopyAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIRayTracingAccelerationStructure* Source, EAccelerationStructureCopyMode CopyMode) override final { }
    virtual void CompactAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64 CompactedSizeInBytes) override final { }
    virtual void SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Source, FRHIBuffer* DstBuffer, uint64 DstOffset) override final { }
    virtual void DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIBuffer* SourceBuffer, uint64 SourceOffset) override final { }

    virtual void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) override final
    {
        if (FNullSwapChainRHI* NullSwapChain = static_cast<FNullSwapChainRHI*>(SwapChain))
        {
            NullSwapChain->Present(bVerticalSync);
        }
    }

    virtual void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace) override final
    {
        FNullSwapChainRHI* NullSwapChain = static_cast<FNullSwapChainRHI*>(SwapChain);
        const uint32 ResolvedWidth  = (Width  > 0u) ? Width  : NullSwapChain->GetDesc().Width;
        const uint32 ResolvedHeight = (Height > 0u) ? Height : NullSwapChain->GetDesc().Height;
        NullSwapChain->Resize(ResolvedWidth, ResolvedHeight, Format, ColorSpace);
    }

    virtual void ClearState() override final { }

    virtual void Flush() override final { }

    virtual void PushEvent(const StringView& Name) override final { }
    virtual void PopEvent()                         override final { }

    virtual void* GetRHINativeCommandList() override final { return nullptr; }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
