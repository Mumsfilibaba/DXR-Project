#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHIValidation.h"

#define RHI_VALIDATION_ERROR(...) \
    do \
    { \
        LOG_ERROR("[RHI VALIDATION ERROR] " __VA_ARGS__); \
        if (CVarEnableValidationDebugBreak.GetValue()) \
        { \
            DEBUG_BREAK(); \
        } \
    } while (false)

static TAutoConsoleVariable<bool> CVarEnableValidationDebugBreak(
    "RHI.EnableValidationDebugBreak",
    "Enables debug-breaks when detecting errors in the custom RHI-validation layer",
    true);

static ERHIType SafeGetRHIType(FRHI* RealRHI)
{
    return RealRHI ? RealRHI->GetType() : ERHIType::Unknown;
}

FRHIValidation::FRHIValidation(FRHI* InRealRHI)
    : FRHI(SafeGetRHIType(InRealRHI))
    , RealRHI(InRealRHI)
{
}

FRHIValidation::~FRHIValidation()
{
}

bool FRHIValidation::Initialize()
{
    return RealRHI->Initialize();
}

void FRHIValidation::BeginFrame()
{
    RealRHI->BeginFrame();
}

void FRHIValidation::EndFrame()
{
    RealRHI->EndFrame();
}

FRHITexture* FRHIValidation::CreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    return RealRHI->CreateTexture(InTextureInfo, InInitialState, InInitialData);
}

FRHIBuffer* FRHIValidation::CreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData)
{
    return RealRHI->CreateBuffer(InBufferInfo, InInitialState, InInitialData);
}

FRHISamplerState* FRHIValidation::CreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo)
{
    return RealRHI->CreateSamplerState(InSamplerInfo);
}

FRHIViewport* FRHIValidation::CreateViewport(const FRHIViewportInfo& InViewportInfo)
{
    if (!InViewportInfo.WindowHandle)
    {
        RHI_VALIDATION_ERROR("Trying to create a viewport with an invalid WindowHandle");
        return nullptr;
    }

    return RealRHI->CreateViewport(InViewportInfo);
}

FRHIRayTracingScene* FRHIValidation::CreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo)
{
    return RealRHI->CreateRayTracingScene(InSceneInfo);
}

FRHIRayTracingGeometry* FRHIValidation::CreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo)
{
    return RealRHI->CreateRayTracingGeometry(InGeometryInfo);
}

FRHIShaderResourceView* FRHIValidation::CreateShaderResourceView(const FRHITextureSRVInfo& InInfo)
{
    if (!InInfo.Texture)
    {
        RHI_VALIDATION_ERROR("Texture cannot be nullptr when creating a ShaderResourceView");
        return nullptr;
    }

    const FRHITextureInfo& TextureInfo = InInfo.Texture->GetInfo();
    if (!TextureInfo.IsShaderResource())
    {
        RHI_VALIDATION_ERROR("Texture must have a the ETextureUsageFlags::ShaderResource to used with a ShaderResourceView");
        return nullptr;
    }

    if (InInfo.Format == EFormat::Unknown)
    {
        RHI_VALIDATION_ERROR("Format cannot be EFormat::Unknown when creating a ShaderResourceView");
        return nullptr;
    }

    if (IsTypelessFormat(InInfo.Format))
    {
        RHI_VALIDATION_ERROR("Format cannot be a typeless format when creating a ShaderResourceView");
        return nullptr;
    }

    const uint32 NumArraySlices = InInfo.FirstArraySlice + InInfo.NumSlices;
    if (NumArraySlices > TextureInfo.NumArraySlices)
    {
        RHI_VALIDATION_ERROR("Trying to create a ShaderResourceView with '%u' ArraySlices, but texture only contains '%u'", NumArraySlices, TextureInfo.NumArraySlices);
        return nullptr;
    }

    const uint32 NumMipLevels = InInfo.FirstMipLevel + InInfo.NumMips;
    if (NumMipLevels > TextureInfo.NumMipLevels)
    {
        RHI_VALIDATION_ERROR("Trying to create a ShaderResourceView with '%u' MipLevels, but texture only contains '%u'", NumMipLevels, TextureInfo.NumMipLevels);
        return nullptr;
    }

    return RealRHI->CreateShaderResourceView(InInfo);
}

