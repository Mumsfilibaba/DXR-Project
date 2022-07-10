#pragma once
#include "RHIModule.h"
#include "RHIResources.h"
#include "RHICommands.h"
#include "RHITimestampQuery.h"

#include "Core/Threading/ThreadManager.h"
#include "Core/Threading/Platform/ConditionVariable.h"

class FRHIRenderTargetView;
class FRHIDepthStencilView;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHIShader;

#define ENABLE_INSERT_DEBUG_CMDLIST_MARKER (0)
#define ENABLE_RHI_EXECUTOR_THREAD         (1)

#if ENABLE_INSERT_DEBUG_CMDLIST_MARKER
    #define INSERT_DEBUG_CMDLIST_MARKER(CmdList, MarkerString) CmdList.InsertMarker(MarkerString);
#else
    #define INSERT_DEBUG_CMDLIST_MARKER(CmdList, MarkerString)
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandAllocator

class RHI_API FRHICommandAllocator
{
public:

    FRHICommandAllocator(uint32 StartSize = 4096);
    ~FRHICommandAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment = STANDARD_ALIGNMENT);
    void  Reset();

    template<typename T>
    FORCEINLINE T* Allocate()
    {
        return reinterpret_cast<T*>(Allocate(sizeof(T), alignof(T)));
    }

    template<typename T>
    FORCEINLINE T* Allocate(uint32 NumElements)
    {
        return reinterpret_cast<T*>(Allocate(sizeof(T) * NumElements, alignof(T)));
    }

    template<typename T, typename... ArgTypes>
    FORCEINLINE T* Construct(ArgTypes&&... Args)
    {
        void* Memory = Allocate<T>();
        return new(Memory) T(Forward<ArgTypes>(Args)...);
    }

    FORCEINLINE uint8* AllocateBytes(uint64 SizeInBytes, uint64 Alignment = STANDARD_ALIGNMENT)
    {
        return reinterpret_cast<uint8*>(Allocate(SizeInBytes, Alignment));
    }

private:

    void ReleaseDiscardedMemory();

    uint8* CurrentMemory;

    uint64 Size;
    uint64 Offset;
    uint64 AverageMemoryUsage;

    TArray<uint8*> DiscardedMemory;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandStatistics

struct FRHICommandStatistics
{
    FRHICommandStatistics()
        : NumDrawCalls(0)
        , NumDispatchCalls(0)
        , NumCommands(0)
    { }

    void Reset()
    {
        NumDrawCalls     = 0;
        NumDispatchCalls = 0;
        NumCommands      = 0;
    }

    uint32 NumDrawCalls;
    uint32 NumDispatchCalls;
    uint32 NumCommands;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandList

class RHI_API FRHICommandList : public FNonCopyable
{
    friend class FRHICommandListExecutor;

public:

    FRHICommandList()
        : Allocator(nullptr)
        , FirstCommand(nullptr)
        , LastCommand(nullptr)
	    , Statistics()
    {
        Allocator = dbg_new FRHICommandAllocator();
    }

    ~FRHICommandList()
    {
        Reset();

        SafeDelete(Allocator);
    }

    void BeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
    {
        InsertCommand<FRHICommandBeginTimeStamp>(TimestampQuery, Index);
    }

    void EndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
    {
        InsertCommand<FRHICommandEndTimeStamp>(TimestampQuery, Index);
    }

    void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const TStaticArray<float, 4>& ClearColor)
    {
        Check(RenderTargetView.Texture != nullptr);
        InsertCommand<FRHICommandClearRenderTargetView>(RenderTargetView, ClearColor);
    }

    void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
    {
        Check(DepthStencilView.Texture != nullptr);
        InsertCommand<FRHICommandClearDepthStencilView>(DepthStencilView, Depth, Stencil);
    }

    void ClearUnorderedAccessView(FRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor)
    {
        Check(UnorderedAccessView != nullptr);
        InsertCommand<FRHICommandClearUnorderedAccessViewFloat>(UnorderedAccessView, ClearColor);
    }

    void BeginRenderPass(const FRHIRenderPassInitializer& RenderPassInitializer)
    {
        Check(bIsRenderPassActive == false);

        InsertCommand<FRHICommandBeginRenderPass>(RenderPassInitializer);
        bIsRenderPassActive = true;
    }

