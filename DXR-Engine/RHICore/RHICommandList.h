#pragma once
#include "RHIResources.h"
#include "RHIRenderCommand.h"
#include "GPUProfiler.h"

#include "Core/Memory/LinearAllocator.h"

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

class CRHICommandList
{
    friend class CRHICommandQueue;

public:
    CRHICommandList()
        : CmdAllocator()
        , First( nullptr )
        , Last( nullptr )
    {
    }

    ~CRHICommandList()
    {
        Reset();
    }

    void BeginTimeStamp( CGPUProfiler* Profiler, uint32 Index )
    {
        SafeAddRef( Profiler );
        InsertCommand<SRHIBeginTimeStampRenderCommand>( Profiler, Index );
    }

    void EndTimeStamp( CGPUProfiler* Profiler, uint32 Index )
    {
        SafeAddRef( Profiler );
        InsertCommand<SRHIEndTimeStampRenderCommand>( Profiler, Index );
    }

    void ClearRenderTargetView( CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor )
    {
        Assert( RenderTargetView != nullptr );

        RenderTargetView->AddRef();
        InsertCommand<SRHIClearRenderTargetViewRenderCommand>( RenderTargetView, ClearColor );
    }

    void ClearDepthStencilView( CRHIDepthStencilView* DepthStencilView, const SDepthStencilF& ClearValue )
    {
        Assert( DepthStencilView != nullptr );

        DepthStencilView->AddRef();
        InsertCommand<SRHIClearDepthStencilViewRenderCommand>( DepthStencilView, ClearValue );
    }

    void ClearUnorderedAccessView( CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor )
    {
        Assert( UnorderedAccessView != nullptr );

        UnorderedAccessView->AddRef();
        InsertCommand<SRHIClearUnorderedAccessViewFloatRenderCommand>( UnorderedAccessView, ClearColor );
    }

    void SetShadingRate( EShadingRate ShadingRate )
    {
        InsertCommand<SRHISetShadingRateRenderCommand>( ShadingRate );
    }

    void SetShadingRateImage( CRHITexture2D* ShadingRateImage )
    {
        SafeAddRef( ShadingRateImage );
        InsertCommand<SRHISetShadingRateImageRenderCommand>( ShadingRateImage );
    }

    void BeginRenderPass()
    {
        InsertCommand<SRHIBeginRenderPassRenderCommand>();
    }

    void EndRenderPass()
    {
        InsertCommand<SRHIEndRenderPassRenderCommand>();
    }

    void SetViewport( float Width, float Height, float MinDepth, float MaxDepth, float x, float y )
    {
        InsertCommand<SRHISetViewportRenderCommand>( Width, Height, MinDepth, MaxDepth, x, y );
    }

    void SetScissorRect( float Width, float Height, float x, float y )
    {
        InsertCommand<SRHISetScissorRectRenderCommand>( Width, Height, x, y );
    }

    void SetBlendFactor( const SColorF& Color )
    {
        InsertCommand<SRHISetBlendFactorRenderCommand>( Color );
    }

    void SetRenderTargets( CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView )
    {
        CRHIRenderTargetView** RenderTargets = new(CmdAllocator) CRHIRenderTargetView * [RenderTargetCount];
        for ( uint32 i = 0; i < RenderTargetCount; i++ )
        {
            RenderTargets[i] = RenderTargetViews[i];
            SafeAddRef( RenderTargets[i] );
        }

        SafeAddRef( DepthStencilView );
        InsertCommand<SRHISetRenderTargetsRenderCommand>( RenderTargets, RenderTargetCount, DepthStencilView );
    }

    void SetPrimitiveTopology( EPrimitiveTopology PrimitveTopologyType )
    {
        InsertCommand<SRHISetPrimitiveTopologyRenderCommand>( PrimitveTopologyType );
    }

    void SetVertexBuffers( CRHIVertexBuffer* const* VertexBuffers, uint32 VertexBufferCount, uint32 BufferSlot )
    {
        CRHIVertexBuffer** Buffers = new(CmdAllocator) CRHIVertexBuffer * [VertexBufferCount];
        for ( uint32 i = 0; i < VertexBufferCount; i++ )
        {
            Buffers[i] = VertexBuffers[i];
            SafeAddRef( Buffers[i] );
        }

        InsertCommand<SRHISetVertexBuffersRenderCommand>( Buffers, VertexBufferCount, BufferSlot );
    }

