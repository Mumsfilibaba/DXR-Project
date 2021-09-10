#pragma once
#include "Resources.h"
#include "RayTracing.h"
#include "RenderCommand.h"
#include "GPUProfiler.h"

#include "Core/Memory/LinearAllocator.h"

class RenderTargetView;
class DepthStencilView;
class ShaderResourceView;
class UnorderedAccessView;
class Shader;

#define ENABLE_INSERT_DEBUG_CMDLIST_MARKER 0

#if ENABLE_INSERT_DEBUG_CMDLIST_MARKER
#define INSERT_DEBUG_CMDLIST_MARKER(CmdList, MarkerString) CmdList.InsertCommandListMarker(MarkerString);
#else
#define INSERT_DEBUG_CMDLIST_MARKER(CmdList, MarkerString)
#endif

class CommandList
{
    friend class CommandListExecutor;

public:
    CommandList()
        : CmdAllocator()
        , First( nullptr )
        , Last( nullptr )
    {
    }

    ~CommandList()
    {
        Reset();
    }

    void BeginTimeStamp( GPUProfiler* Profiler, uint32 Index )
    {
        SafeAddRef( Profiler );
        InsertCommand<BeginTimeStampRenderCommand>( Profiler, Index );
    }

    void EndTimeStamp( GPUProfiler* Profiler, uint32 Index )
    {
        SafeAddRef( Profiler );
        InsertCommand<EndTimeStampRenderCommand>( Profiler, Index );
    }

    void ClearRenderTargetView( RenderTargetView* RenderTargetView, const ColorF& ClearColor )
    {
        Assert( RenderTargetView != nullptr );

        RenderTargetView->AddRef();
        InsertCommand<ClearRenderTargetViewRenderCommand>( RenderTargetView, ClearColor );
    }

    void ClearDepthStencilView( DepthStencilView* DepthStencilView, const DepthStencilF& ClearValue )
    {
        Assert( DepthStencilView != nullptr );

        DepthStencilView->AddRef();
        InsertCommand<ClearDepthStencilViewRenderCommand>( DepthStencilView, ClearValue );
    }

    void ClearUnorderedAccessView( UnorderedAccessView* UnorderedAccessView, const ColorF& ClearColor )
    {
        Assert( UnorderedAccessView != nullptr );

        UnorderedAccessView->AddRef();
        InsertCommand<ClearUnorderedAccessViewFloatRenderCommand>( UnorderedAccessView, ClearColor );
    }

    void SetShadingRate( EShadingRate ShadingRate )
    {
        InsertCommand<SetShadingRateRenderCommand>( ShadingRate );
    }

    void SetShadingRateImage( Texture2D* ShadingRateImage )
    {
        SafeAddRef( ShadingRateImage );
        InsertCommand<SetShadingRateImageRenderCommand>( ShadingRateImage );
    }

    void BeginRenderPass()
    {
        InsertCommand<BeginRenderPassRenderCommand>();
    }

    void EndRenderPass()
    {
        InsertCommand<EndRenderPassRenderCommand>();
    }

    void SetViewport( float Width, float Height, float MinDepth, float MaxDepth, float x, float y )
    {
        InsertCommand<SetViewportRenderCommand>( Width, Height, MinDepth, MaxDepth, x, y );
    }

    void SetScissorRect( float Width, float Height, float x, float y )
    {
        InsertCommand<SetScissorRectRenderCommand>( Width, Height, x, y );
    }

    void SetBlendFactor( const ColorF& Color )
    {
        InsertCommand<SetBlendFactorRenderCommand>( Color );
    }

    void SetRenderTargets( RenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, DepthStencilView* DepthStencilView )
    {
        RenderTargetView** RenderTargets = new(CmdAllocator) RenderTargetView * [RenderTargetCount];
        for ( uint32 i = 0; i < RenderTargetCount; i++ )
        {
            RenderTargets[i] = RenderTargetViews[i];
            SafeAddRef( RenderTargets[i] );
        }

        SafeAddRef( DepthStencilView );
        InsertCommand<SetRenderTargetsRenderCommand>( RenderTargets, RenderTargetCount, DepthStencilView );
    }

    void SetPrimitiveTopology( EPrimitiveTopology PrimitveTopologyType )
    {
        InsertCommand<SetPrimitiveTopologyRenderCommand>( PrimitveTopologyType );
    }

    void SetVertexBuffers( VertexBuffer* const* VertexBuffers, uint32 VertexBufferCount, uint32 BufferSlot )
    {
        VertexBuffer** Buffers = new(CmdAllocator) VertexBuffer * [VertexBufferCount];
        for ( uint32 i = 0; i < VertexBufferCount; i++ )
        {
            Buffers[i] = VertexBuffers[i];
            SafeAddRef( Buffers[i] );
        }

        InsertCommand<SetVertexBuffersRenderCommand>( Buffers, VertexBufferCount, BufferSlot );
    }