FRHIShaderResourceView* FRHIValidation::CreateShaderResourceView(const FRHIBufferSRVInfo& InInfo)
{
    if (!InInfo.Buffer)
    {
        RHI_VALIDATION_ERROR("Buffer cannot be nullptr when creating a ShaderResourceView");
        return nullptr;
    }

    const FRHIBufferInfo& BufferInfo = InInfo.Buffer->GetInfo();
    if (!BufferInfo.IsShaderResource())
    {
        RHI_VALIDATION_ERROR("Buffer must have a the EBufferUsageFlags::ShaderResource to used with a ShaderResourceView");
        return nullptr;
    }

    return RealRHI->CreateShaderResourceView(InInfo);
}

FRHIUnorderedAccessView* FRHIValidation::CreateUnorderedAccessView(const FRHITextureUAVInfo& InInfo)
{
    if (!InInfo.Texture)
    {
        RHI_VALIDATION_ERROR("Texture cannot be nullptr when creating a UnorderedAccessView");
        return nullptr;
    }

    const FRHITextureInfo& TextureInfo = InInfo.Texture->GetInfo();
    if (!TextureInfo.IsUnorderedAccess())
    {
        RHI_VALIDATION_ERROR("Texture must have a the ETextureUsageFlags::UnorderedAccess to used with a UnorderedAccessView");
        return nullptr;
    }

    if (InInfo.Format == EFormat::Unknown)
    {
        RHI_VALIDATION_ERROR("Format cannot be EFormat::Unknown when creating a UnorderedAccessView");
        return nullptr;
    }

    if (IsTypelessFormat(InInfo.Format))
    {
        RHI_VALIDATION_ERROR("Format cannot be a typeless format when creating a UnorderedAccessView");
        return nullptr;
    }

    const uint32 NumArraySlices = InInfo.FirstArraySlice + InInfo.NumSlices;
    if (NumArraySlices > TextureInfo.NumArraySlices)
    {
        RHI_VALIDATION_ERROR("Trying to create a UnorderedAccessView with '%u' ArraySlices, but texture only contains '%u'", NumArraySlices, TextureInfo.NumArraySlices);
        return nullptr;
    }

    if (InInfo.MipLevel >= TextureInfo.NumMipLevels)
    {
        RHI_VALIDATION_ERROR("Trying to create a UnorderedAccessView for MipLevel '%u', but texture only contains '%u'", InInfo.MipLevel, TextureInfo.NumMipLevels);
        return nullptr;
    }

    return RealRHI->CreateUnorderedAccessView(InInfo);
}

FRHIUnorderedAccessView* FRHIValidation::CreateUnorderedAccessView(const FRHIBufferUAVInfo& InInfo)
{
    if (!InInfo.Buffer)
    {
        RHI_VALIDATION_ERROR("Buffer cannot be nullptr when creating a UnorderedAccessView");
        return nullptr;
    }

    const FRHIBufferInfo& BufferInfo = InInfo.Buffer->GetInfo();
    if (!BufferInfo.IsUnorderedAccess())
    {
        RHI_VALIDATION_ERROR("Buffer must have a the EBufferUsageFlags::UnorderedAccess to used with a UnorderedAccessView");
        return nullptr;
    }

    return RealRHI->CreateUnorderedAccessView(InInfo);
}

FRHIComputeShader* FRHIValidation::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateComputeShader(ShaderCode);
}

FRHIVertexShader* FRHIValidation::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateVertexShader(ShaderCode);
}

FRHIHullShader* FRHIValidation::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateHullShader(ShaderCode);
}

FRHIDomainShader* FRHIValidation::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateDomainShader(ShaderCode);
}

FRHIGeometryShader* FRHIValidation::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateGeometryShader(ShaderCode);
}

FRHIMeshShader* FRHIValidation::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateMeshShader(ShaderCode);
}