    void SetIndexBuffer( CRHIIndexBuffer* IndexBuffer )
    {
        SafeAddRef( IndexBuffer );
        InsertCommand<SRHISetIndexBufferRenderCommand>( IndexBuffer );
    }

    void SetRayTracingBindings(
        CRHIRayTracingScene* RayTracingScene,
        CRHIRayTracingPipelineState* PipelineState,
        const SRayTracingShaderResources* GlobalResource,
        const SRayTracingShaderResources* RayGenLocalResources,
        const SRayTracingShaderResources* MissLocalResources,
        const SRayTracingShaderResources* HitGroupResources, 
        uint32 NumHitGroupResources )
    {
        SafeAddRef( RayTracingScene );
        SafeAddRef( PipelineState );
        InsertCommand<SRHISetRayTracingBindingsRenderCommand>( RayTracingScene, PipelineState, GlobalResource, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources );
    }

    void SetGraphicsPipelineState( CRHIGraphicsPipelineState* PipelineState )
    {
        SafeAddRef( PipelineState );
        InsertCommand<SRHISetGraphicsPipelineStateRenderCommand>( PipelineState );
    }

    void SetComputePipelineState( CRHIComputePipelineState* PipelineState )
    {
        SafeAddRef( PipelineState );
        InsertCommand<SRHISetComputePipelineStateRenderCommand>( PipelineState );
    }

    void Set32BitShaderConstants( CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants )
    {
        const uint32 Num32BitConstantsInBytes = Num32BitConstants * 4;
        void* Shader32BitConstantsMemory = CmdAllocator.Allocate( Num32BitConstantsInBytes, 1 );
        CMemory::Memcpy( Shader32BitConstantsMemory, Shader32BitConstants, Num32BitConstantsInBytes );

        SafeAddRef( Shader );
        InsertCommand<SRHISet32BitShaderConstantsRenderCommand>( Shader, Shader32BitConstantsMemory, Num32BitConstants );
    }

    void SetShaderResourceView( CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex )
    {
        SafeAddRef( Shader );
        SafeAddRef( ShaderResourceView );
        InsertCommand<SRHISetShaderResourceViewRenderCommand>( Shader, ShaderResourceView, ParameterIndex );
    }

    void SetShaderResourceViews( CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceViews, uint32 NumShaderResourceViews, uint32 ParameterIndex )
    {
        SafeAddRef( Shader );

        CRHIShaderResourceView** TempShaderResourceViews = new(CmdAllocator) CRHIShaderResourceView * [NumShaderResourceViews];
        for ( uint32 i = 0; i < NumShaderResourceViews; i++ )
        {
            TempShaderResourceViews[i] = ShaderResourceViews[i];
            SafeAddRef( TempShaderResourceViews[i] );
        }

        InsertCommand<SRHISetShaderResourceViewsRenderCommand>( Shader, TempShaderResourceViews, NumShaderResourceViews, ParameterIndex );
    }

    void SetUnorderedAccessView( CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex )
    {
        SafeAddRef( Shader );
        SafeAddRef( UnorderedAccessView );
        InsertCommand<SRHISetUnorderedAccessViewRenderCommand>( Shader, UnorderedAccessView, ParameterIndex );
    }

    void SetUnorderedAccessViews( CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex )
    {
        CRHIUnorderedAccessView** TempUnorderedAccessViews = new(CmdAllocator) CRHIUnorderedAccessView * [NumUnorderedAccessViews];
        for ( uint32 i = 0; i < NumUnorderedAccessViews; i++ )
        {
            TempUnorderedAccessViews[i] = UnorderedAccessViews[i];
            SafeAddRef( TempUnorderedAccessViews[i] );
        }

        SafeAddRef( Shader );
        InsertCommand<SRHISetUnorderedAccessViewsRenderCommand>( Shader, TempUnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex );
    }

    void SetConstantBuffer( CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex )
    {
        SafeAddRef( Shader );
        SafeAddRef( ConstantBuffer );
        InsertCommand<SRHISetConstantBufferRenderCommand>( Shader, ConstantBuffer, ParameterIndex );
    }

    void SetConstantBuffers( CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex )
    {
        CRHIConstantBuffer** TempConstantBuffers = new(CmdAllocator) CRHIConstantBuffer * [NumConstantBuffers];
        for ( uint32 i = 0; i < NumConstantBuffers; i++ )
        {
            TempConstantBuffers[i] = ConstantBuffers[i];
            SafeAddRef( TempConstantBuffers[i] );
        }

        SafeAddRef( Shader );
        InsertCommand<SRHISetConstantBuffersRenderCommand>( Shader, TempConstantBuffers, NumConstantBuffers, ParameterIndex );
    }