    void SetIndexBuffer( IndexBuffer* IndexBuffer )
    {
        SafeAddRef( IndexBuffer );
        InsertCommand<SetIndexBufferRenderCommand>( IndexBuffer );
    }

    void SetRayTracingBindings(
        RayTracingScene* RayTracingScene,
        RayTracingPipelineState* PipelineState,
        const RayTracingShaderResources* GlobalResource,
        const RayTracingShaderResources* RayGenLocalResources,
        const RayTracingShaderResources* MissLocalResources,
        const RayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources )
    {
        SafeAddRef( RayTracingScene );
        SafeAddRef( PipelineState );
        InsertCommand<SetRayTracingBindingsRenderCommand>(
            RayTracingScene,
            PipelineState,
            GlobalResource,
            RayGenLocalResources,
            MissLocalResources,
            HitGroupResources,
            NumHitGroupResources );
    }

    void SetGraphicsPipelineState( GraphicsPipelineState* PipelineState )
    {
        SafeAddRef( PipelineState );
        InsertCommand<SetGraphicsPipelineStateRenderCommand>( PipelineState );
    }

    void SetComputePipelineState( ComputePipelineState* PipelineState )
    {
        SafeAddRef( PipelineState );
        InsertCommand<SetComputePipelineStateRenderCommand>( PipelineState );
    }

    void Set32BitShaderConstants( Shader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants )
    {
        const uint32 Num32BitConstantsInBytes = Num32BitConstants * 4;
        void* Shader32BitConstantsMemory = CmdAllocator.Allocate( Num32BitConstantsInBytes, 1 );
        Memory::Memcpy( Shader32BitConstantsMemory, Shader32BitConstants, Num32BitConstantsInBytes );

        SafeAddRef( Shader );
        InsertCommand<Set32BitShaderConstantsRenderCommand>( Shader, Shader32BitConstantsMemory, Num32BitConstants );
    }

    void SetShaderResourceView( Shader* Shader, ShaderResourceView* ShaderResourceView, uint32 ParameterIndex )
    {
        SafeAddRef( Shader );
        SafeAddRef( ShaderResourceView );
        InsertCommand<SetShaderResourceViewRenderCommand>( Shader, ShaderResourceView, ParameterIndex );
    }

    void SetShaderResourceViews( Shader* Shader, ShaderResourceView* const* ShaderResourceViews, uint32 NumShaderResourceViews, uint32 ParameterIndex )
    {
        SafeAddRef( Shader );

        ShaderResourceView** TempShaderResourceViews = new(CmdAllocator) ShaderResourceView * [NumShaderResourceViews];
        for ( uint32 i = 0; i < NumShaderResourceViews; i++ )
        {
            TempShaderResourceViews[i] = ShaderResourceViews[i];
            SafeAddRef( TempShaderResourceViews[i] );
        }

        InsertCommand<SetShaderResourceViewsRenderCommand>( Shader, TempShaderResourceViews, NumShaderResourceViews, ParameterIndex );
    }

    void SetUnorderedAccessView( Shader* Shader, UnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex )
    {
        SafeAddRef( Shader );
        SafeAddRef( UnorderedAccessView );
        InsertCommand<SetUnorderedAccessViewRenderCommand>( Shader, UnorderedAccessView, ParameterIndex );
    }

    void SetUnorderedAccessViews( Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex )
    {
        UnorderedAccessView** TempUnorderedAccessViews = new(CmdAllocator) UnorderedAccessView * [NumUnorderedAccessViews];
        for ( uint32 i = 0; i < NumUnorderedAccessViews; i++ )
        {
            TempUnorderedAccessViews[i] = UnorderedAccessViews[i];
            SafeAddRef( TempUnorderedAccessViews[i] );
        }

        SafeAddRef( Shader );
        InsertCommand<SetUnorderedAccessViewsRenderCommand>( Shader, TempUnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex );
    }

    void SetConstantBuffer( Shader* Shader, ConstantBuffer* ConstantBuffer, uint32 ParameterIndex )
    {
        SafeAddRef( Shader );
        SafeAddRef( ConstantBuffer );
        InsertCommand<SetConstantBufferRenderCommand>( Shader, ConstantBuffer, ParameterIndex );
    }

    void SetConstantBuffers( Shader* Shader, ConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex )
    {
        ConstantBuffer** TempConstantBuffers = new(CmdAllocator) ConstantBuffer * [NumConstantBuffers];
        for ( uint32 i = 0; i < NumConstantBuffers; i++ )
        {
            TempConstantBuffers[i] = ConstantBuffers[i];
            SafeAddRef( TempConstantBuffers[i] );
        }

        SafeAddRef( Shader );
        InsertCommand<SetConstantBuffersRenderCommand>( Shader, TempConstantBuffers, NumConstantBuffers, ParameterIndex );
    }

