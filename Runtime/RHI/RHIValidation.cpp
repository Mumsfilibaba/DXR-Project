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

FRHISwapChain* FRHIValidation::CreateSwapChain(const FRHISwapChainInfo& InSwapChainInfo)
{
    if (!InSwapChainInfo.WindowHandle)
    {
        RHI_VALIDATION_ERROR("Trying to create a viewport with an invalid WindowHandle");
        return nullptr;
    }

    return RealRHI->CreateSwapChain(InSwapChainInfo);
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

void FRHIValidationCommandContext::BeginFrame()
{
    RealContext->BeginFrame();
}

void FRHIValidationCommandContext::EndFrame()
{
    RealContext->EndFrame();
}

void FRHIValidationCommandContext::StartContext()
{
    if (ContextPhase >= ECommandContextPhase::Recording)
    {
        RHI_VALIDATION_ERROR("Invalid to call StartContext when FinishContext has not been called in-between");
    }

    RealContext->StartContext();
    ContextPhase = ECommandContextPhase::Recording;
}

void FRHIValidationCommandContext::FinishContext()
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call FinishContext when inside a renderpass");
    }
    else if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("Invalid to call FinishContext before a call to StartContext");
    }

    RealContext->FinishContext();
    ContextPhase = ECommandContextPhase::Finished;
}

void FRHIValidationCommandContext::BeginQuery(FRHIQuery* Query)
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

    RealContext->BeginQuery(Query);
}

void FRHIValidationCommandContext::EndQuery(FRHIQuery* Query)
{
    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call EndQuery when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType == EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("EndQuery does not support a Query of type Timestamp");
        return;
    }

    RealContext->EndQuery(Query);
}

void FRHIValidationCommandContext::QueryTimestamp(FRHIQuery* Query)
{
    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call QueryTimestamp when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType != EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("QueryTimestamp only support a Query of type Timestamp");
        return;
    }

    RealContext->QueryTimestamp(Query);
}

void FRHIValidationCommandContext::ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    if (!RenderTargetView.Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearRenderTargetView when Texture is nullptr");
        return;
    }

    RealContext->ClearRenderTargetView(RenderTargetView, ClearColor);
}

void FRHIValidationCommandContext::ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, const uint8 Stencil)
{
    if (!DepthStencilView.Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearDepthStencilView when Texture is nullptr");
        return;
    }

    RealContext->ClearDepthStencilView(DepthStencilView, Depth, Stencil);
}

void FRHIValidationCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    if (!UnorderedAccessView)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearUnorderedAccessViewFloat when UnorderedAccessView is nullptr");
        return;
    }

    RealContext->ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
}

void FRHIValidationCommandContext::BeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo)
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginRenderPass before calling EndRenderPass");
    }
    else if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginRenderPass before calling StartContext");
    }

    if (BeginRenderPassInfo.NumRenderTargets > RHI_MAX_RENDER_TARGETS)
    {
        RHI_VALIDATION_ERROR("Trying to bind to many render-targets in a render-pass. Max is '%u' but this call is trying to bind '%u'", RHI_MAX_RENDER_TARGETS, BeginRenderPassInfo.NumRenderTargets);
        return;
    }

    RealContext->BeginRenderPass(BeginRenderPassInfo);
    ContextPhase = ECommandContextPhase::InsideRenderPass;
}

void FRHIValidationCommandContext::EndRenderPass()
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call EndRenderPass before calling RHIBeginRenderPass");
    }

    RealContext->EndRenderPass();
    ContextPhase = ECommandContextPhase::Recording;
}

void FRHIValidationCommandContext::SetViewport(const FViewportRegion& ViewportRegion)
{
    RealContext->SetViewport(ViewportRegion);
}

void FRHIValidationCommandContext::SetScissorRect(const FScissorRegion& ScissorRegion)
{
    RealContext->SetScissorRect(ScissorRegion);
}

void FRHIValidationCommandContext::SetBlendFactor(const FVector4& Color)
{
    RealContext->SetBlendFactor(Color);
}

void FRHIValidationCommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    RealContext->SetVertexBuffers(InVertexBuffers, BufferSlot);
}

void FRHIValidationCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    RealContext->SetIndexBuffer(IndexBuffer, IndexFormat);
}

void FRHIValidationCommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    RealContext->SetGraphicsPipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
    RealContext->SetComputePipelineState(PipelineState);
}

void FRHIValidationCommandContext::Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call Set32BitShaderConstants when Shader is nullptr");
        return;
    }

    RealContext->Set32BitShaderConstants(Shader, Shader32BitConstants, Num32BitConstants);
}

void FRHIValidationCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderResourceView when Shader is nullptr");
        return;
    }

    RealContext->SetShaderResourceView(Shader, ShaderResourceView, ParameterIndex);
}

void FRHIValidationCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderResourceViews when Shader is nullptr");
        return;
    }

    RealContext->SetShaderResourceViews(Shader, InShaderResourceViews, ParameterIndex);
}

void FRHIValidationCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetUnorderedAccessView when Shader is nullptr");
        return;
    }

    RealContext->SetUnorderedAccessView(Shader, UnorderedAccessView, ParameterIndex);
}

void FRHIValidationCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetUnorderedAccessViews when Shader is nullptr");
        return;
    }

    RealContext->SetUnorderedAccessViews(Shader, InUnorderedAccessViews, ParameterIndex);
}

void FRHIValidationCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetConstantBuffer when Shader is nullptr");
        return;
    }

    RealContext->SetConstantBuffer(Shader, ConstantBuffer, ParameterIndex);
}

void FRHIValidationCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetConstantBuffers when Shader is nullptr");
        return;
    }

    RealContext->SetConstantBuffers(Shader, InConstantBuffers, ParameterIndex);
}

void FRHIValidationCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetSamplerState when Shader is nullptr");
        return;
    }

    RealContext->SetSamplerState(Shader, SamplerState, ParameterIndex);
}

void FRHIValidationCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetSamplerStates when Shader is nullptr");
        return;
    }

    RealContext->SetSamplerStates(Shader, InSamplerStates, ParameterIndex);
}

void FRHIValidationCommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateBuffer when Dst is nullptr");
        return;
    }

    if (!SrcData)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateBuffer when SrcData is nullptr");
        return;
    }

    RealContext->UpdateBuffer(Dst, BufferRegion, SrcData);
}

void FRHIValidationCommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateTexture2D when Dst is nullptr");
        return;
    }

    if (!SrcData)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateTexture2D when SrcData is nullptr");
        return;
    }

    RealContext->UpdateTexture2D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch);
}

void FRHIValidationCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call ResolveTexture when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call ResolveTexture when Src is nullptr");
        return;
    }

    RealContext->ResolveTexture(Dst, Src);
}

void FRHIValidationCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyBuffer when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyBuffer when Src is nullptr");
        return;
    }

    RealContext->CopyBuffer(Dst, Src, CopyDesc);
}

void FRHIValidationCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTexture when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTexture when Src is nullptr");
        return;
    }

    RealContext->CopyTexture(Dst, Src);
}

void FRHIValidationCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc)
{
    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureRegion when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureRegion when Src is nullptr");
        return;
    }

    RealContext->CopyTextureRegion(Dst, Src, CopyDesc);
}

void FRHIValidationCommandContext::DiscardContents(FRHITexture* Texture)
{
    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call DiscardContents when Texture is nullptr");
        return;
    }

    RealContext->DiscardContents(Texture);
}

void FRHIValidationCommandContext::BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo)
{
    if (!RayTracingScene)
    {
        RHI_VALIDATION_ERROR("Invalid to call BuildRayTracingScene when RayTracingScene is nullptr");
        return;
    }

    RealContext->BuildRayTracingScene(RayTracingScene, BuildInfo);
}

void FRHIValidationCommandContext::BuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo)
{
    if (!RayTracingGeometry)
    {
        RHI_VALIDATION_ERROR("Invalid to call BuildRayTracingGeometry when RayTracingGeometry is nullptr");
        return;
    }

    RealContext->BuildRayTracingGeometry(RayTracingGeometry, BuildInfo);
}

void FRHIValidationCommandContext::SetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources)
{
    if (!RayTracingScene)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetRayTracingBindings when RayTracingScene is nullptr");
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetRayTracingBindings when PipelineState is nullptr");
        return;
    }

    RealContext->SetRayTracingBindings(RayTracingScene, PipelineState, GlobalResource, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
}

void FRHIValidationCommandContext::TransitionTexture(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call TransitionTexture when Texture is nullptr");
        return;
    }

    RealContext->TransitionTexture(Texture, TextureTransition);
}

void FRHIValidationCommandContext::TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call TransitionBuffer when Buffer is nullptr");
        return;
    }

    RealContext->TransitionBuffer(Buffer, BeforeState, AfterState);
}

void FRHIValidationCommandContext::UnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessTextureBarrier when Texture is nullptr");
        return;
    }

    RealContext->UnorderedAccessTextureBarrier(Texture);
}

void FRHIValidationCommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessBufferBarrier when Buffer is nullptr");
        return;
    }

    RealContext->UnorderedAccessBufferBarrier(Buffer);
}

void FRHIValidationCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call Draw before entering a render-pass");
        return;
    }

    RealContext->Draw(VertexCount, StartVertexLocation);
}

void FRHIValidationCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DrawIndexed before entering a render-pass");
        return;
    }

    RealContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void FRHIValidationCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DrawInstanced before entering a render-pass");
        return;
    }

    RealContext->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FRHIValidationCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DrawIndexedInstanced before entering a render-pass");
        return;
    }

    RealContext->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FRHIValidationCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    RealContext->Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
}

void FRHIValidationCommandContext::DispatchRays(FRHIRayTracingScene* Scene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
{
    RealContext->DispatchRays(Scene, PipelineState, Width, Height, Depth);
}

void FRHIValidationCommandContext::PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync)
{
    if (!SwapChain)
    {
        RHI_VALIDATION_ERROR("Invalid to call PresentSwapChain when SwapChain is nullptr");
        return;
    }

    RealContext->PresentSwapChain(SwapChain, bVerticalSync);
}

void FRHIValidationCommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height)
{
    if (!SwapChain)
    {
        RHI_VALIDATION_ERROR("Invalid to call ResizeSwapChain when SwapChain is nullptr");
        return;
    }

    RealContext->ResizeSwapChain(SwapChain, Width, Height);
}

void FRHIValidationCommandContext::ClearState()
{
    RealContext->ClearState();
}

void FRHIValidationCommandContext::Flush()
{
    RealContext->Flush();
}

void FRHIValidationCommandContext::InsertMarker(const FStringView& Message)
{
    RealContext->InsertMarker(Message);
}

void FRHIValidationCommandContext::BeginExternalCapture()
{
    RealContext->BeginExternalCapture();
}

void FRHIValidationCommandContext::EndExternalCapture()
{
    RealContext->EndExternalCapture();
}

void* FRHIValidationCommandContext::GetNativeCommandList()
{
    return RealContext->GetNativeCommandList();
}
