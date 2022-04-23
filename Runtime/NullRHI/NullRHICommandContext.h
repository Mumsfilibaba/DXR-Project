#pragma once
#include "RHI/IRHICommandContext.h"

#include "Core/Containers/SharedRef.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHICommandContext

class CNullRHICommandContext final : public IRHICommandContext
{
private:

    CNullRHICommandContext()  = default;
    ~CNullRHICommandContext() = default;

public:

    static FORCEINLINE CNullRHICommandContext* Make() { return dbg_new CNullRHICommandContext(); }

    virtual void StartContext()  override final { }
    virtual void FinishContext() override final { }

    virtual void BeginTimeStamp(CRHITimeQuery* Query, uint32 Index) override final { }
    virtual void EndTimeStamp(CRHITimeQuery* Query, uint32 Index)   override final { }

    virtual void ClearRenderTargetView(const CRHIRenderTargetView& RenderTargetView, const TStaticArray<float, 4>& ClearColor)         override final { }
    virtual void ClearDepthStencilView(const CRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)                 override final { }
    virtual void ClearUnorderedAccessTextureFloat(CRHITexture* Texture, const TStaticArray<float, 4>& ClearColor)                      override final { }
    virtual void ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor) override final { }
    virtual void ClearUnorderedAccessTextureUint(CRHITexture* Texture, const TStaticArray<uint32, 4>& ClearColor)                      override final { }
    virtual void ClearUnorderedAccessViewUint(CRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<uint32, 4>& ClearColor) override final { }

    virtual void BeginRenderPass(const CRHIRenderPassInitializer& Initializer) override final { }
    virtual void EndRenderPass()                                               override final { }

    virtual void SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y) override final { }
    virtual void SetScissorRect(float Width, float Height, float x, float y)                              override final { }

    virtual void SetBlendFactor(const TStaticArray<float, 4>& Color) override final { }

    virtual void SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 NumVertexBuffers, uint32 StartBufferSlot) override final { }
    virtual void SetIndexBuffer(CRHIIndexBuffer* IndexBuffer)                                                              override final { }

    virtual void SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) override final { }

    virtual void SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState) override final { }
    virtual void SetComputePipelineState(class CRHIComputePipelineState* PipelineState)   override final { }

    virtual void Set32BitShaderConstants(CRHIShader* Shader, const SSetShaderConstantsInfo& ShaderConstantsInfo) override final { }

    virtual void SetShaderResourceTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex)                                                                 override final { }
    virtual void SetShaderResourceTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex)                               override final { }
    virtual void SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)                                              override final { }
    virtual void SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceViews, uint32 NumShaderResourceViews, uint32 StartParameterIndex) override final { }

    virtual void SetUnorderedAccessTexture(CRHIShader* Shader, CRHITexture* Texture, uint32 ParameterIndex)                                                                    override final { }
    virtual void SetUnorderedAccessTextures(CRHIShader* Shader, CRHITexture* const* Textures, uint32 NumTextures, uint32 StartParameterIndex)                                  override final { }
    virtual void SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)                                               override final { }
    virtual void SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 StartParameterIndex) override final { }

    virtual void SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)                                          override final { }
    virtual void SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 StartParameterIndex) override final { }

    virtual void SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex)                                        override final { }
    virtual void SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 StartParameterIndex) override final { }

    virtual void UpdateBuffer(CRHIBuffer* Dst, const void* SrcData, uint64 Offset, uint64 Size)                         override final { }
    virtual void UpdateTexture2D(CRHITexture2D* Dst, const void* SrcData, uint32 Width, uint32 Height, uint32 MipLevel) override final { }
    virtual void UpdateTexture2DArray( CRHITexture2DArray* Dst
                                     , const void* SrcData
                                     , uint32 Width
                                     , uint32 Height
                                     , uint32 MipLevel
                                     , uint32 ArrayIndex
                                     , uint32 NumArraySlices) override final { }

    virtual void ResolveTexture(CRHITexture* Dst, CRHITexture* Src) override final { }

    virtual void CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src)                                                                          override final { }
    virtual void CopyBufferRegion(const SCopyBufferRegionInfo& CopyDesc)                                                               override final { }
    virtual void CopyTexture(CRHITexture* Dst, CRHITexture* Src)                                                                       override final { }
    virtual void CopyTexture2DRegion(CRHITexture2D* Dst, CRHITexture2D* Src, const SCopyTexture2DRegionInfo& CopyInfo)                 override final { }
    virtual void CopyTexture2DArrayRegion(CRHITexture2DArray* Dst, CRHITexture2DArray* Src, const SCopyTexture2DArrayRegion& CopyInfo) override final { }

    virtual void DestroyResource(CRHIResource* Resource) override final { }
    virtual void DiscardContents(CRHIResource* Resource) override final { }

    virtual void BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, const SBuildRayTracingGeometryInfo& BuildDesc) override final { }
    virtual void BuildRayTracingScene(CRHIRayTracingScene* Scene, const SBuildRayTracingSceneInfo& BuildDesc)             override final { }

    virtual void GenerateMips(CRHITexture* Texture) override final { }

    virtual void TransitionTexture(CRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState) override final { }
    virtual void TransitionBuffer(CRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)    override final { }

    virtual void UnorderedAccessTextureBarrier(CRHITexture* Texture) override final { }
    virtual void UnorderedAccessBufferBarrier(CRHIBuffer* Buffer)    override final { }

    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final { }
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)                                                                         override final { }
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)                                 override final { }
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final { }

    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final { }

    virtual void DispatchRays(CRHIRayTracingScene* Scene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) override final { }

    virtual void PresentViewport(CRHIViewport* Viewport, bool bVerticalSync) override final { }

    virtual void ClearState() override final { }
    virtual void Flush()      override final { }

    virtual void InsertMarker(const String& Message) override final { }

    virtual void BeginExternalCapture() override final { }
    virtual void EndExternalCapture()   override final { }

    virtual void* GetRHIHandle() const override final { }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif