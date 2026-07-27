#pragma once
#include "Core/Memory/MemoryStack.h"
#include "Core/Threading/Runnable.h"
#include "Core/Platform/PlatformThread.h"
#include "Core/Platform/PlatformEvent.h"
#include "Core/Platform/ConditionVariable.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/Queue.h"
#include "RHI/RHI.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommands.h"
#include "RHI/RHIRayTracing.h"
#include "RHI/RHIStats.h"

class FRHIRenderTargetView;
class FRHIDepthStencilView;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHIShader;
class FRHISwapChain;

class FRHICommandList;
class FRHIImmediateCommandList;

class FRHICommandList : FNonCopyable
{
public:
    RHI_API FRHICommandList() noexcept;
    RHI_API ~FRHICommandList() noexcept;

    RHI_API void Execute() noexcept;
    RHI_API void ExecuteWithContext(IRHICommandContext& InCommandContext) noexcept;
    RHI_API void Reset() noexcept;
    RHI_API void ExchangeState(FRHICommandList& Other) noexcept;

    RHI_API void FlushDeletedResources() noexcept;

    FORCEINLINE void* Allocate(uint64 Size, uint32 Alignment) noexcept
    {
        return Memory.Allocate(static_cast<int32>(Size), static_cast<int32>(Alignment));
    }

    template<typename T>
    FORCEINLINE T* Allocate() noexcept
    {
        return reinterpret_cast<T*>(Allocate(sizeof(T), alignof(T)));
    }

    template<typename T>
    FORCEINLINE TArrayView<T> AllocateArray(const TArrayView<T>& Array) noexcept
    {
        void* NewArray = Allocate(Array.Size() * sizeof(T), alignof(T));
        Memory::Memcpy(NewArray, Array.Data(), Array.SizeInBytes());
        return TArrayView<T>(reinterpret_cast<T*>(NewArray), Array.Size());
    }

    FORCEINLINE void* AllocateCommand(int32 Size, int32 Alignment) noexcept
    {
        FRHICommand* NewCommand = reinterpret_cast<FRHICommand*>(Allocate(Size, Alignment));
        *CommandPointer = NewCommand;
        CommandPointer = &NewCommand->NextCommand;
        ++NumCommands;
        return NewCommand;
    }

    FORCEINLINE CHAR_T* AllocateString(const CHAR_T* String) noexcept
    {
        const int32 Length = CString::Strlen(String);
        CHAR_T* NewString = reinterpret_cast<CHAR_T*>(Allocate(sizeof(CHAR_T) * Length, alignof(CHAR_T)));
        return CString::Strcpy(NewString, String);
    }

    template<typename CommandType, typename... ArgTypes>
    FORCEINLINE void EmplaceCommand(ArgTypes&&... Args) noexcept
    {
        new(AllocateCommand(sizeof(CommandType), alignof(CommandType))) CommandType(Forward<ArgTypes>(Args)...);
    }

    template<typename T, typename... ArgTypes>
    FORCEINLINE T* EmplaceObject(ArgTypes&&... Args) noexcept
    {
        return new(Allocate(sizeof(T), alignof(T))) T(Forward<ArgTypes>(Args)...);
    }

    FORCEINLINE void SetCommandContext(IRHICommandContext* InCommandContext) noexcept
    {
        CommandContext = InCommandContext;
    }

    FORCEINLINE IRHICommandContext& GetCommandContext() const noexcept
    {
        CHECK(CommandContext != nullptr);
        return *CommandContext;
    }

    FORCEINLINE void SetEvent(FGenericPlatformEvent* InEvent) noexcept
    {
        CHECK(InEvent != nullptr);
        FinishedEvent = InEvent;
    }

    FORCEINLINE FGenericPlatformEvent* GetEvent() const noexcept
    {
        return FinishedEvent;
    }

    FORCEINLINE bool HasCommands() const noexcept
    {
        return NumCommands > 0;
    }

    // Record commands
    template<typename LambdaType>
    FORCEINLINE void ExecuteLambda(LambdaType Lambda) noexcept
    {
        EmplaceCommand<TRHICommandExecuteLambda<LambdaType>>(Lambda);
    }