    void SetSamplerState( CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex )
    {
        SafeAddRef( Shader );
        SafeAddRef( SamplerState );
        InsertCommand<SRHISetSamplerStateRenderCommand>( Shader, SamplerState, ParameterIndex );
    }

    void SetSamplerStates( CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex )
    {
        CRHISamplerState** TempSamplerStates = new(CmdAllocator) CRHISamplerState * [NumSamplerStates];
        for ( uint32 i = 0; i < NumSamplerStates; i++ )
        {
            TempSamplerStates[i] = SamplerStates[i];
            SafeAddRef( TempSamplerStates[i] );
        }

        SafeAddRef( Shader );
        InsertCommand<SRHISetSamplerStatesRenderCommand>( Shader, TempSamplerStates, NumSamplerStates, ParameterIndex );
    }

    void ResolveTexture( CRHITexture* Destination, CRHITexture* Source )
    {
        SafeAddRef( Destination );
        SafeAddRef( Source );
        InsertCommand<SRHIResolveTextureRenderCommand>( Destination, Source );
    }

    void UpdateBuffer( CRHIBuffer* Destination, uint64 DestinationOffsetInBytes, uint64 SizeInBytes, const void* SourceData )
    {
        void* TempSourceData = CmdAllocator.Allocate( SizeInBytes, 1 );
        CMemory::Memcpy( TempSourceData, SourceData, SizeInBytes );

        SafeAddRef( Destination );
        InsertCommand<SRHIUpdateBufferRenderCommand>( Destination, DestinationOffsetInBytes, SizeInBytes, TempSourceData );
    }

    void UpdateTexture2D( CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData )
    {
        Assert( Destination != nullptr );

        const uint32 SizeInBytes = Width * Height * GetByteStrideFromFormat( Destination->GetFormat() );

        void* TempSourceData = CmdAllocator.Allocate( SizeInBytes, 1 );
        CMemory::Memcpy( TempSourceData, SourceData, SizeInBytes );

        Destination->AddRef();
        InsertCommand<SRHIUpdateTexture2DRenderCommand>( Destination, Width, Height, MipLevel, TempSourceData );
    }

    void CopyBuffer( CRHIBuffer* Destination, CRHIBuffer* Source, const SCopyBufferInfo& CopyInfo )
    {
        SafeAddRef( Destination );
        SafeAddRef( Source );
        InsertCommand<SRHICopyBufferRenderCommand>( Destination, Source, CopyInfo );
    }

    void CopyTexture( CRHITexture* Destination, CRHITexture* Source )
    {
        SafeAddRef( Destination );
        SafeAddRef( Source );
        InsertCommand<SRHICopyTextureRenderCommand>( Destination, Source );
    }

    void CopyTextureRegion( CRHITexture* Destination, CRHITexture* Source, const SCopyTextureInfo& CopyTextureInfo )
    {
        SafeAddRef( Destination );
        SafeAddRef( Source );
        InsertCommand<SRHICopyTextureRegionRenderCommand>( Destination, Source, CopyTextureInfo );
    }

    void DiscardResource( CRHIResource* Resource )
    {
        SafeAddRef( Resource );
        InsertCommand<SRHIDiscardResourceRenderCommand>( Resource );
    }

    void BuildRayTracingGeometry( CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool Update )
    {
        Assert( Geometry != nullptr );
        Assert( !Update || (Update && Geometry->GetFlags() & RayTracingStructureBuildFlag_AllowUpdate) );

        SafeAddRef( Geometry );
        SafeAddRef( VertexBuffer );
        SafeAddRef( IndexBuffer );
        InsertCommand<SRHIBuildRayTracingGeometryRenderCommand>( Geometry, VertexBuffer, IndexBuffer, Update );
    }

    void BuildRayTracingScene( CRHIRayTracingScene* Scene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool Update )
    {
        Assert( Scene != nullptr );
        Assert( !Update || (Update && Scene->GetFlags() & RayTracingStructureBuildFlag_AllowUpdate) );

        SafeAddRef( Scene );
        InsertCommand<SRHIBuildRayTracingSceneRenderCommand>( Scene, Instances, NumInstances, Update );
    }