FRHIAmplificationShader* FRHIValidation::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateAmplificationShader(ShaderCode);
}

FRHIPixelShader* FRHIValidation::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreatePixelShader(ShaderCode);
}

FRHIRayGenShader* FRHIValidation::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateRayGenShader(ShaderCode);
}

FRHIRayAnyHitShader* FRHIValidation::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateRayAnyHitShader(ShaderCode);
}

FRHIRayClosestHitShader* FRHIValidation::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateRayClosestHitShader(ShaderCode);
}

FRHIRayMissShader* FRHIValidation::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return RealRHI->CreateRayMissShader(ShaderCode);
}

FRHIDepthStencilState* FRHIValidation::CreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
{
    return RealRHI->CreateDepthStencilState(InInitializer);
}

FRHIRasterizerState* FRHIValidation::CreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
{
    return RealRHI->CreateRasterizerState(InInitializer);
}

FRHIBlendState* FRHIValidation::CreateBlendState(const FRHIBlendStateInitializer& InInitializer)
{
    return RealRHI->CreateBlendState(InInitializer);
}

FRHIVertexLayout* FRHIValidation::CreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList)
{
    return RealRHI->CreateVertexLayout(InInitializerList);
}

FRHIGraphicsPipelineState* FRHIValidation::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer)
{
    return RealRHI->CreateGraphicsPipelineState(InInitializer);
}

FRHIComputePipelineState* FRHIValidation::CreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer)
{
    return RealRHI->CreateComputePipelineState(InInitializer);
}

FRHIRayTracingPipelineState* FRHIValidation::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer)
{
    return RealRHI->CreateRayTracingPipelineState(InInitializer);
}

FRHIQuery* FRHIValidation::CreateQuery(EQueryType InQueryType)
{
    return RealRHI->CreateQuery(InQueryType);
}

IRHICommandContext* FRHIValidation::ObtainCommandContext()
{
    IRHICommandContext* RealContext = RealRHI->ObtainCommandContext();
    if (!RealContext)
    {
        return nullptr;
    }

    if (FRHIValidationCommandContext** ExistingValidationContextContext = RealContextToValidationContextMap.Find(RealContext))
    {
        return *ExistingValidationContextContext;
    }
    else
    {
        FRHIValidationCommandContext* NewValitationContext = new FRHIValidationCommandContext(RealContext);
        return RealContextToValidationContextMap.Add(RealContext, NewValitationContext);
    }
}

bool FRHIValidation::GetQueryResult(FRHIQuery* Query, uint64& OutResult)
{
    if (!Query)
    {
        RHI_VALIDATION_ERROR("Cannot retrieve Query-result from a nullptr Query");
        return false;
    }

    return RealRHI->GetQueryResult(Query, OutResult);
}

void FRHIValidation::EnqueueResourceDeletion(FRHIResource* Resource)
{
    RealRHI->EnqueueResourceDeletion(Resource);
}

void* FRHIValidation::GetNativeAdapter()
{
    return RealRHI->GetNativeAdapter();
}

void* FRHIValidation::GetNativeDevice()
{
    return RealRHI->GetNativeDevice();
}

void* FRHIValidation::GetNativeDirectCommandQueue()
{
    return RealRHI->GetNativeDirectCommandQueue();
}

void* FRHIValidation::GetNativeComputeCommandQueue()
{
    return RealRHI->GetNativeComputeCommandQueue();
}

void* FRHIValidation::GetNativeCopyCommandQueue()
{
    return RealRHI->GetNativeCopyCommandQueue();
}

bool FRHIValidation::QueryUAVFormatSupport(EFormat Format) const
{
    return RealRHI->QueryUAVFormatSupport(Format);
}

bool FRHIValidation::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryStats) const
{
    return RealRHI->QueryVideoMemoryInfo(MemoryType, OutMemoryStats);
}

FString FRHIValidation::GetAdapterName() const
{
    return RealRHI->GetAdapterName();
}

FRHIValidationCommandContext::FRHIValidationCommandContext(IRHICommandContext* InRealContext)
    : IRHICommandContext()
    , RealContext(InRealContext)
    , ContextPhase(ECommandContextPhase::Finished)
{
}

