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

#define ENABLE_INSERT_DEBUG_CMDLIST_MARKER 1

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
        VALIDATE(IsRecording == false);

        InsertCommand<BeginCommand>();
        IsRecording = true;
    }

    void End()
    {
        VALIDATE(IsRecording == true);

        InsertCommand<EndCommand>();
        IsRecording = false;
    }

    void ClearRenderTargetView(RenderTargetView* RenderTargetView, const ColorF& ClearColor)
    {
        VALIDATE(RenderTargetView != nullptr);

        RenderTargetView->AddRef();
        InsertCommand<ClearRenderTargetViewCommand>(RenderTargetView, ClearColor);
    }

    void ClearDepthStencilView(DepthStencilView* DepthStencilView, const DepthStencilF& ClearValue)
    {
        VALIDATE(DepthStencilView != nullptr);

        DepthStencilView->AddRef();
        InsertCommand<ClearDepthStencilViewCommand>(DepthStencilView, ClearValue);
    }

    void ClearUnorderedAccessView(UnorderedAccessView* UnorderedAccessView, const Float ClearColor[4])
    {
        VALIDATE(UnorderedAccessView != nullptr);

        UnorderedAccessView->AddRef();
        InsertCommand<ClearUnorderedAccessViewFloatCommand>(UnorderedAccessView, ClearColor);
    }

    void SetShadingRate(EShadingRate ShadingRate)
    {
        InsertCommand<SetShadingRateCommand>(ShadingRate);
    }

    void SetShadingRateImage(Texture2D* ShadingRateImage)
    {
        ShadingRateImage->AddRef();
        InsertCommand<SetShadingRateImageCommand>(ShadingRateImage);
    }

    void BeginRenderPass()
    {
        InsertCommand<BeginRenderPassCommand>();
    }

    void EndRenderPass()
    {
        InsertCommand<EndRenderPassCommand>();
    }

    void BindViewport(Float Width, Float Height, Float MinDepth, Float MaxDepth, Float x, Float y)
    {
        InsertCommand<BindViewportCommand>(Width, Height, MinDepth, MaxDepth, x, y);
    }

    void BindScissorRect(Float Width, Float Height, Float x, Float y)
    {
        InsertCommand<BindScissorRectCommand>(Width, Height, x, y);
    }

    void BindBlendFactor(const ColorF& Color)
    {
        InsertCommand<BindBlendFactorCommand>(Color);
    }

    void BindRenderTargets(RenderTargetView* const* RenderTargetViews, UInt32 RenderTargetCount, DepthStencilView* DepthStencilView)
    {
        RenderTargetView** RenderTargets = new(CmdAllocator) RenderTargetView*[RenderTargetCount];
        for (UInt32 i = 0; i < RenderTargetCount; i++)
        {
            RenderTargets[i] = RenderTargetViews[i];
            SAFEADDREF(RenderTargets[i]);
        }

        SAFEADDREF(DepthStencilView);
        InsertCommand<BindRenderTargetsCommand>(RenderTargets, RenderTargetCount, DepthStencilView);
    }

    void BindPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
    {
        InsertCommand<BindPrimitiveTopologyCommand>(PrimitveTopologyType);
    }

    void BindVertexBuffers(VertexBuffer* const* VertexBuffers, UInt32 VertexBufferCount, UInt32 BufferSlot)
    {
        VertexBuffer** Buffers = new(CmdAllocator) VertexBuffer*[VertexBufferCount];
        for (UInt32 i = 0; i < VertexBufferCount; i++)
        {
            Buffers[i] = VertexBuffers[i];
            SAFEADDREF(Buffers[i]);
        }

        InsertCommand<BindVertexBuffersCommand>(Buffers, VertexBufferCount, BufferSlot);
    }

    void BindIndexBuffer(IndexBuffer* IndexBuffer)
    {
        SAFEADDREF(IndexBuffer);
        InsertCommand<BindIndexBufferCommand>(IndexBuffer);
    }

    void BindRayTracingScene(RayTracingScene* RayTracingScene)
    {
        SAFEADDREF(RayTracingScene);
        InsertCommand<BindRayTracingSceneCommand>(RayTracingScene);
    }

    void BindGraphicsPipelineState(GraphicsPipelineState* PipelineState)
    {
        SAFEADDREF(PipelineState);
        InsertCommand<BindGraphicsPipelineStateCommand>(PipelineState);
    }

    void BindComputePipelineState(ComputePipelineState* PipelineState)
    {
        SAFEADDREF(PipelineState);
        InsertCommand<BindComputePipelineStateCommand>(PipelineState);
    }

    void BindRayTracingPipelineState(RayTracingPipelineState* PipelineState)
    {
        SAFEADDREF(PipelineState);
        InsertCommand<BindRayTracingPipelineStateCommand>(PipelineState);
    }

    void Bind32BitShaderConstants(EShaderStage ShaderStage, const Void* Shader32BitConstants, UInt32 Num32BitConstants)
    {
        const UInt32 Num32BitConstantsInBytes = Num32BitConstants * 4;
        Void* Shader32BitConstantsMemory = CmdAllocator.Allocate(Num32BitConstantsInBytes, 1);
        Memory::Memcpy(Shader32BitConstantsMemory, Shader32BitConstants, Num32BitConstantsInBytes);

        InsertCommand<Bind32BitShaderConstantsCommand>(ShaderStage, Shader32BitConstantsMemory, Num32BitConstants);
    }

    void BindShaderResourceViews(
        EShaderStage ShaderStage, 
        ShaderResourceView* const* ShaderResourceViews, 
        UInt32 ShaderResourceViewCount, 
        UInt32 StartSlot)
    {
        VALIDATE(ShaderResourceViews != nullptr);

        ShaderResourceView** Views = new(CmdAllocator) ShaderResourceView*[ShaderResourceViewCount];
        for (UInt32 i = 0; i < ShaderResourceViewCount; i++)
        {
            Views[i] = ShaderResourceViews[i];
            SAFEADDREF(Views[i]);
        }

        InsertCommand<BindShaderResourceViewsCommand>(ShaderStage, Views, ShaderResourceViewCount, StartSlot);
    }

    void BindSamplerStates(EShaderStage ShaderStage, SamplerState* const* SamplerStates, UInt32 SamplerStateCount, UInt32 StartSlot)
    {
        VALIDATE(SamplerStates != nullptr);

        SamplerState** Samplers = new(CmdAllocator) SamplerState*[SamplerStateCount];
        for (UInt32 i = 0; i < SamplerStateCount; i++)
        {
            Samplers[i] = SamplerStates[i];
            SAFEADDREF(Samplers[i]);
        }

        InsertCommand<BindSamplerStatesCommand>(ShaderStage, Samplers, SamplerStateCount, StartSlot);
    }

    void BindUnorderedAccessViews(
        EShaderStage ShaderStage, 
        UnorderedAccessView* const* UnorderedAccessViews, 
        UInt32 UnorderedAccessViewCount, 
        UInt32 StartSlot)
    {
        VALIDATE(UnorderedAccessViews != nullptr);

        UnorderedAccessView** Views = new(CmdAllocator) UnorderedAccessView*[UnorderedAccessViewCount];
        for (UInt32 i = 0; i < UnorderedAccessViewCount; i++)
        {
            Views[i] = UnorderedAccessViews[i];
            SAFEADDREF(Views[i]);
        }

        InsertCommand<BindUnorderedAccessViewsCommand>(ShaderStage, Views, UnorderedAccessViewCount, StartSlot);
    }

    void BindConstantBuffers(EShaderStage ShaderStage, ConstantBuffer* const* ConstantBuffers, UInt32 ConstantBufferCount, UInt32 StartSlot)
    {
        VALIDATE(ConstantBuffers != nullptr);

        ConstantBuffer** Buffers = new(CmdAllocator) ConstantBuffer*[ConstantBufferCount];
        for (UInt32 i = 0; i < ConstantBufferCount; i++)
        {
            Buffers[i] = ConstantBuffers[i];
            SAFEADDREF(Buffers[i]);
        }

        InsertCommand<BindConstantBuffersCommand>(ShaderStage, Buffers, ConstantBufferCount, StartSlot);
    }

    void ResolveTexture(Texture* Destination, Texture* Source)
    {
        SAFEADDREF(Destination);
        SAFEADDREF(Source);
        InsertCommand<ResolveTextureCommand>(Destination, Source);
    }

    void UpdateBuffer(Buffer* Destination, UInt64 DestinationOffsetInBytes, UInt64 SizeInBytes, const Void* SourceData)
    {
        Void* TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
        Memory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        SAFEADDREF(Destination);
        InsertCommand<UpdateBufferCommand>(Destination, DestinationOffsetInBytes, SizeInBytes, TempSourceData);
    }

    void UpdateTexture2D(Texture2D* Destination, UInt32 Width, UInt32 Height, UInt32 MipLevel, const Void* SourceData)
    {
        VALIDATE(Destination != nullptr);

        const UInt32 SizeInBytes = Width * Height * GetByteStrideFromFormat(Destination->GetFormat());
        Void* TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
        Memory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        Destination->AddRef();
        InsertCommand<UpdateTexture2DCommand>(Destination, Width, Height, MipLevel, TempSourceData);
    }

    void CopyBuffer(Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo)
    {
        SAFEADDREF(Destination);
        SAFEADDREF(Source);
        InsertCommand<CopyBufferCommand>(Destination, Source, CopyInfo);
    }

    void CopyTexture(Texture* Destination, Texture* Source)
    {
        SAFEADDREF(Destination);
        SAFEADDREF(Source);
        InsertCommand<CopyTextureCommand>(Destination, Source);
    }

    void CopyTextureRegion(Texture* Destination, Texture* Source, const CopyTextureInfo& CopyTextureInfo)
    {
        SAFEADDREF(Destination);
        SAFEADDREF(Source);
        InsertCommand<CopyTextureRegionCommand>(Destination, Source, CopyTextureInfo);
    }

    void DestroyResource(Resource* Resource)
    {
        SAFEADDREF(Resource);
        InsertCommand<DestroyResourceCommand>(Resource);
    }

    void BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry)
    {
        SAFEADDREF(RayTracingGeometry);
        InsertCommand<BuildRayTracingGeometryCommand>(RayTracingGeometry);
    }

    void BuildRayTracingScene(RayTracingScene* RayTracingScene)
    {
        SAFEADDREF(RayTracingScene);
        InsertCommand<BuildRayTracingSceneCommand>(RayTracingScene);
    }

    void GenerateMips(Texture* Texture)
    {
        VALIDATE(Texture != nullptr);

        Texture->AddRef();
        InsertCommand<GenerateMipsCommand>(Texture);
    }

    void TransitionTexture(Texture* Texture, EResourceState BeforeState, EResourceState AfterState)
    {
        VALIDATE(Texture != nullptr);

        if (BeforeState != AfterState)
        {
            Texture->AddRef();
            InsertCommand<TransitionTextureCommand>(Texture, BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '" + Texture->GetName() + "' Was transitioned with the same Before- and AfterState (=" + ToString(BeforeState) + ")");
        }
    }

    void TransitionBuffer(Buffer* Buffer, EResourceState BeforeState, EResourceState AfterState)
    {
        VALIDATE(Buffer != nullptr);

        if (BeforeState != AfterState)
        {
            Buffer->AddRef();
            InsertCommand<TransitionBufferCommand>(Buffer, BeforeState, AfterState);
        }
    }

    void UnorderedAccessTextureBarrier(Texture* Texture)
    {
        VALIDATE(Texture != nullptr);

        Texture->AddRef();
        InsertCommand<UnorderedAccessTextureBarrierCommand>(Texture);
    }

    void Draw(UInt32 VertexCount, UInt32 StartVertexLocation)
    {
        InsertCommand<DrawCommand>(VertexCount, StartVertexLocation);
        NumDrawCalls++;
    }

    void DrawIndexed(UInt32 IndexCount, UInt32 StartIndexLocation, UInt32 BaseVertexLocation)
    {
        InsertCommand<DrawIndexedCommand>(IndexCount, StartIndexLocation, BaseVertexLocation);
        NumDrawCalls++;
    }

    void DrawInstanced(UInt32 VertexCountPerInstance, UInt32 InstanceCount, UInt32 StartVertexLocation, UInt32 StartInstanceLocation)
    {
        InsertCommand<DrawInstancedCommand>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    void DrawIndexedInstanced(
        UInt32 IndexCountPerInstance, 
        UInt32 InstanceCount, 
        UInt32 StartIndexLocation, 
        UInt32 BaseVertexLocation, 
        UInt32 StartInstanceLocation)
    {
        InsertCommand<DrawIndexedInstancedCommand>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    void Dispatch(UInt32 ThreadGroupCountX, UInt32 ThreadGroupCountY, UInt32 ThreadGroupCountZ)
    {
        InsertCommand<DispatchComputeCommand>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        NumDispatchCalls++;
    }

    void DispatchRays(UInt32 Width, UInt32 Height, UInt32 Depth)
    {
        InsertCommand<DispatchRaysCommand>(Width, Height, Depth);
    }

    void InsertCommandListMarker(const std::string& Marker)
    {
        InsertCommand<InsertCommandListMarkerCommand>(Marker);
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

    UInt32 GetNumDrawCalls() const { return NumDrawCalls; }
    UInt32 GetNumDispatchCalls() const { return NumDispatchCalls; }
    UInt32 GetNumCommands() const { return NumCommands; }

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

private:
    LinearAllocator CmdAllocator;
    RenderCommand* First;
    RenderCommand* Last;

    UInt32 NumDrawCalls     = 0;
    UInt32 NumDispatchCalls = 0;
    UInt32 NumCommands      = 0;

    Bool IsRecording = false;
};

class CommandListExecutor
{
public:
    CommandListExecutor()  = default;
    ~CommandListExecutor() = default;

    void ExecuteCommandList(CommandList& CmdList);
    void WaitForGPU();

    void SetContext(ICommandContext* InCmdContext)
    {
        VALIDATE(InCmdContext != nullptr);
        CmdContext = InCmdContext;
    }

    ICommandContext& GetContext()
    {
        VALIDATE(CmdContext != nullptr);
        return *CmdContext;
    }

private:
    ICommandContext* CmdContext = nullptr;
};