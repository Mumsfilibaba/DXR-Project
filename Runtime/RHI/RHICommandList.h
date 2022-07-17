#pragma once
#include "RHIModule.h"
#include "RHIResources.h"
#include "RHICommands.h"
#include "RHITimestampQuery.h"

#include "Core/Memory/MemoryStack.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Threading/Platform/ConditionVariable.h"
#include "Core/Containers/ArrayView.h"

class FRHIRenderTargetView;
class FRHIDepthStencilView;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHIShader;

#define ENABLE_INSERT_DEBUG_CMDLIST_MARKER (0)
#define ENABLE_RHI_EXECUTOR_THREAD         (1)

#if ENABLE_INSERT_DEBUG_CMDLIST_MARKER
    #define INSERT_DEBUG_CMDLIST_MARKER(CommandList, MarkerString) (CommandList).InsertMarker(MarkerString)
#else
    #define INSERT_DEBUG_CMDLIST_MARKER(CommandList, MarkerString)
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandStatistics

struct FRHICommandStatistics
{
    FORCEINLINE FRHICommandStatistics()
        : NumDrawCalls(0)
        , NumDispatchCalls(0)
    { }

    FORCEINLINE void Reset()
    {
        NumDrawCalls     = 0;
        NumDispatchCalls = 0;
    }

    uint32 NumDrawCalls;
    uint32 NumDispatchCalls;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandList

class RHI_API FRHICommandList : public FNonCopyable
{
    friend class FRHICommandListExecutor;

public:

    FRHICommandList()
        : Memory()
        , CommandPointer(nullptr)
        , FirstCommand(nullptr)
	    , Statistics()
        , NumCommands(0)
    {
        CommandPointer = &FirstCommand;
    }

    ~FRHICommandList()
    {
        Reset();
    }

    FORCEINLINE void* Allocate(int32 Size, int32 Alignment)
    {
        return Memory.Allocate(Size, Alignment);
    }

    template<typename T>
    FORCEINLINE T* Allocate()
    {
        return reinterpret_cast<T*>(Allocate(sizeof(T), alignof(T)));
    }

    template<typename T>
    FORCEINLINE TArrayView<T> AllocateArray(const TArrayView<T>& Array)
    {
        void* NewArray = Allocate(Array.Size() * sizeof(T), alignof(T));
        FMemory::Memcpy(NewArray, Array.Data(), Array.SizeInBytes());
        return TArrayView<T>(reinterpret_cast<T*>(NewArray), Array.Size());
    }

    FORCEINLINE void* AllocateCommand(int32 Size, int32 Alignment)
    {
        FRHICommand* NewCommand = reinterpret_cast<FRHICommand*>(Allocate(Size, Alignment));
        *CommandPointer = NewCommand;
        CommandPointer  = &NewCommand->NextCommand;
        ++NumCommands;
        return NewCommand;
    }

    FORCEINLINE tchar* AllocateString(const tchar* String)
    {
        int32  Length    = FCString::Length(String);
        tchar* NewString = reinterpret_cast<tchar*>(Allocate(sizeof(tchar) * Length, alignof(tchar)));
        return FCString::Copy(NewString, String);
    }

    template<typename CommandType, typename... ArgTypes>
    FORCEINLINE void EmplaceCommand(ArgTypes&&... Args)
    {
        new(AllocateCommand(sizeof(CommandType), alignof(CommandType))) CommandType(Forward<ArgTypes>(Args)...);
    }

public:

    FORCEINLINE void BeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
    {
        EmplaceCommand<FRHICommandBeginTimeStamp>(TimestampQuery, Index);
    }

    FORCEINLINE void EndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
    {
        EmplaceCommand<FRHICommandEndTimeStamp>(TimestampQuery, Index);
    }

    FORCEINLINE void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const TStaticArray<float, 4>& ClearColor)
    {
        Check(RenderTargetView.Texture != nullptr);
        EmplaceCommand<FRHICommandClearRenderTargetView>(RenderTargetView, ClearColor);
    }

    FORCEINLINE void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
    {
        Check(DepthStencilView.Texture != nullptr);
        EmplaceCommand<FRHICommandClearDepthStencilView>(DepthStencilView, Depth, Stencil);
    }

    FORCEINLINE void ClearUnorderedAccessView(FRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor)
    {
        Check(UnorderedAccessView != nullptr);
        EmplaceCommand<FRHICommandClearUnorderedAccessViewFloat>(UnorderedAccessView, ClearColor);
    }

    FORCEINLINE void BeginRenderPass(const FRHIRenderPassInitializer& RenderPassInitializer)
    {
        Check(bIsRenderPassActive == false);

        EmplaceCommand<FRHICommandBeginRenderPass>(RenderPassInitializer);
        bIsRenderPassActive = true;
    }

