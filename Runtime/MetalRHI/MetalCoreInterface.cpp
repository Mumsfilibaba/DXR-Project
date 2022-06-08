#include "MetalCoreInterface.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalCoreInterface

CMetalCoreInterface::CMetalCoreInterface()
	: CRHICoreInterface(ERHIInstanceType::Metal)
	, CommandContext()
{ }

CMetalCoreInterface::~CMetalCoreInterface()
{
	SafeDelete(CommandContext);
	SafeDelete(DeviceContext);
}

CMetalCoreInterface* CMetalCoreInterface::CreateMetalCoreInterface()
{
	return dbg_new CMetalCoreInterface();
}

bool CMetalCoreInterface::Initialize(bool bEnableDebug)
{
	UNREFERENCED_VARIABLE(bEnableDebug);
	
	DeviceContext = CMetalDeviceContext::CreateContext(this);
	if (!DeviceContext)
	{
		METAL_ERROR("Failed to create DeviceContext");
		return false;
	}
	
	METAL_INFO("Created DeviceContext");
	
    CommandContext = CMetalCommandContext::CreateMetalContext(GetDeviceContext());
    if (!CommandContext)
    {
        METAL_ERROR("Failed to create CommandContext");
        return false;
    }
    
    return true;
}

CRHITexture2D* CMetalCoreInterface::RHICreateTexture2D(const CRHITexture2DInitializer& Initializer)
{
    return CreateTexture<CMetalTexture2D>(Initializer);
}

CRHITexture2DArray* CMetalCoreInterface::RHICreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer)
{
    return CreateTexture<CMetalTexture2DArray>(Initializer);
}

CRHITextureCube* CMetalCoreInterface::RHICreateTextureCube(const CRHITextureCubeInitializer& Initializer)
{
    return CreateTexture<CMetalTextureCube>(Initializer);
}

CRHITextureCubeArray* CMetalCoreInterface::RHICreateTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer)
{
    return CreateTexture<CMetalTextureCubeArray>(Initializer);
}

CRHITexture3D* CMetalCoreInterface::RHICreateTexture3D(const CRHITexture3DInitializer& Initializer)
{
    return CreateTexture<CMetalTexture3D>(Initializer);
}

