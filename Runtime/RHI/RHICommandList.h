#pragma once
#include "RHIModule.h"
#include "RHIResources.h"
#include "RHICommand.h"

class CRHIRenderTargetView;
class CRHIDepthStencilView;
class CRHIShaderResourceView;
class CRHIUnorderedAccessView;
class CRHIShader;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CommandList Marker

#define ENABLE_INSERT_COMMAND_LIST_MARKER (0)

#if ENABLE_INSERT_COMMAND_LIST_MARKER
    #define INSERT_COMMAND_LIST_MARKER(CommandList, MarkerString) CommandList.InsertCommandListMarker(MarkerString);
#else
    #define INSERT_COMMAND_LIST_MARKER(CommandList, MarkerString)
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCommandAllocator

class RHI_API CRHICommandAllocator
{
public:

    CRHICommandAllocator(uint32 StartSize = 4096);
    ~CRHICommandAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment = STANDARD_ALIGNMENT);

    void Reset();

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

    uint8*         CurrentMemory;

    uint64         Size;
    uint64         Offset;
    uint64         AverageMemoryUsage;

    TArray<uint8*> DiscardedMemory;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandExecutionManager

class RHI_API CRHICommandExecutionManager
{
public:

    static bool Initialize();

    static CRHICommandExecutionManager& Get();

    void ExecuteCommandList(class CRHICommandList& CmdList);
    void ExecuteCommandLists(class CRHICommandList* const* CmdLists, uint32 NumCmdLists);

    void WaitForGPU();

    void SetContext(IRHICommandContext* InCmdContext) { CommandContext = InCmdContext; }

    IRHICommandContext& GetContext()
    {
        Check(CommandContext != nullptr);
        return *CommandContext;
    }

    FORCEINLINE uint32 GetNumDrawCalls()     const { return NumDrawCalls; }
    FORCEINLINE uint32 GetNumDispatchCalls() const { return NumDispatchCalls; }
    FORCEINLINE uint32 GetNumCommands()      const { return NumCommands; }

private:

    CRHICommandExecutionManager();
    ~CRHICommandExecutionManager() = default;

    /** Internal function for executing the CommandList */
    void InternalExecuteCommandList(class CRHICommandList& CmdList);

    FORCEINLINE void ResetStatistics()
    {
        NumDrawCalls     = 0;
        NumDispatchCalls = 0;
        NumCommands      = 0;
    }

    IRHICommandContext* CommandContext;

    // Statistics
    uint32 NumDrawCalls;
    uint32 NumDispatchCalls;
    uint32 NumCommands;

    static CRHICommandExecutionManager Instance;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandList

class CRHICommandList
{
    friend class CRHICommandExecutionManager;

public:

    CRHICommandList()
        : CommandAllocator()
        , FirstCommand(nullptr)
        , LastCommand(nullptr)
    { }

    ~CRHICommandList()
    {
        Reset();
    }

    void BeginTimeStamp(CRHITimeQuery* TimestampQuery, uint32 Index)
    {
        InsertCommand<CRHICommandBeginTimeStamp>(TimestampQuery, Index);
    }

    void EndTimeStamp(CRHITimeQuery* TimestampQuery, uint32 Index)
    {
        InsertCommand<CRHICommandEndTimeStamp>(TimestampQuery, Index);
    }

    void ClearRenderTargetTexture(CRHIResource* Texture, const float ClearColor[4])
    {
        Check(Texture != nullptr);
        InsertCommand<CRHICommandClearRenderTargetTexture>(Texture, ClearColor);
    }

    void ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const float ClearColor[4])
    {
        Check(RenderTargetView != nullptr);
        InsertCommand<CRHICommandClearRenderTargetView>(RenderTargetView, ClearColor);
    }

    void ClearDepthStencilTexture(CRHIResource* Texture, const CTextureDepthStencilValue& ClearValue)
    {
        Check(Texture != nullptr);
        InsertCommand<CRHICommandClearDepthStencilTexture>(Texture, ClearValue);
    }