    FORCEINLINE void EndRenderPass()
    {
        Check(bIsRenderPassActive == true);

        EmplaceCommand<FRHICommandEndRenderPass>();
        bIsRenderPassActive = false;
    }

    FORCEINLINE void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
    {
        EmplaceCommand<FRHICommandSetViewport>(Width, Height, MinDepth, MaxDepth, x, y);
    }

    FORCEINLINE void SetScissorRect(float Width, float Height, float x, float y)
    {
        EmplaceCommand<FRHICommandSetScissorRect>(Width, Height, x, y);
    }

    FORCEINLINE void SetBlendFactor(const TStaticArray<float, 4>& Color)
    {
        EmplaceCommand<FRHICommandSetBlendFactor>(Color);
    }

    // TODO: Use arrayview
    FORCEINLINE void SetVertexBuffers(FRHIVertexBuffer* const* InVertexBuffers, uint32 NumVertexBuffers, uint32 BufferSlot)
    {
        TArrayView<FRHIVertexBuffer*> VertexBuffers = AllocateArray(MakeArrayView((FRHIVertexBuffer**)InVertexBuffers, NumVertexBuffers));
        if (!InVertexBuffers)
        {
            VertexBuffers.Memzero();
        }

        EmplaceCommand<FRHICommandSetVertexBuffers>(VertexBuffers.Data(), VertexBuffers.Size(), BufferSlot);
    }

    FORCEINLINE void SetIndexBuffer(FRHIIndexBuffer* IndexBuffer)
    {
        EmplaceCommand<FRHICommandSetIndexBuffer>(IndexBuffer);
    }

    FORCEINLINE void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
    {
        EmplaceCommand<FRHICommandSetPrimitiveTopology>(PrimitveTopologyType);
    }

    FORCEINLINE void SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
    {
        EmplaceCommand<FRHICommandSetGraphicsPipelineState>(PipelineState);
    }

    FORCEINLINE void SetComputePipelineState(FRHIComputePipelineState* PipelineState)
    {
        EmplaceCommand<FRHICommandSetComputePipelineState>(PipelineState);
    }

    FORCEINLINE void Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
    {
        Check(Num32BitConstants <= kRHIMaxShaderConstants);

        int32 Size       = Num32BitConstants * sizeof(uint32);
        void* SourceData = Allocate(Size, alignof(uint32));
        FMemory::Memcpy(SourceData, Shader32BitConstants, Size);

        EmplaceCommand<FRHICommandSet32BitShaderConstants>(Shader, SourceData, Num32BitConstants);
    }

    FORCEINLINE void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
    {
        EmplaceCommand<FRHICommandSetShaderResourceView>(Shader, ShaderResourceView, ParameterIndex);
    }

    // TODO: Use arrayview
    FORCEINLINE void SetShaderResourceViews(FRHIShader* Shader, FRHIShaderResourceView* const* InShaderResourceViews, uint32 NumShaderResourceViews, uint32 ParameterIndex)
    {
        TArrayView<FRHIShaderResourceView*> ShaderResourceViews = AllocateArray(MakeArrayView((FRHIShaderResourceView**)InShaderResourceViews, NumShaderResourceViews));
        if (!InShaderResourceViews)
        {
            ShaderResourceViews.Memzero();
        }

        EmplaceCommand<FRHICommandSetShaderResourceViews>(Shader, ShaderResourceViews.Data(), ShaderResourceViews.Size(), ParameterIndex);
    }

    FORCEINLINE void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
    {
        EmplaceCommand<FRHICommandSetUnorderedAccessView>(Shader, UnorderedAccessView, ParameterIndex);
    }

