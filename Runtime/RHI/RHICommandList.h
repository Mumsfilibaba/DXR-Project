#pragma once
#include "RHI.h"
#include "RHIResources.h"
#include "RHICommands.h"
#include "RHIRayTracing.h"
#include "Core/Memory/MemoryStack.h"
#include "Core/Threading/ThreadInterface.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Platform/ConditionVariable.h"
#include "Core/Containers/ArrayView.h"

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

struct FRHICommandStatistics
{
    FORCEINLINE FRHICommandStatistics()
        : NumDrawCalls(0)
        , NumDispatchCalls(0)
        , NumCommands(0)
    {
    }

    FORCEINLINE void Reset()
    {
        NumDrawCalls     = 0;
        NumDispatchCalls = 0;
        NumCommands      = 0;
    }

    uint32 NumDrawCalls;
    uint32 NumDispatchCalls;
    uint32 NumCommands;
};

class RHI_API FRHICommandList : FNonCopyable
{
public:
    FRHICommandList() noexcept
        : Memory()
        , CommandPointer(nullptr)
        , FirstCommand(nullptr)
        , CommandContext(nullptr)
        , Statistics()
        , NumCommands(0)
        , bIsRenderPassActive(false)
    {
        CommandPointer = &FirstCommand;
    }

    ~FRHICommandList() noexcept
    {
        Reset();
    }

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

    FORCEINLINE TCHAR* AllocateString(const TCHAR* String) noexcept
    {
        int32  Length    = FCString::Strlen(String);
        TCHAR* NewString = reinterpret_cast<TCHAR*>(Allocate(sizeof(TCHAR) * Length, alignof(TCHAR)));
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

    FORCEINLINE void Execute() noexcept
    {
        IRHICommandContext& CommandContextRef = GetCommandContext();
        CommandContextRef.RHIStartContext();
        ExecuteWithContext(CommandContextRef);
        CommandContextRef.RHIFinishContext();
    }

    FORCEINLINE void ExecuteWithContext(IRHICommandContext& InCommandContext) noexcept
    {
        FRHICommand* CurrentCommand = FirstCommand;
        while (CurrentCommand != nullptr)
        {
            FRHICommand* PreviousCommand = CurrentCommand;
            CurrentCommand = CurrentCommand->NextCommand;
            PreviousCommand->ExecuteAndRelease(InCommandContext);
        }

        FirstCommand = nullptr;
        Reset();
    }

    FORCEINLINE void Reset() noexcept
    {
        if (FirstCommand != nullptr)
        {
            // Call destructor on all commands that has not been executed
            FRHICommand* Command = FirstCommand;
            while (Command != nullptr)
            {
                FRHICommand* PreviousCommand = Command;
                Command = Command->NextCommand;
                PreviousCommand->~FRHICommand();
            }

            FirstCommand = nullptr;
        }

        CommandPointer = &FirstCommand;
        CommandContext = nullptr;
        NumCommands    = 0;

        bIsRenderPassActive = false;

        Statistics.Reset();
        Memory.Reset();
    }

    FORCEINLINE void ExchangeState(FRHICommandList& Other) noexcept
    {
        // This works fine in this case
        FMemory::Memswap(this, &Other, sizeof(FRHICommandList));

        if (CommandPointer == &Other.FirstCommand)
        {
            CommandPointer = &FirstCommand;
        }

        if (Other.CommandPointer == &FirstCommand)
        {
            Other.CommandPointer = &Other.FirstCommand;
        }
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

    FORCEINLINE bool HasCommands() const noexcept
    {
        return NumCommands > 0;
    }

    FORCEINLINE uint32 GetNumDrawCalls() const noexcept
    {
        return Statistics.NumDrawCalls;
    }

    FORCEINLINE uint32 GetNumDispatchCalls() const noexcept
    {
        return Statistics.NumDispatchCalls;
    }

    FORCEINLINE uint32 GetNumCommands() const noexcept
    {
        return NumCommands;
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

    FORCEINLINE void BeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index) noexcept
    {
        EmplaceCommand<FRHICommandBeginTimeStamp>(TimestampQuery, Index);
    }

    FORCEINLINE void EndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index) noexcept
    {
        EmplaceCommand<FRHICommandEndTimeStamp>(TimestampQuery, Index);
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

    FORCEINLINE void BeginRenderPass(const FRHIRenderPassDesc& RenderPassInitializer) noexcept
    {
        CHECK(bIsRenderPassActive == false);
        EmplaceCommand<FRHICommandBeginRenderPass>(RenderPassInitializer);
        bIsRenderPassActive = true;
    }

    FORCEINLINE void EndRenderPass() noexcept
    {
        CHECK(bIsRenderPassActive == true);
        EmplaceCommand<FRHICommandEndRenderPass>();
        bIsRenderPassActive = false;
    }

    FORCEINLINE void SetViewport(const FRHIViewportRegion& ViewportRegion) noexcept
    {
        EmplaceCommand<FRHICommandSetViewport>(ViewportRegion);
    }

    FORCEINLINE void SetScissorRect(const FRHIScissorRegion& ScissorRegion) noexcept
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
        CHECK(Num32BitConstants <= FRHILimits::MaxShaderConstants);

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

    FORCEINLINE void UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* InSrcData, uint32 InSrcRowPitch) noexcept
    {
        const uint32 SizeInBytes = InSrcRowPitch * TextureRegion.Height;
        void* SrcData = Allocate(SizeInBytes, alignof(uint8));
        FMemory::Memcpy(SrcData, InSrcData, SizeInBytes);
        EmplaceCommand<FRHICommandUpdateTexture2D>(Dst, TextureRegion, MipLevel, SrcData, InSrcRowPitch);
    }

    FORCEINLINE void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) noexcept
    {
        EmplaceCommand<FRHICommandResolveTexture>(Dst, Src);
    }