    FORCEINLINE void ExecuteCommandList(FRHICommandList& CommandList) noexcept
    {
        // Cannot execute CommandList in CommandList in CommandList
        FRHICommandList* NewCommandList = EmplaceObject<FRHICommandList>();
        NewCommandList->ExchangeState(CommandList);
        EmplaceCommand<FRHICommandExecuteCommandList>(NewCommandList);
    }

    FORCEINLINE void BeginFrame() noexcept
    {
        EmplaceCommand<FRHICommandBeginFrame>();
    }

    FORCEINLINE void EndFrame() noexcept
    {
        EmplaceCommand<FRHICommandEndFrame>();
    }

    FORCEINLINE void BeginQuery(FRHIQuery* Query) noexcept
    {
        EmplaceCommand<FRHICommandBeginQuery>(Query);
    }

    FORCEINLINE void EndQuery(FRHIQuery* Query) noexcept
    {
        EmplaceCommand<FRHICommandEndQuery>(Query);
    }

    FORCEINLINE void QueryTimestamp(FRHIQuery* Query) noexcept
    {
        EmplaceCommand<FRHICommandQueryTimestamp>(Query);
    }

    FORCEINLINE void ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor) noexcept
    {
        EmplaceCommand<FRHICommandClearRenderTargetView>(RenderTargetView, ClearColor);
    }

    FORCEINLINE void ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil) noexcept
    {
        EmplaceCommand<FRHICommandClearDepthStencilView>(DepthStencilView, Depth, Stencil);
    }

    FORCEINLINE void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor) noexcept
    {
        EmplaceCommand<FRHICommandClearUnorderedAccessViewFloat>(UnorderedAccessView, ClearColor);
    }

    FORCEINLINE void ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4]) noexcept
    {
        EmplaceCommand<FRHICommandClearUnorderedAccessViewUint>(UnorderedAccessView, Values);
    }

    FORCEINLINE void BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc) noexcept
    {
        EmplaceCommand<FRHICommandBeginRenderPass>(BeginRenderPassDesc);
    }

    FORCEINLINE void EndRenderPass() noexcept
    {
        EmplaceCommand<FRHICommandEndRenderPass>();
    }

    FORCEINLINE void SetViewport(const FViewportRegion& ViewportRegion) noexcept
    {
        EmplaceCommand<FRHICommandSetViewport>(ViewportRegion);
    }

    FORCEINLINE void SetScissorRect(const FScissorRegion& ScissorRegion) noexcept
    {
        EmplaceCommand<FRHICommandSetScissorRect>(ScissorRegion);
    }

    FORCEINLINE void SetBlendFactor(const Vector4& Color) noexcept
    {
        EmplaceCommand<FRHICommandSetBlendFactor>(Color);
    }

    FORCEINLINE void SetStencilRef(uint32 StencilRef) noexcept
    {
        EmplaceCommand<FRHICommandSetStencilRef>(StencilRef);
    }

    FORCEINLINE void SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias) noexcept
    {
        EmplaceCommand<FRHICommandSetDepthBias>(DepthBias, DepthBiasClamp, SlopeScaledDepthBias);
    }

    FORCEINLINE void SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) noexcept
    {
        TArrayView<FRHIBuffer* const> VertexBuffers = AllocateArray(InVertexBuffers);
        EmplaceCommand<FRHICommandSetVertexBuffers>(VertexBuffers, BufferSlot);
    }

    FORCEINLINE void SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) noexcept
    {
        EmplaceCommand<FRHICommandSetIndexBuffer>(IndexBuffer, IndexFormat);
    }

    FORCEINLINE void SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> InBuffers, const uint64* InOffsets) noexcept
    {
        TArrayView<FRHIBuffer* const> Buffers = AllocateArray(InBuffers);
        EmplaceCommand<FRHICommandSetStreamOutputTargets>(Buffers, InOffsets);
    }

    FORCEINLINE void SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState) noexcept
    {
        EmplaceCommand<FRHICommandSetGraphicsPipelineState>(PipelineState);
    }

    FORCEINLINE void SetComputePipelineState(FRHIComputePipelineState* PipelineState) noexcept
    {
        EmplaceCommand<FRHICommandSetComputePipelineState>(PipelineState);
    }

    FORCEINLINE void SetMeshletPipelineState(FRHIMeshletPipelineState* PipelineState) noexcept
    {
        EmplaceCommand<FRHICommandSetMeshletPipelineState>(PipelineState);
    }

    FORCEINLINE void SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants) noexcept
    {
        const int32 Size = NumShaderConstants * sizeof(uint32);
        void* SourceData = Allocate(Size, alignof(uint32));
        Memory::Memcpy(SourceData, ShaderConstants, Size);
        EmplaceCommand<FRHICommandSetShaderConstants>(Shader, SourceData, NumShaderConstants);
    }

    FORCEINLINE void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex) noexcept
    {
        EmplaceCommand<FRHICommandSetShaderResourceView>(Shader, ShaderResourceView, RegisterIndex);
    }

    FORCEINLINE void SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex) noexcept
    {
        TArrayView<FRHIShaderResourceView* const> ShaderResourceViews = AllocateArray(InShaderResourceViews);
        EmplaceCommand<FRHICommandSetShaderResourceViews>(Shader, ShaderResourceViews, RegisterIndex);
    }

    FORCEINLINE void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex) noexcept
    {
        EmplaceCommand<FRHICommandSetUnorderedAccessView>(Shader, UnorderedAccessView, RegisterIndex);
    }

    FORCEINLINE void SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex) noexcept
    {
        TArrayView<FRHIUnorderedAccessView* const> UnorderedAccessViews = AllocateArray(InUnorderedAccessViews);
        EmplaceCommand<FRHICommandSetUnorderedAccessViews>(Shader, UnorderedAccessViews, RegisterIndex);
    }

    FORCEINLINE void SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex) noexcept
    {
        EmplaceCommand<FRHICommandSetConstantBuffer>(Shader, ConstantBuffer, RegisterIndex);
    }

    FORCEINLINE void SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex) noexcept
    {
        TArrayView<FRHIBuffer* const> ConstantBuffers = AllocateArray(InConstantBuffers);
        EmplaceCommand<FRHICommandSetConstantBuffers>(Shader, ConstantBuffers, RegisterIndex);
    }

    FORCEINLINE void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex) noexcept
    {
        EmplaceCommand<FRHICommandSetSamplerState>(Shader, SamplerState, RegisterIndex);
    }

    FORCEINLINE void SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex) noexcept
    {
        TArrayView<FRHISamplerState* const> SamplerStates = AllocateArray(InSamplerStates);
        EmplaceCommand<FRHICommandSetSamplerStates>(Shader, SamplerStates, RegisterIndex);
    }

    FORCEINLINE void UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* InSrcData) noexcept
    {
        void* SrcData = Allocate(BufferRegion.Size, alignof(uint8));
        Memory::Memcpy(SrcData, InSrcData, BufferRegion.Size);
        EmplaceCommand<FRHICommandUpdateBuffer>(Dst, BufferRegion, SrcData);
    }

    FORCEINLINE void UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* InSrcData, uint32 SrcRowPitch) noexcept
    {
        const uint32 SizeInBytes = SrcRowPitch * TextureRegion.Height;
        void* SrcData = Allocate(SizeInBytes, alignof(uint8));
        Memory::Memcpy(SrcData, InSrcData, SizeInBytes);
        EmplaceCommand<FRHICommandUpdateTexture2D>(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch);
    }

    FORCEINLINE void UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* InSrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch) noexcept
    {
        const uint32 SizeInBytes = SrcDepthPitch * TextureRegion.Depth;
        void* SrcData = Allocate(SizeInBytes, alignof(uint8));
        Memory::Memcpy(SrcData, InSrcData, SizeInBytes);
        EmplaceCommand<FRHICommandUpdateTexture3D>(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch, SrcDepthPitch);
    }

    FORCEINLINE void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) noexcept
    {
        EmplaceCommand<FRHICommandResolveTexture>(Dst, Src);
    }

    FORCEINLINE void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc) noexcept
    {
        EmplaceCommand<FRHICommandCopyBuffer>(Dst, Src, CopyDesc);
    }

    FORCEINLINE void CopyTexture(FRHITexture* Dst, FRHITexture* Src) noexcept
    {
        EmplaceCommand<FRHICommandCopyTexture>(Dst, Src);
    }

    FORCEINLINE void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyTextureInfo) noexcept
    {
        EmplaceCommand<FRHICommandCopyTextureRegion>(Dst, Src, CopyTextureInfo);
    }

    FORCEINLINE void CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) noexcept
    {
        EmplaceCommand<FRHICommandCopyTextureRegionToBuffer>(Dst, DstOffset, Src, SrcRegion, SrcMipLevel);
    }

    FORCEINLINE void CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice) noexcept
    {
        EmplaceCommand<FRHICommandCopyTextureSubresourceToBuffer>(Dst, DstOffset, Src, SrcRegion, SrcMipLevel, SrcArraySlice);
    }

    FORCEINLINE void WriteFence(FRHIFence* Fence) noexcept
    {
        EmplaceCommand<FRHICommandWriteFence>(Fence);
    }

    FORCEINLINE void DiscardContents(FRHITexture* Texture) noexcept
    {
        EmplaceCommand<FRHICommandDiscardContents>(Texture);
    }

    FORCEINLINE void BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc) noexcept
    {
        EmplaceCommand<FRHICommandBuildSceneAccelerationStructure>(RayTracingScene, BuildDesc);
    }

    FORCEINLINE void BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc) noexcept
    {
        EmplaceCommand<FRHICommandBuildGeometryAccelerationStructure>(RayTracingGeometry, BuildDesc);
    }

    FORCEINLINE void TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition) noexcept
    {
        EmplaceCommand<FRHICommandTransitionTextureState>(Texture, TextureTransition);
    }

    FORCEINLINE void TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) noexcept
    {
        EmplaceCommand<FRHICommandTransitionBufferState>(Buffer, BeforeState, AfterState);
    }

    FORCEINLINE void RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState) noexcept
    {
        EmplaceCommand<FRHICommandRequireTextureState>(Texture, RequiredState);
    }

    FORCEINLINE void RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState) noexcept
    {
        EmplaceCommand<FRHICommandRequireBufferState>(Buffer, RequiredState);
    }

    FORCEINLINE void UnorderedAccessTextureBarrier(FRHITexture* Texture) noexcept
    {
        EmplaceCommand<FRHICommandUnorderedAccessTextureBarrier>(Texture);
    }

    FORCEINLINE void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer) noexcept
    {
        EmplaceCommand<FRHICommandUnorderedAccessBufferBarrier>(Buffer);
    }

    FORCEINLINE void Draw(uint32 VertexCount, uint32 StartVertexLocation) noexcept
    {
        EmplaceCommand<FRHICommandDraw>(VertexCount, StartVertexLocation);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }

    FORCEINLINE void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) noexcept
    {
        EmplaceCommand<FRHICommandDrawIndexed>(IndexCount, StartIndexLocation, BaseVertexLocation);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }

    FORCEINLINE void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) noexcept
    {
        EmplaceCommand<FRHICommandDrawInstanced>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }
     
    FORCEINLINE void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) noexcept
    {
        EmplaceCommand<FRHICommandDrawIndexedInstanced>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }

    FORCEINLINE void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) noexcept
    {
        EmplaceCommand<FRHICommandDispatch>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        STAT_ADD(STAT_RHI_DispatchCalls, 1);
    }

    FORCEINLINE void DispatchMesh(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) noexcept
    {
        EmplaceCommand<FRHICommandDispatchMesh>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }

    FORCEINLINE void DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) noexcept
    {
        EmplaceCommand<FRHICommandDrawIndirect>(ArgumentBuffer, ArgumentBufferOffset, CommandCount);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }

    FORCEINLINE void DrawIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) noexcept
    {
        EmplaceCommand<FRHICommandDrawIndirectCount>(ArgumentBuffer, ArgumentBufferOffset, CountBuffer, CountBufferOffset, MaxCommandCount);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }

    FORCEINLINE void DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) noexcept
    {
        EmplaceCommand<FRHICommandDrawIndexedIndirect>(ArgumentBuffer, ArgumentBufferOffset, CommandCount);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }

    FORCEINLINE void DrawIndexedIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) noexcept
    {
        EmplaceCommand<FRHICommandDrawIndexedIndirectCount>(ArgumentBuffer, ArgumentBufferOffset, CountBuffer, CountBufferOffset, MaxCommandCount);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }

    FORCEINLINE void DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset) noexcept
    {
        EmplaceCommand<FRHICommandDispatchIndirect>(ArgumentBuffer, ArgumentBufferOffset);
        STAT_ADD(STAT_RHI_DispatchCalls, 1);
    }

    FORCEINLINE void DispatchMeshIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount) noexcept
    {
        EmplaceCommand<FRHICommandDispatchMeshIndirect>(ArgumentBuffer, ArgumentBufferOffset, CommandCount);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }

    FORCEINLINE void DispatchMeshIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount) noexcept
    {
        EmplaceCommand<FRHICommandDispatchMeshIndirectCount>(ArgumentBuffer, ArgumentBufferOffset, CountBuffer, CountBufferOffset, MaxCommandCount);
        STAT_ADD(STAT_RHI_DrawCalls, 1);
    }

    FORCEINLINE void SetHitRecordLocalShaderBindings(FRHIShaderBindingTable* ShaderBindingTable, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings) noexcept
    {
        EmplaceCommand<FRHICommandSetHitRecordLocalShaderBindings>(ShaderBindingTable, RecordKind, RecordIndex, Bindings, NumBindings);
    }

    FORCEINLINE void BuildShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable) noexcept
    {
        EmplaceCommand<FRHICommandBuildShaderBindingTable>(ShaderBindingTable);
    }

    FORCEINLINE void ResetShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable) noexcept
    {
        EmplaceCommand<FRHICommandResetShaderBindingTable>(ShaderBindingTable);
    }

    FORCEINLINE void DispatchRaysIndirect(FRHIShaderBindingTable* ShaderBindingTable, FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset) noexcept
    {
        EmplaceCommand<FRHICommandDispatchRaysIndirect>(ShaderBindingTable, ArgumentBuffer, ArgumentBufferOffset);
        STAT_ADD(STAT_RHI_DispatchCalls, 1);
    }

    FORCEINLINE void BuildOpacityMicromap(FRHIOpacityMicromap* OpacityMicromap, const FRHIOpacityMicromapBuildDesc& BuildDesc) noexcept
    {
        EmplaceCommand<FRHICommandBuildOpacityMicromap>(OpacityMicromap, BuildDesc);
    }

    FORCEINLINE void ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* Operations, uint32 NumOperations) noexcept
    {
        EmplaceCommand<FRHICommandExecuteIndirectRayTracingAccelerationStructureOperations>(Operations, NumOperations);
    }

    FORCEINLINE void WriteAccelerationStructurePostBuildInfo(FRHIBuffer* DstBuffer, uint64 DstOffset, EAccelerationStructurePostBuildInfoType InfoType, FRHIRayTracingAccelerationStructure* const* Sources, uint32 NumSources) noexcept
    {
        EmplaceCommand<FRHICommandWriteAccelerationStructurePostBuildInfo>(DstBuffer, DstOffset, InfoType, Sources, NumSources);
    }

    FORCEINLINE void CopyAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIRayTracingAccelerationStructure* Source, EAccelerationStructureCopyMode CopyMode) noexcept
    {
        EmplaceCommand<FRHICommandCopyAccelerationStructure>(Destination, Source, CopyMode);
    }

    FORCEINLINE void CompactAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64 CompactedSizeInBytes) noexcept
    {
        EmplaceCommand<FRHICommandCompactAccelerationStructure>(AccelerationStructure, CompactedSizeInBytes);
    }

    FORCEINLINE void SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Source, FRHIBuffer* DstBuffer, uint64 DstOffset) noexcept
    {
        EmplaceCommand<FRHICommandSerializeAccelerationStructure>(DstBuffer, DstOffset, Source);
    }

    FORCEINLINE void DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIBuffer* SourceBuffer, uint64 SourceOffset) noexcept
    {
        EmplaceCommand<FRHICommandDeserializeAccelerationStructure>(Destination, SourceBuffer, SourceOffset);
    }

    FORCEINLINE void SetRayTracingPipelineState(FRHIRayTracingPipelineState* PipelineState) noexcept
    {
        EmplaceCommand<FRHICommandSetRayTracingPipelineState>(PipelineState);
    }

    FORCEINLINE void DispatchRays(FRHIShaderBindingTable* ShaderBindingTable, uint32 Width, uint32 Height, uint32 Depth) noexcept
    {
        EmplaceCommand<FRHICommandDispatchRaysShaderBindingTable>(ShaderBindingTable, Width, Height, Depth);
        STAT_ADD(STAT_RHI_DispatchCalls, 1);
    }

    FORCEINLINE void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) noexcept
    {
        EmplaceCommand<FRHICommandPresentSwapChain>(SwapChain, bVerticalSync);
    }

    FORCEINLINE void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format = EFormat::Unknown, EColorSpace ColorSpace = EColorSpace::Unknown) noexcept
    {
        EmplaceCommand<FRHICommandResizeSwapChain>(SwapChain, Width, Height, Format, ColorSpace);
    }

    FORCEINLINE void PushEvent(const StringView& Name) noexcept
    {
        StringView AllocatedName = AllocateString(*Name);
        EmplaceCommand<FRHICommandPushEvent>(AllocatedName);
    }

    FORCEINLINE void PopEvent() noexcept
    {
        EmplaceCommand<FRHICommandPopEvent>();
    }
    
    FORCEINLINE void DebugBreak() noexcept
    {
        EmplaceCommand<FRHICommandDebugBreak>();
    }

