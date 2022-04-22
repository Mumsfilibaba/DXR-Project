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
private:

    CRHICommandExecutionManager();
    ~CRHICommandExecutionManager() = default;

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
        Check(TimestampQuery != nullptr);
        InsertCommand<CRHICommandBeginTimeStamp>(TimestampQuery, Index);
    }

    void EndTimeStamp(CRHITimeQuery* TimestampQuery, uint32 Index)
    {
        Check(TimestampQuery != nullptr);
        InsertCommand<CRHICommandEndTimeStamp>(TimestampQuery, Index);
    }

    void ClearRenderTargetView(const CRHIRenderTargetView& RenderTargetView, const TStaticArray<float, 4>& ClearColor)
    {
        Check(RenderTargetView.Texture != nullptr);
        InsertCommand<CRHICommandClearRenderTargetView>(RenderTargetView, ClearColor);
    }

    void ClearDepthStencilView(const CRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
    {
        Check(DepthStencilView.Texture != nullptr);
        InsertCommand<CRHICommandClearDepthStencilView>(DepthStencilView, Depth, Stencil);
    }

    void ClearUnorderedAccessTextureFloat(CRHITexture* Texture, const TStaticArray<float, 4>& ClearColor)
    {
        Check(Texture != nullptr);
        InsertCommand<CRHICommandClearUnorderedAccessTextureFloat>(Texture, ClearColor);
    }

    void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor)
    {
        Check(UnorderedAccessView != nullptr);
        InsertCommand<CRHICommandClearUnorderedAccessViewFloat>(UnorderedAccessView, ClearColor);
    }

    void ClearUnorderedAccessTextureUint(CRHITexture* Texture, const TStaticArray<uint32, 4>& ClearColor)
    {
        Check(Texture != nullptr);
        InsertCommand<CRHICommandClearUnorderedAccessTextureUint>(Texture, ClearColor);
    }

    void ClearUnorderedAccessViewUint(CRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<uint32, 4>& ClearColor)
    {
        Check(UnorderedAccessView != nullptr);
        InsertCommand<CRHICommandClearUnorderedAccessViewUint>(UnorderedAccessView, ClearColor);
    }

    void BeginRenderPass(const CRHIRenderPassInitializer& Initializer)
    {
        InsertCommand<CRHICommandBeginRenderPass>(Initializer);
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

    void SetBlendFactor(const TStaticArray<float, 4>& Color)
    {
        InsertCommand<CRHICommandSetBlendFactor>(Color);
    }

    void SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 NumVertexBuffers, uint32 BufferSlot)
    {
        CRHIVertexBuffer** TempBuffers = CommandAllocator.Allocate<CRHIVertexBuffer*>(NumVertexBuffers);
        
        // Setting VertexBuffers to nullptr clears all the buffers in the CommandContext
        if (VertexBuffers)
        {
            CMemory::Memcpy(TempBuffers, VertexBuffers, NumVertexBuffers);
        }
        else
        {
            CMemory::Memzero(TempBuffers, NumVertexBuffers);
        }

        InsertCommand<CRHICommandSetVertexBuffers>(TempBuffers, NumVertexBuffers, BufferSlot);
    }

    void SetIndexBuffer(CRHIIndexBuffer* IndexBuffer)
    {
        InsertCommand<CRHICommandSetIndexBuffer>(IndexBuffer);
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

    void Set32BitShaderConstants(CRHIShader* Shader, const SSetShaderConstantsInfo& ShaderConstantsInfo)
    {
        Check(Shader != nullptr);
        InsertCommand<CRHICommandSet32BitShaderConstants>(Shader, ShaderConstantsInfo);
    }

    void SetShaderResourceTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex)
    {
        Check(Shader != nullptr);
        InsertCommand<CRHICommandSetShaderResourceTexture>(Shader, Texture, ParameterIndex);
    }

    void SetShaderResourceTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex)
    {
        Check(Shader != nullptr);

        CRHITexture** TempTextures = CommandAllocator.Allocate<CRHITexture*>(NumTextures);

        // Setting Textures to nullptr clears all the buffers in the CommandContext
        if (Textures)
        {
            CMemory::Memcpy(TempTextures, Textures, NumTextures);
        }
        else
        {
            CMemory::Memzero(TempTextures, NumTextures);
        }

        InsertCommand<CRHICommandSetShaderResourceTextures>(Shader, TempTextures, NumTextures, StartParameterIndex);
    }

    void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
    {
        Check(Shader != nullptr);
        InsertCommand<CRHICommandSetShaderResourceView>(Shader, ShaderResourceView, ParameterIndex);
    }

    void SetShaderResourceViews( CRHIShader* Shader
                               , CRHIShaderResourceView* const* ShaderResourceViews
                               , uint32 NumShaderResourceViews
                               , uint32 StartParameterIndex)
    {
        Check(Shader != nullptr);

        CRHIShaderResourceView** TempShaderResourceViews = CommandAllocator.Allocate<CRHIShaderResourceView*>(NumShaderResourceViews);
        
        // Setting ShaderResourceViews to nullptr clears all the buffers in the CommandContext
        if (ShaderResourceViews)
        {
            CMemory::Memcpy(TempShaderResourceViews, ShaderResourceViews, NumShaderResourceViews);
        }
        else
        {
            CMemory::Memzero(TempShaderResourceViews, NumShaderResourceViews);
        }

        InsertCommand<CRHICommandSetShaderResourceViews>(Shader, TempShaderResourceViews, NumShaderResourceViews, StartParameterIndex);
    }

    void SetUnorderedAccessTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex)
    {
        Check(Shader != nullptr);
        InsertCommand<CRHICommandSetUnorderedAccessTexture>(Shader, Texture, ParameterIndex);
    }

    void SetUnorderedAccessTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex)
    {
        Check(Shader != nullptr);

        CRHITexture** TempTextures = CommandAllocator.Allocate<CRHITexture*>(NumTextures);
        
        // Setting Textures to nullptr clears all the buffers in the CommandContext
        if (Textures)
        {
            CMemory::Memcpy(TempTextures, Textures, NumTextures);
        }
        else
        {
            CMemory::Memzero(TempTextures, NumTextures);
        }

        InsertCommand<CRHICommandSetUnorderedAccessTextures>(Shader, TempTextures, NumTextures, StartParameterIndex);
    }

    void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
    {
        Check(Shader != nullptr);
        InsertCommand<CRHICommandSetUnorderedAccessView>(Shader, UnorderedAccessView, ParameterIndex);
    }

    void SetUnorderedAccessViews( CRHIShader* Shader
                                , CRHIUnorderedAccessView* const* UnorderedAccessViews
                                , uint32 NumUnorderedAccessViews
                                , uint32 ParameterIndex)
    {
        Check(Shader != nullptr);

        CRHIUnorderedAccessView** TempUnorderedAccessViews = CommandAllocator.Allocate<CRHIUnorderedAccessView*>(NumUnorderedAccessViews);
        
        // Setting UnorderedAccessViews to nullptr clears all the buffers in the CommandContext
        if (UnorderedAccessViews)
        {
            CMemory::Memcpy(TempUnorderedAccessViews, UnorderedAccessViews, NumUnorderedAccessViews);
        }
        else
        {
            CMemory::Memzero(TempUnorderedAccessViews, NumUnorderedAccessViews);
        }

        InsertCommand<CRHICommandSetUnorderedAccessViews>(Shader, TempUnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex);
    }

    void SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
    {
        Check(Shader != nullptr);
        InsertCommand<CRHICommandSetConstantBuffer>(Shader, ConstantBuffer, ParameterIndex);
    }

    void SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 StartParameterIndex)
    {
        Check(Shader != nullptr);

        CRHIConstantBuffer** TempConstantBuffers = CommandAllocator.Allocate<CRHIConstantBuffer*>(NumConstantBuffers);
        
        // Setting ConstantBuffers to nullptr clears all the buffers in the CommandContext
        if (ConstantBuffers)
        {
            CMemory::Memcpy(TempConstantBuffers, ConstantBuffers, NumConstantBuffers);
        }
        else
        {
            CMemory::Memzero(TempConstantBuffers, NumConstantBuffers);
        }

        InsertCommand<CRHICommandSetConstantBuffers>(Shader, TempConstantBuffers, NumConstantBuffers, StartParameterIndex);
    }

    void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex)
    {
        Check(Shader != nullptr);
        InsertCommand<CRHICommandSetSamplerState>(Shader, SamplerState, ParameterIndex);
    }

    void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 StartParameterIndex)
    {
        Check(Shader != nullptr);

        CRHISamplerState** TempSamplerStates = CommandAllocator.Allocate<CRHISamplerState*>(NumSamplerStates);
        
        // Setting SamplerStates to nullptr clears all the Samplers in the CommandContext
        if (SamplerStates)
        {
            CMemory::Memcpy(TempSamplerStates, SamplerStates, NumSamplerStates);
        }
        else
        {
            CMemory::Memzero(TempSamplerStates, NumSamplerStates);
        }

        InsertCommand<CRHICommandSetSamplerStates>(Shader, TempSamplerStates, NumSamplerStates, StartParameterIndex);
    }

    void UpdateBuffer(CRHIBuffer* Dst, const void* SrcData, uint32 Offset, uint32 Size)
    {
        Check(Dst != nullptr);

        void* TempSrcData = CommandAllocator.Allocate(static_cast<uint32>(Size));
        CMemory::Memcpy(TempSrcData, SrcData, Size);

        InsertCommand<CRHICommandUpdateBuffer>(Dst, TempSrcData, Offset, Size);
    }

    void UpdateTexture2D(CRHITexture2D* Dst, const void* SrcData, uint16 Width, uint16 Height, uint16 MipLevel)
    {
        Check(Dst     != nullptr);
        Check(SrcData != nullptr);

        const uint32 Size = Width * Height * GetByteStrideFromFormat(Dst->GetFormat());

        void* TempSrcData = CommandAllocator.Allocate(Size);
        CMemory::Memcpy(TempSrcData, SrcData, Size);

        InsertCommand<CRHICommandUpdateTexture2D>(Dst, TempSrcData, Width, Height, MipLevel);
    }

    void UpdateTexture2DArray( CRHITexture2DArray* Dst
                             , const void* SrcData
                             , uint16 Width
                             , uint16 Height
                             , uint16 MipLevel
                             , uint16 ArrayIndex
                             , uint16 NumArraySlices)
    {
        Check(Dst     != nullptr);
        Check(SrcData != nullptr);

        const uint32 Size = Width * Height * GetByteStrideFromFormat(Dst->GetFormat());

        void* TempSrcData = CommandAllocator.Allocate(Size);
        CMemory::Memcpy(TempSrcData, SrcData, Size);

        InsertCommand<CRHICommandUpdateTexture2DArray>(Dst, TempSrcData, Width, Height, MipLevel, ArrayIndex, NumArraySlices);
    }

    void ResolveTexture(CRHITexture* Dst, CRHITexture* Src)
    {
        Check(Dst != nullptr);
        Check(Src != nullptr);
        InsertCommand<CRHICommandResolveTexture>(Dst, Src);
    }

    void CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src)
    {
        Check(Dst != nullptr);
        Check(Src != nullptr);
        InsertCommand<CRHICommandCopyBuffer>(Dst, Src);
    }

    void CopyBufferRegion(const SCopyBufferRegionInfo& CopyInfo)
    {
        Check(CopyInfo.Dst != nullptr);
        Check(CopyInfo.Src != nullptr);
        InsertCommand<CRHICommandCopyBufferRegion>(CopyInfo);
    }

    void CopyTexture(CRHITexture* Dst, CRHITexture* Src)
    {
        Check(Dst != nullptr);
        Check(Src != nullptr);
        InsertCommand<CRHICommandCopyTexture>(Dst, Src);
    }

    void CopyTexture2DRegion(CRHITexture2D* Dst, CRHITexture2D* Src, const SCopyTexture2DRegionInfo& CopyInfo)
    {
        InsertCommand<CRHICommandCopyTexture2DRegion>(Dst, Src, CopyInfo);
    }

    void CopyTexture2DArrayRegion(CRHITexture2DArray* Dst, CRHITexture2DArray* Src, const SCopyTexture2DArrayRegion& CopyInfo)
    {
        InsertCommand<CRHICommandCopyTexture2DArrayRegion>(Dst, Src, CopyInfo);
    }

    void DestroyResource(CRHIResource* Resource)
    {
        InsertCommand<CRHICommandDestroyResource>(Resource);
    }

    void DiscardContents(CRHIResource* Resource)
    {
        InsertCommand<CRHICommandDiscardContents>(Resource);
    }

    void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, const SBuildRayTracingGeometryInfo& BuildInfo)
    {
        Check(Geometry != nullptr);
        Check((BuildInfo.BuildType != ERayTracingStructureBuildType::Update) ||
              (BuildInfo.BuildType == ERayTracingStructureBuildType::Update && Geometry->GetFlags() & ERayTracingStructureBuildFlag::AllowUpdate));
        
        InsertCommand<CRHICommandBuildRayTracingGeometry>(Geometry, BuildInfo);
    }

    void BuildRayTracingScene(CRHIRayTracingScene* Scene, const SBuildRayTracingSceneInfo& BuildInfo)
    {
        Check(Scene != nullptr);
        Check((BuildInfo.BuildType != ERayTracingStructureBuildType::Update) ||
              (BuildInfo.BuildType == ERayTracingStructureBuildType::Update && Scene->GetFlags() & ERayTracingStructureBuildFlag::AllowUpdate));

        InsertCommand<CRHICommandBuildRayTracingScene>(Scene, BuildInfo);
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
        Check(VertexCount > 0);

        InsertCommand<CRHICommandDraw>(VertexCount, StartVertexLocation);
        NumDrawCalls++;
    }

    void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
    {
        Check(IndexCount > 0);

        InsertCommand<CRHICommandDrawIndexed>(IndexCount, StartIndexLocation, BaseVertexLocation);
        NumDrawCalls++;
    }

    void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
    {
        Check(VertexCountPerInstance > 0);
        Check(InstanceCount          > 0);

        InsertCommand<CRHICommandDrawInstanced>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    void DrawIndexedInstanced( uint32 IndexCountPerInstance
                             , uint32 InstanceCount
                             , uint32 StartIndexLocation
                             , uint32 BaseVertexLocation
                             , uint32 StartInstanceLocation)
    {
        Check(IndexCountPerInstance > 0);
        Check(InstanceCount         > 0);

        InsertCommand<CRHICommandDrawIndexedInstanced>( IndexCountPerInstance
                                                      , InstanceCount
                                                      , StartIndexLocation
                                                      , BaseVertexLocation
                                                      , StartInstanceLocation);

        NumDrawCalls++;
    }

    void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
    {
        Check(ThreadGroupCountX > 0);
        Check(ThreadGroupCountY > 0);
        Check(ThreadGroupCountZ > 0);

        InsertCommand<CRHICommandDispatch>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        NumDispatchCalls++;
    }

    void DispatchRays(CRHIRayTracingScene* Scene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
    {
        Check(Width  > 0);
        Check(Height > 0);
        Check(Depth  > 0);

        InsertCommand<CRHICommandDispatchRays>(Scene, PipelineState, Width, Height, Depth);
    }

    void PresentViewport(CRHIViewport* Viewport, bool bVerticalSync)
    {
        Check(Viewport != nullptr);
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

    /** @brief: Begins a PIX capture event, currently only available on D3D12 */
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
        CommandType* Command = CommandAllocator.Construct<CommandType>(Forward<ArgTypes>(Args)...);
        if (LastCommand)
        {
            LastCommand->NextCommand = Command;
            LastCommand = LastCommand->NextCommand;
        }
        else
        {
            FirstCommand = Command;
            LastCommand  = FirstCommand;
        }

        ++NumCommands;
    }

    CRHICommandAllocator CommandAllocator;

    CRHICommand*         FirstCommand     = nullptr;
    CRHICommand*         LastCommand      = nullptr;
                         
    uint32               NumDrawCalls     = 0;
    uint32               NumDispatchCalls = 0;
    uint32               NumCommands      = 0;
};
