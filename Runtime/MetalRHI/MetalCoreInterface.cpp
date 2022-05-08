#include "MetalCoreInterface.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalCoreInterface

CMetalCoreInterface::CMetalCoreInterface()
	: CRHICoreInterface(ERHIInstanceType::Metal)
	, CommandContext()
{ }

CMetalCoreInterface::~CMetalCoreInterface()
{
	SafeDelete(CommandContext);
	SafeDelete(Device);
}

CMetalCoreInterface* CMetalCoreInterface::CreateMetalCoreInterface()
{
	return dbg_new CMetalCoreInterface();
}

bool CMetalCoreInterface::Initialize(bool bEnableDebug)
{
	UNREFERENCED_VARIABLE(bEnableDebug);
	
	Device = CMetalDeviceContext::CreateContext(this);
	if (!Device)
	{
		METAL_ERROR("Failed to create DeviceContext");
		return false;
	}
	
	METAL_INFO("Created DeviceContext");
	
    CommandContext = CMetalCommandContext::CreateMetalContext(Device);
    if (!CommandContext)
    {
        METAL_ERROR("Failed to create CommandContext");
        return false;
    }
    
    return true;
}

CRHITexture2D* CMetalCoreInterface::RHICreateTexture2D(const CRHITexture2DInitializer& Initializer)
{
    return dbg_new TMetalTexture<CMetalTexture2D>(Initializer);
}

CRHITexture2DArray* CMetalCoreInterface::RHICreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer)
{
    return dbg_new TMetalTexture<CMetalTexture2DArray>(Initializer);
}

CRHITextureCube* CMetalCoreInterface::RHICreateTextureCube(const CRHITextureCubeInitializer& Initializer)
{
    return dbg_new TMetalTexture<CMetalTextureCube>(Initializer);
}

CRHITextureCubeArray* CMetalCoreInterface::RHICreateTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer)
{
    return dbg_new TMetalTexture<CMetalTextureCubeArray>(Initializer);
}

CRHITexture3D* CMetalCoreInterface::RHICreateTexture3D(const CRHITexture3DInitializer& Initializer)
{
    return dbg_new TMetalTexture<CMetalTexture3D>(Initializer);
}

CRHISamplerState* CMetalCoreInterface::RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer)
{
    return dbg_new CMetalSamplerState();
}

CRHIVertexBuffer* CMetalCoreInterface::RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
{
    return dbg_new TMetalBuffer<CMetalVertexBuffer>(Initializer);
}

CRHIIndexBuffer* CMetalCoreInterface::RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
{
    return dbg_new TMetalBuffer<CMetalIndexBuffer>(Initializer);
}

CRHIGenericBuffer* CMetalCoreInterface::RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
{
    return dbg_new TMetalBuffer<CMetalGenericBuffer>(Initializer);
}

CRHIConstantBuffer* CMetalCoreInterface::RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
{
    return dbg_new TMetalBuffer<CMetalConstantBuffer>(Initializer);
}

CRHIRayTracingScene* CMetalCoreInterface::RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer)
{
    return dbg_new CMetalRayTracingScene(Initializer);
}

CRHIRayTracingGeometry* CMetalCoreInterface::RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer)
{
    return dbg_new CMetalRayTracingGeometry(Initializer);
}

CRHIShaderResourceView* CMetalCoreInterface::RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer)
{
    return dbg_new CMetalShaderResourceView(Initializer.Texture);
}

CRHIShaderResourceView* CMetalCoreInterface::RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer)
{
    return dbg_new CMetalShaderResourceView(Initializer.Buffer);
}

CRHIUnorderedAccessView* CMetalCoreInterface::RHICreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer)
{
    return dbg_new CMetalUnorderedAccessView(Initializer.Texture);
}

CRHIUnorderedAccessView* CMetalCoreInterface::RHICreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer)
{
    return dbg_new CMetalUnorderedAccessView(Initializer.Buffer);
}

CRHIComputeShader* CMetalCoreInterface::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TMetalShader<CMetalComputeShader>();
}

CRHIVertexShader* CMetalCoreInterface::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TMetalShader<CRHIVertexShader>();
}

CRHIHullShader* CMetalCoreInterface::RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIDomainShader* CMetalCoreInterface::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIGeometryShader* CMetalCoreInterface::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIMeshShader* CMetalCoreInterface::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIAmplificationShader* CMetalCoreInterface::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

CRHIPixelShader* CMetalCoreInterface::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TMetalShader<CRHIPixelShader>();
}

CRHIRayGenShader* CMetalCoreInterface::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TMetalShader<CRHIRayGenShader>();
}

CRHIRayAnyHitShader* CMetalCoreInterface::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TMetalShader<CRHIRayAnyHitShader>();
}

CRHIRayClosestHitShader* CMetalCoreInterface::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TMetalShader<CRHIRayClosestHitShader>();
}

CRHIRayMissShader* CMetalCoreInterface::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new TMetalShader<CRHIRayMissShader>();
}

CRHIDepthStencilState* CMetalCoreInterface::RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer)
{
    return dbg_new CMetalDepthStencilState();
}

CRHIRasterizerState* CMetalCoreInterface::RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer)
{
    return dbg_new CMetalRasterizerState();
}

CRHIBlendState* CMetalCoreInterface::RHICreateBlendState(const CRHIBlendStateInitializer& Initializer)
{
    return dbg_new CMetalBlendState();
}

CRHIVertexInputLayout* CMetalCoreInterface::RHICreateVertexInputLayout(const CRHIVertexInputLayoutInitializer& Initializer)
{
    return dbg_new CMetalInputLayoutState();
}

CRHIGraphicsPipelineState* CMetalCoreInterface::RHICreateGraphicsPipelineState(const CRHIGraphicsPipelineStateInitializer& Initializer)
{
    return dbg_new CMetalGraphicsPipelineState();
}

CRHIComputePipelineState* CMetalCoreInterface::RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer)
{
    return dbg_new CMetalComputePipelineState();
}

CRHIRayTracingPipelineState* CMetalCoreInterface::RHICreateRayTracingPipelineState(const CRHIRayTracingPipelineStateInitializer& Initializer)
{
    return dbg_new CMetalRayTracingPipelineState();
}

CRHITimestampQuery* CMetalCoreInterface::RHICreateTimestampQuery()
{
    return dbg_new CMetalTimestampQuery();
}

CRHIViewport* CMetalCoreInterface::RHICreateViewport(const CRHIViewportInitializer& Initializer)
{
    return dbg_new CMetalViewport(Initializer);
}

IRHICommandContext* CMetalCoreInterface::RHIGetDefaultCommandContext()
{
    return CommandContext;
}

String CMetalCoreInterface::GetAdapterName() const
{
    return String();
}

void CMetalCoreInterface::RHIQueryRayTracingSupport(SRayTracingSupport& OutSupport) const
{
    OutSupport = SRayTracingSupport();
}

void CMetalCoreInterface::RHIQueryShadingRateSupport(SShadingRateSupport& OutSupport) const
{
    OutSupport = SShadingRateSupport();
}

bool CMetalCoreInterface::RHIQueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