template<typename MetalTextureType, typename InitializerType>
MetalTextureType* CMetalCoreInterface::CreateTexture(const InitializerType& Initializer)
{
    SCOPED_AUTORELEASE_POOL();
    
    TSharedRef<MetalTextureType> NewTexture = dbg_new MetalTextureType(GetDeviceContext(), Initializer);

    MTLTextureDescriptor* TextureDescriptor = [[MTLTextureDescriptor new] autorelease];
    TextureDescriptor.textureType               = GetMTLTextureType(NewTexture.Get());
    TextureDescriptor.pixelFormat               = ConvertFormat(Initializer.Format);
    TextureDescriptor.usage                     = ConvertTextureFlags(Initializer.UsageFlags);
    TextureDescriptor.allowGPUOptimizedContents = NO;
    TextureDescriptor.swizzle                   = MTLTextureSwizzleChannelsMake(MTLTextureSwizzleRed, MTLTextureSwizzleGreen, MTLTextureSwizzleBlue, MTLTextureSwizzleAlpha);
    
    const CIntVector3 Extent = NewTexture->GetExtent();
    TextureDescriptor.width  = Extent.x;
    TextureDescriptor.height = Extent.y;
    
    if constexpr (TIsSame<MetalTextureType, CMetalTexture3D>::Value)
    {
        TextureDescriptor.depth       = Extent.z;
        TextureDescriptor.arrayLength = 1;
    }
    else
    {
        TextureDescriptor.depth       = 1;
        TextureDescriptor.arrayLength = Extent.z;
    }
    
    TextureDescriptor.mipmapLevelCount   = Initializer.NumMips;
    TextureDescriptor.sampleCount        = NewTexture->GetNumSamples();
    TextureDescriptor.resourceOptions    = MTLResourceCPUCacheModeWriteCombined;
    TextureDescriptor.cpuCacheMode       = MTLCPUCacheModeWriteCombined;
    TextureDescriptor.storageMode        = MTLStorageModePrivate;
    TextureDescriptor.hazardTrackingMode = MTLHazardTrackingModeDefault;
    
    id<MTLDevice>  Device = GetDeviceContext()->GetMTLDevice();
    id<MTLTexture> NewMTLTexture = [Device newTextureWithDescriptor:TextureDescriptor];
    if (!NewMTLTexture)
    {
        return nullptr;
    }
    
    NewTexture->SetMTLTexture(NewMTLTexture);
    
    // TODO: Fix upload for other resources than Texture2D
    constexpr bool bIsTexture2D = TIsSame<MetalTextureType, CMetalTexture2D>::Value;
    
    if constexpr (bIsTexture2D)
    {
        CRHITextureDataInitializer* InitialData = Initializer.InitialData;
        if (InitialData)
        {
            MTLRegion Region;
            Region.origin = { 0, 0, 0 };
            Region.size   = { NSUInteger(Extent.x), NSUInteger(Extent.y), 1 };
            
            @autoreleasepool
            {
                id<MTLBuffer> StagingBuffer = [Device newBufferWithLength:InitialData->Size options:MTLResourceOptionCPUCacheModeDefault];
                CMemory::Memcpy(StagingBuffer.contents, InitialData->TextureData, InitialData->Size);
                
                id<MTLCommandQueue>       CommandQueue  = GetDeviceContext()->GetMTLCommandQueue();
                id<MTLCommandBuffer>      CommandBuffer = [CommandQueue commandBuffer];
                id<MTLBlitCommandEncoder> CopyEncoder   = [CommandBuffer blitCommandEncoder];
                
                const NSUInteger BytesPerRow = NSUInteger(Extent.x) * GetByteStrideFromFormat(Initializer.Format);
                
                [CopyEncoder copyFromBuffer:StagingBuffer
                               sourceOffset:0
                          sourceBytesPerRow:BytesPerRow
                        sourceBytesPerImage:0
                                 sourceSize:Region.size
                                  toTexture:NewMTLTexture
                           destinationSlice:0
                           destinationLevel:0
                          destinationOrigin:Region.origin];
                
                [CopyEncoder endEncoding];

                // TODO: we do not want to wait here
                [CommandBuffer commit];
                [CommandBuffer waitUntilCompleted];
            
                [StagingBuffer release];
            }
        }
    }
    
    return NewTexture.ReleaseOwnership();
}

CRHISamplerState* CMetalCoreInterface::RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer)
{
    return dbg_new CMetalSamplerState();
}

CRHIVertexBuffer* CMetalCoreInterface::RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
{
    return CreateBuffer<CMetalVertexBuffer>(Initializer);
}

CRHIIndexBuffer* CMetalCoreInterface::RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
{
    return CreateBuffer<CMetalIndexBuffer>(Initializer);
}

CRHIGenericBuffer* CMetalCoreInterface::RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
{
    return CreateBuffer<CMetalGenericBuffer>(Initializer);
}

CRHIConstantBuffer* CMetalCoreInterface::RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
{
    return CreateBuffer<CMetalConstantBuffer>(Initializer);
}

