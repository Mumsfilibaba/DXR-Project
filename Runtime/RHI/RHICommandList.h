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

struct FRHIRenderTargetView;
struct FRHIDepthStencilView;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHIShader;
class FRHIViewport;

#define ENABLE_INSERT_DEBUG_CMDLIST_MARKER (0)

#if ENABLE_INSERT_DEBUG_CMDLIST_MARKER
    #define INSERT_DEBUG_CMDLIST_MARKER(CommandList, MarkerString) (CommandList).InsertMarker(MarkerString)
#else
    #define INSERT_DEBUG_CMDLIST_MARKER(CommandList, MarkerString)
#endif

class RHI_API FRHICommandList : FNonCopyable
{
public:
    FRHICommandList() noexcept;
    ~FRHICommandList() noexcept;

    void Execute() noexcept;
    void ExecuteWithContext(IRHICommandContext& InCommandContext) noexcept;
    void Reset() noexcept;
    void ExchangeState(FRHICommandList& Other) noexcept;

    void FlushDeletedResources() noexcept;

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
        CommandPointer  = &NewCommand->NextCommand;
        ++NumCommands;
        return NewCommand;
    }

    FORCEINLINE CHAR_T* AllocateString(const CHAR_T* String) noexcept
    {
        int32  Length    = FCString::Strlen(String);
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

public:
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

public:
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

    FORCEINLINE void ClearUnorderedAccessView(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) noexcept
    {
        EmplaceCommand<FRHICommandClearUnorderedAccessViewFloat>(UnorderedAccessView, ClearColor);
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

    FORCEINLINE void SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) noexcept
    {
        TArrayView<FRHIBuffer* const> VertexBuffers = AllocateArray(InVertexBuffers);
        EmplaceCommand<FRHICommandSetVertexBuffers>(VertexBuffers, BufferSlot);
    }

    FORCEINLINE void SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) noexcept
    {
        EmplaceCommand<FRHICommandSetIndexBuffer>(IndexBuffer, IndexFormat);
    }

    FORCEINLINE void SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState) noexcept
    {
        EmplaceCommand<FRHICommandSetGraphicsPipelineState>(PipelineState);
    }

    FORCEINLINE void SetComputePipelineState(FRHIComputePipelineState* PipelineState) noexcept
    {
        EmplaceCommand<FRHICommandSetComputePipelineState>(PipelineState);
    }

    FORCEINLINE void Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) noexcept
    {
        const int32 Size = Num32BitConstants * sizeof(uint32);
        void* SourceData = Allocate(Size, alignof(uint32));
        FMemory::Memcpy(SourceData, Shader32BitConstants, Size);
        EmplaceCommand<FRHICommandSet32BitShaderConstants>(Shader, SourceData, Num32BitConstants);
    }

    FORCEINLINE void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) noexcept
    {
        EmplaceCommand<FRHICommandSetShaderResourceView>(Shader, ShaderResourceView, ParameterIndex);
    }

    FORCEINLINE void SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex) noexcept
    {
        TArrayView<FRHIShaderResourceView* const> ShaderResourceViews = AllocateArray(InShaderResourceViews);
        EmplaceCommand<FRHICommandSetShaderResourceViews>(Shader, ShaderResourceViews, ParameterIndex);
    }

    FORCEINLINE void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) noexcept
    {
        EmplaceCommand<FRHICommandSetUnorderedAccessView>(Shader, UnorderedAccessView, ParameterIndex);
    }

    FORCEINLINE void SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex) noexcept
    {
        TArrayView<FRHIUnorderedAccessView* const> UnorderedAccessViews = AllocateArray(InUnorderedAccessViews);
        EmplaceCommand<FRHICommandSetUnorderedAccessViews>(Shader, UnorderedAccessViews, ParameterIndex);
    }

    FORCEINLINE void SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex) noexcept
    {
        EmplaceCommand<FRHICommandSetConstantBuffer>(Shader, ConstantBuffer, ParameterIndex);
    }

    FORCEINLINE void SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex) noexcept
    {
        TArrayView<FRHIBuffer* const> ConstantBuffers = AllocateArray(InConstantBuffers);
        EmplaceCommand<FRHICommandSetConstantBuffers>(Shader, ConstantBuffers, ParameterIndex);
    }

    FORCEINLINE void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex) noexcept
    {
        EmplaceCommand<FRHICommandSetSamplerState>(Shader, SamplerState, ParameterIndex);
    }

    FORCEINLINE void SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex) noexcept
    {
        TArrayView<FRHISamplerState* const> SamplerStates = AllocateArray(InSamplerStates);
        EmplaceCommand<FRHICommandSetSamplerStates>(Shader, SamplerStates, ParameterIndex);
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

    FORCEINLINE void GenerateMips(FRHITexture* Texture) noexcept
    {
        EmplaceCommand<FRHICommandGenerateMips>(Texture);
    }

    FORCEINLINE void TransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) noexcept
    {
        if (BeforeState != AfterState)
        {
            EmplaceCommand<FRHICommandTransitionTexture>(Texture, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '%s' Was transitioned with the same Before- and AfterState (=%s)", *Texture->GetDebugName(),  ToString(BeforeState));
        }
    }

    FORCEINLINE void TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) noexcept
    {
        if (BeforeState != AfterState)
        {
            EmplaceCommand<FRHICommandTransitionBuffer>(Buffer, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '%s' Was transitioned with the same Before- and AfterState (=%s)", *Buffer->GetDebugName(),  ToString(BeforeState));
        }
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
        GRHINumDrawCalls++;
    }

    FORCEINLINE void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) noexcept
    {
        EmplaceCommand<FRHICommandDrawIndexed>(IndexCount, StartIndexLocation, BaseVertexLocation);
        GRHINumDrawCalls++;
    }

    FORCEINLINE void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) noexcept
    {
        EmplaceCommand<FRHICommandDrawInstanced>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        GRHINumDrawCalls++;
    }
     
    FORCEINLINE void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) noexcept
    {
        EmplaceCommand<FRHICommandDrawIndexedInstanced>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        GRHINumDrawCalls++;
    }

    FORCEINLINE void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) noexcept
    {
        EmplaceCommand<FRHICommandDispatch>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        GRHINumDrawCalls++;
    }

    FORCEINLINE void DispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) noexcept
    {
        EmplaceCommand<FRHICommandDispatchRays>(Scene, PipelineState, Width, Height, Depth);
    }

    FORCEINLINE void PresentViewport(FRHIViewport* Viewport, bool bVerticalSync) noexcept
    {
        EmplaceCommand<FRHICommandPresentViewport>(Viewport, bVerticalSync);
    }

    FORCEINLINE void ResizeViewport(FRHIViewport* Viewport, uint32 Width, uint32 Height) noexcept
    {
        EmplaceCommand<FRHICommandResizeViewport>(Viewport, Width, Height);
    }

    FORCEINLINE void InsertMarker(const FStringView& Marker) noexcept
    {
        FStringView NewMarker = AllocateString(*Marker);
        EmplaceCommand<FRHICommandInsertMarker>(NewMarker);
    }
    
    FORCEINLINE void DebugBreak() noexcept
    {
        EmplaceCommand<FRHICommandDebugBreak>();
    }

    FORCEINLINE void BeginExternalCapture() noexcept
    {
        EmplaceCommand<FRHICommandBeginExternalCapture>();
    }

    FORCEINLINE void EndExternalCapture() noexcept
    {
        EmplaceCommand<FRHICommandEndExternalCapture>();
    }

private:
    FMemoryStack        Memory;
    FRHICommand**       CommandPointer; // NOTE: Pointer to FirstCommand to avoid branching
    FRHICommand*        FirstCommand;
    IRHICommandContext* CommandContext;
    FGenericEvent*      FinishedEvent;
    uint32              NumCommands;
};

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
    FRHICommandListExecutor();
    ~FRHICommandListExecutor();

    bool Initialize();
    void Release();

    void Tick();

    void WaitForCommands();
    void WaitForGPU();
    void ExecuteCommandList(class FRHICommandList& CommandList);

    void EnqueueResourceDeletion(FRHIResource* InResource);
    void FlushDeletedResources();

    void SetContext(IRHICommandContext* InCmdContext) 
    { 
        CommandContext = InCmdContext; 
    }

    IRHICommandContext& GetContext()
    {
        CHECK(CommandContext != nullptr);
        return *CommandContext;
    }

private:
    TArray<FRHIResource*> DeletedResources;
    FCriticalSection      DeletedResourcesCS;
    IRHICommandContext*   CommandContext;
    FRHIThread*           RHIThread;
};

extern RHI_API FRHICommandListExecutor GRHICommandExecutor;
