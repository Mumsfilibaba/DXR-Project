#pragma once
#include "Core/Containers/Map.h"
#include "RHI/RHI.h"

class FRHIValidationCommandContext;

class RHI_API FRHIValidation : public FRHI
{
public:
    FRHIValidation(FRHI* InRealRHI);
    ~FRHIValidation();

    virtual bool Initialize();

    virtual void BeginFrame();
    virtual void EndFrame();

    virtual FRHITexture* CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer* CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData) override final;
    virtual FRHISamplerState* CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc) override final;
    virtual FRHISwapChain* CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc) override final;
    virtual FRHISceneAccelerationStructure* CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc) override final;
    virtual FRHIGeometryAccelerationStructure* CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc) override final;
    virtual FRHIShaderResourceView* CreateShaderResourceView(const FRHIShaderResourceViewDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView* CreateUnorderedAccessView(const FRHIUnorderedAccessViewDesc& InDesc) override final;
    virtual FRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIHullShader* CreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIPixelShader* CreatePixelShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayAnyHitShader* CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayMissShader* CreateRayMissShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIDepthStencilState* CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) override final;
    virtual FRHIRasterizerState* CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) override final;
    virtual FRHIBlendState* CreateBlendState(const FRHIBlendStateDesc& InDesc) override final;
    virtual FRHIInputLayout* CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements) override final;
    virtual FRHIGraphicsPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) override final;
    virtual FRHIComputePipelineState* CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) override final;
    virtual FRHIRayTracingPipelineState* CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final;
    virtual FRHIQuery* CreateQuery(EQueryType InQueryType) override final;
    virtual FRHIFence* CreateFence() override final;
    virtual IRHICommandContext* ObtainCommandContext() override final;

    virtual bool GetQueryResult(FRHIQuery* Query, uint64& OutResult) override final;

    virtual void EnqueueResourceDeletion(FRHIResource* Resource) override final;

    virtual void* GetNativeAdapter() override final;
    virtual void* GetNativeDevice() override final;
    virtual void* GetNativeDirectCommandQueue() override final;
    virtual void* GetNativeComputeCommandQueue() override final;
    virtual void* GetNativeCopyCommandQueue() override final;

    virtual bool QueryUAVFormatSupport(EFormat Format) const override final;
    virtual bool QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const override final;

    virtual FString GetAdapterName() const override final;

private:
    FRHI* RealRHI;
    TMap<IRHICommandContext*, FRHIValidationCommandContext*> RealContextToValidationContextMap;
};

class RHI_API FRHIValidationCommandContext : public IRHICommandContext
{
public:
    FRHIValidationCommandContext(IRHICommandContext* InRealContext);
    ~FRHIValidationCommandContext();

    virtual void BeginFrame() override final;
    virtual void EndFrame() override final;

    virtual void StartContext() override final;
    virtual void FinishContext() override final;

    virtual void BeginQuery(FRHIQuery* Query) override final;
    virtual void EndQuery(FRHIQuery* Query) override final;
    virtual void QueryTimestamp(FRHIQuery* Query) override final;

    virtual void ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor) override final;
    virtual void ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, const uint8 Stencil) override final;
    virtual void ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor) override final;

    virtual void BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc) override final;
    virtual void EndRenderPass() override final;

    virtual void SetViewport(const FViewportRegion& ViewportRegion) override final;
    virtual void SetScissorRect(const FScissorRegion& ScissorRegion) override final;
    virtual void SetBlendFactor(const FVector4& Color) override final;
    virtual void SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot) override final;
    virtual void SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat) override final;
    virtual void SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState) override final;
    virtual void SetComputePipelineState(class FRHIComputePipelineState* PipelineState) override final;
    virtual void SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants) override final;
    virtual void SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex) override final;
    virtual void SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex) override final;
    virtual void SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex) override final;
    virtual void SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex) override final;
    virtual void SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex) override final;
    virtual void SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex) override final;
    virtual void SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex) override final;
    virtual void SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex) override final;
    
    virtual void UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData) override final;
    virtual void UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) override final;
    virtual void ResolveTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc) override final;
    virtual void CopyTexture(FRHITexture* Dst, FRHITexture* Src) override final;
    virtual void CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& InCopyDesc) override final;
    virtual void CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) override final;
    virtual void WriteFence(FRHIFence* Fence) override final;
    virtual void DiscardContents(class FRHITexture* Texture) override final;

    virtual void BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc) override final;
    virtual void BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc) override final;
    virtual void SetRayTracingBindings(FRHISceneAccelerationStructure* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources) override final;
    
    virtual void TransitionTexture(FRHITexture* Texture, const FRHITextureTransition& TextureTransition) override final;
    virtual void TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState) override final;
    virtual void RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState) override final;
    virtual void RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState) override final;
    virtual void EnableResourceStateTracking(FRHITexture* Texture, EResourceAccess InitialState) override final;
    virtual void DisableResourceStateTracking(FRHITexture* Texture, EResourceAccess TargetState) override final;
    virtual void EnableResourceStateTracking(FRHIBuffer* Buffer, EResourceAccess InitialState) override final;
    virtual void DisableResourceStateTracking(FRHIBuffer* Buffer, EResourceAccess TargetState) override final;
    virtual void UnorderedAccessTextureBarrier(FRHITexture* Texture) override final;
    virtual void UnorderedAccessBufferBarrier(FRHIBuffer* Buffer) override final;

    virtual void Draw(uint32 VertexCount, uint32 StartVertexLocation) override final;
    virtual void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation) override final;
    virtual void DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) override final;
    virtual void Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ) override final;
    virtual void DispatchRays(FRHISceneAccelerationStructure* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth) override final;

    virtual void PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync) override final;
    virtual void ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height) override final;

    virtual void ClearState() override final;
    virtual void Flush() override final;

    virtual void InsertMarker(const FStringView& Message) override final;

    virtual void BeginExternalCapture() override final;
    virtual void EndExternalCapture() override final;

    virtual void* GetNativeCommandList() override final;

private:
    IRHICommandContext*  RealContext;
    ECommandContextPhase ContextPhase;
};
