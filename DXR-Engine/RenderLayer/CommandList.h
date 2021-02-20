#pragma once
#include "Resources.h"
#include "RayTracing.h"
#include "RenderCommand.h"

#include "Memory/LinearAllocator.h"

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
        , First(nullptr)
        , Last(nullptr)
    {
    }

     ~CommandList()
    {
        Reset();
    }

    void Begin()
    {
        Assert(IsRecording == false);

        InsertCommand<BeginRenderCommand>();
        IsRecording = true;
    }

    void End()
    {
        Assert(IsRecording == true);

        InsertCommand<EndRenderCommand>();
        IsRecording = false;
    }

    void ClearRenderTargetView(RenderTargetView* RenderTargetView, const ColorF& ClearColor)
    {
        Assert(RenderTargetView != nullptr);

        RenderTargetView->AddRef();
        InsertCommand<ClearRenderTargetViewRenderCommand>(RenderTargetView, ClearColor);
    }

    void ClearDepthStencilView(DepthStencilView* DepthStencilView, const DepthStencilF& ClearValue)
    {
        Assert(DepthStencilView != nullptr);

        DepthStencilView->AddRef();
        InsertCommand<ClearDepthStencilViewRenderCommand>(DepthStencilView, ClearValue);
    }

    void ClearUnorderedAccessView(UnorderedAccessView* UnorderedAccessView, const Float ClearColor[4])
    {
        Assert(UnorderedAccessView != nullptr);

        UnorderedAccessView->AddRef();
        InsertCommand<ClearUnorderedAccessViewFloatRenderCommand>(UnorderedAccessView, ClearColor);
    }

    void SetShadingRate(EShadingRate ShadingRate)
    {
        InsertCommand<SetShadingRateRenderCommand>(ShadingRate);
    }

    void SetShadingRateImage(Texture2D* ShadingRateImage)
    {
        SafeAddRef(ShadingRateImage);
        InsertCommand<SetShadingRateImageRenderCommand>(ShadingRateImage);
    }

    void BeginRenderPass()
    {
        InsertCommand<BeginRenderPassRenderCommand>();
    }

    void EndRenderPass()
    {
        InsertCommand<EndRenderPassRenderCommand>();
    }

    void BindViewport(Float Width, Float Height, Float MinDepth, Float MaxDepth, Float x, Float y)
    {
        InsertCommand<BindViewportRenderCommand>(Width, Height, MinDepth, MaxDepth, x, y);
    }

    void BindScissorRect(Float Width, Float Height, Float x, Float y)
    {
        InsertCommand<BindScissorRectRenderCommand>(Width, Height, x, y);
    }

    void BindBlendFactor(const ColorF& Color)
    {
        InsertCommand<BindBlendFactorRenderCommand>(Color);
    }

    void BindRenderTargets(RenderTargetView* const* RenderTargetViews, UInt32 RenderTargetCount, DepthStencilView* DepthStencilView)
    {
        RenderTargetView** RenderTargets = new(CmdAllocator) RenderTargetView*[RenderTargetCount];
        for (UInt32 i = 0; i < RenderTargetCount; i++)
        {
            RenderTargets[i] = RenderTargetViews[i];
            SafeAddRef(RenderTargets[i]);
        }

        SafeAddRef(DepthStencilView);
        InsertCommand<BindRenderTargetsRenderCommand>(RenderTargets, RenderTargetCount, DepthStencilView);
    }

    void BindPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
    {
        InsertCommand<BindPrimitiveTopologyRenderCommand>(PrimitveTopologyType);
    }

    void BindVertexBuffers(VertexBuffer* const* VertexBuffers, UInt32 VertexBufferCount, UInt32 BufferSlot)
    {
        VertexBuffer** Buffers = new(CmdAllocator) VertexBuffer*[VertexBufferCount];
        for (UInt32 i = 0; i < VertexBufferCount; i++)
        {
            Buffers[i] = VertexBuffers[i];
            SafeAddRef(Buffers[i]);
        }

        InsertCommand<BindVertexBuffersRenderCommand>(Buffers, VertexBufferCount, BufferSlot);
    }

    void BindIndexBuffer(IndexBuffer* IndexBuffer)
    {
        SafeAddRef(IndexBuffer);
        InsertCommand<BindIndexBufferRenderCommand>(IndexBuffer);
    }

    void SetHitGroups(RayTracingScene* RayTracingScene, RayTracingPipelineState* PipelineState, const TArrayView<RayTracingShaderResources>& LocalShaderResources)
    {
        SafeAddRef(RayTracingScene);
        SafeAddRef(PipelineState);
        InsertCommand<SetHitGroupsRenderCommand>(RayTracingScene, PipelineState, LocalShaderResources);
    }

    void BindGraphicsPipelineState(GraphicsPipelineState* PipelineState)
    {
        SafeAddRef(PipelineState);
        InsertCommand<BindGraphicsPipelineStateRenderCommand>(PipelineState);
    }

    void BindComputePipelineState(ComputePipelineState* PipelineState)
    {
        SafeAddRef(PipelineState);
        InsertCommand<BindComputePipelineStateRenderCommand>(PipelineState);
    }

    void Bind32BitShaderConstants(EShaderStage ShaderStage, const Void* Shader32BitConstants, UInt32 Num32BitConstants)
    {
        const UInt32 Num32BitConstantsInBytes = Num32BitConstants * 4;
        Void* Shader32BitConstantsMemory = CmdAllocator.Allocate(Num32BitConstantsInBytes, 1);
        Memory::Memcpy(Shader32BitConstantsMemory, Shader32BitConstants, Num32BitConstantsInBytes);

        InsertCommand<Bind32BitShaderConstantsRenderCommand>(ShaderStage, Shader32BitConstantsMemory, Num32BitConstants);
    }

    void BindShaderResourceViews(
        EShaderStage ShaderStage, 
        ShaderResourceView* const* ShaderResourceViews, 
        UInt32 ShaderResourceViewCount, 
        UInt32 StartSlot)
    {
        Assert(ShaderResourceViews != nullptr);

        ShaderResourceView** Views = new(CmdAllocator) ShaderResourceView*[ShaderResourceViewCount];
        for (UInt32 i = 0; i < ShaderResourceViewCount; i++)
        {
            Views[i] = ShaderResourceViews[i];
            SafeAddRef(Views[i]);
        }

        InsertCommand<BindShaderResourceViewsRenderCommand>(ShaderStage, Views, ShaderResourceViewCount, StartSlot);
    }

    void BindSamplerStates(EShaderStage ShaderStage, SamplerState* const* SamplerStates, UInt32 SamplerStateCount, UInt32 StartSlot)
    {
        Assert(SamplerStates != nullptr);

        SamplerState** Samplers = new(CmdAllocator) SamplerState*[SamplerStateCount];
        for (UInt32 i = 0; i < SamplerStateCount; i++)
        {
            Samplers[i] = SamplerStates[i];
            SafeAddRef(Samplers[i]);
        }

        InsertCommand<BindSamplerStatesRenderCommand>(ShaderStage, Samplers, SamplerStateCount, StartSlot);
    }

    void BindUnorderedAccessViews(
        EShaderStage ShaderStage, 
        UnorderedAccessView* const* UnorderedAccessViews, 
        UInt32 UnorderedAccessViewCount, 
        UInt32 StartSlot)
    {
        Assert(UnorderedAccessViews != nullptr);

        UnorderedAccessView** Views = new(CmdAllocator) UnorderedAccessView*[UnorderedAccessViewCount];
        for (UInt32 i = 0; i < UnorderedAccessViewCount; i++)
        {
            Views[i] = UnorderedAccessViews[i];
            SafeAddRef(Views[i]);
        }

        InsertCommand<BindUnorderedAccessViewsRenderCommand>(ShaderStage, Views, UnorderedAccessViewCount, StartSlot);
    }

    void BindConstantBuffers(EShaderStage ShaderStage, ConstantBuffer* const* ConstantBuffers, UInt32 ConstantBufferCount, UInt32 StartSlot)
    {
        Assert(ConstantBuffers != nullptr);

        ConstantBuffer** Buffers = new(CmdAllocator) ConstantBuffer*[ConstantBufferCount];
        for (UInt32 i = 0; i < ConstantBufferCount; i++)
        {
            Buffers[i] = ConstantBuffers[i];
            SafeAddRef(Buffers[i]);
        }

        InsertCommand<BindConstantBuffersRenderCommand>(ShaderStage, Buffers, ConstantBufferCount, StartSlot);
    }

    void SetShaderResourceView(Shader* Shader, ShaderResourceView* ShaderResourceView, UInt32 ParameterIndex);
    void SetShaderResourceViews(Shader* Shader, ShaderResourceView* const* ShaderResourceView, UInt32 NumShaderResourceViews, UInt32 ParameterIndex);

    void SetUnorderedAccessView(Shader* Shader, UnorderedAccessView* UnorderedAccessView, UInt32 ParameterIndex);
    void SetUnorderedAccessViews(Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, UInt32 NumUnorderedAccessViews, UInt32 ParameterIndex);

    void SetConstantBuffer(Shader* Shader, ConstantBuffer* ConstantBuffer, UInt32 ParameterIndex);
    void SetConstantBuffers(Shader* Shader, ConstantBuffer* const* ConstantBuffers, UInt32 NumConstantBuffers, UInt32 ParameterIndex);

    void SetSamplerState(Shader* Shader, SamplerState* SamplerState, UInt32 ParameterIndex);
    void SetSamplerStates(Shader* Shader, SamplerState* const* SamplerStates, UInt32 NumSamplerStates, UInt32 ParameterIndex);

    void ResolveTexture(Texture* Destination, Texture* Source)
    {
        SafeAddRef(Destination);
        SafeAddRef(Source);
        InsertCommand<ResolveTextureRenderCommand>(Destination, Source);
    }

    void UpdateBuffer(Buffer* Destination, UInt64 DestinationOffsetInBytes, UInt64 SizeInBytes, const Void* SourceData)
    {
        Void* TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
        Memory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        SafeAddRef(Destination);
        InsertCommand<UpdateBufferRenderCommand>(Destination, DestinationOffsetInBytes, SizeInBytes, TempSourceData);
    }

    void UpdateTexture2D(Texture2D* Destination, UInt32 Width, UInt32 Height, UInt32 MipLevel, const Void* SourceData)
    {
        Assert(Destination != nullptr);

        const UInt32 SizeInBytes = Width * Height * GetByteStrideFromFormat(Destination->GetFormat());

        Void* TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
        Memory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        Destination->AddRef();
        InsertCommand<UpdateTexture2DRenderCommand>(Destination, Width, Height, MipLevel, TempSourceData);
    }

    void CopyBuffer(Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo)
    {
        SafeAddRef(Destination);
        SafeAddRef(Source);
        InsertCommand<CopyBufferRenderCommand>(Destination, Source, CopyInfo);
    }

    void CopyTexture(Texture* Destination, Texture* Source)
    {
        SafeAddRef(Destination);
        SafeAddRef(Source);
        InsertCommand<CopyTextureRenderCommand>(Destination, Source);
    }

    void CopyTextureRegion(Texture* Destination, Texture* Source, const CopyTextureInfo& CopyTextureInfo)
    {
        SafeAddRef(Destination);
        SafeAddRef(Source);
        InsertCommand<CopyTextureRegionRenderCommand>(Destination, Source, CopyTextureInfo);
    }

    void DiscardResource(Resource* Resource)
    {
        SafeAddRef(Resource);
        InsertCommand<DiscardResourceRenderCommand>(Resource);
    }

    void BuildRayTracingGeometry(RayTracingGeometry* Geometry, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer, Bool Update)
    {
        Assert(Geometry != nullptr);
        Assert(!Update || (Update && Geometry->GetFlags() & RayTracingStructureBuildFlag_AllowUpdate));

        SafeAddRef(Geometry);
        SafeAddRef(VertexBuffer);
        SafeAddRef(IndexBuffer);
        InsertCommand<BuildRayTracingGeometryRenderCommand>(Geometry, VertexBuffer, IndexBuffer, Update);
    }

    void BuildRayTracingScene(RayTracingScene* Scene, const TArrayView<RayTracingGeometryInstance>& Instances, Bool Update)
    {
        Assert(Scene != nullptr);
        Assert(!Update || (Update && Scene->GetFlags() & RayTracingStructureBuildFlag_AllowUpdate));

        SafeAddRef(Scene);
        InsertCommand<BuildRayTracingSceneRenderCommand>(Scene, Instances, Update);
    }

    void GenerateMips(Texture* Texture)
    {
        Assert(Texture != nullptr);

        Texture->AddRef();
        InsertCommand<GenerateMipsRenderCommand>(Texture);
    }

    void TransitionTexture(Texture* Texture, EResourceState BeforeState, EResourceState AfterState)
    {
        Assert(Texture != nullptr);

        if (BeforeState != AfterState)
        {
            Texture->AddRef();
            InsertCommand<TransitionTextureRenderCommand>(Texture, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '" + Texture->GetName() + "' Was transitioned with the same Before- and AfterState (=" + ToString(BeforeState) + ")");
        }
    }

    void TransitionBuffer(Buffer* Buffer, EResourceState BeforeState, EResourceState AfterState)
    {
        Assert(Buffer != nullptr);

        if (BeforeState != AfterState)
        {
            Buffer->AddRef();
            InsertCommand<TransitionBufferRenderCommand>(Buffer, BeforeState, AfterState);
        }
    }

    void UnorderedAccessTextureBarrier(Texture* Texture)
    {
        Assert(Texture != nullptr);

        Texture->AddRef();
        InsertCommand<UnorderedAccessTextureBarrierRenderCommand>(Texture);
    }

    void UnorderedAccessBufferBarrier(Buffer* Buffer)
    {
        Assert(Buffer != nullptr);

        Buffer->AddRef();
        InsertCommand<UnorderedAccessBufferBarrierRenderCommand>(Buffer);
    }

    void Draw(UInt32 VertexCount, UInt32 StartVertexLocation)
    {
        InsertCommand<DrawRenderCommand>(VertexCount, StartVertexLocation);
        NumDrawCalls++;
    }

    void DrawIndexed(UInt32 IndexCount, UInt32 StartIndexLocation, UInt32 BaseVertexLocation)
    {
        InsertCommand<DrawIndexedRenderCommand>(IndexCount, StartIndexLocation, BaseVertexLocation);
        NumDrawCalls++;
    }

    void DrawInstanced(UInt32 VertexCountPerInstance, UInt32 InstanceCount, UInt32 StartVertexLocation, UInt32 StartInstanceLocation)
    {
        InsertCommand<DrawInstancedRenderCommand>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    void DrawIndexedInstanced(
        UInt32 IndexCountPerInstance, 
        UInt32 InstanceCount, 
        UInt32 StartIndexLocation, 
        UInt32 BaseVertexLocation, 
        UInt32 StartInstanceLocation)
    {
        InsertCommand<DrawIndexedInstancedRenderCommand>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    void Dispatch(UInt32 ThreadGroupCountX, UInt32 ThreadGroupCountY, UInt32 ThreadGroupCountZ)
    {
        InsertCommand<DispatchComputeRenderCommand>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        NumDispatchCalls++;
    }

    void DispatchRays(
        RayTracingScene* Scene, 
        Texture2D* OutputImage, 
        RayTracingPipelineState* PipelineState, 
        const RayTracingShaderResources& GlobalShaderResources,
        UInt32 Width, 
        UInt32 Height, 
        UInt32 Depth)
    {
        Assert(OutputImage->IsUAV());

        SafeAddRef(Scene);
        SafeAddRef(OutputImage);
        SafeAddRef(PipelineState);

        InsertCommand<DispatchRaysRenderCommand>(Scene, OutputImage, PipelineState, GlobalShaderResources, Width, Height, Depth);
    }

    void InsertCommandListMarker(const std::string& Marker)
    {
        InsertCommand<InsertCommandListMarkerRenderCommand>(Marker);
    }

    void Reset()
    {
        if (First != nullptr)
        {
            RenderCommand* Cmd = First;
            while (Cmd != nullptr)
            {
                RenderCommand* Old = Cmd;
                Cmd = Cmd->NextCmd;
                Old->~RenderCommand();
            }

            First = nullptr;
            Last  = nullptr;
        }

        NumDrawCalls     = 0;
        NumDispatchCalls = 0;
        NumCommands      = 0;

        CmdAllocator.Reset();
    }

    UInt32 GetNumDrawCalls()     const { return NumDrawCalls; }
    UInt32 GetNumDispatchCalls() const { return NumDispatchCalls; }
    UInt32 GetNumCommands()      const { return NumCommands; }

private:
    template<typename TCommand, typename... TArgs>
    void InsertCommand(TArgs&&... Args)
    {
        TCommand* Cmd = new(CmdAllocator) TCommand(Forward<TArgs>(Args)...);
        if (Last)
        {
            Last->NextCmd = Cmd;
            Last = Last->NextCmd;
        }
        else
        {
            First = Cmd;
            Last  = First;
        }

        NumCommands++;
    }

    LinearAllocator CmdAllocator;
    RenderCommand*  First;
    RenderCommand*  Last;

    UInt32 NumDrawCalls     = 0;
    UInt32 NumDispatchCalls = 0;
    UInt32 NumCommands      = 0;

    Bool IsRecording = false;
};

inline void CommandList::SetShaderResourceView(Shader* Shader, ShaderResourceView* ShaderResourceView, UInt32 ParameterIndex)
{
    SafeAddRef(Shader);
    SafeAddRef(ShaderResourceView);
    InsertCommand<SetShaderResourceViewRenderCommand>(Shader, ShaderResourceView, ParameterIndex);
}

inline void CommandList::SetShaderResourceViews(Shader* Shader, ShaderResourceView* const* ShaderResourceViews, UInt32 NumShaderResourceViews, UInt32 ParameterIndex)
{
    SafeAddRef(Shader);

    ShaderResourceView** TempShaderResourceViews = new(CmdAllocator) ShaderResourceView*[NumShaderResourceViews];
    for (UInt32 i = 0; i < NumShaderResourceViews; i++)
    {
        TempShaderResourceViews[i] = ShaderResourceViews[i];
        SafeAddRef(TempShaderResourceViews[i]);
    }

    InsertCommand<SetShaderResourceViewsRenderCommand>(Shader, TempShaderResourceViews, NumShaderResourceViews, ParameterIndex);
}

inline void CommandList::SetUnorderedAccessView(Shader* Shader, UnorderedAccessView* UnorderedAccessView, UInt32 ParameterIndex)
{
    SafeAddRef(Shader);
    SafeAddRef(UnorderedAccessView);
    InsertCommand<SetUnorderedAccessViewRenderCommand>(Shader, UnorderedAccessView, ParameterIndex);
}

inline void CommandList::SetUnorderedAccessViews(Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, UInt32 NumUnorderedAccessViews, UInt32 ParameterIndex)
{
    UnorderedAccessView** TempUnorderedAccessViews = new(CmdAllocator) UnorderedAccessView*[NumUnorderedAccessViews];
    for (UInt32 i = 0; i < NumUnorderedAccessViews; i++)
    {
        TempUnorderedAccessViews[i] = UnorderedAccessViews[i];
        SafeAddRef(TempUnorderedAccessViews[i]);
    }

    SafeAddRef(Shader);
    InsertCommand<SetUnorderedAccessViewsRenderCommand>(Shader, TempUnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex);
}

inline void CommandList::SetConstantBuffer(Shader* Shader, ConstantBuffer* ConstantBuffer, UInt32 ParameterIndex)
{
    SafeAddRef(Shader);
    SafeAddRef(ConstantBuffer);
    InsertCommand<SetConstantBufferRenderCommand>(Shader, ConstantBuffer, ParameterIndex);
}

inline void CommandList::SetConstantBuffers(Shader* Shader, ConstantBuffer* const* ConstantBuffers, UInt32 NumConstantBuffers, UInt32 ParameterIndex)
{
    ConstantBuffer** TempConstantBuffers = new(CmdAllocator) ConstantBuffer*[NumConstantBuffers];
    for (UInt32 i = 0; i < NumConstantBuffers; i++)
    {
        TempConstantBuffers[i] = ConstantBuffers[i];
        SafeAddRef(TempConstantBuffers[i]);
    }

    SafeAddRef(Shader);
    InsertCommand<SetConstantBuffersRenderCommand>(Shader, TempConstantBuffers, NumConstantBuffers, ParameterIndex);
}

inline void CommandList::SetSamplerState(Shader* Shader, SamplerState* SamplerState, UInt32 ParameterIndex)
{
    SafeAddRef(Shader);
    SafeAddRef(SamplerState);
    InsertCommand<SetSamplerStateRenderCommand>(Shader, SamplerState, ParameterIndex);
}

inline void CommandList::SetSamplerStates(Shader* Shader, SamplerState* const* SamplerStates, UInt32 NumSamplerStates, UInt32 ParameterIndex)
{
    SamplerState** TempSamplerStates = new(CmdAllocator) SamplerState*[NumSamplerStates];
    for (UInt32 i = 0; i < NumSamplerStates; i++)
    {
        TempSamplerStates[i] = SamplerStates[i];
        SafeAddRef(TempSamplerStates[i]);
    }

    SafeAddRef(Shader);
    InsertCommand<SetSamplerStatesRenderCommand>(Shader, TempSamplerStates, NumSamplerStates, ParameterIndex);
}

class CommandListExecutor
{
public:
    CommandListExecutor()  = default;
    ~CommandListExecutor() = default;

    void ExecuteCommandList(CommandList& CmdList);
    void WaitForGPU();

    void SetContext(ICommandContext* InCmdContext)
    {
        Assert(InCmdContext != nullptr);
        CmdContext = InCmdContext;
    }

    ICommandContext& GetContext()
    {
        Assert(CmdContext != nullptr);
        return *CmdContext;
    }

private:
    ICommandContext* CmdContext = nullptr;
};