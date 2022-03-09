#pragma once
#include "RHIModule.h"
#include "RHIResources.h"
#include "RHICommand.h"
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
// CCommandAllocator

class RHI_API CCommandAllocator
{
public:

    CCommandAllocator(uint32 StartSize = 4096);
    ~CCommandAllocator();

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

    uint8* CurrentMemory;

    uint64 Size;
    uint64 Offset;
    uint64 AverageMemoryUsage;

    TArray<uint8*> DiscardedMemory;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandQueue

class RHI_API CRHICommandQueue
{
public:

    /**
     * Retrieve the RHICommandQueue instance
     * 
     * @return: Returns the instance of the RHICommandQueue
     */
    static CRHICommandQueue& Get();

    /**
     * Execute a single RHICommandList
     * 
     * @param CmdList: CommandList to execute
     */
    void ExecuteCommandList(class CRHICommandList& CmdList);

    /**
     * Execute multiple RHICommandLists
     *
     * @param CmdLists: CommandLists to execute
     * @param NumCmdLists: Number of CommandLists to execute
     */
    void ExecuteCommandLists(class CRHICommandList* const* CmdLists, uint32 NumCmdLists);

    /**
     * Wait for the GPU to finish all submitted operations
     */
    void WaitForGPU();

    /**
     * Set the context that should be used
     * 
     * @param InCmdContext: CommandContext to use when executing CommandLists
     */
    FORCEINLINE void SetContext(IRHICommandContext* InCmdContext)
    {
        CmdContext = InCmdContext;
    }

    /**
     * Retrieve the CommandContext that is used when executing CommandLists
     * 
     * @return: Returns a reference to the CommandContext that is currently being used
     */
    FORCEINLINE IRHICommandContext& GetContext()
    {
        Assert(CmdContext != nullptr);
        return *CmdContext;
    }

    /**
     * Retrieve the number of draw-calls that where in the previously executed CommandList
     * 
     * @return: Returns the number of draw-calls in the previously executed CommandList
     */
    FORCEINLINE uint32 GetNumDrawCalls() const
    {
        return NumDrawCalls;
    }

    /**
     * Retrieve the number of dispatch-calls that where in the previously executed CommandList
     *
     * @return: Returns the number of dispatch-calls in the previously executed CommandList
     */
    FORCEINLINE uint32 GetNumDispatchCalls() const
    {
        return NumDispatchCalls;
    }

    /**
     * Retrieve the number of commands that where in the previously executed CommandList
     *
     * @return: Returns the number of commands in the previously executed CommandList
     */
    FORCEINLINE uint32 GetNumCommands() const
    {
        return NumCommands;
    }

private:

    CRHICommandQueue();
    ~CRHICommandQueue() = default;

    /** Internal function for executing the CommandList */
    void InternalExecuteCommandList(class CRHICommandList& CmdList);

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandList

class CRHICommandList
{
    friend class CRHICommandQueue;

public:

    /**
     * Default constructor
     */
    CRHICommandList()
        : CmdAllocator()
        , First(nullptr)
        , Last(nullptr)
    {
    }

    /**
     * Destructor
     */
    ~CRHICommandList()
    {
        Reset();
    }

    /**
     * Begins the timestamp with the specified index in the TimestampQuery
     *
     * @param TimestampQuery: Timestamp-Query object to work on
     * @param Index: Timestamp index within the query object to begin
     */
    void BeginTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index)
    {
        InsertCommand<CRHIBeginTimeStampCommand>(AddRef(TimestampQuery), Index);
    }

    /**
     * Ends the timestamp with the specified index in the TimestampQuery
     *
     * @param TimestampQuery: Timestamp-Query object to work on
     * @param Index: Timestamp index within the query object to end
     */
    void EndTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index)
    {
        InsertCommand<CRHIEndTimeStampCommand>(AddRef(TimestampQuery), Index);
    }