FRHIValidationCommandContext::~FRHIValidationCommandContext()
{
}

void FRHIValidationCommandContext::RHIBeginFrame()
{
    RealContext->RHIBeginFrame();
}

void FRHIValidationCommandContext::RHIEndFrame()
{
    RealContext->RHIEndFrame();
}

void FRHIValidationCommandContext::RHIStartContext()
{
    if (ContextPhase >= ECommandContextPhase::Recording)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIStartContext when RHIFinishContext has not been called in-between");
    }

    RealContext->RHIStartContext();
    ContextPhase = ECommandContextPhase::Recording;
}

void FRHIValidationCommandContext::RHIFinishContext()
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIFinishContext when inside a renderpass");
    }
    else if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIFinishContext before a call to RHIStartContext");
    }

    RealContext->RHIFinishContext();
    ContextPhase = ECommandContextPhase::Finished;
}

void FRHIValidationCommandContext::RHIBeginQuery(FRHIQuery* Query)
{
    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginQuery when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType == EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("RHIBeginQuery does not support a Query of type Timestamp");
        return;
    }

    RealContext->RHIBeginQuery(Query);
}

void FRHIValidationCommandContext::RHIEndQuery(FRHIQuery* Query)
{
    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIEndQuery when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType == EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("RHIEndQuery does not support a Query of type Timestamp");
        return;
    }

    RealContext->RHIEndQuery(Query);
}

void FRHIValidationCommandContext::RHIQueryTimestamp(FRHIQuery* Query)
{
    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIQueryTimestamp when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType != EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("RHIQueryTimestamp only support a Query of type Timestamp");
        return;
    }

    RealContext->RHIQueryTimestamp(Query);
}

void FRHIValidationCommandContext::RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    if (!RenderTargetView.Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIClearRenderTargetView when Texture is nullptr");
        return;
    }

    RealContext->RHIClearRenderTargetView(RenderTargetView, ClearColor);
}

void FRHIValidationCommandContext::RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, const uint8 Stencil)
{
    if (!DepthStencilView.Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIClearDepthStencilView when Texture is nullptr");
        return;
    }

    RealContext->RHIClearDepthStencilView(DepthStencilView, Depth, Stencil);
}

void FRHIValidationCommandContext::RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    if (!UnorderedAccessView)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIClearUnorderedAccessViewFloat when UnorderedAccessView is nullptr");
        return;
    }

    RealContext->RHIClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
}

void FRHIValidationCommandContext::RHIBeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo)
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginRenderPass before calling RHIEndRenderPass");
    }
    else if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginRenderPass before calling RHIStartContext");
    }

    if (BeginRenderPassInfo.NumRenderTargets > RHI_MAX_RENDER_TARGETS)
    {
        RHI_VALIDATION_ERROR("Trying to bind to many render-targets in a render-pass. Max is '%u' but this call is trying to bind '%u'", RHI_MAX_RENDER_TARGETS, BeginRenderPassInfo.NumRenderTargets);
        return;
    }

    RealContext->RHIBeginRenderPass(BeginRenderPassInfo);
    ContextPhase = ECommandContextPhase::InsideRenderPass;
}

void FRHIValidationCommandContext::RHIEndRenderPass()
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIEndRenderPass before calling RHIBeginRenderPass");
    }

    RealContext->RHIEndRenderPass();
    ContextPhase = ECommandContextPhase::Recording;
}

void FRHIValidationCommandContext::RHISetViewport(const FViewportRegion& ViewportRegion)
{
    RealContext->RHISetViewport(ViewportRegion);
}

void FRHIValidationCommandContext::RHISetScissorRect(const FScissorRegion& ScissorRegion)
{
    RealContext->RHISetScissorRect(ScissorRegion);
}

void FRHIValidationCommandContext::RHISetBlendFactor(const FVector4& Color)
{
    RealContext->RHISetBlendFactor(Color);
}