    FORCEINLINE void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyInfo) noexcept
    {
        EmplaceCommand<FRHICommandCopyBuffer>(Dst, Src, CopyInfo);
    }

    FORCEINLINE void CopyTexture(FRHITexture* Dst, FRHITexture* Src) noexcept
    {
        EmplaceCommand<FRHICommandCopyTexture>(Dst, Src);
    }

    FORCEINLINE void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyTextureInfo) noexcept
    {
        EmplaceCommand<FRHICommandCopyTextureRegion>(Dst, Src, CopyTextureInfo);
    }

    FORCEINLINE void DestroyResource(FRHIResource* Resource) noexcept
    {
        EmplaceCommand<FRHICommandDestroyResource>(Resource);
    }

    FORCEINLINE void DiscardContents(FRHITexture* Texture) noexcept
    {
        EmplaceCommand<FRHICommandDiscardContents>(Texture);
    }

    FORCEINLINE void BuildRayTracingGeometry(
        FRHIRayTracingGeometry* RayTracingGeometry,
        FRHIBuffer*             VertexBuffer,
        uint32                  NumVertices,
        FRHIBuffer*             IndexBuffer,
        uint32                  NumIndices,
        EIndexFormat            IndexFormat,
        bool                    bUpdate) noexcept
    {
        CHECK(!bUpdate || (bUpdate && RayTracingGeometry && IsEnumFlagSet(RayTracingGeometry->GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate)));
        EmplaceCommand<FRHICommandBuildRayTracingGeometry>(RayTracingGeometry, VertexBuffer, NumVertices, IndexBuffer, NumIndices, IndexFormat, bUpdate);
    }

    FORCEINLINE void BuildRayTracingScene(FRHIRayTracingScene* Scene, const TArrayView<const FRHIRayTracingGeometryInstance> Instances, bool bUpdate) noexcept
    {
        CHECK(!bUpdate || (bUpdate && Scene && IsEnumFlagSet(Scene->GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate)));
        EmplaceCommand<FRHICommandBuildRayTracingScene>(Scene, Instances, bUpdate);
    }

    // TODO: Refactor
    FORCEINLINE void SetRayTracingBindings(
        FRHIRayTracingScene*              RayTracingScene,
        FRHIRayTracingPipelineState*      PipelineState,
        const FRayTracingShaderResources* GlobalResource,
        const FRayTracingShaderResources* RayGenLocalResources,
        const FRayTracingShaderResources* MissLocalResources,
        const FRayTracingShaderResources* HitGroupResources,
        uint32                            NumHitGroupResources) noexcept
    {
        EmplaceCommand<FRHICommandSetRayTracingBindings>(RayTracingScene, PipelineState, GlobalResource, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
    }

    FORCEINLINE void GenerateMips(FRHITexture* Texture) noexcept
    {
        CHECK(Texture != nullptr);
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
            LOG_WARNING("Texture '%s' Was transitioned with the same Before- and AfterState (=%s)", Texture->GetName().GetCString(),  ToString(BeforeState));
        }
    }

    FORCEINLINE void TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) noexcept
    {
        CHECK(Buffer != nullptr);

        if (BeforeState != AfterState)
        {
            EmplaceCommand<FRHICommandTransitionBuffer>(Buffer, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '%s' Was transitioned with the same Before- and AfterState (=%s)", Buffer->GetName().GetCString(),  ToString(BeforeState));
        }
    }

    FORCEINLINE void UnorderedAccessTextureBarrier(FRHITexture* Texture) noexcept
    {
        CHECK(Texture != nullptr);
        EmplaceCommand<FRHICommandUnorderedAccessTextureBarrier>(Texture);
    }

    FORCEINLINE void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer) noexcept
    {
        CHECK(Buffer != nullptr);
        EmplaceCommand<FRHICommandUnorderedAccessBufferBarrier>(Buffer);
    }

    FORCEINLINE void Draw(uint32 VertexCount, uint32 StartVertexLocation) noexcept
    {
        CHECK(VertexCount > 0);
        EmplaceCommand<FRHICommandDraw>(VertexCount, StartVertexLocation);
        Statistics.NumDrawCalls++;
    }

    FORCEINLINE void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) noexcept
    {
        CHECK(IndexCount > 0);
        EmplaceCommand<FRHICommandDrawIndexed>(IndexCount, StartIndexLocation, BaseVertexLocation);
        Statistics.NumDrawCalls++;
    }

    FORCEINLINE void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) noexcept
    {
        CHECK(VertexCountPerInstance > 0 && InstanceCount > 0);
        EmplaceCommand<FRHICommandDrawInstanced>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        Statistics.NumDrawCalls++;
    }
     
    FORCEINLINE void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) noexcept
    {
        CHECK(IndexCountPerInstance > 0 && InstanceCount > 0);
        EmplaceCommand<FRHICommandDrawIndexedInstanced>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        Statistics.NumDrawCalls++;
    }

    FORCEINLINE void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) noexcept
    {
        CHECK(ThreadGroupCountX > 0 || ThreadGroupCountY > 0 || ThreadGroupCountZ > 0);
        EmplaceCommand<FRHICommandDispatch>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        Statistics.NumDispatchCalls++;
    }

    FORCEINLINE void DispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) noexcept
    {
        CHECK(Width > 0 || Height > 0 || Depth > 0);
        EmplaceCommand<FRHICommandDispatchRays>(Scene, PipelineState, Width, Height, Depth);
    }

    FORCEINLINE void PresentViewport(FRHIViewport* Viewport, bool bVerticalSync) noexcept
    {
        CHECK(Viewport != nullptr);
        EmplaceCommand<FRHICommandPresentViewport>(Viewport, bVerticalSync);
    }

    FORCEINLINE void ResizeViewport(FRHIViewport* Viewport, uint32 Width, uint32 Height) noexcept
    {
        CHECK(Viewport != nullptr);
        EmplaceCommand<FRHICommandResizeViewport>(Viewport, Width, Height);
    }

    FORCEINLINE void InsertMarker(const FStringView& Marker) noexcept
    {
        FStringView NewMarker = AllocateString(Marker.GetCString());
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
    FMemoryStack          Memory;

    /** @brief - pointer to FirstCommand to avoid branching */
    FRHICommand**         CommandPointer;
    FRHICommand*          FirstCommand;
    IRHICommandContext*   CommandContext;

    FRHICommandStatistics Statistics;
    uint32                NumCommands;

    bool                  bIsRenderPassActive = false;
};