    void SetSamplerState( Shader* Shader, SamplerState* SamplerState, uint32 ParameterIndex )
    {
        SafeAddRef( Shader );
        SafeAddRef( SamplerState );
        InsertCommand<SetSamplerStateRenderCommand>( Shader, SamplerState, ParameterIndex );
    }

    void SetSamplerStates( Shader* Shader, SamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex )
    {
        SamplerState** TempSamplerStates = new(CmdAllocator) SamplerState * [NumSamplerStates];
        for ( uint32 i = 0; i < NumSamplerStates; i++ )
        {
            TempSamplerStates[i] = SamplerStates[i];
            SafeAddRef( TempSamplerStates[i] );
        }

        SafeAddRef( Shader );
        InsertCommand<SetSamplerStatesRenderCommand>( Shader, TempSamplerStates, NumSamplerStates, ParameterIndex );
    }

    void ResolveTexture( Texture* Destination, Texture* Source )
    {
        SafeAddRef( Destination );
        SafeAddRef( Source );
        InsertCommand<ResolveTextureRenderCommand>( Destination, Source );
    }

    void UpdateBuffer( Buffer* Destination, uint64 DestinationOffsetInBytes, uint64 SizeInBytes, const void* SourceData )
    {
        void* TempSourceData = CmdAllocator.Allocate( SizeInBytes, 1 );
        Memory::Memcpy( TempSourceData, SourceData, SizeInBytes );

        SafeAddRef( Destination );
        InsertCommand<UpdateBufferRenderCommand>( Destination, DestinationOffsetInBytes, SizeInBytes, TempSourceData );
    }

    void UpdateTexture2D( Texture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData )
    {
        Assert( Destination != nullptr );

        const uint32 SizeInBytes = Width * Height * GetByteStrideFromFormat( Destination->GetFormat() );

        void* TempSourceData = CmdAllocator.Allocate( SizeInBytes, 1 );
        Memory::Memcpy( TempSourceData, SourceData, SizeInBytes );

        Destination->AddRef();
        InsertCommand<UpdateTexture2DRenderCommand>( Destination, Width, Height, MipLevel, TempSourceData );
    }

    void CopyBuffer( Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo )
    {
        SafeAddRef( Destination );
        SafeAddRef( Source );
        InsertCommand<CopyBufferRenderCommand>( Destination, Source, CopyInfo );
    }

    void CopyTexture( Texture* Destination, Texture* Source )
    {
        SafeAddRef( Destination );
        SafeAddRef( Source );
        InsertCommand<CopyTextureRenderCommand>( Destination, Source );
    }

    void CopyTextureRegion( Texture* Destination, Texture* Source, const CopyTextureInfo& CopyTextureInfo )
    {
        SafeAddRef( Destination );
        SafeAddRef( Source );
        InsertCommand<CopyTextureRegionRenderCommand>( Destination, Source, CopyTextureInfo );
    }

    void DiscardResource( Resource* Resource )
    {
        SafeAddRef( Resource );
        InsertCommand<DiscardResourceRenderCommand>( Resource );
    }

    void BuildRayTracingGeometry( RayTracingGeometry* Geometry, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer, bool Update )
    {
        Assert( Geometry != nullptr );
        Assert( !Update || (Update && Geometry->GetFlags() & RayTracingStructureBuildFlag_AllowUpdate) );

        SafeAddRef( Geometry );
        SafeAddRef( VertexBuffer );
        SafeAddRef( IndexBuffer );
        InsertCommand<BuildRayTracingGeometryRenderCommand>( Geometry, VertexBuffer, IndexBuffer, Update );
    }

    void BuildRayTracingScene( RayTracingScene* Scene, const RayTracingGeometryInstance* Instances, uint32 NumInstances, bool Update )
    {
        Assert( Scene != nullptr );
        Assert( !Update || (Update && Scene->GetFlags() & RayTracingStructureBuildFlag_AllowUpdate) );

        SafeAddRef( Scene );
        InsertCommand<BuildRayTracingSceneRenderCommand>( Scene, Instances, NumInstances, Update );
    }

    void GenerateMips( Texture* Texture )
    {
        Assert( Texture != nullptr );

        Texture->AddRef();
        InsertCommand<GenerateMipsRenderCommand>( Texture );
    }

    void TransitionTexture( Texture* Texture, EResourceState BeforeState, EResourceState AfterState )
    {
        Assert( Texture != nullptr );

        if ( BeforeState != AfterState )
        {
            Texture->AddRef();
            InsertCommand<TransitionTextureRenderCommand>( Texture, BeforeState, AfterState );
        }
        else
        {
            LOG_WARNING( "Texture '" + Texture->GetName() + "' Was transitioned with the same Before- and AfterState (=" + ToString( BeforeState ) + ")" );
        }
    }

