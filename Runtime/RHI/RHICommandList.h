#pragma once
#include "RHIModule.h"
#include "RHIResources.h"
#include "RHICommands.h"
#include "RHITimestampQuery.h"

class FRHIRenderTargetView;
class FRHIDepthStencilView;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHIShader;

#define ENABLE_INSERT_DEBUG_CMDLIST_MARKER (0)

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
// FRHICommandQueue

class RHI_API FRHICommandQueue
{
public:

    /**
     * @brief: Retrieve the RHICommandQueue instance
     * 
     * @return: Returns the instance of the RHICommandQueue
     */
    static FRHICommandQueue& Get();

    /**
     * @brief: Execute a single RHICommandList
     * 
     * @param CmdList: CommandList to execute
     */
    void ExecuteCommandList(class FRHICommandList& CmdList);

    /**
     * @brief: Execute multiple RHICommandLists
     *
     * @param CmdLists: CommandLists to execute
     * @param NumCmdLists: Number of CommandLists to execute
     */
    void ExecuteCommandLists(class FRHICommandList* const* CmdLists, uint32 NumCmdLists);

    /**
     * @brief: Wait for the GPU to finish all submitted operations
     */
    void WaitForGPU();

    /**
     * @brief: Set the context that should be used
     * 
     * @param InCmdContext: CommandContext to use when executing CommandLists
     */
    FORCEINLINE void SetContext(IRHICommandContext* InCmdContext)
    {
        CmdContext = InCmdContext;
    }

    /**
     * @brief: Retrieve the CommandContext that is used when executing CommandLists
     * 
     * @return: Returns a reference to the CommandContext that is currently being used
     */
    FORCEINLINE IRHICommandContext& GetContext()
    {
        Check(CmdContext != nullptr);
        return *CmdContext;
    }

    /**
     * @brief: Retrieve the number of draw-calls that where in the previously executed CommandList
     * 
     * @return: Returns the number of draw-calls in the previously executed CommandList
     */
    FORCEINLINE uint32 GetNumDrawCalls() const
    {
        return NumDrawCalls;
    }

    /**
     * @brief: Retrieve the number of dispatch-calls that where in the previously executed CommandList
     *
     * @return: Returns the number of dispatch-calls in the previously executed CommandList
     */
    FORCEINLINE uint32 GetNumDispatchCalls() const
    {
        return NumDispatchCalls;
    }

    /**
     * @brief: Retrieve the number of commands that where in the previously executed CommandList
     *
     * @return: Returns the number of commands in the previously executed CommandList
     */
    FORCEINLINE uint32 GetNumCommands() const
    {
        return NumCommands;
    }

private:

    FRHICommandQueue();
    ~FRHICommandQueue() = default;

    /** Internal function for executing the CommandList */
    void InternalExecuteCommandList(class FRHICommandList& CmdList);

    FORCEINLINE void ResetStatistics()
    {
        NumDrawCalls     = 0;
        NumDispatchCalls = 0;
        NumCommands      = 0;
    }

    IRHICommandContext* CmdContext;

    // Statistics
    uint32              NumDrawCalls;
    uint32              NumDispatchCalls;
    uint32              NumCommands;

    static FRHICommandQueue Instance;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandList

class FRHICommandList
{
    friend class FRHICommandQueue;

public:

    /**
     * @brief: Default constructor
     */
    FRHICommandList()
        : CmdAllocator()
        , FirstCommand(nullptr)
        , LastCommand(nullptr)
	    , NumDrawCalls(0)
	    , NumDispatchCalls(0)
	    , NumCommands(0)
    { }

    /**
     * @brief: Destructor
     */
    ~FRHICommandList()
    {
        Reset();
    }

    /**
     * @brief: Begins the timestamp with the specified index in the TimestampQuery
     *
     * @param TimestampQuery: Timestamp-Query object to work on
     * @param Index: Timestamp index within the query object to begin
     */
    void BeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
    {
        InsertCommand<FRHICommandBeginTimeStamp>(TimestampQuery, Index);
    }