void FRHICommandExecuteCommandList::Execute(IRHICommandContext& CommandContext)
{
    CommandList->ExecuteWithContext(CommandContext);
    CommandList->~FRHICommandList();
}


struct FRHIThreadTask : FNonCopyable
{
    FORCEINLINE FRHIThreadTask() noexcept
        : CommandList(nullptr)
    {
    }

    FORCEINLINE explicit FRHIThreadTask(FRHICommandList* InCommandList) noexcept
        : CommandList(InCommandList)
    {
    }

    FORCEINLINE FRHIThreadTask(FRHIThreadTask&& Other) noexcept
        : CommandList(Other.CommandList)
    {
        Other.CommandList = nullptr;
    }

    FORCEINLINE ~FRHIThreadTask() noexcept
    {
        SAFE_DELETE(CommandList);
    }

    FORCEINLINE FRHIThreadTask& operator=(FRHIThreadTask&& RHS) noexcept
    {
        CommandList = RHS.CommandList;
        RHS.CommandList = nullptr;
        return *this;
    }

    FORCEINLINE operator bool() const noexcept
    {
        return (CommandList != nullptr);
    }

    FRHICommandList* CommandList;
};

class RHI_API FRHIThread : public FThreadInterface, FNonCopyable
{
public:
    static bool Startup();
    static void Shutdown();

    static bool IsRunning()
    { 
        return GInstance != nullptr;
    }

    static FRHIThread& Get();

    virtual bool Start() override final;

    virtual int32 Run() override final;

    virtual void Stop() override final;

    void Execute(FRHIThreadTask&& NewTask);

    void WaitForOutstandingTasks();

private:
    FRHIThread();
    ~FRHIThread() = default;

    bool Create();

    TSharedRef<FGenericThread> Thread;

    FCriticalSection   WaitCS;
    FConditionVariable WaitCondition;

    FAtomicInt64 NumSubmittedTasks;
    FAtomicInt64 NumCompletedTasks;

    TArray<FRHIThreadTask> Tasks;
    FCriticalSection TasksCS;

    bool bIsRunning;

    static FRHIThread* GInstance;
};

class RHI_API FRHICommandListExecutor : FNonCopyable
{
public:
    FRHICommandListExecutor();
    ~FRHICommandListExecutor() = default;

    bool Initialize();
    void Release();

    void Tick();

    void WaitForOutstandingTasks();
    void WaitForGPU();

    void ExecuteCommandList(class FRHICommandList& CmdList);

    FORCEINLINE void SetContext(IRHICommandContext* InCmdContext) 
    { 
        CommandContext = InCmdContext; 
    }

    FORCEINLINE IRHICommandContext& GetContext()
    {
        CHECK(CommandContext != nullptr);
        return *CommandContext;
    }

    FORCEINLINE const FRHICommandStatistics& GetStatistics() const 
    { 
        return Statistics; 
    }

private:
    FRHICommandStatistics Statistics;
    IRHICommandContext* CommandContext;
};

extern RHI_API FRHICommandListExecutor GRHICommandExecutor;