    void ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const CTextureDepthStencilValue& ClearValue)
    {
        Check(DepthStencilView != nullptr);
        InsertCommand<CRHICommandClearDepthStencilView>(DepthStencilView, ClearValue);
    }

    void ClearUnorderedAccessTextureFloat(CRHIResource* Texture, const float ClearColor[4])
    {
        Check(Texture != nullptr);
        InsertCommand<CRHICommandClearUnorderedAccessTextureFloat>(Texture, ClearColor);
    }

    void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const float ClearColor[4])
    {
        Check(UnorderedAccessView != nullptr);
        InsertCommand<CRHICommandClearUnorderedAccessViewFloat>(UnorderedAccessView, ClearColor);
    }

    void ClearUnorderedAccessTextureUint(CRHIResource* Texture, const uint32 ClearColor[4])
    {
        Check(Texture != nullptr);
        InsertCommand<CRHICommandClearUnorderedAccessTextureUint>(Texture, ClearColor);
    }

    void ClearUnorderedAccessViewUint(CRHIUnorderedAccessView* UnorderedAccessView, const uint32 ClearColor[4])
    {
        Check(UnorderedAccessView != nullptr);
        InsertCommand<CRHICommandClearUnorderedAccessViewUint>(UnorderedAccessView, ClearColor);
    }

    void SetShadingRate(EShadingRate ShadingRate)
    {
        InsertCommand<CRHICommandSetShadingRate>(ShadingRate);
    }

    void SetShadingRateTexture(CRHIResource* ShadingRateTexture)
    {
        InsertCommand<CRHICommandSetShadingRateTexture>(ShadingRateTexture);
    }

    void BeginRenderPass(const CRHIRenderPass& RenderPassDesc)
    {
        InsertCommand<CRHICommandBeginRenderPass>();
    }

    void EndRenderPass()
    {
        InsertCommand<CRHICommandEndRenderPass>();
    }

    void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
    {
        InsertCommand<CRHICommandSetViewport>(Width, Height, MinDepth, MaxDepth, x, y);
    }

    void SetScissorRect(float Width, float Height, float x, float y)
    {
        InsertCommand<CRHICommandSetScissorRect>(Width, Height, x, y);
    }

    void SetBlendFactor(const CFloatColor& Color)
    {
        InsertCommand<CRHICommandSetBlendFactor>(Color);
    }

    void SetVertexBuffers(CRHIBuffer* const* VertexBuffers, uint32 VertexBufferCount, uint32 BufferSlot)
    {
        CRHIBuffer** Buffers = CommandAllocator.Allocate<CRHIBuffer*>(VertexBufferCount);
        for (uint32 i = 0; i < VertexBufferCount; i++)
        {
            Buffers[i] = VertexBuffers[i];
        }

        InsertCommand<CRHICommandSetVertexBuffers>(Buffers, VertexBufferCount, BufferSlot);
    }

    void SetIndexBuffer(CRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
    {
        InsertCommand<CRHICommandSetIndexBuffer>(IndexBuffer, IndexFormat);
    }

    void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
    {
        InsertCommand<CRHICommandSetPrimitiveTopology>(PrimitveTopologyType);
    }

    void SetGraphicsPipelineState(CRHIGraphicsPipelineState* PipelineState)
    {
        InsertCommand<CRHICommandSetGraphicsPipelineState>(PipelineState);
    }

    void SetComputePipelineState(CRHIComputePipelineState* PipelineState)
    {
        InsertCommand<CRHICommandSetComputePipelineState>(PipelineState);
    }

    void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
    {
        const uint32 Num32BitConstantsInBytes = Num32BitConstants * 4;

        void* Shader32BitConstantsMemory = CommandAllocator.Allocate(Num32BitConstantsInBytes, 1);
        CMemory::Memcpy(Shader32BitConstantsMemory, Shader32BitConstants, Num32BitConstantsInBytes);

        InsertCommand<CRHICommandSet32BitShaderConstants>(Shader, Shader32BitConstantsMemory, Num32BitConstants);
    }

    void SetShaderResourceTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex)
    {
        InsertCommand<CRHICommandSetShaderResourceTexture>(Shader, Texture, ParameterIndex);
    }

    void SetShaderResourceTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex)
    {
        CRHITexture** TempTextures = CommandAllocator.Allocate<CRHITexture*>(NumTextures);
        for (uint32 i = 0; i < NumTextures; i++)
        {
            TempTextures[i] = Textures[i];
        }

        InsertCommand<CRHICommandSetShaderResourceTextures>(Shader, TempTextures, NumTextures, StartParameterIndex);
    }

    void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
    {
        InsertCommand<CRHICommandSetShaderResourceView>(Shader, ShaderResourceView, ParameterIndex);
    }

    void SetShaderResourceViews( CRHIShader* Shader
                               , CRHIShaderResourceView* const* ShaderResourceViews
                               , uint32 NumShaderResourceViews
                               , uint32 StartParameterIndex)
    {
        CRHIShaderResourceView** TempShaderResourceViews = CommandAllocator.Allocate<CRHIShaderResourceView*>(NumShaderResourceViews);
        for (uint32 i = 0; i < NumShaderResourceViews; i++)
        {
            TempShaderResourceViews[i] = ShaderResourceViews[i];
        }

        InsertCommand<CRHICommandSetShaderResourceViews>(Shader, TempShaderResourceViews, NumShaderResourceViews, StartParameterIndex);
    }

    void SetUnorderedAccessTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex)
    {
        InsertCommand<CRHICommandSetUnorderedAccessTexture>(Shader, Texture, ParameterIndex);
    }

    void SetUnorderedAccessTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex)
    {
        CRHITexture** TempTextures = CommandAllocator.Allocate<CRHITexture*>(NumTextures);
        for (uint32 i = 0; i < NumTextures; i++)
        {
            TempTextures[i] = Textures[i];
        }

        InsertCommand<CRHICommandSetUnorderedAccessTextures>(Shader, TempTextures, NumTextures, StartParameterIndex);
    }

    void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
    {
        InsertCommand<CRHICommandSetUnorderedAccessView>(Shader, UnorderedAccessView, ParameterIndex);
    }

    void SetUnorderedAccessViews( CRHIShader* Shader
                                , CRHIUnorderedAccessView* const* UnorderedAccessViews
                                , uint32 NumUnorderedAccessViews
                                , uint32 ParameterIndex)
    {
        CRHIUnorderedAccessView** TempUnorderedAccessViews = CommandAllocator.Allocate<CRHIUnorderedAccessView*>(NumUnorderedAccessViews);
        for (uint32 i = 0; i < NumUnorderedAccessViews; i++)
        {
            TempUnorderedAccessViews[i] = UnorderedAccessViews[i];
        }

        InsertCommand<CRHICommandSetUnorderedAccessViews>(Shader, TempUnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex);
    }

    void SetConstantBuffer(CRHIShader* Shader, CRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
    {
        InsertCommand<CRHICommandSetConstantBuffer>(Shader, ConstantBuffer, ParameterIndex);
    }

    void SetConstantBuffers(CRHIShader* Shader, CRHIBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 StartParameterIndex)
    {
        CRHIBuffer** TempConstantBuffers = CommandAllocator.Allocate<CRHIBuffer*>(NumConstantBuffers);
        for (uint32 i = 0; i < NumConstantBuffers; i++)
        {
            TempConstantBuffers[i] = ConstantBuffers[i];
        }

        InsertCommand<CRHICommandSetConstantBuffers>(Shader, TempConstantBuffers, NumConstantBuffers, StartParameterIndex);
    }

    void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex)
    {
        InsertCommand<CRHICommandSetSamplerState>(Shader, SamplerState, ParameterIndex);
    }

    void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 StartParameterIndex)
    {
        CRHISamplerState** TempSamplerStates = CommandAllocator.Allocate<CRHISamplerState*>(NumSamplerStates);
        for (uint32 i = 0; i < NumSamplerStates; i++)
        {
            TempSamplerStates[i] = SamplerStates[i];
        }

        InsertCommand<CRHICommandSetSamplerStates>(Shader, TempSamplerStates, NumSamplerStates, StartParameterIndex);
    }

    void UpdateBuffer(CRHIBuffer* Dst, uint64 DestinationOffsetInBytes, uint64 SizeInBytes, const void* SourceData)
    {
        void* TempSourceData = CommandAllocator.Allocate(static_cast<uint32>(SizeInBytes), 1);
        CMemory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        InsertCommand<CRHICommandUpdateBuffer>(Dst, DestinationOffsetInBytes, SizeInBytes, TempSourceData);
    }

    void UpdateTexture2D(CRHITexture* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
    {
        const uint32 SizeInBytes = Width * Height * GetByteStrideFromFormat(Dst->GetFormat());

        void* TempSourceData = CommandAllocator.Allocate(SizeInBytes, 1);
        CMemory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        InsertCommand<CRHICommandUpdateTexture2D>(Dst, Width, Height, MipLevel, TempSourceData);
    }

    void ResolveTexture(CRHITexture* Dst, CRHITexture* Src)
    {
        InsertCommand<CRHICommandResolveTexture>(Dst, Src);
    }

    void CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src, const SCopyBufferDesc& CopyInfo)
    {
        InsertCommand<CRHICommandCopyBuffer>(Dst, Src, CopyInfo);
    }

    void CopyTexture(CRHITexture* Dst, CRHITexture* Src)
    {
        InsertCommand<CRHICommandCopyTexture>(Dst, Src);
    }

    void CopyTextureRegion(CRHITexture* Dst, CRHITexture* Src, const SCopyTextureDesc& CopyTextureInfo)
    {
        InsertCommand<CRHICommandCopyTextureRegion>(Dst, Src, CopyTextureInfo);
    }

    void DestroyResource(CRHIResource* Resource)
    {
        InsertCommand<CRHICommandDestroyResource>(Resource);
    }

    void DiscardContents(CRHIResource* Resource)
    {
        InsertCommand<CRHICommandDiscardContents>(Resource);
    }

    void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIBuffer* VertexBuffer, CRHIBuffer* IndexBuffer, bool bUpdate)
    {
        Check((Geometry != nullptr) && (!bUpdate || (bUpdate && Geometry->GetFlags() & ERayTracingStructureFlag::AllowUpdate)));
        InsertCommand<CRHICommandBuildRayTracingGeometry>(Geometry, VertexBuffer, IndexBuffer, bUpdate);
    }

    void BuildRayTracingScene(CRHIRayTracingScene* Scene, const CRHIRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate)
    {
        Check((Scene != nullptr) && (!bUpdate || (bUpdate && Scene->GetFlags() & ERayTracingStructureFlag::AllowUpdate)));
        InsertCommand<CRHICommandBuildRayTracingScene>(Scene, Instances, NumInstances, bUpdate);
    }

    void GenerateMips(CRHITexture* Texture)
    {
        Check(Texture != nullptr);
        InsertCommand<CRHICommandGenerateMips>(Texture);
    }

    void TransitionTexture(CRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
    {
        Check(Texture != nullptr);

        if (BeforeState != AfterState)
        {
            InsertCommand<CRHICommandTransitionTexture>(Texture, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '" + Texture->GetName() + "' Was transitioned with the same Before- and AfterState (=" + ToString(BeforeState) + ")");
        }
    }

    void TransitionBuffer(CRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
    {
        Check(Buffer != nullptr);

        if (BeforeState != AfterState)
        {
            InsertCommand<CRHICommandTransitionBuffer>(Buffer, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Buffer '" + Buffer->GetName() + "' Was transitioned with the same Before- and AfterState (=" + ToString(BeforeState) + ")");
        }
    }

    void UnorderedAccessTextureBarrier(CRHITexture* Texture)
    {
        Check(Texture != nullptr);
        InsertCommand<CRHICommandUnorderedAccessTextureBarrier>(Texture);
    }

    void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer)
    {
        Check(Buffer != nullptr);
        InsertCommand<CRHICommandUnorderedAccessBufferBarrier>(Buffer);
    }

    void Draw(uint32 VertexCount, uint32 StartVertexLocation)
    {
        InsertCommand<CRHICommandDraw>(VertexCount, StartVertexLocation);
        NumDrawCalls++;
    }

    void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
    {
        InsertCommand<CRHICommandDrawIndexed>(IndexCount, StartIndexLocation, BaseVertexLocation);
        NumDrawCalls++;
    }

    void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
    {
        InsertCommand<CRHICommandDrawInstanced>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    void DrawIndexedInstanced( uint32 IndexCountPerInstance
                             , uint32 InstanceCount
                             , uint32 StartIndexLocation
                             , uint32 BaseVertexLocation
                             , uint32 StartInstanceLocation)
    {
        InsertCommand<CRHICommandDrawIndexedInstanced>( IndexCountPerInstance
                                                      , InstanceCount
                                                      , StartIndexLocation
                                                      , BaseVertexLocation
                                                      , StartInstanceLocation);

        NumDrawCalls++;
    }

    void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
    {
        InsertCommand<CRHICommandDispatch>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        NumDispatchCalls++;
    }

    void DispatchRays(CRHIRayTracingScene* Scene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
    {
        InsertCommand<CRHICommandDispatchRays>(Scene, PipelineState, Width, Height, Depth);
    }

    void PresentViewport(CRHIViewport* Viewport, bool bVerticalSync)
    {
        InsertCommand<CRHICommandPresentViewport>(Viewport, bVerticalSync);
    }

    void InsertCommandListMarker(const String& Marker)
    {
        InsertCommand<CRHICommandInsertMarker>(Marker);
    }
    
    void DebugBreak()
    {
        InsertCommand<CRHICommandDebugBreak>();
    }

    /** Begins a PIX capture event, currently only available on D3D12 */
    void BeginExternalCapture()
    {
        InsertCommand<CRHICommandBeginExternalCapture>();
    }

    /** Ends a PIX capture event, currently only available on D3D12 */
    void EndExternalCapture()
    {
        InsertCommand<CRHICommandEndExternalCapture>();
    }

    void Reset()
    {
        if (FirstCommand != nullptr)
        {
            CRHICommand* Command = FirstCommand;
            while (Command != nullptr)
            {
                CRHICommand* PreviousCommand = Command;
                Command = Command->NextCommand;

                PreviousCommand->~CRHICommand();
            }

            FirstCommand = nullptr;
            LastCommand  = nullptr;
        }

        NumDrawCalls     = 0;
        NumDispatchCalls = 0;
        NumCommands      = 0;

        CommandAllocator.Reset();
    }

    FORCEINLINE uint32 GetNumDrawCalls()     const { return NumDrawCalls; }
    FORCEINLINE uint32 GetNumDispatchCalls() const { return NumDispatchCalls; }
    FORCEINLINE uint32 GetNumCommands()      const { return NumCommands; }

private:

    template<typename CommandType, typename... ArgTypes>
    void InsertCommand(ArgTypes&&... Args)
    {
        CommandType* Cmd = CommandAllocator.Construct<CommandType>(Forward<ArgTypes>(Args)...);
        if (LastCommand)
        {
            LastCommand->NextCommand = Cmd;
            LastCommand = LastCommand->NextCommand;
        }
        else
        {
            FirstCommand = Cmd;
            LastCommand  = FirstCommand;
        }

        ++NumCommands;
    }

    CRHICommandAllocator CommandAllocator;

    CRHICommand*      FirstCommand = nullptr;
    CRHICommand*      LastCommand  = nullptr;

    uint32            NumDrawCalls     = 0;
    uint32            NumDispatchCalls = 0;
    uint32            NumCommands      = 0;
};
