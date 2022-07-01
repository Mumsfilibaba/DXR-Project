#include "MetalCoreInterface.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalCoreInterface

CMetalCoreInterface::CMetalCoreInterface()
	: FRHICoreInterface(ERHIInstanceType::Metal)
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

FRHITexture2D* CMetalCoreInterface::RHICreateTexture2D(const FRHITexture2DInitializer& Initializer)
{
    return CreateTexture<CMetalTexture2D>(Initializer);
}

FRHITexture2DArray* CMetalCoreInterface::RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer)
{
    return CreateTexture<CMetalTexture2DArray>(Initializer);
}

FRHITextureCube* CMetalCoreInterface::RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer)
{
    return CreateTexture<CMetalTextureCube>(Initializer);
}

FRHITextureCubeArray* CMetalCoreInterface::RHICreateTextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer)
{
    return CreateTexture<CMetalTextureCubeArray>(Initializer);
}

FRHITexture3D* CMetalCoreInterface::RHICreateTexture3D(const FRHITexture3DInitializer& Initializer)
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
    
    const FIntVector3 Extent = NewTexture->GetExtent();
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
        FRHITextureDataInitializer* InitialData = Initializer.InitialData;
        if (InitialData)
        {
            MTLRegion Region;
            Region.origin = { 0, 0, 0 };
            Region.size   = { NSUInteger(Extent.x), NSUInteger(Extent.y), 1 };
            
            @autoreleasepool
            {
                id<MTLBuffer> StagingBuffer = [Device newBufferWithLength:InitialData->Size options:MTLResourceOptionCPUCacheModeDefault];
                FMemory::Memcpy(StagingBuffer.contents, InitialData->TextureData, InitialData->Size);
                
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

                id<MTLBuffer> StagingBuffer = [Device newBufferWithLength:InitialData->Size options:MTLResourceOptionCPUCacheModeDefault];
                FMemory::Memcpy(StagingBuffer.contents, InitialData->TextureData, InitialData->Size);
                
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

FRHISamplerState* CMetalCoreInterface::RHICreateSamplerState(const FRHISamplerStateInitializer& Initializer)
{
    return dbg_new CMetalSamplerState();
}

FRHIVertexBuffer* CMetalCoreInterface::RHICreateVertexBuffer(const FRHIVertexBufferInitializer& Initializer)
{
    return CreateBuffer<CMetalVertexBuffer>(Initializer);
}

FRHIIndexBuffer* CMetalCoreInterface::RHICreateIndexBuffer(const FRHIIndexBufferInitializer& Initializer)
{
    return CreateBuffer<CMetalIndexBuffer>(Initializer);
}

FRHIGenericBuffer* CMetalCoreInterface::RHICreateGenericBuffer(const FRHIGenericBufferInitializer& Initializer)
{
    return CreateBuffer<CMetalGenericBuffer>(Initializer);
}

FRHIConstantBuffer* CMetalCoreInterface::RHICreateConstantBuffer(const FRHIConstantBufferInitializer& Initializer)
{
    return CreateBuffer<CMetalConstantBuffer>(Initializer);
}

template<typename MetalBufferType, typename InitializerType>
MetalBufferType* CMetalCoreInterface::CreateBuffer(const InitializerType& Initializer)
{
    SCOPED_AUTORELEASE_POOL();
    
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
    
    FRHIBufferDataInitializer* InitialData = Initializer.InitialData;
    if (InitialData)
    {
        if (Initializer.IsDynamic())
        {
            FMemory::Memcpy(NewMTLBuffer.contents, InitialData->BufferData, InitialData->Size);
        }
        else
        {
            @autoreleasepool
            {
                id<MTLBuffer> StagingBuffer = [Device newBufferWithLength:InitialData->Size options:MTLResourceOptionCPUCacheModeDefault];
                FMemory::Memcpy(StagingBuffer.contents, InitialData->BufferData, InitialData->Size);
                
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

FRHIRayTracingScene* CMetalCoreInterface::RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
{
    return dbg_new CMetalRayTracingScene(GetDeviceContext(), Initializer);
}

FRHIRayTracingGeometry* CMetalCoreInterface::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
{
    return dbg_new CMetalRayTracingGeometry(Initializer);
}

FRHIShaderResourceView* CMetalCoreInterface::RHICreateShaderResourceView(const FRHITextureSRVInitializer& Initializer)
{
    return dbg_new CMetalShaderResourceView(GetDeviceContext(), Initializer.Texture);
}

FRHIShaderResourceView* CMetalCoreInterface::RHICreateShaderResourceView(const FRHIBufferSRVInitializer& Initializer)
{
    return dbg_new CMetalShaderResourceView(GetDeviceContext(), Initializer.Buffer);
}

FRHIUnorderedAccessView* CMetalCoreInterface::RHICreateUnorderedAccessView(const FRHITextureUAVInitializer& Initializer)
{
    return dbg_new CMetalUnorderedAccessView(GetDeviceContext(), Initializer.Texture);
}

FRHIUnorderedAccessView* CMetalCoreInterface::RHICreateUnorderedAccessView(const FRHIBufferUAVInitializer& Initializer)
{
    return dbg_new CMetalUnorderedAccessView(GetDeviceContext(), Initializer.Buffer);
}

FRHIComputeShader* CMetalCoreInterface::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalComputeShader(GetDeviceContext(), ShaderCode);
}

FRHIVertexShader* CMetalCoreInterface::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalVertexShader(GetDeviceContext(), ShaderCode);
}

FRHIHullShader* CMetalCoreInterface::RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIDomainShader* CMetalCoreInterface::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIGeometryShader* CMetalCoreInterface::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIMeshShader* CMetalCoreInterface::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIAmplificationShader* CMetalCoreInterface::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIPixelShader* CMetalCoreInterface::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalPixelShader(GetDeviceContext(), ShaderCode);
}

FRHIRayGenShader* CMetalCoreInterface::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalRayGenShader(GetDeviceContext(), ShaderCode);
}

FRHIRayAnyHitShader* CMetalCoreInterface::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalRayAnyHitShader(GetDeviceContext(), ShaderCode);
}

FRHIRayClosestHitShader* CMetalCoreInterface::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalRayClosestHitShader(GetDeviceContext(), ShaderCode);
}