    /**
     * @brief: Ends the timestamp with the specified index in the TimestampQuery
     *
     * @param TimestampQuery: Timestamp-Query object to work on
     * @param Index: Timestamp index within the query object to end
     */
    void EndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
    {
        InsertCommand<FRHICommandEndTimeStamp>(TimestampQuery, Index);
    }

    /**
     * @brief: Clears a RenderTargetView with a specific color
     *
     * @param RenderTargetView: RenderTargetView to clear
     * @param ClearColor: Color to set each pixel within the RenderTargetView to
     */
    void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const TStaticArray<float, 4>& ClearColor)
    {
        Check(RenderTargetView.Texture != nullptr);
        InsertCommand<FRHICommandClearRenderTargetView>(RenderTargetView, ClearColor);
    }

    /**
     * @brief: Clears a DepthStencilView with a specific value
     *
     * @param DepthStencilView: DepthStencilView to clear
     * @param ClearValue: Value to set each pixel within the DepthStencilView to
     */
    void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
    {
        Check(DepthStencilView.Texture != nullptr);
        InsertCommand<FRHICommandClearDepthStencilView>(DepthStencilView, Depth, Stencil);
    }

    /**
     * @brief: Clears a UnorderedAccessView with a specific value
     *
     * @param UnorderedAccessView: UnorderedAccessView to clear
     * @param ClearColor: Value to set each pixel within the UnorderedAccessView to
     */
    void ClearUnorderedAccessView(FRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor)
    {
        Check(UnorderedAccessView != nullptr);
        InsertCommand<FRHICommandClearUnorderedAccessViewFloat>(UnorderedAccessView, ClearColor);
    }

    /**
     * @brief: Begin a RenderPass
     */
    void BeginRenderPass(const FRHIRenderPassInitializer& RenderPassInitializer)
    {
        Check(bIsRenderPassActive == false);

        InsertCommand<FRHICommandBeginRenderPass>(RenderPassInitializer);
        bIsRenderPassActive = true;
    }

    /**
     * @brief: Ends a RenderPass
     */
    void EndRenderPass()
    {
        Check(bIsRenderPassActive == true);

        InsertCommand<FRHICommandEndRenderPass>();
        bIsRenderPassActive = false;
    }

    /**
     * @brief: Set the current viewport settings
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
        InsertCommand<FRHICommandSetViewport>(Width, Height, MinDepth, MaxDepth, x, y);
    }

    /**
     * @brief: Set the current scissor settings
     *
     * @param Width: Width of the viewport
     * @param Height: Height of the viewport
     * @param x: x-position of the viewport
     * @param y: y-position of the viewport
     */
    void SetScissorRect(float Width, float Height, float x, float y)
    {
        InsertCommand<FRHICommandSetScissorRect>(Width, Height, x, y);
    }

    /**
     * @brief: Set the BlendFactor color
     *
     * @param Color: New blend-factor to use
     */
    void SetBlendFactor(const TStaticArray<float, 4>& Color)
    {
        InsertCommand<FRHICommandSetBlendFactor>(Color);
    }