    void GenerateMips( CRHITexture* Texture )
    {
        Assert( Texture != nullptr );

        Texture->AddRef();
        InsertCommand<SRHIGenerateMipsRenderCommand>( Texture );
    }

    void TransitionTexture( CRHITexture* Texture, EResourceState BeforeState, EResourceState AfterState )
    {
        Assert( Texture != nullptr );

        if ( BeforeState != AfterState )
        {
            Texture->AddRef();
            InsertCommand<SRHITransitionTextureRenderCommand>( Texture, BeforeState, AfterState );
        }
        else
        {
            LOG_WARNING( "Texture '" + Texture->GetName() + "' Was transitioned with the same Before- and AfterState (=" + ToString( BeforeState ) + ")" );
        }
    }

    void TransitionBuffer( CRHIBuffer* Buffer, EResourceState BeforeState, EResourceState AfterState )
    {
        Assert( Buffer != nullptr );

        if ( BeforeState != AfterState )
        {
            Buffer->AddRef();
            InsertCommand<SRHITransitionBufferRenderCommand>( Buffer, BeforeState, AfterState );
        }
    }

    void UnorderedAccessTextureBarrier( CRHITexture* Texture )
    {
        Assert( Texture != nullptr );

        Texture->AddRef();
        InsertCommand<SRHIUnorderedAccessTextureBarrierRenderCommand>( Texture );
    }

    void UnorderedAccessBufferBarrier( CRHIBuffer* Buffer )
    {
        Assert( Buffer != nullptr );

        Buffer->AddRef();
        InsertCommand<SRHIUnorderedAccessBufferBarrierRenderCommand>( Buffer );
    }

    void Draw( uint32 VertexCount, uint32 StartVertexLocation )
    {
        InsertCommand<SRHIDrawRenderCommand>( VertexCount, StartVertexLocation );
        NumDrawCalls++;
    }

    void DrawIndexed( uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation )
    {
        InsertCommand<SRHIDrawIndexedRenderCommand>( IndexCount, StartIndexLocation, BaseVertexLocation );
        NumDrawCalls++;
    }

    void DrawInstanced( uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation )
    {
        InsertCommand<SRHIDrawInstancedRenderCommand>( VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation );
        NumDrawCalls++;
    }

    void DrawIndexedInstanced(
        uint32 IndexCountPerInstance,
        uint32 InstanceCount,
        uint32 StartIndexLocation,
        uint32 BaseVertexLocation,
        uint32 StartInstanceLocation )
    {
        InsertCommand<SRHIDrawIndexedInstancedRenderCommand>( IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation );
        NumDrawCalls++;
    }

    void Dispatch( uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ )
    {
        InsertCommand<SRHIDispatchComputeRenderCommand>( ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ );
        NumDispatchCalls++;
    }

    void DispatchRays( CRHIRayTracingScene* Scene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth )
    {
        SafeAddRef( Scene );
        SafeAddRef( PipelineState );
        InsertCommand<SRHIDispatchRaysRenderCommand>( Scene, PipelineState, Width, Height, Depth );
    }

    void InsertCommandListMarker( const CString& Marker )
    {
        InsertCommand<SRHIInsertCommandListMarkerRenderCommand>( Marker );
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
        if ( First != nullptr )
        {
            SRHIRenderCommand* Cmd = First;
            while ( Cmd != nullptr )
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

    CLinearAllocator CmdAllocator;
    SRHIRenderCommand* First;
    SRHIRenderCommand* Last;

    uint32 NumDrawCalls = 0;
    uint32 NumDispatchCalls = 0;
    uint32 NumCommands = 0;
};

class CRHICommandQueue
{
public:
    void ExecuteCommandList( CRHICommandList& CmdList );
    void ExecuteCommandLists( CRHICommandList* const* CmdLists, uint32 NumCmdLists );

    void WaitForGPU();

    FORCEINLINE void SetContext( IRHICommandContext* InCmdContext )
    {
        Assert( InCmdContext != nullptr );
        CmdContext = InCmdContext;
    }

    FORCEINLINE IRHICommandContext& GetContext()
    {
        Assert( CmdContext != nullptr );
        return *CmdContext;
    }

private:
    void InternalExecuteCommandList( CRHICommandList& CmdList );

    IRHICommandContext* CmdContext = nullptr;
};

extern CRHICommandQueue GCmdListExecutor;