    void EndRenderPass()
    {
        Check(bIsRenderPassActive == true);

        InsertCommand<FRHICommandEndRenderPass>();
        bIsRenderPassActive = false;
    }

    void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
    {
        InsertCommand<FRHICommandSetViewport>(Width, Height, MinDepth, MaxDepth, x, y);
    }

    void SetScissorRect(float Width, float Height, float x, float y)
    {
        InsertCommand<FRHICommandSetScissorRect>(Width, Height, x, y);
    }

    void SetBlendFactor(const TStaticArray<float, 4>& Color)
    {
        InsertCommand<FRHICommandSetBlendFactor>(Color);
    }

    void SetVertexBuffers(FRHIVertexBuffer* const* InVertexBuffers, uint32 NumVertexBuffers, uint32 BufferSlot)
    {
        Check(Allocator != nullptr);

        FRHIVertexBuffer** TempVertexBuffers = Allocator->Allocate<FRHIVertexBuffer*>(NumVertexBuffers);
        if (InVertexBuffers)
        {
            FMemory::MemcpyTyped(TempVertexBuffers, InVertexBuffers, NumVertexBuffers);
        }
        else
        {
            FMemory::Memzero(TempVertexBuffers, NumVertexBuffers);
        }

        InsertCommand<FRHICommandSetVertexBuffers>(TempVertexBuffers, NumVertexBuffers, BufferSlot);
    }

    void SetIndexBuffer(FRHIIndexBuffer* IndexBuffer)
    {
        InsertCommand<FRHICommandSetIndexBuffer>(IndexBuffer);
    }

    void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
    {
        InsertCommand<FRHICommandSetPrimitiveTopology>(PrimitveTopologyType);
    }

    void SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
    {
        InsertCommand<FRHICommandSetGraphicsPipelineState>(PipelineState);
    }

    void SetComputePipelineState(FRHIComputePipelineState* PipelineState)
    {
        InsertCommand<FRHICommandSetComputePipelineState>(PipelineState);
    }

    void Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
    {
        Check(Allocator != nullptr);

        const uint32 Num32BitConstantsInBytes = Num32BitConstants * 4;

        void* Shader32BitConstantsMemory = Allocator->Allocate(Num32BitConstantsInBytes, 1);
        FMemory::Memcpy(Shader32BitConstantsMemory, Shader32BitConstants, Num32BitConstantsInBytes);

        InsertCommand<FRHICommandSet32BitShaderConstants>(Shader, Shader32BitConstantsMemory, Num32BitConstants);
    }

    void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
    {
        InsertCommand<FRHICommandSetShaderResourceView>(Shader, ShaderResourceView, ParameterIndex);
    }

    void SetShaderResourceViews(FRHIShader* Shader, FRHIShaderResourceView* const* ShaderResourceViews, uint32 NumShaderResourceViews, uint32 ParameterIndex)
    {
        Check(Allocator != nullptr);

        FRHIShaderResourceView** TempShaderResourceViews = Allocator->Allocate<FRHIShaderResourceView*>(NumShaderResourceViews);
        if (ShaderResourceViews)
        {
            FMemory::MemcpyTyped(TempShaderResourceViews, ShaderResourceViews, NumShaderResourceViews);
        }
        else
        {
            FMemory::Memzero(TempShaderResourceViews, NumShaderResourceViews);
        }

        InsertCommand<FRHICommandSetShaderResourceViews>(Shader, TempShaderResourceViews, NumShaderResourceViews, ParameterIndex);
    }

    void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
    {
        InsertCommand<FRHICommandSetUnorderedAccessView>(Shader, UnorderedAccessView, ParameterIndex);
    }