    /**
     * @brief: Set the VertexBuffers to be used
     *
     * @param VertexBuffers: Array of VertexBuffers to use
     * @param VertexBufferCount: Number of VertexBuffers in the array
     * @param BufferSlot: Slot to start bind the array to
     */
    void SetVertexBuffers(FRHIVertexBuffer* const* InVertexBuffers, uint32 NumVertexBuffers, uint32 BufferSlot)
    {
        bool bNeedsBinding = false;
        for (uint32 Index = 0; Index < NumVertexBuffers; ++Index)
        {
            const uint32 RealIndex = BufferSlot + Index;
            if (VertexBuffers[RealIndex] != InVertexBuffers[Index])
            {
                bNeedsBinding = true;
                VertexBuffers[RealIndex] = InVertexBuffers[Index];
                break;
            }
        }

        if (bNeedsBinding)
        {
            FRHIVertexBuffer** TempVertexBuffers = CmdAllocator.Allocate<FRHIVertexBuffer*>(NumVertexBuffers);
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
    }

    /**
     * @brief: Set the current IndexBuffer
     *
     * @param IndexBuffer: IndexBuffer to use
     */
    void SetIndexBuffer(FRHIIndexBuffer* IndexBuffer)
    {
        InsertCommand<FRHICommandSetIndexBuffer>(IndexBuffer);
    }

    /**
     * @brief: Set the primitive topology
     *
     * @param PrimitveTopologyType: New primitive topology to use
     */
    void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
    {
        InsertCommand<FRHICommandSetPrimitiveTopology>(PrimitveTopologyType);
    }

    /**
     * @brief: Sets the current graphics PipelineState
     *
     * @param PipelineState: New PipelineState to use
     */
    void SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
    {
        InsertCommand<FRHICommandSetGraphicsPipelineState>(PipelineState);
    }

    /**
     * @brief: Sets the current compute PipelineState
     *
     * @param PipelineState: New PipelineState to use
     */
    void SetComputePipelineState(FRHIComputePipelineState* PipelineState)
    {
        InsertCommand<FRHICommandSetComputePipelineState>(PipelineState);
    }

    /**
     * @brief: Set shader constants
     *
     * @param Shader: Shader to bind the constants to
     * @param Shader32BitConstants: Array of 32-bit constants
     * @param Num32bitConstants: Number o 32-bit constants (Each is 4 bytes)
     */
    void Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
    {
        const uint32 Num32BitConstantsInBytes = Num32BitConstants * 4;

        void* Shader32BitConstantsMemory = CmdAllocator.Allocate(Num32BitConstantsInBytes, 1);
        FMemory::Memcpy(Shader32BitConstantsMemory, Shader32BitConstants, Num32BitConstantsInBytes);

        InsertCommand<FRHICommandSet32BitShaderConstants>(Shader, Shader32BitConstantsMemory, Num32BitConstants);
    }

    /**
     * @brief: Sets a single ShaderResourceView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ShaderResourceView: ShaderResourceView to bind
     * @param ParameterIndex: ShaderResourceView-index to bind to
     */
    void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
    {
        InsertCommand<FRHICommandSetShaderResourceView>(Shader, ShaderResourceView, ParameterIndex);
    }

    /**
     * @brief: Sets a multiple ShaderResourceViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ShaderResourceViews: Array of ShaderResourceViews to bind
     * @param NumShaderResourceViews: Number of ShaderResourceViews in the array
     * @param ParameterIndex: ShaderResourceView-index to bind to
     */
    void SetShaderResourceViews(FRHIShader* Shader, FRHIShaderResourceView* const* ShaderResourceViews, uint32 NumShaderResourceViews, uint32 ParameterIndex)
    {
        FRHIShaderResourceView** TempShaderResourceViews = CmdAllocator.Allocate<FRHIShaderResourceView*>(NumShaderResourceViews);
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

    /**
     * @brief: Sets a single UnorderedAccessView to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param UnorderedAccessView: UnorderedAccessView to bind
     * @param ParameterIndex: UnorderedAccessView-index to bind to
     */
    void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
    {
        InsertCommand<FRHICommandSetUnorderedAccessView>(Shader, UnorderedAccessView, ParameterIndex);
    }

    /**
     * @brief: Sets a multiple UnorderedAccessViews to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param UnorderedAccessViews: Array of UnorderedAccessViews to bind
     * @param NumUnorderedAccessViews: Number of UnorderedAccessViews in the array
     * @param ParameterIndex: UnorderedAccessView-index to bind to
     */
    void SetUnorderedAccessViews(FRHIShader* Shader, FRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
    {
        FRHIUnorderedAccessView** TempUnorderedAccessViews = CmdAllocator.Allocate<FRHIUnorderedAccessView*>(NumUnorderedAccessViews);
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

    /**
     * @brief: Sets a single ConstantBuffer to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ConstantBuffer: ConstantBuffer to bind
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    void SetConstantBuffer(FRHIShader* Shader, FRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
    {
        InsertCommand<FRHICommandSetConstantBuffer>(Shader, ConstantBuffer, ParameterIndex);
    }

    /**
     * @brief: Sets a multiple ConstantBuffers to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param ConstantBuffers: Array of ConstantBuffers to bind
     * @param NumConstantBuffers: Number of ConstantBuffers in the array
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    void SetConstantBuffers(FRHIShader* Shader, FRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
    {
        FRHIConstantBuffer** TempConstantBuffers = CmdAllocator.Allocate<FRHIConstantBuffer*>(NumConstantBuffers);
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

    /**
     * @brief: Sets a single SamplerState to the ParameterIndex, this must be a valid index in the specified shader, which can be queried from the shader-object
     *
     * @param Shader: Shader to bind sampler to
     * @param SamplerState: SamplerState to bind
     * @param ParameterIndex: SamplerState-index to bind to
     */
    void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
    {
        InsertCommand<FRHICommandSetSamplerState>(Shader, SamplerState, ParameterIndex);
    }

    /**
     * @brief: Sets a multiple SamplerStates to the ParameterIndex (For arrays in the shader), this must be a valid index in the specified shader,
     * which can be queried from the shader-object
     *
     * @param Shader: Shader to bind resource to
     * @param SamplerStates: Array of SamplerStates to bind
     * @param NumConstantBuffers: Number of ConstantBuffers in the array
     * @param ParameterIndex: ConstantBuffer-index to bind to
     */
    void SetSamplerStates(FRHIShader* Shader, FRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
    {
        FRHISamplerState** TempSamplerStates = CmdAllocator.Allocate<FRHISamplerState*>(NumSamplerStates);
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

    /**
     * @brief: Updates the contents of a Buffer
     *
     * @param Dst: Destination buffer to update
     * @param OffsetInBytes: Offset in bytes inside the destination-buffer
     * @param SizeInBytes: Number of bytes to copy over to the buffer
     * @param SourceData: SourceData to copy to the GPU
     */
    void UpdateBuffer(FRHIBuffer* Dst, uint32 DestinationOffsetInBytes, uint32 SizeInBytes, const void* SourceData)
    {
        void* TempSourceData = CmdAllocator.Allocate(SizeInBytes);
        FMemory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        InsertCommand<FRHICommandUpdateBuffer>(Dst, DestinationOffsetInBytes, SizeInBytes, TempSourceData);
    }

    /**
     * @brief: Updates the contents of a Texture2D
     *
     * @param Dst: Destination Texture2D to update
     * @param Width: Width of the texture to update
     * @param Height: Height of the texture to update
     * @param MipLevel: MipLevel of the texture to update
     * @param SourceData: SourceData to copy to the GPU
     */
    void UpdateTexture2D(FRHITexture2D* Dst, uint16 Width, uint16 Height, uint16 MipLevel, const void* SourceData)
    {
        const uint32 SizeInBytes = Width * Height * GetByteStrideFromFormat(Dst->GetFormat());

        void* TempSourceData = CmdAllocator.Allocate(SizeInBytes);
        FMemory::Memcpy(TempSourceData, SourceData, SizeInBytes);

        InsertCommand<FRHICommandUpdateTexture2D>(Dst, Width, Height, MipLevel, TempSourceData);
    }

    /**
     * @brief: Resolves a multi-sampled texture, must have the same sizes and compatible formats
     *
     * @param Dst: Destination texture, must have a single sample
     * @param Src: Source texture to resolve
     */
    void ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
    {
        InsertCommand<FRHICommandResolveTexture>(Dst, Src);
    }

    /**
     * @brief: Copies the contents from one buffer to another
     *
     * @param Dst: Destination buffer to copy to
     * @param Src: Source buffer to copy from
     * @param CopyInfo: Information about the copy operation
     */
    void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHICopyBufferInfo& CopyInfo)
    {
        InsertCommand<FRHICommandCopyBuffer>(Dst, Src, CopyInfo);
    }

    /**
     * @brief: Copies the entire contents of one texture to another, which require the size and formats to be the same
     *
     * @param Dst: Destination texture
     * @param Src: Source texture
     */
    void CopyTexture(FRHITexture* Dst, FRHITexture* Src)
    {
        InsertCommand<FRHICommandCopyTexture>(Dst, Src);
    }

    /**
     * @brief: Copies contents of a texture region of one texture to another, which require the size and formats to be the same
     *
     * @param Dst: Destination texture
     * @param Src: Source texture
     * @param CopyTextureInfo: Information about the copy operation
     */
    void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHICopyTextureInfo& CopyTextureInfo)
    {
        InsertCommand<FRHICommandCopyTextureRegion>(Dst, Src, CopyTextureInfo);
    }

    /**
     * @brief: Destroys a resource, this can be used to not having to deal with resource life time, the resource will be destroyed when the underlying command-list is completed
     *
     * @param Resource: Resource to destroy
     */
    void DestroyResource(IRefCounted* Resource)
    {
        InsertCommand<FRHICommandDestroyResource>(Resource);
    }

    /**
     * @brief: Signal the driver that the contents can be discarded
     *
     * @param Texture: Texture to discard contents of
     */
    void DiscardContents(FRHITexture* Texture)
    {
        InsertCommand<FRHICommandDiscardContents>(Texture);
    }

    /**
     * @brief: Builds the Bottom-Level Acceleration-Structure for ray tracing
     *
     * @param Geometry: Bottom-level acceleration-structure to build or update
     * @param VertexBuffer: VertexBuffer to build Geometry of
     * @param IndexBuffer: IndexBuffer to build Geometry of
     * @param bUpdate: True if the build should be an update, false if it should build from the ground up
     */
    void BuildRayTracingGeometry(FRHIRayTracingGeometry* Geometry, FRHIVertexBuffer* VertexBuffer, FRHIIndexBuffer* IndexBuffer, bool bUpdate)
    {
        Check((Geometry != nullptr) && (!bUpdate || (bUpdate && (Geometry->GetFlags() & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None)));
        InsertCommand<FRHICommandBuildRayTracingGeometry>(Geometry, VertexBuffer, IndexBuffer, bUpdate);
    }

    /**
     * @brief: Builds the Top-Level Acceleration-Structure for ray tracing
     *
     * @param Scene: Top-level acceleration-structure to build or update
     * @param Instances: Instances to build the scene of
     * @param bUpdate: True if the build should be an update, false if it should build from the ground up
     */
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

    /**
     * @brief: Generate MipLevels for a texture. Works with Texture2D and TextureCubes.
     *
     * @param Texture: Texture to generate MipLevels for
     */
    void GenerateMips(FRHITexture* Texture)
    {
        Check(Texture != nullptr);
        InsertCommand<FRHICommandGenerateMips>(Texture);
    }

    /**
     * @brief: Transition the ResourceState of a Texture resource
     *
     * @param Texture: Texture to transition ResourceState for
     * @param BeforeState: State that the Texture had before the transition
     * @param AfterState: State that the Texture have after the transition
     */
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

    /**
     * @brief: Transition the ResourceState of a Buffer resource
     *
     * @param Buffer: Buffer to transition ResourceState for
     * @param BeforeState: State that the Buffer had before the transition
     * @param AfterState: State that the Buffer have after the transition
     */
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

    /**
     * @brief: Add a UnorderedAccessBarrier for a Texture resource, which should be issued before reading of a resource in UnorderedAccessState
     *
     * @param Texture: Texture to issue barrier for
     */
    void UnorderedAccessTextureBarrier(FRHITexture* Texture)
    {
        Check(Texture != nullptr);
        InsertCommand<FRHICommandUnorderedAccessTextureBarrier>(Texture);
    }

    /**
     * @brief: Add a UnorderedAccessBarrier for a Buffer resource, which should be issued before reading of a resource in UnorderedAccessState
     *
     * @param Buffer: Buffer to issue barrier for
     */
    void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
    {
        Check(Buffer != nullptr);
        InsertCommand<FRHICommandUnorderedAccessBufferBarrier>(Buffer);
    }

    /**
     * @brief: Issue a draw-call
     *
     * @param VertexCount: Number of vertices
     * @param StartVertexLocation: Offset of the vertices
     */
    void Draw(uint32 VertexCount, uint32 StartVertexLocation)
    {
        InsertCommand<FRHICommandDraw>(VertexCount, StartVertexLocation);
        NumDrawCalls++;
    }

    /**
     * @brief: Issue a draw-call for drawing with an IndexBuffer
     *
     * @param IndexCount: Number of indices
     * @param StartIndexLocation: Offset in the index-buffer
     * @param BaseVertexLocation: Index of the vertex that should be considered as index zero
     */
    void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
    {
        InsertCommand<FRHICommandDrawIndexed>(IndexCount, StartIndexLocation, BaseVertexLocation);
        NumDrawCalls++;
    }

    /**
     * @brief: Issue a draw-call for drawing instanced
     *
     * @param VertexCountPerInstance: Number of vertices per instance
     * @param InstanceCount: Number of instances
     * @param StartVertexLocation: Offset of the vertices
     * @param StartInstanceLocation: Offset of the instances
     */
    void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
    {
        InsertCommand<FRHICommandDrawInstanced>(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    /**
     * @brief: Issue a draw-call for drawing instanced with an IndexBuffer
     *
     * @param IndexCountPerInstance: Number of indices per instance
     * @param InstanceCount: Number of instances
     * @param StartIndexLocation: Offset of the index to start with
     * @param BaseVertexLocation: Offset of the vertices
     * @param StartInstanceLocation: Offset of the instances
     */
    void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
    {
        InsertCommand<FRHICommandDrawIndexedInstanced>(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        NumDrawCalls++;
    }

    /**
     * @brief: Issues a compute dispatch
     *
     * @param WorkGroupX: Number of work-groups in x-direction
     * @param WorkGroupY: Number of work-groups in y-direction
     * @param WorkGroupZ: Number of work-groups in z-direction
     */
    void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
    {
        InsertCommand<FRHICommandDispatch>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        NumDispatchCalls++;
    }

    /**
     * @brief: Issues a ray generation dispatch
     *
     * @param Scene: Scene to trace rays in
     * @param PipelineState: PipelineState to use when tracing
     * @param Width: Number of rays in x-direction
     * @param Height: Number of rays in y-direction
     * @param Depth: Number of rays in z-direction
     */
    void DispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
    {
        InsertCommand<FRHICommandDispatchRays>(Scene, PipelineState, Width, Height, Depth);
    }

    /**
     * @brief: Inserts a marker on the GPU timeline
     *
     * @param Message: Message for the marker
     */
    void InsertMarker(const FString& Marker)
    {
        InsertCommand<FRHICommandInsertMarker>(Marker);
    }
    
    /**
     * @brief: Insert a debug-break into the command-list
     */
    void DebugBreak()
    {
        InsertCommand<FRHICommandDebugBreak>();
    }

    /**
     * @brief:  Begins a PIX capture event, currently only available on D3D12
     */
    void BeginExternalCapture()
    {
        InsertCommand<FRHICommandBeginExternalCapture>();
    }

    /**
     * @brief: Ends a PIX capture event, currently only available on D3D12
     */
    void EndExternalCapture()
    {
        InsertCommand<FRHICommandEndExternalCapture>();
    }

    /**
     * @brief: Resets the CommandList
     */
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

        NumDrawCalls     = 0;
        NumDispatchCalls = 0;
        NumCommands      = 0;

        CmdAllocator.Reset();

        VertexBuffers.Memzero();

        bIsRenderPassActive = false;
    }

    /**
     * @brief: Retrieve the number of recorded draw-calls
     * 
     * @return: Returns the number of draw-calls
     */
    FORCEINLINE uint32 GetNumDrawCalls() const
    {
        return NumDrawCalls;
    }

    /**
     * @brief: Retrieve the number of recorded dispatch-calls
     *
     * @return: Returns the number of dispatch-calls
     */
    FORCEINLINE uint32 GetNumDispatchCalls() const
    {
        return NumDispatchCalls;
    }

    /**
     * @brief: Retrieve the number of recorded Commands
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

        NumCommands++;
    }

private:

    FRHICommandAllocator CmdAllocator;

    FRHICommand*      FirstCommand;
    FRHICommand*      LastCommand;

    uint32            NumDrawCalls     = 0;
    uint32            NumDispatchCalls = 0;
    uint32            NumCommands      = 0;

    bool              bIsRenderPassActive = false;

    TStaticArray<FRHIVertexBuffer*, kRHIMaxVertexBuffers> VertexBuffers;
};