void FRHIValidationCommandContext::RHISetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    RealContext->RHISetVertexBuffers(InVertexBuffers, BufferSlot);
}

void FRHIValidationCommandContext::RHISetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    RealContext->RHISetIndexBuffer(IndexBuffer, IndexFormat);
}

void FRHIValidationCommandContext::RHISetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    RealContext->RHISetGraphicsPipelineState(PipelineState);
}

void FRHIValidationCommandContext::RHISetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
    RealContext->RHISetComputePipelineState(PipelineState);
}

void FRHIValidationCommandContext::RHISet32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISet32BitShaderConstants when Shader is nullptr");
        return;
    }

    RealContext->RHISet32BitShaderConstants(Shader, Shader32BitConstants, Num32BitConstants);
}

void FRHIValidationCommandContext::RHISetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISetShaderResourceView when Shader is nullptr");
        return;
    }

    RealContext->RHISetShaderResourceView(Shader, ShaderResourceView, ParameterIndex);
}

void FRHIValidationCommandContext::RHISetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISetShaderResourceViews when Shader is nullptr");
        return;
    }

    RealContext->RHISetShaderResourceViews(Shader, InShaderResourceViews, ParameterIndex);
}

void FRHIValidationCommandContext::RHISetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISetUnorderedAccessView when Shader is nullptr");
        return;
    }

    RealContext->RHISetUnorderedAccessView(Shader, UnorderedAccessView, ParameterIndex);
}

void FRHIValidationCommandContext::RHISetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISetUnorderedAccessViews when Shader is nullptr");
        return;
    }

    RealContext->RHISetUnorderedAccessViews(Shader, InUnorderedAccessViews, ParameterIndex);
}

void FRHIValidationCommandContext::RHISetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISetConstantBuffer when Shader is nullptr");
        return;
    }

    RealContext->RHISetConstantBuffer(Shader, ConstantBuffer, ParameterIndex);
}

void FRHIValidationCommandContext::RHISetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISetConstantBuffers when Shader is nullptr");
        return;
    }

    RealContext->RHISetConstantBuffers(Shader, InConstantBuffers, ParameterIndex);
}

void FRHIValidationCommandContext::RHISetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISetSamplerState when Shader is nullptr");
        return;
    }

    RealContext->RHISetSamplerState(Shader, SamplerState, ParameterIndex);
}

void FRHIValidationCommandContext::RHISetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISetSamplerStates when Shader is nullptr");
        return;
    }

    RealContext->RHISetSamplerStates(Shader, InSamplerStates, ParameterIndex);
}

void FRHIValidationCommandContext::RHIUpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIUpdateBuffer when Dst is nullptr");
        return;
    }

    if (!SrcData)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIUpdateBuffer when SrcData is nullptr");
        return;
    }

    RealContext->RHIUpdateBuffer(Dst, BufferRegion, SrcData);
}

void FRHIValidationCommandContext::RHIUpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIUpdateTexture2D when Dst is nullptr");
        return;
    }

    if (!SrcData)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIUpdateTexture2D when SrcData is nullptr");
        return;
    }

    RealContext->RHIUpdateTexture2D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch);
}

void FRHIValidationCommandContext::RHIResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIResolveTexture when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIResolveTexture when Src is nullptr");
        return;
    }

    RealContext->RHIResolveTexture(Dst, Src);
}

void FRHIValidationCommandContext::RHICopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHICopyBuffer when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHICopyBuffer when Src is nullptr");
        return;
    }

    RealContext->RHICopyBuffer(Dst, Src, CopyDesc);
}

void FRHIValidationCommandContext::RHICopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHICopyTexture when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHICopyTexture when Src is nullptr");
        return;
    }

    RealContext->RHICopyTexture(Dst, Src);
}

void FRHIValidationCommandContext::RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHICopyTextureRegion when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHICopyTextureRegion when Src is nullptr");
        return;
    }

    RealContext->RHICopyTextureRegion(Dst, Src, CopyDesc);
}

void FRHIValidationCommandContext::RHIDiscardContents(FRHITexture* Texture)
{
    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIDiscardContents when Texture is nullptr");
        return;
    }

    RealContext->RHIDiscardContents(Texture);
}

