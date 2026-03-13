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

struct FRHIRenderTargetView;
struct FRHIDepthStencilView;
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
        FMemory::Memcpy(NewArray, Array.Data(), Array.SizeInBytes());
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
        const int32 Length = FCString::Strlen(String);
        CHAR_T* NewString = reinterpret_cast<CHAR_T*>(Allocate(sizeof(CHAR_T) * Length, alignof(CHAR_T)));
        return FCString::Strcpy(NewString, String);
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

    FORCEINLINE void SetEvent(FGenericEvent* InEvent) noexcept
    {
        CHECK(InEvent != nullptr);
        FinishedEvent = InEvent;
    }

    FORCEINLINE FGenericEvent* GetEvent() const noexcept
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

    FORCEINLINE void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) noexcept
    {
        EmplaceCommand<FRHICommandClearRenderTargetView>(RenderTargetView, ClearColor);
    }

    FORCEINLINE void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil) noexcept
    {
        EmplaceCommand<FRHICommandClearDepthStencilView>(DepthStencilView, Depth, Stencil);
    }

    FORCEINLINE void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) noexcept
    {
        EmplaceCommand<FRHICommandClearUnorderedAccessViewFloat>(UnorderedAccessView, ClearColor);
    }

    FORCEINLINE void ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4]) noexcept
    {
        EmplaceCommand<FRHICommandClearUnorderedAccessViewUint>(UnorderedAccessView, Values);
    }

    FORCEINLINE void BeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo) noexcept
    {
        EmplaceCommand<FRHICommandBeginRenderPass>(BeginRenderPassInfo);
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

    FORCEINLINE void SetBlendFactor(const FVector4& Color) noexcept
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

    FORCEINLINE void SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants) noexcept
    {
        const int32 Size = NumShaderConstants * sizeof(uint32);
        void* SourceData = Allocate(Size, alignof(uint32));
        FMemory::Memcpy(SourceData, ShaderConstants, Size);
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
        FMemory::Memcpy(SrcData, InSrcData, BufferRegion.Size);
        EmplaceCommand<FRHICommandUpdateBuffer>(Dst, BufferRegion, SrcData);
    }

    FORCEINLINE void UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* InSrcData, uint32 SrcRowPitch) noexcept
    {
        const uint32 SizeInBytes = SrcRowPitch * TextureRegion.Height;
        void* SrcData = Allocate(SizeInBytes, alignof(uint8));
        FMemory::Memcpy(SrcData, InSrcData, SizeInBytes);
        EmplaceCommand<FRHICommandUpdateTexture2D>(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch);
    }

    FORCEINLINE void UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* InSrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch) noexcept
    {
        const uint32 SizeInBytes = SrcDepthPitch * TextureRegion.Depth;
        void* SrcData = Allocate(SizeInBytes, alignof(uint8));
        FMemory::Memcpy(SrcData, InSrcData, SizeInBytes);
        EmplaceCommand<FRHICommandUpdateTexture3D>(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch, SrcDepthPitch);
    }

    FORCEINLINE void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) noexcept
    {
        EmplaceCommand<FRHICommandResolveTexture>(Dst, Src);
    }

    FORCEINLINE void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyInfo) noexcept
    {
        EmplaceCommand<FRHICommandCopyBuffer>(Dst, Src, CopyInfo);
    }

    FORCEINLINE void CopyTexture(FRHITexture* Dst, FRHITexture* Src) noexcept
    {
        EmplaceCommand<FRHICommandCopyTexture>(Dst, Src);
    }

    FORCEINLINE void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyTextureInfo) noexcept
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

    FORCEINLINE void WriteFence(FRHIGpuFence* Fence) noexcept
    {
        EmplaceCommand<FRHICommandWriteFence>(Fence);
    }

    FORCEINLINE void DiscardContents(FRHITexture* Texture) noexcept
    {
        EmplaceCommand<FRHICommandDiscardContents>(Texture);
    }

    FORCEINLINE void BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo) noexcept
    {
        EmplaceCommand<FRHICommandBuildRayTracingScene>(RayTracingScene, BuildInfo);
    }

    FORCEINLINE void BuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo) noexcept
    {
        EmplaceCommand<FRHICommandBuildRayTracingGeometry>(RayTracingGeometry, BuildInfo);
    }

    // TODO: Refactor
    FORCEINLINE void SetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources) noexcept
    {
        EmplaceCommand<FRHICommandSetRayTracingBindings>(RayTracingScene, PipelineState, GlobalResource, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
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
        RHIStatistics::NumDrawCalls++;
    }

    FORCEINLINE void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) noexcept
    {
        EmplaceCommand<FRHICommandDrawIndexed>(IndexCount, StartIndexLocation, BaseVertexLocation);
        RHIStatistics::NumDrawCalls++;
    }

    FORCEINLINE void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) noexcept
    {
        EmplaceCommand<FRHICommandDrawInstanced>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        RHIStatistics::NumDrawCalls++;
    }
     
    FORCEINLINE void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) noexcept
    {
        EmplaceCommand<FRHICommandDrawIndexedInstanced>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        RHIStatistics::NumDrawCalls++;
    }

    FORCEINLINE void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) noexcept
    {
        EmplaceCommand<FRHICommandDispatch>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        RHIStatistics::NumDispatchCalls++;
    }

    FORCEINLINE void DispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) noexcept
    {
        EmplaceCommand<FRHICommandDispatchRays>(Scene, PipelineState, Width, Height, Depth);
    }

    FORCEINLINE void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) noexcept
    {
        EmplaceCommand<FRHICommandPresentSwapChain>(SwapChain, bVerticalSync);
    }

    FORCEINLINE void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height) noexcept
    {
        EmplaceCommand<FRHICommandResizeSwapChain>(SwapChain, Width, Height);
    }

    FORCEINLINE void PushEvent(const FStringView& Name) noexcept
    {
        FStringView AllocatedName = AllocateString(*Name);
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
    FMemoryStack        Memory;
    FRHICommand**       CommandPointer;
    FRHICommand*        FirstCommand;
    IRHICommandContext* CommandContext;
    FGenericEvent*      FinishedEvent;
    uint32              NumCommands;
};

struct FRHIScopedEvent
{
    FORCEINLINE FRHIScopedEvent(FRHICommandList& InCommandList, const FStringView& Name)
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

class RHI_API FRHIThread : public FRunnable, FNonCopyable
{
    typedef TQueue<FRHICommandList*, EQueueType::MPSC> FRHIThreadTaskQueue;
    
public:
    FRHIThread();
    ~FRHIThread();

    virtual bool Start() override final;
    virtual int32 Run() override final;
    virtual void Stop() override final;

    bool Startup();
    void Execute(FRHICommandList* InCommandList);
    void WaitForOutstandingTasks();

private:
    FGenericThread*     Thread;
    FRHIThreadTaskQueue Tasks;
    FAtomicInt64        NumSubmittedTasks;
    FAtomicInt64        NumCompletedTasks;
    bool                bIsRunning;
};

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

    bool InitializeRHIThread();
    void ReleaseRHIThread();

    TArray<FRHIResource*> DeletedResources;
    FCriticalSection      DeletedResourcesCS;
    IRHICommandContext*   DefaultCommandContext;
    FRHIThread*           RHIThread;

    static FRHICommandListExecutor* GCommandListExecutor;
};