    /**
     * Clears a RenderTargetView with a specific color
     *
     * @param RenderTargetView: RenderTargetView to clear
     * @param ClearColor: Color to set each pixel within the RenderTargetView to
     */
    void ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor)
    {
        Assert(RenderTargetView != nullptr);
        InsertCommand<CRHIClearRenderTargetViewCommand>(AddRef(RenderTargetView), ClearColor);
    }

    /**
     * Clears a DepthStencilView with a specific value
     *
     * @param DepthStencilView: DepthStencilView to clear
     * @param ClearValue: Value to set each pixel within the DepthStencilView to
     */
    void ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const SDepthStencil& ClearValue)
    {
        Assert(DepthStencilView != nullptr);
        InsertCommand<CRHIClearDepthStencilViewCommand>(AddRef(DepthStencilView), ClearValue);
    }

    /**
     * Clears a UnorderedAccessView with a specific value
     *
     * @param UnorderedAccessView: UnorderedAccessView to clear
     * @param ClearColor: Value to set each pixel within the UnorderedAccessView to
     */
    void ClearUnorderedAccessView(CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor)
    {
        Assert(UnorderedAccessView != nullptr);
        InsertCommand<CRHIClearUnorderedAccessViewFloatCommand>(AddRef(UnorderedAccessView), ClearColor);
    }

    /**
     * Sets the Shading-Rate for the fullscreen
     *
     * @param ShadingRate: New shading-rate for the upcoming draw-calls
     */
    void SetShadingRate(ERHIShadingRate ShadingRate)
    {
        InsertCommand<CRHISetShadingRateCommand>(ShadingRate);
    }

    /**
     * Set the Shading-Rate image that should be used
     *
     * @param ShadingImage: Image containing the shading rate for the next upcoming draw-calls
     */
    void SetShadingRateImage(CRHITexture2D* ShadingRateImage)
    {
        InsertCommand<CRHISetShadingRateImageCommand>(AddRef(ShadingRateImage));
    }

    /**
     * Begin a RenderPass
     */
    void BeginRenderPass()
    {
        InsertCommand<CRHIBeginRenderPassCommand>();
    }

    /**
     * Ends a RenderPass
     */
    void EndRenderPass()
    {
        InsertCommand<CRHIEndRenderPassCommand>();
    }

    /**
     * Set the current viewport settings
     *
     * @param Width: Width of the viewport
     * @param Height: Height of the viewport
     * @param MinDepth: Minimum-depth of the viewport
     * @param MaxDepth: Maximum-depth of the viewport
     * @param x: x-position of the viewport
     * @param y: y-position of the viewport
     */
    void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
    {
        InsertCommand<CRHISetViewportCommand>(Width, Height, MinDepth, MaxDepth, x, y);
    }

    /**
     * Set the current scissor settings
     *
     * @param Width: Width of the viewport
     * @param Height: Height of the viewport
     * @param x: x-position of the viewport
     * @param y: y-position of the viewport
     */
    void SetScissorRect(float Width, float Height, float x, float y)
    {
        InsertCommand<CRHISetScissorRectCommand>(Width, Height, x, y);
    }

    /**
     * Set the BlendFactor color
     *
     * @param Color: New blend-factor to use
     */
    void SetBlendFactor(const SColorF& Color)
    {
        InsertCommand<CRHISetBlendFactorCommand>(Color);
    }

    /**
     * Set all the RenderTargetViews and the DepthStencilView that should be used, nullptr is valid if the slot should not be used
     *
     * @param RenderTargetViews: Array of RenderTargetViews to use, each pointer in the array must be valid
     * @param RenderTargetCount: Number of RenderTargetViews in the array
     * @param DepthStencilView: DepthStencilView to set
     */
    void SetRenderTargets(CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView)
    {
        CRHIRenderTargetView** RenderTargets = CmdAllocator.Allocate<CRHIRenderTargetView*>(RenderTargetCount);
        for (uint32 i = 0; i < RenderTargetCount; i++)
        {
            RenderTargets[i] = AddRef<CRHIRenderTargetView>(RenderTargetViews[i]);
        }

        InsertCommand<CRHISetRenderTargetsCommand>(RenderTargets, RenderTargetCount, MakeSharedRef<CRHIDepthStencilView>(DepthStencilView));
    }

    /**
     * Set the VertexBuffers to be used
     *
     * @param VertexBuffers: Array of VertexBuffers to use
     * @param VertexBufferCount: Number of VertexBuffers in the array
     * @param BufferSlot: Slot to start bind the array to
     */
    void SetVertexBuffers(CRHIBuffer* const* VertexBuffers, uint32 VertexBufferCount, uint32 BufferSlot)
    {
        CRHIBuffer** Buffers = CmdAllocator.Allocate<CRHIBuffer*>(VertexBufferCount);
        for (uint32 i = 0; i < VertexBufferCount; i++)
        {
            Buffers[i] = AddRef<CRHIBuffer>(VertexBuffers[i]);
        }

        InsertCommand<CRHISetVertexBuffersCommand>(Buffers, VertexBufferCount, BufferSlot);
    }

    /**
     * Set the current IndexBuffer
     *
     * @param IndexBuffer: IndexBuffer to use
     */
    void SetIndexBuffer(CRHIBuffer* IndexBuffer)
    {
        InsertCommand<CRHISetIndexBufferCommand>(MakeSharedRef<CRHIBuffer>(IndexBuffer));
    }

    /**
     * Set the primitive topology
     *
     * @param PrimitveTopologyType: New primitive topology to use
     */
    void SetPrimitiveTopology(ERHIPrimitiveTopology PrimitveTopologyType)
    {
        InsertCommand<CRHISetPrimitiveTopologyCommand>(PrimitveTopologyType);
    }

    /**
     * Sets the current graphics PipelineState
     *
     * @param PipelineState: New PipelineState to use
     */
    void SetGraphicsPipelineState(CRHIGraphicsPipelineState* PipelineState)
    {
        InsertCommand<CRHISetGraphicsPipelineStateCommand>(AddRef(PipelineState));
    }

    /**
     * Sets the current compute PipelineState
     *
     * @param PipelineState: New PipelineState to use
     */
    void SetComputePipelineState(CRHIComputePipelineState* PipelineState)
    {
        InsertCommand<CRHISetComputePipelineStateCommand>(AddRef(PipelineState));
    }

    /**
     * Set shader constants
     *
     * @param Shader: Shader to bind the constants to
     * @param Shader32BitConstants: Array of 32-bit constants
     * @param Num32bitConstants: Number o 32-bit constants (Each is 4 bytes)
     */
    void Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
    {
        const uint32 Num32BitConstantsInBytes = Num32BitConstants * 4;

        void* Shader32BitConstantsMemory = CmdAllocator.Allocate(Num32BitConstantsInBytes, 1);
        CMemory::Memcpy(Shader32BitConstantsMemory, Shader32BitConstants, Num32BitConstantsInBytes);

        InsertCommand<CRHISet32BitShaderConstantsCommand>(AddRef(Shader), Shader32BitConstantsMemory, Num32BitConstants);
    }

    /**
     * Sets a single ShaderResourceView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ShaderResourceView: ShaderResourceView to bind
     * @param ParameterIndex: ShaderResourceView-index to bind to
     */
    void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
    {
        InsertCommand<CRHISetShaderResourceViewCommand>(AddRef(Shader), AddRef(ShaderResourceView), ParameterIndex);
    }

    /**
     * Sets a multiple ShaderResourceViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ShaderResourceViews: Array of ShaderResourceViews to bind
     * @param NumShaderResourceViews: Number of ShaderResourceViews in the array
     * @param ParameterIndex: ShaderResourceView-index to bind to
     */
    void SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceViews, uint32 NumShaderResourceViews, uint32 ParameterIndex)
    {
        CRHIShaderResourceView** TempShaderResourceViews = CmdAllocator.Allocate<CRHIShaderResourceView*>(NumShaderResourceViews);
        for (uint32 i = 0; i < NumShaderResourceViews; i++)
        {
            TempShaderResourceViews[i] = AddRef(ShaderResourceViews[i]);
        }

        InsertCommand<CRHISetShaderResourceViewsCommand>(AddRef(Shader), TempShaderResourceViews, NumShaderResourceViews, ParameterIndex);
    }

    /**
     * Sets a single UnorderedAccessView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param UnorderedAccessView: UnorderedAccessView to bind
     * @param ParameterIndex: UnorderedAccessView-index to bind to
     */
    void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
    {
        InsertCommand<CRHISetUnorderedAccessViewCommand>(AddRef(Shader), AddRef(UnorderedAccessView), ParameterIndex);
    }

    /**
     * Sets a multiple UnorderedAccessViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param UnorderedAccessViews: Array of UnorderedAccessViews to bind
     * @param NumUnorderedAccessViews: Number of UnorderedAccessViews in the array
     * @param ParameterIndex: UnorderedAccessView-index to bind to
     */
    void SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
    {
        CRHIUnorderedAccessView** TempUnorderedAccessViews = CmdAllocator.Allocate<CRHIUnorderedAccessView*>(NumUnorderedAccessViews);
        for (uint32 i = 0; i < NumUnorderedAccessViews; i++)
        {
            TempUnorderedAccessViews[i] = AddRef(UnorderedAccessViews[i]);
        }

        InsertCommand<CRHISetUnorderedAccessViewsCommand>(AddRef(Shader), TempUnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex);
    }

    /**
     * Sets a single ConstantBuffer to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ConstantBuffer: ConstantBuffer to bind
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    void SetConstantBuffer(CRHIShader* Shader, CRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
    {
        InsertCommand<CRHISetConstantBufferCommand>(AddRef(Shader), AddRef(ConstantBuffer), ParameterIndex);
    }

    /**
     * Sets a multiple ConstantBuffers to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ConstantBuffers: Array of ConstantBuffers to bind
     * @param NumConstantBuffers: Number of ConstantBuffers in the array
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    void SetConstantBuffers(CRHIShader* Shader, CRHIBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
    {
        CRHIBuffer** TempConstantBuffers = CmdAllocator.Allocate<CRHIBuffer*>(NumConstantBuffers);
        for (uint32 i = 0; i < NumConstantBuffers; i++)
        {
            TempConstantBuffers[i] = AddRef(ConstantBuffers[i]);
        }

        InsertCommand<CRHISetConstantBuffersCommand>(AddRef(Shader), TempConstantBuffers, NumConstantBuffers, ParameterIndex);
    }

    /**
     * Sets a single SamplerState to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind sampler to
     * @param SamplerState: SamplerState to bind
     * @param ParameterIndex: SamplerState-index to bind to
     */
    void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex)
    {
        InsertCommand<CRHISetSamplerStateCommand>(AddRef(Shader), AddRef(SamplerState), ParameterIndex);
    }

    /**
     * Sets a multiple SamplerStates to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param SamplerStates: Array of SamplerStates to bind
     * @param NumConstantBuffers: Number of ConstantBuffers in the array
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
    {
        CRHISamplerState** TempSamplerStates = CmdAllocator.Allocate<CRHISamplerState*>(NumSamplerStates);
        for (uint32 i = 0; i < NumSamplerStates; i++)
        {
            TempSamplerStates[i] = AddRef(SamplerStates[i]);
        }

        InsertCommand<CRHISetSamplerStatesCommand>(AddRef(Shader), TempSamplerStates, NumSamplerStates, ParameterIndex);
    }

    /**
     * Updates the contents of a Buffer
     *
     * @param Dst: Destination buffer to update
     * @param OffsetInBytes: Offset in bytes inside the destination-buffer
     * @param SizeInBytes: Number of bytes to copy over to the buffer
     * @param SourceData: SourceData to copy to the GPU
     */
    void UpdateBuffer(CRHIBuffer* Dst, uint64 DestinationOffsetInBytes, uint64 SizeInBytes, const void* SourceData)
    {
        void* TempSourceData = CmdAllocator.Allocate(static_cast<uint32>(SizeInBytes), 1);
        CMemory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        InsertCommand<CRHIUpdateBufferCommand>(AddRef(Dst), DestinationOffsetInBytes, SizeInBytes, TempSourceData);
    }

    /**
     * Updates the contents of a Texture2D
     *
     * @param Dst: Destination Texture2D to update
     * @param Width: Width of the texture to update
     * @param Height: Height of the texture to update
     * @param MipLevel: MipLevel of the texture to update
     * @param SourceData: SourceData to copy to the GPU
     */
    void UpdateTexture2D(CRHITexture2D* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
    {
        const uint32 SizeInBytes = Width * Height * GetByteStrideFromFormat(Dst->GetFormat());

        void* TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
        CMemory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        InsertCommand<CRHIUpdateTexture2DCommand>(AddRef(Dst), Width, Height, MipLevel, TempSourceData);
    }

    /**
     * Resolves a multi-sampled texture, must have the same sizes and compatible formats
     *
     * @param Dst: Destination texture, must have a single sample
     * @param Src: Source texture to resolve
     */
    void ResolveTexture(CRHITexture* Dst, CRHITexture* Src)
    {
        InsertCommand<CRHIResolveTextureCommand>(AddRef(Dst), AddRef(Src));
    }

    /**
     * Copies the contents from one buffer to another
     *
     * @param Dst: Destination buffer to copy to
     * @param Src: Source buffer to copy from
     * @param CopyInfo: Information about the copy operation
     */
    void CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src, const SRHICopyBufferInfo& CopyInfo)
    {
        InsertCommand<CRHICopyBufferCommand>(AddRef(Dst), AddRef(Src), CopyInfo);
    }

    /**
     * Copies the entire contents of one texture to another, which require the size and formats to be the same
     *
     * @param Dst: Destination texture
     * @param Src: Source texture
     */
    void CopyTexture(CRHITexture* Dst, CRHITexture* Src)
    {
        InsertCommand<CRHICopyTextureCommand>(AddRef(Dst), AddRef(Src));
    }

    /**
     * Copies contents of a texture region of one texture to another, which require the size and formats to be the same
     *
     * @param Dst: Destination texture
     * @param Src: Source texture
     * @param CopyTextureInfo: Information about the copy operation
     */
    void CopyTextureRegion(CRHITexture* Dst, CRHITexture* Src, const SRHICopyTextureInfo& CopyTextureInfo)
    {
        InsertCommand<CRHICopyTextureRegionCommand>(AddRef(Dst), AddRef(Src), CopyTextureInfo);
    }

    /**
     * Destroys a resource, this can be used to not having to deal with resource life time, the resource will be destroyed when the underlying command-list is completed
     *
     * @param Resource: Resource to destroy
     */
    void DestroyResource(CRHIObject* Resource)
    {
        InsertCommand<CRHIDestroyResourceCommand>(AddRef(Resource));
    }

    /**
     * Signal the driver that the contents can be discarded
     *
     * @param Resource: Resource to discard contents of
     */
    void DiscardContents(CRHIResource* Resource)
    {
        InsertCommand<CRHIDiscardContentsCommand>(AddRef(Resource));
    }

    /**
     * Builds the Bottom-Level Acceleration-Structure for ray tracing
     *
     * @param Geometry: Bottom-level acceleration-structure to build or update
     * @param VertexBuffer: VertexBuffer to build Geometry of
     * @param IndexBuffer: IndexBuffer to build Geometry of
     * @param bUpdate: True if the build should be an update, false if it should build from the ground up
     */
    void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIBuffer* VertexBuffer, CRHIBuffer* IndexBuffer, bool bUpdate)
    {
        Assert((Geometry != nullptr) && (!bUpdate || (bUpdate && Geometry->GetFlags() & RayTracingStructureBuildFlag_AllowUpdate)));
        InsertCommand<CRHIBuildRayTracingGeometryCommand>(AddRef(Geometry), AddRef(VertexBuffer), AddRef(IndexBuffer), bUpdate);
    }

    /**
     * Builds the Top-Level Acceleration-Structure for ray tracing
     *
     * @param Scene: Top-level acceleration-structure to build or update
     * @param Instances: Instances to build the scene of
     * @param NumInstances: Number of instances to build
     * @param bUpdate: True if the build should be an update, false if it should build from the ground up
     */
    void BuildRayTracingScene(CRHIRayTracingScene* Scene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate)
    {
        Assert((Scene != nullptr) && (!bUpdate || (bUpdate && Scene->GetFlags() & RayTracingStructureBuildFlag_AllowUpdate)));
        InsertCommand<CRHIBuildRayTracingSceneCommand>(AddRef(Scene), Instances, NumInstances, bUpdate);
    }

    // TODO: Refactor
    void SetRayTracingBindings(
        CRHIRayTracingScene* RayTracingScene,
        CRHIRayTracingPipelineState* PipelineState,
        const SRayTracingShaderResources* GlobalResource,
        const SRayTracingShaderResources* RayGenLocalResources,
        const SRayTracingShaderResources* MissLocalResources,
        const SRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources)
    {
        InsertCommand<CRHISetRayTracingBindingsCommand>(AddRef(RayTracingScene), AddRef(PipelineState), GlobalResource, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
    }

    /**
     * Generate MipLevels for a texture. Works with Texture2D and TextureCubes.
     *
     * @param Texture: Texture to generate MipLevels for
     */
    void GenerateMips(CRHITexture* Texture)
    {
        Assert(Texture != nullptr);
        InsertCommand<CRHIGenerateMipsCommand>(AddRef(Texture));
    }

    /**
     * Transition the ResourceState of a Texture resource
     *
     * @param Texture: Texture to transition ResourceState for
     * @param BeforeState: State that the Texture had before the transition
     * @param AfterState: State that the Texture have after the transition
     */
    void TransitionTexture(CRHITexture* Texture, ERHIResourceState BeforeState, ERHIResourceState AfterState)
    {
        if (BeforeState != AfterState)
        {
            InsertCommand<CRHITransitionTextureCommand>(AddRef(Texture), BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Texture '" + Texture->GetName() + "' Was transitioned with the same Before- and AfterState (=" + ToString(BeforeState) + ")");
        }
    }

    /**
     * Transition the ResourceState of a Buffer resource
     *
     * @param Buffer: Buffer to transition ResourceState for
     * @param BeforeState: State that the Buffer had before the transition
     * @param AfterState: State that the Buffer have after the transition
     */
    void TransitionBuffer(CRHIBuffer* Buffer, ERHIResourceState BeforeState, ERHIResourceState AfterState)
    {
        Assert(Buffer != nullptr);

        if (BeforeState != AfterState)
        {
            InsertCommand<CRHITransitionBufferCommand>(AddRef(Buffer), BeforeState, AfterState);
        }
        else
        {
            LOG_WARNING("Buffer '" + Buffer->GetName() + "' Was transitioned with the same Before- and AfterState (=" + ToString(BeforeState) + ")");
        }
    }

    /**
     * Add a UnorderedAccessBarrier for a Texture resource, which should be issued before reading of a resource in UnorderedAccessState
     *
     * @param Texture: Texture to issue barrier for
     */
    void UnorderedAccessTextureBarrier(CRHITexture* Texture)
    {
        Assert(Texture != nullptr);
        InsertCommand<CRHIUnorderedAccessTextureBarrierCommand>(AddRef(Texture));
    }

    /**
     * Add a UnorderedAccessBarrier for a Buffer resource, which should be issued before reading of a resource in UnorderedAccessState
     *
     * @param Buffer: Buffer to issue barrier for
     */
    void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer)
    {
        Assert(Buffer != nullptr);
        InsertCommand<CRHIUnorderedAccessBufferBarrierCommand>(Buffer);
    }

    /**
     * Issue a draw-call
     *
     * @param VertexCount: Number of vertices
     * @param StartVertexLocation: Offset of the vertices
     */
    void Draw(uint32 VertexCount, uint32 StartVertexLocation)
    {
        InsertCommand<CRHIDrawCommand>(VertexCount, StartVertexLocation);
        NumDrawCalls++;
    }

    /**
     * Issue a draw-call for drawing with an IndexBuffer
     *
     * @param IndexCount: Number of indices
     * @param StartIndexLocation: Offset in the index-buffer
     * @param BaseVertexLocation: Index of the vertex that should be considered as index zero
     */
    void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
    {
        InsertCommand<CRHIDrawIndexedCommand>(IndexCount, StartIndexLocation, BaseVertexLocation);
        NumDrawCalls++;
    }

    /**
     * Issue a draw-call for drawing instanced
     *
     * @param VertexCountPerInstance: Number of vertices per instance
     * @param InstanceCount: Number of instances
     * @param StartVertexLocation: Offset of the vertices
     * @param StartInstanceLocation: Offset of the instances
     */
    void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
    {
        InsertCommand<CRHIDrawInstancedCommand>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    /**
     * Issue a draw-call for drawing instanced with an IndexBuffer
     *
     * @param IndexCountPerInstance: Number of indices per instance
     * @param InstanceCount: Number of instances
     * @param StartIndexLocation: Offset of the index to start with
     * @param BaseVertexLocation: Offset of the vertices
     * @param StartInstanceLocation: Offset of the instances
     */
    void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
    {
        InsertCommand<CRHIDrawIndexedInstancedCommand>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    /**
     * Issues a compute dispatch
     *
     * @param WorkGroupX: Number of work-groups in x-direction
     * @param WorkGroupY: Number of work-groups in y-direction
     * @param WorkGroupZ: Number of work-groups in z-direction
     */
    void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
    {
        InsertCommand<CRHIDispatchComputeCommand>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        NumDispatchCalls++;
    }

    /**
     * Issues a ray generation dispatch
     *
     * @param Scene: Scene to trace rays in
     * @param PipelineState: PipelineState to use when tracing
     * @param Width: Number of rays in x-direction
     * @param Height: Number of rays in y-direction
     * @param Depth: Number of rays in z-direction
     */
    void DispatchRays(CRHIRayTracingScene* Scene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
    {
        InsertCommand<CRHIDispatchRaysCommand>(AddRef(Scene), AddRef(PipelineState), Width, Height, Depth);
    }

    /**
     * Inserts a marker on the GPU timeline
     *
     * @param Message: Message for the marker
     */
    void InsertCommandListMarker(const String& Marker)
    {
        InsertCommand<CRHIInsertCommandListMarkerCommand>(Marker);
    }
    
    /**
     * Insert a debug-break into the command-list
     */
    void DebugBreak()
    {
        InsertCommand<CRHIDebugBreakCommand>();
    }

    /**
     *  Begins a PIX capture event, currently only available on D3D12
     */
    void BeginExternalCapture()
    {
        InsertCommand<CRHIBeginExternalCaptureCommand>();
    }

    /**
     * Ends a PIX capture event, currently only available on D3D12
     */
    void EndExternalCapture()
    {
        InsertCommand<CRHIEndExternalCaptureCommand>();
    }

    /**
     * Resets the CommandList
     */
    void Reset()
    {
        if (First != nullptr)
        {
            CRHICommand* Cmd = First;
            while (Cmd != nullptr)
            {
                CRHICommand* Old = Cmd;
                Cmd = Cmd->NextCmd;
                Old->~CRHICommand();
            }

            First = nullptr;
            Last = nullptr;
        }

        NumDrawCalls = 0;
        NumDispatchCalls = 0;
        NumCommands = 0;

        CmdAllocator.Reset();
    }

    /**
     * Retrieve the number of recorded draw-calls
     * 
     * @return: Returns the number of draw-calls
     */
    FORCEINLINE uint32 GetNumDrawCalls() const
    {
        return NumDrawCalls;
    }

    /**
     * Retrieve the number of recorded dispatch-calls
     *
     * @return: Returns the number of dispatch-calls
     */
    FORCEINLINE uint32 GetNumDispatchCalls() const
    {
        return NumDispatchCalls;
    }

    /**
     * Retrieve the number of recorded Commands
     *
     * @return: Returns the number of Commands
     */
    FORCEINLINE uint32 GetNumCommands() const
    {
        return NumCommands;
    }

private:

    template<typename CommandType, typename... ArgTypes>
    void InsertCommand(ArgTypes&&... Args)
    {
        CommandType* Cmd = CmdAllocator.Construct<CommandType>(Forward<ArgTypes>(Args)...);
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

    CRHICommand* First;
    CRHICommand* Last;

    uint32 NumDrawCalls = 0;
    uint32 NumDispatchCalls = 0;
    uint32 NumCommands = 0;
};