    void SetUnorderedAccessViews(FRHIShader* Shader, FRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
    {
        Check(Allocator != nullptr);

        FRHIUnorderedAccessView** TempUnorderedAccessViews = Allocator->Allocate<FRHIUnorderedAccessView*>(NumUnorderedAccessViews);
        if (UnorderedAccessViews)
        {
            FMemory::MemcpyTyped(TempUnorderedAccessViews, UnorderedAccessViews, NumUnorderedAccessViews);
        }
        else
        {
            FMemory::Memzero(TempUnorderedAccessViews, NumUnorderedAccessViews);
        }

        InsertCommand<FRHICommandSetUnorderedAccessViews>(Shader, TempUnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex);
    }

    void SetConstantBuffer(FRHIShader* Shader, FRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
    {
        InsertCommand<FRHICommandSetConstantBuffer>(Shader, ConstantBuffer, ParameterIndex);
    }

    void SetConstantBuffers(FRHIShader* Shader, FRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
    {
        Check(Allocator != nullptr);

        FRHIConstantBuffer** TempConstantBuffers = Allocator->Allocate<FRHIConstantBuffer*>(NumConstantBuffers);
        if (ConstantBuffers)
        {
            FMemory::MemcpyTyped(TempConstantBuffers, ConstantBuffers, NumConstantBuffers);
        }
        else
        {
            FMemory::Memzero(TempConstantBuffers, NumConstantBuffers);
        }

        InsertCommand<FRHICommandSetConstantBuffers>(Shader, TempConstantBuffers, NumConstantBuffers, ParameterIndex);
    }

    void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
    {
        InsertCommand<FRHICommandSetSamplerState>(Shader, SamplerState, ParameterIndex);
    }

    void SetSamplerStates(FRHIShader* Shader, FRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
    {
        Check(Allocator != nullptr);

        FRHISamplerState** TempSamplerStates = Allocator->Allocate<FRHISamplerState*>(NumSamplerStates);
        if (SamplerStates)
        {
            FMemory::MemcpyTyped(TempSamplerStates, SamplerStates, NumSamplerStates);
        }
        else
        {
            FMemory::Memzero(TempSamplerStates, NumSamplerStates);
        }

        InsertCommand<FRHICommandSetSamplerStates>(Shader, TempSamplerStates, NumSamplerStates, ParameterIndex);
    }

    void UpdateBuffer(FRHIBuffer* Dst, uint32 DestinationOffsetInBytes, uint32 SizeInBytes, const void* SourceData)
    {
        Check(Allocator != nullptr);

        void* TempSourceData = Allocator->Allocate(SizeInBytes);
        FMemory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        InsertCommand<FRHICommandUpdateBuffer>(Dst, DestinationOffsetInBytes, SizeInBytes, TempSourceData);
    }

    void UpdateTexture2D(FRHITexture2D* Dst, uint16 Width, uint16 Height, uint16 MipLevel, const void* SourceData)
    {
        Check(Allocator != nullptr);

        const uint32 SizeInBytes = Width * Height * GetByteStrideFromFormat(Dst->GetFormat());

        void* TempSourceData = Allocator->Allocate(SizeInBytes);
        FMemory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        InsertCommand<FRHICommandUpdateTexture2D>(Dst, Width, Height, MipLevel, TempSourceData);
    }

    void ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
    {
        InsertCommand<FRHICommandResolveTexture>(Dst, Src);
    }

    void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHICopyBufferInfo& CopyInfo)
    {
        InsertCommand<FRHICommandCopyBuffer>(Dst, Src, CopyInfo);
    }

    void CopyTexture(FRHITexture* Dst, FRHITexture* Src)
    {
        InsertCommand<FRHICommandCopyTexture>(Dst, Src);
    }

    void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHICopyTextureInfo& CopyTextureInfo)
    {
        InsertCommand<FRHICommandCopyTextureRegion>(Dst, Src, CopyTextureInfo);
    }

    void DestroyResource(IRefCounted* Resource)
    {
        InsertCommand<FRHICommandDestroyResource>(Resource);
    }

    void DiscardContents(FRHITexture* Texture)
    {
        InsertCommand<FRHICommandDiscardContents>(Texture);
    }

    void BuildRayTracingGeometry(FRHIRayTracingGeometry* Geometry, FRHIVertexBuffer* VertexBuffer, FRHIIndexBuffer* IndexBuffer, bool bUpdate)
    {
        Check((Geometry != nullptr) && (!bUpdate || (bUpdate && (Geometry->GetFlags() & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None)));
        InsertCommand<FRHICommandBuildRayTracingGeometry>(Geometry, VertexBuffer, IndexBuffer, bUpdate);
    }

    void BuildRayTracingScene(FRHIRayTracingScene* Scene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
    {
        Check((Scene != nullptr) && (!bUpdate || (bUpdate && (Scene->GetFlags() & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None)));
        InsertCommand<FRHICommandBuildRayTracingScene>(Scene, Instances, bUpdate);
    }

    // TODO: Refactor
    void SetRayTracingBindings( FRHIRayTracingScene* RayTracingScene
                              , FRHIRayTracingPipelineState* PipelineState
                              , const FRayTracingShaderResources* GlobalResource
                              , const FRayTracingShaderResources* RayGenLocalResources
                              , const FRayTracingShaderResources* MissLocalResources
                              , const FRayTracingShaderResources* HitGroupResources
                              , uint32 NumHitGroupResources)
    {
        InsertCommand<FRHICommandSetRayTracingBindings>(RayTracingScene, PipelineState, GlobalResource, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
    }

    void GenerateMips(FRHITexture* Texture)
    {
        Check(Texture != nullptr);
        InsertCommand<FRHICommandGenerateMips>(Texture);
    }

    void TransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
    {
        if (BeforeState != AfterState)
        {
            InsertCommand<FRHICommandTransitionTexture>(Texture, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '%s' Was transitioned with the same Before- and AfterState (=%s)", Texture->GetName().CStr(),  ToString(BeforeState));
        }
    }

    void TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
    {
        Check(Buffer != nullptr);

        if (BeforeState != AfterState)
        {
            InsertCommand<FRHICommandTransitionBuffer>(Buffer, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '%s' Was transitioned with the same Before- and AfterState (=%s)", Buffer->GetName().CStr(),  ToString(BeforeState));
        }
    }

    void UnorderedAccessTextureBarrier(FRHITexture* Texture)
    {
        Check(Texture != nullptr);
        InsertCommand<FRHICommandUnorderedAccessTextureBarrier>(Texture);
    }

    void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
    {
        Check(Buffer != nullptr);
        InsertCommand<FRHICommandUnorderedAccessBufferBarrier>(Buffer);
    }

    void Draw(uint32 VertexCount, uint32 StartVertexLocation)
    {
        if (VertexCount > 0)
        {
            InsertCommand<FRHICommandDraw>(VertexCount, StartVertexLocation);
            Statistics.NumDrawCalls++;
        }
    }

    void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
    {
        if (IndexCount > 0)
        {
            InsertCommand<FRHICommandDrawIndexed>(IndexCount, StartIndexLocation, BaseVertexLocation);
            Statistics.NumDrawCalls++;
        }
    }

    void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
    {
        if ((VertexCountPerInstance > 0) && (InstanceCount > 0))
        {
            InsertCommand<FRHICommandDrawInstanced>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
            Statistics.NumDrawCalls++;
        }
    }

    void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
    {
        if ((IndexCountPerInstance > 0) && (InstanceCount > 0))
        {
            InsertCommand<FRHICommandDrawIndexedInstanced>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
            Statistics.NumDrawCalls++;
        }
    }

    void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
    {
        if ((ThreadGroupCountX > 0) || (ThreadGroupCountY > 0) || (ThreadGroupCountZ > 0))
        {
            InsertCommand<FRHICommandDispatch>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
            Statistics.NumDispatchCalls++;
        }
    }

    void DispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
    {
        if ((Width > 0) || (Height > 0) || (Depth > 0))
        {
            InsertCommand<FRHICommandDispatchRays>(Scene, PipelineState, Width, Height, Depth);
        }
    }

    void InsertMarker(const FString& Marker)
    {
        InsertCommand<FRHICommandInsertMarker>(Marker);
    }
    
    void DebugBreak()
    {
        InsertCommand<FRHICommandDebugBreak>();
    }

    void BeginExternalCapture()
    {
        InsertCommand<FRHICommandBeginExternalCapture>();
    }

    void EndExternalCapture()
    {
        InsertCommand<FRHICommandEndExternalCapture>();
    }

    void Reset()
    {
        if (FirstCommand != nullptr)
        {
            FRHICommand* Command = FirstCommand;
            while (Command != nullptr)
            {
                FRHICommand* PreviousCommand = Command;
                Command = Command->NextCommand;
                PreviousCommand->~FRHICommand();
            }

            FirstCommand = nullptr;
            LastCommand  = nullptr;
        }

        Statistics.Reset();

        if (Allocator)
        {
            Allocator->Reset();
        }

        bIsRenderPassActive = false;
    }

    void Swap(FRHICommandList& Other)
    {
        FRHICommandAllocator* TempAllocator           = Allocator;
        FRHICommand*          TempFirstCommand        = FirstCommand;
        FRHICommand*          TempLastCommand         = LastCommand;
        FRHICommandStatistics TempStatistics          = Statistics;
        bool                  bTempIsRenderPassActive = bIsRenderPassActive;

        Allocator           = Other.Allocator;
        FirstCommand        = Other.FirstCommand;
        LastCommand         = Other.LastCommand;
        Statistics          = Other.Statistics;
        bIsRenderPassActive = Other.bIsRenderPassActive;

        Other.Allocator           = TempAllocator;
        Other.FirstCommand        = TempFirstCommand;
        Other.LastCommand         = TempLastCommand;
        Other.Statistics          = TempStatistics;
        Other.bIsRenderPassActive = bTempIsRenderPassActive;
    }

    FORCEINLINE uint32 GetNumDrawCalls() const
    {
        return Statistics.NumDrawCalls;
    }

    FORCEINLINE uint32 GetNumDispatchCalls() const
    {
        return Statistics.NumDispatchCalls;
    }

    FORCEINLINE uint32 GetNumCommands() const
    {
        return Statistics.NumCommands;
    }

private:

    template<typename CommandType, typename... ArgTypes>
    void InsertCommand(ArgTypes&&... Args)
    {
        Check(Allocator != nullptr);

        CommandType* Cmd = Allocator->Construct<CommandType>(Forward<ArgTypes>(Args)...);
        if (LastCommand)
        {
            LastCommand->NextCommand = Cmd;
            LastCommand              = LastCommand->NextCommand;
        }
        else
        {
            FirstCommand = Cmd;
            LastCommand  = FirstCommand;
        }

        Statistics.NumCommands++;
    }

private:
    FRHICommandAllocator* Allocator;
    FRHICommand*          FirstCommand;
    FRHICommand*          LastCommand;

    FRHICommandStatistics Statistics;

    bool                  bIsRenderPassActive = false;
};


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIExecutorTask

struct FRHIExecutorTask
{
    FRHIExecutorTask()
        : Function()
    { }

    FRHIExecutorTask(const TFunction<void()>& InWork)
        : Function(InWork)
    { }

    FORCEINLINE void operator()()
    {
        Function();
    }

    TFunction<void()> Function;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIExecutorThread

class RHI_API FRHIExecutorThread : public FNonCopyable
{
public:

    FRHIExecutorThread();
    ~FRHIExecutorThread() = default;

    static FORCEINLINE const char* GetStaticThreadName() { return "RHI Executor-Thread"; }

    bool Start();
    void StopExecution();

    void Execute(const FRHIExecutorTask& ExecutionTask);

private:
    void Worker();
    
    FGenericThreadRef  Thread;

    FCriticalSection   WaitCS;
    FConditionVariable WaitCondition;

    FRHIExecutorTask   CurrentTask;
    FCriticalSection   CurrentTaskCS;

    bool               bIsRunning;
    bool               bIsExecuting;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandListExecutor

class RHI_API FRHICommandListExecutor : public FNonCopyable
{
public:

    FRHICommandListExecutor();
    ~FRHICommandListExecutor() = default;

public:

    static bool Initialize();
    static void Release();
    static FRHICommandListExecutor& Get();

    void WaitForGPU();

    void ExecuteCommandList(class FRHICommandList& CmdList);
    void ExecuteCommandLists(class FRHICommandList* const* CmdLists, uint32 NumCmdLists);

    FORCEINLINE void SetContext(IRHICommandContext* InCmdContext) { CommandContext = InCmdContext; }

    FORCEINLINE IRHICommandContext& GetContext()
    {
        Check(CommandContext != nullptr);
        return *CommandContext;
    }

    FORCEINLINE const FRHICommandStatistics& GetStatistics() const { return Statistics; }

private:
    void InternalExecuteCommandList(class FRHICommandList& CmdList);

    FRHIExecutorThread    ExecutorThread;
    FRHICommandStatistics Statistics;

    IRHICommandContext*   CommandContext;
};