    void TransitionBuffer( Buffer* Buffer, EResourceState BeforeState, EResourceState AfterState )
    {
        Assert( Buffer != nullptr );

        if ( BeforeState != AfterState )
        {
            Buffer->AddRef();
            InsertCommand<TransitionBufferRenderCommand>( Buffer, BeforeState, AfterState );
        }
    }

    void UnorderedAccessTextureBarrier( Texture* Texture )
    {
        Assert( Texture != nullptr );

        Texture->AddRef();
        InsertCommand<UnorderedAccessTextureBarrierRenderCommand>( Texture );
    }

    void UnorderedAccessBufferBarrier( Buffer* Buffer )
    {
        Assert( Buffer != nullptr );

        Buffer->AddRef();
        InsertCommand<UnorderedAccessBufferBarrierRenderCommand>( Buffer );
    }

    void Draw( uint32 VertexCount, uint32 StartVertexLocation )
    {
        InsertCommand<DrawRenderCommand>( VertexCount, StartVertexLocation );
        NumDrawCalls++;
    }

    void DrawIndexed( uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation )
    {
        InsertCommand<DrawIndexedRenderCommand>( IndexCount, StartIndexLocation, BaseVertexLocation );
        NumDrawCalls++;
    }

    void DrawInstanced( uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation )
    {
        InsertCommand<DrawInstancedRenderCommand>( VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation );
        NumDrawCalls++;
    }

    void DrawIndexedInstanced(
        uint32 IndexCountPerInstance,
        uint32 InstanceCount,
        uint32 StartIndexLocation,
        uint32 BaseVertexLocation,
        uint32 StartInstanceLocation )
    {
        InsertCommand<DrawIndexedInstancedRenderCommand>( IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation );
        NumDrawCalls++;
    }

    void Dispatch( uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ )
    {
        InsertCommand<DispatchComputeRenderCommand>( ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ );
        NumDispatchCalls++;
    }

    void DispatchRays(
        RayTracingScene* Scene,
        RayTracingPipelineState* PipelineState,
        uint32 Width,
        uint32 Height,
        uint32 Depth )
    {
        SafeAddRef( Scene );
        SafeAddRef( PipelineState );
        InsertCommand<DispatchRaysRenderCommand>( Scene, PipelineState, Width, Height, Depth );
    }

    void InsertCommandListMarker( const std::string& Marker )
    {
        InsertCommand<InsertCommandListMarkerRenderCommand>( Marker );
    }

    void DebugBreak()
    {
        InsertCommand<DebugBreakRenderCommand>();
    }

    void BeginExternalCapture()
    {
        InsertCommand<BeginExternalCaptureRenderCommand>();
    }

    void EndExternalCapture()
    {
        InsertCommand<EndExternalCaptureRenderCommand>();
    }

    void Reset()
    {
        if ( First != nullptr )
        {
            RenderCommand* Cmd = First;
            while ( Cmd != nullptr )
            {
                RenderCommand* Old = Cmd;
                Cmd = Cmd->NextCmd;
                Old->~RenderCommand();
            }

            First = nullptr;
            Last = nullptr;
        }

        NumDrawCalls = 0;
        NumDispatchCalls = 0;
        NumCommands = 0;

        CmdAllocator.Reset();
    }

    uint32 GetNumDrawCalls()     const
    {
        return NumDrawCalls;
    }
    uint32 GetNumDispatchCalls() const
    {
        return NumDispatchCalls;
    }
    uint32 GetNumCommands()      const
    {
        return NumCommands;
    }

private:
    template<typename TCommand, typename... TArgs>
    void InsertCommand( TArgs&&... Args )
    {
        TCommand* Cmd = new(CmdAllocator) TCommand( Forward<TArgs>( Args )... );
        if ( Last )
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

    LinearAllocator CmdAllocator;
    RenderCommand* First;
    RenderCommand* Last;

    uint32 NumDrawCalls = 0;
    uint32 NumDispatchCalls = 0;
    uint32 NumCommands = 0;
};

class CommandListExecutor
{
public:
    void ExecuteCommandList( CommandList& CmdList );
    void ExecuteCommandLists( CommandList* const* CmdLists, uint32 NumCmdLists );

    void WaitForGPU();

    void SetContext( ICommandContext* InCmdContext )
    {
        Assert( InCmdContext != nullptr );
        CmdContext = InCmdContext;
    }

    ICommandContext& GetContext()
    {
        Assert( CmdContext != nullptr );
        return *CmdContext;
    }

private:
    void InternalExecuteCommandList( CommandList& CmdList );

    ICommandContext* CmdContext = nullptr;
};

extern CommandListExecutor GCmdListExecutor;