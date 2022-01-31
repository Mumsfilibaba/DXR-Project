#pragma once
#include "RHIModule.h"
#include "RHIResources.h"
#include "RHIRenderCommand.h"
#include "RHITimestampQuery.h"

class CRHIRenderTargetView;
class CRHIDepthStencilView;
class CRHIShaderResourceView;
class CRHIUnorderedAccessView;
class CRHIShader;

#define ENABLE_INSERT_DEBUG_CMDLIST_MARKER 0

#if ENABLE_INSERT_DEBUG_CMDLIST_MARKER
#define INSERT_DEBUG_CMDLIST_MARKER(CmdList, MarkerString) CmdList.InsertCommandListMarker(MarkerString);
#else
#define INSERT_DEBUG_CMDLIST_MARKER(CmdList, MarkerString)
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class RHI_API CCommandAllocator
{
public:

    CCommandAllocator(uint32 StartSize = 4096);
    ~CCommandAllocator();

    /* Allocate memory, */
    void* Allocate(uint64 SizeInBytes, uint64 Alignment = STANDARD_ALIGNMENT);

    /* Resets the allocator*/
    void Reset();

    /* Typed version of allocate */
    template<typename T>
    FORCEINLINE T* Allocate()
    {
        return reinterpret_cast<T*>(Allocate(sizeof(T), alignof(T)));
    }

    /* Types array version of allocate */
    template<typename T>
    FORCEINLINE T* Allocate(uint32 NumElements)
    {
        return reinterpret_cast<T*>(Allocate(sizeof(T) * NumElements, alignof(T)));
    }

    /* Constructs a new item by calling the constructor */
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

    /* The current pointer */
    uint8* CurrentMemory;

    /* Size of Current Memory*/
    uint64 Size;

    /* Offset into Current Memory*/
    uint64 Offset;

    /* Holds the average number of bytes allocated between each reset */
    uint64 AverageMemoryUsage;

    /* Discarded memory, kept alive so until reset is called */
    TArray<uint8*> DiscardedMemory;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHICommandList
{
    friend class CRHICommandQueue;

public:

    CRHICommandList()
        : CmdAllocator()
        , First(nullptr)
        , Last(nullptr)
    {
    }

    ~CRHICommandList()
    {
        Reset();
    }

    void BeginTimeStamp(CRHITimestampQuery* Profiler, uint32 Index)
    {
        InsertCommand<SRHIBeginTimeStampRenderCommand>(AddRef(Profiler), Index);
    }

    void EndTimeStamp(CRHITimestampQuery* Profiler, uint32 Index)
    {
        InsertCommand<SRHIEndTimeStampRenderCommand>(AddRef(Profiler), Index);
    }

    void ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor)
    {
        Assert(RenderTargetView != nullptr);
        InsertCommand<SRHIClearRenderTargetViewRenderCommand>(AddRef(RenderTargetView), ClearColor);
    }

    void ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const SDepthStencil& ClearValue)
    {
        Assert(DepthStencilView != nullptr);
        InsertCommand<SRHIClearDepthStencilViewRenderCommand>(AddRef(DepthStencilView), ClearValue);
    }

    void ClearUnorderedAccessView(CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor)
    {
        Assert(UnorderedAccessView != nullptr);
        InsertCommand<SRHIClearUnorderedAccessViewFloatRenderCommand>(AddRef(UnorderedAccessView), ClearColor);
    }

    void SetShadingRate(EShadingRate ShadingRate)
    {
        InsertCommand<SRHISetShadingRateRenderCommand>(ShadingRate);
    }

    void SetShadingRateImage(CRHITexture2D* ShadingRateImage)
    {
        InsertCommand<SRHISetShadingRateImageRenderCommand>(AddRef(ShadingRateImage));
    }

    void BeginRenderPass()
    {
        InsertCommand<SRHIBeginRenderPassRenderCommand>();
    }

    void EndRenderPass()
    {
        InsertCommand<SRHIEndRenderPassRenderCommand>();
    }

    void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
    {
        InsertCommand<SRHISetViewportRenderCommand>(Width, Height, MinDepth, MaxDepth, x, y);
    }

    void SetScissorRect(float Width, float Height, float x, float y)
    {
        InsertCommand<SRHISetScissorRectRenderCommand>(Width, Height, x, y);
    }

    void SetBlendFactor(const SColorF& Color)
    {
        InsertCommand<SRHISetBlendFactorRenderCommand>(Color);
    }

    void SetRenderTargets(CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView)
    {
        CRHIRenderTargetView** RenderTargets = CmdAllocator.Allocate<CRHIRenderTargetView*>(RenderTargetCount);
        for (uint32 i = 0; i < RenderTargetCount; i++)
        {
            RenderTargets[i] = AddRef<CRHIRenderTargetView>(RenderTargetViews[i]);
        }

        InsertCommand<SRHISetRenderTargetsRenderCommand>(RenderTargets, RenderTargetCount, MakeSharedRef<CRHIDepthStencilView>(DepthStencilView));
    }

    void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
    {
        InsertCommand<SRHISetPrimitiveTopologyRenderCommand>(PrimitveTopologyType);
    }

    void SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 VertexBufferCount, uint32 BufferSlot)
    {
        CRHIVertexBuffer** Buffers = CmdAllocator.Allocate<CRHIVertexBuffer*>(VertexBufferCount);
        for (uint32 i = 0; i < VertexBufferCount; i++)
        {
            Buffers[i] = AddRef<CRHIVertexBuffer>(VertexBuffers[i]);
        }

        InsertCommand<SRHISetVertexBuffersRenderCommand>(Buffers, VertexBufferCount, BufferSlot);
    }

    void SetIndexBuffer(CRHIIndexBuffer* IndexBuffer)
    {
        InsertCommand<SRHISetIndexBufferRenderCommand>(MakeSharedRef<CRHIIndexBuffer>(IndexBuffer));
    }

    void SetRayTracingBindings(
        CRHIRayTracingScene* RayTracingScene,
        CRHIRayTracingPipelineState* PipelineState,
        const SRayTracingShaderResources* GlobalResource,
        const SRayTracingShaderResources* RayGenLocalResources,
        const SRayTracingShaderResources* MissLocalResources,
        const SRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources)
    {
        InsertCommand<SRHISetRayTracingBindingsRenderCommand>(AddRef(RayTracingScene), AddRef(PipelineState), GlobalResource, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
    }

    void SetGraphicsPipelineState(CRHIGraphicsPipelineState* PipelineState)
    {
        InsertCommand<SRHISetGraphicsPipelineStateRenderCommand>(AddRef(PipelineState));
    }

    void SetComputePipelineState(CRHIComputePipelineState* PipelineState)
    {
        InsertCommand<SRHISetComputePipelineStateRenderCommand>(AddRef(PipelineState));
    }

    void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
    {
        const uint32 Num32BitConstantsInBytes = Num32BitConstants * 4;

        void* Shader32BitConstantsMemory = CmdAllocator.Allocate(Num32BitConstantsInBytes, 1);
        CMemory::Memcpy(Shader32BitConstantsMemory, Shader32BitConstants, Num32BitConstantsInBytes);

        InsertCommand<SRHISet32BitShaderConstantsRenderCommand>(AddRef(Shader), Shader32BitConstantsMemory, Num32BitConstants);
    }

    void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
    {
        InsertCommand<SRHISetShaderResourceViewRenderCommand>(AddRef(Shader), AddRef(ShaderResourceView), ParameterIndex);
    }

    void SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceViews, uint32 NumShaderResourceViews, uint32 ParameterIndex)
    {
        CRHIShaderResourceView** TempShaderResourceViews = CmdAllocator.Allocate<CRHIShaderResourceView*>(NumShaderResourceViews);
        for (uint32 i = 0; i < NumShaderResourceViews; i++)
        {
            TempShaderResourceViews[i] = AddRef(ShaderResourceViews[i]);
        }

        InsertCommand<SRHISetShaderResourceViewsRenderCommand>(AddRef(Shader), TempShaderResourceViews, NumShaderResourceViews, ParameterIndex);
    }

    void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
    {
        InsertCommand<SRHISetUnorderedAccessViewRenderCommand>(AddRef(Shader), AddRef(UnorderedAccessView), ParameterIndex);
    }

    void SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
    {
        CRHIUnorderedAccessView** TempUnorderedAccessViews = CmdAllocator.Allocate<CRHIUnorderedAccessView*>(NumUnorderedAccessViews);
        for (uint32 i = 0; i < NumUnorderedAccessViews; i++)
        {
            TempUnorderedAccessViews[i] = AddRef(UnorderedAccessViews[i]);
        }

        InsertCommand<SRHISetUnorderedAccessViewsRenderCommand>(AddRef(Shader), TempUnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex);
    }

    void SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
    {
        InsertCommand<SRHISetConstantBufferRenderCommand>(AddRef(Shader), AddRef(ConstantBuffer), ParameterIndex);
    }

    void SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
    {
        CRHIConstantBuffer** TempConstantBuffers = CmdAllocator.Allocate<CRHIConstantBuffer*>(NumConstantBuffers);
        for (uint32 i = 0; i < NumConstantBuffers; i++)
        {
            TempConstantBuffers[i] = AddRef(ConstantBuffers[i]);
        }

        InsertCommand<SRHISetConstantBuffersRenderCommand>(AddRef(Shader), TempConstantBuffers, NumConstantBuffers, ParameterIndex);
    }

    void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex)
    {
        InsertCommand<SRHISetSamplerStateRenderCommand>(AddRef(Shader), AddRef(SamplerState), ParameterIndex);
    }

    void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
    {
        CRHISamplerState** TempSamplerStates = CmdAllocator.Allocate<CRHISamplerState*>(NumSamplerStates);
        for (uint32 i = 0; i < NumSamplerStates; i++)
        {
            TempSamplerStates[i] = AddRef(SamplerStates[i]);
        }

        InsertCommand<SRHISetSamplerStatesRenderCommand>(AddRef(Shader), TempSamplerStates, NumSamplerStates, ParameterIndex);
    }

    void ResolveTexture(CRHITexture* Destination, CRHITexture* Source)
    {
        InsertCommand<SRHIResolveTextureRenderCommand>(AddRef(Destination), AddRef(Source));
    }

    void UpdateBuffer(CRHIBuffer* Destination, uint64 DestinationOffsetInBytes, uint64 SizeInBytes, const void* SourceData)
    {
        void* TempSourceData = CmdAllocator.Allocate(static_cast<uint32>(SizeInBytes), 1);
        CMemory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        InsertCommand<SRHIUpdateBufferRenderCommand>(AddRef(Destination), DestinationOffsetInBytes, SizeInBytes, TempSourceData);
    }

    void UpdateTexture2D(CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
    {
        const uint32 SizeInBytes = Width * Height * GetByteStrideFromFormat(Destination->GetFormat());

        void* TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
        CMemory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        InsertCommand<SRHIUpdateTexture2DRenderCommand>(AddRef(Destination), Width, Height, MipLevel, TempSourceData);
    }

    void CopyBuffer(CRHIBuffer* Destination, CRHIBuffer* Source, const SCopyBufferInfo& CopyInfo)
    {
        InsertCommand<SRHICopyBufferRenderCommand>(AddRef(Destination), AddRef(Source), CopyInfo);
    }

    void CopyTexture(CRHITexture* Destination, CRHITexture* Source)
    {
        InsertCommand<SRHICopyTextureRenderCommand>(AddRef(Destination), AddRef(Source));
    }

    void CopyTextureRegion(CRHITexture* Destination, CRHITexture* Source, const SCopyTextureInfo& CopyTextureInfo)
    {
        InsertCommand<SRHICopyTextureRegionRenderCommand>(AddRef(Destination), AddRef(Source), CopyTextureInfo);
    }

    void DestroyResource(CRHIObject* Resource)
    {
        InsertCommand<SRHIDestroyResourceRenderCommand>(AddRef(Resource));
    }

    void DiscardResource(CRHIResource* Resource)
    {
        InsertCommand<SRHIDiscardResourceRenderCommand>(AddRef(Resource));
    }

    void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool bUpdate)
    {
        Assert((Geometry != nullptr) && (!bUpdate || (bUpdate && Geometry->GetFlags() & RayTracingStructureBuildFlag_AllowUpdate)));
        InsertCommand<SRHIBuildRayTracingGeometryRenderCommand>(AddRef(Geometry), AddRef(VertexBuffer), AddRef(IndexBuffer), bUpdate);
    }

    void BuildRayTracingScene(CRHIRayTracingScene* Scene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate)
    {
        Assert((Scene != nullptr) && (!bUpdate || (bUpdate && Scene->GetFlags() & RayTracingStructureBuildFlag_AllowUpdate)));
        InsertCommand<SRHIBuildRayTracingSceneRenderCommand>(AddRef(Scene), Instances, NumInstances, bUpdate);
    }

    void GenerateMips(CRHITexture* Texture)
    {
        Assert(Texture != nullptr);
        InsertCommand<SRHIGenerateMipsRenderCommand>(AddRef(Texture));
    }

    void TransitionTexture(CRHITexture* Texture, EResourceState BeforeState, EResourceState AfterState)
    {
        if (BeforeState != AfterState)
        {
            InsertCommand<SRHITransitionTextureRenderCommand>(AddRef(Texture), BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '" + Texture->GetName() + "' Was transitioned with the same Before- and AfterState (=" + ToString(BeforeState) + ")");
        }
    }

    void TransitionBuffer(CRHIBuffer* Buffer, EResourceState BeforeState, EResourceState AfterState)
    {
        Assert(Buffer != nullptr);

        if (BeforeState != AfterState)
        {
            InsertCommand<SRHITransitionBufferRenderCommand>(AddRef(Buffer), BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Buffer '" + Buffer->GetName() + "' Was transitioned with the same Before- and AfterState (=" + ToString(BeforeState) + ")");
        }
    }

    void UnorderedAccessTextureBarrier(CRHITexture* Texture)
    {
        Assert(Texture != nullptr);
        InsertCommand<SRHIUnorderedAccessTextureBarrierRenderCommand>(AddRef(Texture));
    }

    void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer)
    {
        Assert(Buffer != nullptr);
        InsertCommand<SRHIUnorderedAccessBufferBarrierRenderCommand>(Buffer);
    }

    void Draw(uint32 VertexCount, uint32 StartVertexLocation)
    {
        InsertCommand<SRHIDrawRenderCommand>(VertexCount, StartVertexLocation);
        NumDrawCalls++;
    }

    void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
    {
        InsertCommand<SRHIDrawIndexedRenderCommand>(IndexCount, StartIndexLocation, BaseVertexLocation);
        NumDrawCalls++;
    }

    void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
    {
        InsertCommand<SRHIDrawInstancedRenderCommand>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    void DrawIndexedInstanced(
        uint32 IndexCountPerInstance,
        uint32 InstanceCount,
        uint32 StartIndexLocation,
        uint32 BaseVertexLocation,
        uint32 StartInstanceLocation)
    {
        InsertCommand<SRHIDrawIndexedInstancedRenderCommand>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
    {
        InsertCommand<SRHIDispatchComputeRenderCommand>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        NumDispatchCalls++;
    }

    void DispatchRays(CRHIRayTracingScene* Scene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
    {
        InsertCommand<SRHIDispatchRaysRenderCommand>(AddRef(Scene), AddRef(PipelineState), Width, Height, Depth);
    }

    void InsertCommandListMarker(const CString& Marker)
    {
        InsertCommand<SRHIInsertCommandListMarkerRenderCommand>(Marker);
    }

    void DebugBreak()
    {
        InsertCommand<SRHIDebugBreakRenderCommand>();
    }

    void BeginExternalCapture()
    {
        InsertCommand<SRHIBeginExternalCaptureRenderCommand>();
    }

    void EndExternalCapture()
    {
        InsertCommand<SRHIEndExternalCaptureRenderCommand>();
    }

    void Reset()
    {
        if (First != nullptr)
        {
            SRHIRenderCommand* Cmd = First;
            while (Cmd != nullptr)
            {
                SRHIRenderCommand* Old = Cmd;
                Cmd = Cmd->NextCmd;
                Old->~SRHIRenderCommand();
            }

            First = nullptr;
            Last = nullptr;
        }

        NumDrawCalls = 0;
        NumDispatchCalls = 0;
        NumCommands = 0;

        CmdAllocator.Reset();
    }

    FORCEINLINE uint32 GetNumDrawCalls() const
    {
        return NumDrawCalls;
    }

    FORCEINLINE uint32 GetNumDispatchCalls() const
    {
        return NumDispatchCalls;
    }

    FORCEINLINE uint32 GetNumCommands() const
    {
        return NumCommands;
    }

private:

    template<typename TCommand, typename... TArgs>
    void InsertCommand(TArgs&&... Args)
    {
        TCommand* Cmd = CmdAllocator.Construct<TCommand>(Forward<TArgs>(Args)...);
        if (Last)
        {
            Last->NextCmd = Cmd;
            Last = Last->NextCmd;
        }
        else
        {
            First = Cmd;
            Last = First;
        }

        NumCommands++;
    }

    CCommandAllocator CmdAllocator;

    SRHIRenderCommand* First;
    SRHIRenderCommand* Last;

    uint32 NumDrawCalls = 0;
    uint32 NumDispatchCalls = 0;
    uint32 NumCommands = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class RHI_API CRHICommandQueue
{
public:

    static FORCEINLINE CRHICommandQueue& Get()
    {
        return Instance;
    }

    void ExecuteCommandList(CRHICommandList& CmdList);
    void ExecuteCommandLists(CRHICommandList* const* CmdLists, uint32 NumCmdLists);

    void WaitForGPU();

    FORCEINLINE void SetContext(IRHICommandContext* InCmdContext)
    {
        CmdContext = InCmdContext;
    }

    FORCEINLINE IRHICommandContext& GetContext()
    {
        Assert(CmdContext != nullptr);
        return *CmdContext;
    }

    FORCEINLINE uint32 GetNumDrawCalls() const
    {
        return NumDrawCalls;
    }

    FORCEINLINE uint32 GetNumDispatchCalls() const
    {
        return NumDispatchCalls;
    }

    FORCEINLINE uint32 GetNumCommands() const
    {
        return NumCommands;
    }

private:

    CRHICommandQueue();
    ~CRHICommandQueue() = default;

    /* Goes through all the commands and executes them on a command list */
    void InternalExecuteCommandList(CRHICommandList& CmdList);

    FORCEINLINE void ResetStatistics()
    {
        NumDrawCalls = 0;
        NumDispatchCalls = 0;
        NumCommands = 0;
    }

    IRHICommandContext* CmdContext;

    // Statistics
    uint32 NumDrawCalls;
    uint32 NumDispatchCalls;
    uint32 NumCommands;

    static CRHICommandQueue Instance;
};