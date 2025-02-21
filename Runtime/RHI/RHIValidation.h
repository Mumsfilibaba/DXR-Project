#pragma once
#include "Core/Containers/Map.h"
#include "RHI/RHI.h"

class FRHIValidationCommandContext;

class RHI_API FRHIValidation : public FRHI
{
public:
    FRHIValidation(FRHI* InRealRHI);
    ~FRHIValidation();

public:
    virtual bool Initialize();

    virtual void RHIBeginFrame();
    virtual void RHIEndFrame();

    virtual FRHITexture* RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo) override final;
    virtual FRHIViewport* RHICreateViewport(const FRHIViewportInfo& InViewportInfo) override final;
    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo) override final;
    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo) override final;
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInfo& InInfo) override final;
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInfo& InInfo) override final;
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInfo& InInfo) override final;
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInfo& InInfo) override final;
    virtual FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer) override final;
    virtual FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer) override final;
    virtual FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& InInitializer) override final;
    virtual FRHIVertexLayout* RHICreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList) override final;
    virtual FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer) override final;
    virtual FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer) override final;
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer) override final;
    virtual FRHIQuery* RHICreateQuery(EQueryType InQueryType) override final;
    virtual IRHICommandContext* RHIObtainCommandContext() override final;

    virtual bool RHIGetQueryResult(FRHIQuery* Query, uint64& OutResult) override final;

    virtual void RHIEnqueueResourceDeletion(FRHIResource* Resource) override final;

    virtual void* RHIGetAdapter() override final;
    virtual void* RHIGetDevice() override final;

    virtual void* RHIGetDirectCommandQueue() override final;
    virtual void* RHIGetComputeCommandQueue() override final;
    virtual void* RHIGetCopyCommandQueue() override final;

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final;
    virtual bool RHIQueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryStats) const override final;

    virtual FString RHIGetAdapterName() const override final;

private:
    FRHI* RealRHI;
    TMap<IRHICommandContext*, FRHIValidationCommandContext*> RealContextToValidationContextMap;
};

class RHI_API FRHIValidationCommandContext : public IRHICommandContext
{
public:
    FRHIValidationCommandContext(IRHICommandContext* InRealContext);
    ~FRHIValidationCommandContext();

public:
    virtual void RHIBeginFrame() override final;
    virtual void RHIEndFrame() override final;

    virtual void RHIStartContext() override final;
    virtual void RHIFinishContext() override final;

    virtual void RHIBeginQuery(FRHIQuery* Query) override final;
    virtual void RHIEndQuery(FRHIQuery* Query) override final;
    virtual void RHIQueryTimestamp(FRHIQuery* Query) override final;

    virtual void RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) override final;
    virtual void RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, const uint8 Stencil) override final;
    virtual void RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) override final;

    virtual void RHIBeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo) override final;
    virtual void RHIEndRenderPass() override final;

    virtual void RHISetViewport(const FViewportRegion& ViewportRegion) override final;
    virtual void RHISetScissorRect(const FScissorRegion& ScissorRegion) override final;
    virtual void RHISetBlendFactor(const FVector4& Color) override final;
    virtual void RHISetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) override final;
    virtual void RHISetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) override final;
    virtual void RHISetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    virtual void RHISetComputePipelineState(class FRHIComputePipelineState* PipelineState) override final;
    virtual void RHISet32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants) override final;
    virtual void RHISetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) override final;
    virtual void RHISetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex) override final;
    virtual void RHISetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) override final;
    virtual void RHISetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex) override final;
    virtual void RHISetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex) override final;
    virtual void RHISetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex) override final;
    virtual void RHISetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex) override final;
    virtual void RHISetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex) override final;
    
    virtual void RHIUpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) override final;
    virtual void RHIUpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) override final;
    virtual void RHIResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void RHICopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc) override final;
    virtual void RHICopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc) override final;
    virtual void RHIDiscardContents(class FRHITexture* Texture) override final;

    virtual void RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo) override final;
    virtual void RHIBuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo) override final;
    virtual void RHISetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources) override final;
    
    virtual void RHITransitionTexture(FRHITexture* Texture, const FRHITextureTransition& TextureTransition) override final;
    virtual void RHITransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) override final;
    virtual void RHIUnorderedAccessTextureBarrier(FRHITexture* Texture) override final;
    virtual void RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer) override final;

    virtual void RHIDraw(uint32 VertexCount, uint32 StartVertexLocation) override final;
    virtual void RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final;
    virtual void RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void RHIDispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;
    virtual void RHIDispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) override final;

    virtual void RHIPresentViewport(FRHIViewport* Viewport, bool bVerticalSync) override final;
    virtual void RHIResizeViewport(FRHIViewport* Viewport, uint32 Width, uint32 Height) override final;

    virtual void RHIClearState() override final;
    virtual void RHIFlush() override final;

    virtual void RHIInsertMarker(const FStringView& Message) override final;

    virtual void RHIBeginExternalCapture() override final;
    virtual void RHIEndExternalCapture() override final;

    virtual void* RHIGetNativeCommandList() override final;

private:
    IRHICommandContext*  RealContext;
    ECommandContextPhase ContextPhase;
};