private:
    FMemoryStack           Memory;
    FRHICommand**          CommandPointer;
    FRHICommand*           FirstCommand;
    IRHICommandContext*    CommandContext;
    FGenericPlatformEvent* FinishedEvent;
    uint32                 NumCommands;
};

struct FRHIScopedEvent
{
    FORCEINLINE FRHIScopedEvent(FRHICommandList& InCommandList, const StringView& Name)
        : CommandList(InCommandList)
    {
        CommandList.PushEvent(Name);
    }

    FORCEINLINE ~FRHIScopedEvent()
    {
        CommandList.PopEvent();
    }

    FRHICommandList& CommandList;
};

#define RHI_EVENT_SCOPE(CommandList, Name) FRHIScopedEvent STRING_CONCAT(RHIScopedEvent_, __LINE__)(CommandList, Name)

void FRHICommandExecuteCommandList::Execute(IRHICommandContext& CommandContext)
{
    CommandList->ExecuteWithContext(CommandContext);
    CommandList->~FRHICommandList();
}

class RHI_API FRHICommandListExecutor : FNonCopyable
{
public:
    static bool Initialize();
    static void Release();

    static FORCEINLINE bool IsInitialized()
    {
        return GCommandListExecutor != nullptr;
    }

    static FORCEINLINE FRHICommandListExecutor& Get()
    {
        return *GCommandListExecutor;
    }

public:

    // Update RHI layer
    void Tick();

    // Wait for all commands that have been scheduled on the RHI-thread to finish
    void WaitForCommands();

    // Wait for all commands to be finished executing on the CPU and the GPU
    void WaitForGPU();

    // Schedule a new  command-list to be executed
    void ExecuteCommandList(class FRHICommandList& CommandList);

    void EnqueueResourceDeletion(FRHIResource* InResource);
    void FlushDeletedResources();

    IRHICommandContext& GetContext()
    {
        CHECK(DefaultCommandContext != nullptr);
        return *DefaultCommandContext;
    }

private:
    FRHICommandListExecutor(IRHICommandContext* InDefaultCommandContext);
    ~FRHICommandListExecutor();

    TArray<FRHIResource*> DeletedResources;
    FCriticalSection      DeletedResourcesCS;
    IRHICommandContext*   DefaultCommandContext;

    static FRHICommandListExecutor* GCommandListExecutor;
};