    // TODO: Use arrayview
    FORCEINLINE void SetUnorderedAccessViews(FRHIShader* Shader, FRHIUnorderedAccessView* const* InUnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
    {
        TArrayView<FRHIUnorderedAccessView*> UnorderedAccessViews = AllocateArray(MakeArrayView((FRHIUnorderedAccessView**)InUnorderedAccessViews, NumUnorderedAccessViews));
        if (!InUnorderedAccessViews)
        {
            UnorderedAccessViews.Memzero();
        }

        EmplaceCommand<FRHICommandSetUnorderedAccessViews>(Shader, UnorderedAccessViews.Data(), UnorderedAccessViews.Size(), ParameterIndex);
    }

    FORCEINLINE void SetConstantBuffer(FRHIShader* Shader, FRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
    {
        EmplaceCommand<FRHICommandSetConstantBuffer>(Shader, ConstantBuffer, ParameterIndex);
    }

    // TODO: Use arrayview
    FORCEINLINE void SetConstantBuffers(FRHIShader* Shader, FRHIConstantBuffer* const* InConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
    {
        TArrayView<FRHIConstantBuffer*> ConstantBuffers = AllocateArray(MakeArrayView((FRHIConstantBuffer**)InConstantBuffers, NumConstantBuffers));
        if (!InConstantBuffers)
        {
            ConstantBuffers.Memzero();
        }

        EmplaceCommand<FRHICommandSetConstantBuffers>(Shader, ConstantBuffers.Data(), ConstantBuffers.Size(), ParameterIndex);
    }

    FORCEINLINE void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
    {
        EmplaceCommand<FRHICommandSetSamplerState>(Shader, SamplerState, ParameterIndex);
    }

    // TODO: Use arrayview
    FORCEINLINE void SetSamplerStates(FRHIShader* Shader, FRHISamplerState* const* InSamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
    {
        TArrayView<FRHISamplerState*> SamplerStates = AllocateArray(MakeArrayView((FRHISamplerState**)InSamplerStates, NumSamplerStates));
        if (!InSamplerStates)
        {
            SamplerStates.Memzero();
        }

        EmplaceCommand<FRHICommandSetSamplerStates>(Shader, SamplerStates.Data(), SamplerStates.Size(), ParameterIndex);
    }

    FORCEINLINE void UpdateBuffer(FRHIBuffer* Dst, uint32 OffsetInBytes, uint32 SizeInBytes, const void* InSourceData)
    {
        void* SourceData = Allocate(SizeInBytes, alignof(uint8));
        FMemory::Memcpy(SourceData, InSourceData, SizeInBytes);
        EmplaceCommand<FRHICommandUpdateBuffer>(Dst, OffsetInBytes, SizeInBytes, SourceData);
    }

    FORCEINLINE void UpdateTexture2D(FRHITexture2D* Dst, uint16 Width, uint16 Height, uint16 MipLevel, const void* InSourceData)
    {
        const uint32 SizeInBytes = Width * Height * GetByteStrideFromFormat(Dst->GetFormat());

        void* SourceData = Allocate(SizeInBytes, alignof(uint8));
        FMemory::Memcpy(SourceData, InSourceData, SizeInBytes);
        EmplaceCommand<FRHICommandUpdateTexture2D>(Dst, Width, Height, MipLevel, SourceData);
    }

    FORCEINLINE void ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
    {
        EmplaceCommand<FRHICommandResolveTexture>(Dst, Src);
    }

    FORCEINLINE void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHICopyBufferInfo& CopyInfo)
    {
        EmplaceCommand<FRHICommandCopyBuffer>(Dst, Src, CopyInfo);
    }

    FORCEINLINE void CopyTexture(FRHITexture* Dst, FRHITexture* Src)
    {
        EmplaceCommand<FRHICommandCopyTexture>(Dst, Src);
    }

    FORCEINLINE void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHICopyTextureInfo& CopyTextureInfo)
    {
        EmplaceCommand<FRHICommandCopyTextureRegion>(Dst, Src, CopyTextureInfo);
    }

    FORCEINLINE void DestroyResource(IRefCounted* Resource)
    {
        EmplaceCommand<FRHICommandDestroyResource>(Resource);
    }

    FORCEINLINE void DiscardContents(FRHITexture* Texture)
    {
        EmplaceCommand<FRHICommandDiscardContents>(Texture);
    }

    FORCEINLINE void BuildRayTracingGeometry(FRHIRayTracingGeometry* Geometry, FRHIVertexBuffer* VertexBuffer, FRHIIndexBuffer* IndexBuffer, bool bUpdate)
    {
        Check((Geometry != nullptr) && (!bUpdate || (bUpdate && IsEnumFlagSet(Geometry->GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate))));
        EmplaceCommand<FRHICommandBuildRayTracingGeometry>(Geometry, VertexBuffer, IndexBuffer, bUpdate);
    }

    FORCEINLINE void BuildRayTracingScene(FRHIRayTracingScene* Scene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
    {
        Check((Scene != nullptr) && (!bUpdate || (bUpdate && IsEnumFlagSet(Scene->GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate))));
        EmplaceCommand<FRHICommandBuildRayTracingScene>(Scene, Instances, bUpdate);
    }

    // TODO: Refactor
    FORCEINLINE void SetRayTracingBindings( FRHIRayTracingScene* RayTracingScene
                                          , FRHIRayTracingPipelineState* PipelineState
                                          , const FRayTracingShaderResources* GlobalResource
                                          , const FRayTracingShaderResources* RayGenLocalResources
                                          , const FRayTracingShaderResources* MissLocalResources
                                          , const FRayTracingShaderResources* HitGroupResources
                                          , uint32 NumHitGroupResources)
    {
        EmplaceCommand<FRHICommandSetRayTracingBindings>( RayTracingScene
                                                        , PipelineState
                                                        , GlobalResource
                                                        , RayGenLocalResources
                                                        , MissLocalResources
                                                        , HitGroupResources
                                                        , NumHitGroupResources);
    }

    FORCEINLINE void GenerateMips(FRHITexture* Texture)
    {
        Check(Texture != nullptr);
        EmplaceCommand<FRHICommandGenerateMips>(Texture);
    }

    FORCEINLINE void TransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
    {
        if (BeforeState != AfterState)
        {
            EmplaceCommand<FRHICommandTransitionTexture>(Texture, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '%s' Was transitioned with the same Before- and AfterState (=%s)", Texture->GetName().CStr(),  ToString(BeforeState));
        }
    }

    FORCEINLINE void TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
    {
        Check(Buffer != nullptr);

        if (BeforeState != AfterState)
        {
            EmplaceCommand<FRHICommandTransitionBuffer>(Buffer, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '%s' Was transitioned with the same Before- and AfterState (=%s)", Buffer->GetName().CStr(),  ToString(BeforeState));
        }
    }

    FORCEINLINE void UnorderedAccessTextureBarrier(FRHITexture* Texture)
    {
        Check(Texture != nullptr);
        EmplaceCommand<FRHICommandUnorderedAccessTextureBarrier>(Texture);
    }

    FORCEINLINE void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
    {
        Check(Buffer != nullptr);
        EmplaceCommand<FRHICommandUnorderedAccessBufferBarrier>(Buffer);
    }

    FORCEINLINE void Draw(uint32 VertexCount, uint32 StartVertexLocation)
    {
        if (VertexCount > 0)
        {
            EmplaceCommand<FRHICommandDraw>(VertexCount, StartVertexLocation);
            Statistics.NumDrawCalls++;
        }
    }

    FORCEINLINE void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
    {
        if (IndexCount > 0)
        {
            EmplaceCommand<FRHICommandDrawIndexed>(IndexCount, StartIndexLocation, BaseVertexLocation);
            Statistics.NumDrawCalls++;
        }
    }

    FORCEINLINE void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
    {
        if ((VertexCountPerInstance > 0) && (InstanceCount > 0))
        {
            EmplaceCommand<FRHICommandDrawInstanced>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
            Statistics.NumDrawCalls++;
        }
    }

    FORCEINLINE void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
    {
        if ((IndexCountPerInstance > 0) && (InstanceCount > 0))
        {
            EmplaceCommand<FRHICommandDrawIndexedInstanced>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
            Statistics.NumDrawCalls++;
        }
    }

    FORCEINLINE void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
    {
        if ((ThreadGroupCountX > 0) || (ThreadGroupCountY > 0) || (ThreadGroupCountZ > 0))
        {
            EmplaceCommand<FRHICommandDispatch>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
            Statistics.NumDispatchCalls++;
        }
    }

    FORCEINLINE void DispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
    {
        if ((Width > 0) || (Height > 0) || (Depth > 0))
        {
            EmplaceCommand<FRHICommandDispatchRays>(Scene, PipelineState, Width, Height, Depth);
        }
    }

    FORCEINLINE void InsertMarker(const FString& Marker)
    {
        EmplaceCommand<FRHICommandInsertMarker>(Marker);
    }
    
    FORCEINLINE void DebugBreak()
    {
        EmplaceCommand<FRHICommandDebugBreak>();
    }

    FORCEINLINE void BeginExternalCapture()
    {
        EmplaceCommand<FRHICommandBeginExternalCapture>();
    }

    FORCEINLINE void EndExternalCapture()
    {
        EmplaceCommand<FRHICommandEndExternalCapture>();
    }

    FORCEINLINE void Reset()
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
        }

        FirstCommand   = nullptr;
        CommandPointer = &FirstCommand;
        NumCommands    = 0;

        bIsRenderPassActive = false;

        Statistics.Reset();

        Memory.Reset();
    }

    void ExchangeState(FRHICommandList& Other)
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
        return NumCommands;
    }

private:
    FMemoryStack          Memory;

    /** @brief: pointer to FirstCommand to avoid branching */
    FRHICommand**         CommandPointer;
    FRHICommand*          FirstCommand;

    FRHICommandStatistics Statistics;
    uint32                NumCommands;

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

    FAtomicInt64       Fence;

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