FRHIRayMissShader* CMetalCoreInterface::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalRayMissShader(GetDeviceContext(), ShaderCode);
}

FRHIDepthStencilState* CMetalCoreInterface::RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& Initializer)
{
    return dbg_new CMetalDepthStencilState(GetDeviceContext(), Initializer);
}

FRHIRasterizerState* CMetalCoreInterface::RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer)
{
    return dbg_new CMetalRasterizerState(GetDeviceContext(), Initializer);
}

FRHIBlendState* CMetalCoreInterface::RHICreateBlendState(const FRHIBlendStateInitializer& Initializer)
{
    return dbg_new CMetalBlendState();
}

FRHIVertexInputLayout* CMetalCoreInterface::RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer)
{
    return dbg_new CMetalInputLayoutState(GetDeviceContext(), Initializer);
}

FRHIGraphicsPipelineState* CMetalCoreInterface::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    return dbg_new CMetalGraphicsPipelineState(GetDeviceContext(), Initializer);
}

FRHIComputePipelineState* CMetalCoreInterface::RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer)
{
    return dbg_new CMetalComputePipelineState();
}

FRHIRayTracingPipelineState* CMetalCoreInterface::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer)
{
    return dbg_new CMetalRayTracingPipelineState();
}

FRHITimestampQuery* CMetalCoreInterface::RHICreateTimestampQuery()
{
    return dbg_new CMetalTimestampQuery();
}

FRHIViewport* CMetalCoreInterface::RHICreateViewport(const FRHIViewportInitializer& Initializer)
{
    CCocoaWindow* Window = (CCocoaWindow*)Initializer.WindowHandle;
    
    __block NSRect Frame;
    __block NSRect ContentRect;
    MakeMainThreadCall(^
    {
        Frame       = Window.frame;
        ContentRect = [Window contentRectForFrameRect:Window.frame];
    }, true);
    
    FRHIViewportInitializer NewInitializer(Initializer);
    NewInitializer.Width  = ContentRect.size.width;
    NewInitializer.Height = ContentRect.size.height;
    
    return dbg_new CMetalViewport(GetDeviceContext(), NewInitializer);
}

IRHICommandContext* CMetalCoreInterface::RHIGetDefaultCommandContext()
{
    return CommandContext;
}

String CMetalCoreInterface::GetAdapterDescription() const
{
    return String();
}

void CMetalCoreInterface::RHIQueryRayTracingSupport(FRayTracingSupport& OutSupport) const
{
    OutSupport = FRayTracingSupport();
}

void CMetalCoreInterface::RHIQueryShadingRateSupport(FShadingRateSupport& OutSupport) const
{
    OutSupport = FShadingRateSupport();
}

bool CMetalCoreInterface::RHIQueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

#pragma clang diagnostic pop