void FRHIValidationCommandContext::RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo)
{
    if (!RayTracingScene)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBuildRayTracingScene when RayTracingScene is nullptr");
        return;
    }

    RealContext->RHIBuildRayTracingScene(RayTracingScene, BuildInfo);
}

void FRHIValidationCommandContext::RHIBuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo)
{
    if (!RayTracingGeometry)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBuildRayTracingGeometry when RayTracingGeometry is nullptr");
        return;
    }

    RealContext->RHIBuildRayTracingGeometry(RayTracingGeometry, BuildInfo);
}

void FRHIValidationCommandContext::RHISetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources)
{
    if (!RayTracingScene)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISetRayTracingBindings when RayTracingScene is nullptr");
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHISetRayTracingBindings when PipelineState is nullptr");
        return;
    }

    RealContext->RHISetRayTracingBindings(RayTracingScene, PipelineState, GlobalResource, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
}

void FRHIValidationCommandContext::RHITransitionTexture(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHITransitionTexture when Texture is nullptr");
        return;
    }

    RealContext->RHITransitionTexture(Texture, TextureTransition);
}

void FRHIValidationCommandContext::RHITransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHITransitionBuffer when Buffer is nullptr");
        return;
    }

    RealContext->RHITransitionBuffer(Buffer, BeforeState, AfterState);
}

void FRHIValidationCommandContext::RHIUnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIUnorderedAccessTextureBarrier when Texture is nullptr");
        return;
    }

    RealContext->RHIUnorderedAccessTextureBarrier(Texture);
}

void FRHIValidationCommandContext::RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIUnorderedAccessBufferBarrier when Buffer is nullptr");
        return;
    }

    RealContext->RHIUnorderedAccessBufferBarrier(Buffer);
}

void FRHIValidationCommandContext::RHIDraw(uint32 VertexCount, uint32 StartVertexLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIDraw before entering a render-pass");
        return;
    }

    RealContext->RHIDraw(VertexCount, StartVertexLocation);
}

void FRHIValidationCommandContext::RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIDrawIndexed before entering a render-pass");
        return;
    }

    RealContext->RHIDrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void FRHIValidationCommandContext::RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIDrawInstanced before entering a render-pass");
        return;
    }

    RealContext->RHIDrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FRHIValidationCommandContext::RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIDrawIndexedInstanced before entering a render-pass");
        return;
    }

    RealContext->RHIDrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FRHIValidationCommandContext::RHIDispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    RealContext->RHIDispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
}

void FRHIValidationCommandContext::RHIDispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
{
    RealContext->RHIDispatchRays(Scene, PipelineState, Width, Height, Depth);
}

void FRHIValidationCommandContext::RHIPresentViewport(FRHIViewport* Viewport, bool bVerticalSync)
{
    if (!Viewport)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIPresentViewport when Viewport is nullptr");
        return;
    }

    RealContext->RHIPresentViewport(Viewport, bVerticalSync);
}

void FRHIValidationCommandContext::RHIResizeViewport(FRHIViewport* Viewport, uint32 Width, uint32 Height)
{
    if (!Viewport)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIResizeViewport when Viewport is nullptr");
        return;
    }

    RealContext->RHIResizeViewport(Viewport, Width, Height);
}

void FRHIValidationCommandContext::RHIClearState()
{
    RealContext->RHIClearState();
}

void FRHIValidationCommandContext::RHIFlush()
{
    RealContext->RHIFlush();
}

void FRHIValidationCommandContext::RHIInsertMarker(const FStringView& Message)
{
    RealContext->RHIInsertMarker(Message);
}

void FRHIValidationCommandContext::RHIBeginExternalCapture()
{
    RealContext->RHIBeginExternalCapture();
}

void FRHIValidationCommandContext::RHIEndExternalCapture()
{
    RealContext->RHIEndExternalCapture();
}

void* FRHIValidationCommandContext::RHIGetNativeCommandList()
{
    return RealContext->RHIGetNativeCommandList();
}