template<typename MetalBufferType, typename InitializerType>
MetalBufferType* CMetalCoreInterface::CreateBuffer(const InitializerType& Initializer)
{
    TSharedRef<MetalBufferType> NewBuffer = dbg_new MetalBufferType(GetDeviceContext(), Initializer);
    
    MTLResourceOptions ResourceOptions = MTLResourceHazardTrackingModeDefault;
    if (Initializer.IsDynamic())
    {
        ResourceOptions |= MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache;
    }
    else
    {
        ResourceOptions |= MTLResourceStorageModePrivate | MTLResourceCPUCacheModeWriteCombined;
    }
    
    id<MTLDevice> Device       = GetDeviceContext()->GetMTLDevice();
    id<MTLBuffer> NewMTLBuffer = [Device newBufferWithLength:NewBuffer->GetSize() options:ResourceOptions];
    if (!NewMTLBuffer)
    {
        return nullptr;
    }
    
    NewBuffer->SetMTLBuffer(NewMTLBuffer);
    
    CRHIBufferDataInitializer* InitialData = Initializer.InitialData;
    if (InitialData)
    {
        if (Initializer.IsDynamic())
        {
            CMemory::Memcpy(NewMTLBuffer.contents, InitialData->BufferData, InitialData->Size);
        }
        else
        {
            @autoreleasepool
            {
                id<MTLBuffer> StagingBuffer = [Device newBufferWithLength:InitialData->Size options:MTLResourceOptionCPUCacheModeDefault];
                CMemory::Memcpy(StagingBuffer.contents, InitialData->BufferData, InitialData->Size);
                
                id<MTLCommandQueue>       CommandQueue  = GetDeviceContext()->GetMTLCommandQueue();
                id<MTLCommandBuffer>      CommandBuffer = [CommandQueue commandBuffer];
                id<MTLBlitCommandEncoder> CopyEncoder   = [CommandBuffer blitCommandEncoder];
                
                [CopyEncoder copyFromBuffer:StagingBuffer
                               sourceOffset:0
                                   toBuffer:NewMTLBuffer
                          destinationOffset:0
                                       size:InitialData->Size];
                
                [CopyEncoder endEncoding];

                // TODO: we do not want to wait here
                [CommandBuffer commit];
                [CommandBuffer waitUntilCompleted];
            
                [StagingBuffer release];
            }
        }
    }
    
    return NewBuffer.ReleaseOwnership();
}

CRHIRayTracingScene* CMetalCoreInterface::RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer)
{
    return dbg_new CMetalRayTracingScene(GetDeviceContext(), Initializer);
}

CRHIRayTracingGeometry* CMetalCoreInterface::RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer)
{
    return dbg_new CMetalRayTracingGeometry(Initializer);
}

CRHIShaderResourceView* CMetalCoreInterface::RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer)
{
    return dbg_new CMetalShaderResourceView(GetDeviceContext(), Initializer.Texture);
}

CRHIShaderResourceView* CMetalCoreInterface::RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer)
{
    return dbg_new CMetalShaderResourceView(GetDeviceContext(), Initializer.Buffer);
}

CRHIUnorderedAccessView* CMetalCoreInterface::RHICreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer)
{
    return dbg_new CMetalUnorderedAccessView(GetDeviceContext(), Initializer.Texture);
}

CRHIUnorderedAccessView* CMetalCoreInterface::RHICreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer)
{
    return dbg_new CMetalUnorderedAccessView(GetDeviceContext(), Initializer.Buffer);
}

CRHIComputeShader* CMetalCoreInterface::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalComputeShader(GetDeviceContext(), ShaderCode);
}

CRHIVertexShader* CMetalCoreInterface::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalVertexShader(GetDeviceContext(), ShaderCode);
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
    return dbg_new CMetalPixelShader(GetDeviceContext(), ShaderCode);
}

CRHIRayGenShader* CMetalCoreInterface::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalRayGenShader(GetDeviceContext(), ShaderCode);
}

CRHIRayAnyHitShader* CMetalCoreInterface::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalRayAnyHitShader(GetDeviceContext(), ShaderCode);
}

CRHIRayClosestHitShader* CMetalCoreInterface::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalRayClosestHitShader(GetDeviceContext(), ShaderCode);
}

CRHIRayMissShader* CMetalCoreInterface::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalRayMissShader(GetDeviceContext(), ShaderCode);
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
    CCocoaWindow* Window = (CCocoaWindow*)Initializer.WindowHandle;
    
    __block NSRect Frame;
    __block NSRect ContentRect;
    MakeMainThreadCall(^
    {
        Frame       = Window.frame;
        ContentRect = [Window contentRectForFrameRect:Window.frame];
    }, true);
    
    CRHIViewportInitializer NewInitializer(Initializer);
    NewInitializer.Width  = ContentRect.size.width;
    NewInitializer.Height = ContentRect.size.height;
    
    return dbg_new CMetalViewport(GetDeviceContext(), NewInitializer);
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

#pragma clang diagnostic pop
