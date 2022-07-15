#include "MetalCoreInterface.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalCoreInterface

FMetalCoreInterface::FMetalCoreInterface()
	: FRHICoreInterface(ERHIInstanceType::Metal)
	, CommandContext()
{ }

FMetalCoreInterface::~FMetalCoreInterface()
{
	SafeDelete(CommandContext);
	SafeDelete(DeviceContext);
}

FMetalCoreInterface* FMetalCoreInterface::CreateMetalCoreInterface()
{
	return dbg_new FMetalCoreInterface();
}

bool FMetalCoreInterface::Initialize(bool bEnableDebug)
{
	UNREFERENCED_VARIABLE(bEnableDebug);
	
	DeviceContext = FMetalDeviceContext::CreateContext(this);
	if (!DeviceContext)
	{
		METAL_ERROR("Failed to create DeviceContext");
		return false;
	}
	
	METAL_INFO("Created DeviceContext");
	
    CommandContext = FMetalCommandContext::CreateMetalContext(GetDeviceContext());
    if (!CommandContext)
    {
        METAL_ERROR("Failed to create CommandContext");
        return false;
    }
    
    return true;
}

FRHITexture2D* FMetalCoreInterface::RHICreateTexture2D(const FRHITexture2DInitializer& Initializer)
{
    return CreateTexture<FMetalTexture2D>(Initializer);
}

FRHITexture2DArray* FMetalCoreInterface::RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer)
{
    return CreateTexture<FMetalTexture2DArray>(Initializer);
}

FRHITextureCube* FMetalCoreInterface::RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer)
{
    return CreateTexture<FMetalTextureCube>(Initializer);
}

FRHITextureCubeArray* FMetalCoreInterface::RHICreateTextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer)
{
    return CreateTexture<FMetalTextureCubeArray>(Initializer);
}

FRHITexture3D* FMetalCoreInterface::RHICreateTexture3D(const FRHITexture3DInitializer& Initializer)
{
    return CreateTexture<FMetalTexture3D>(Initializer);
}

template<typename MetalTextureType, typename InitializerType>
MetalTextureType* FMetalCoreInterface::CreateTexture(const InitializerType& Initializer)
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
    
    if constexpr (TIsSame<MetalTextureType, FMetalTexture3D>::Value)
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
    constexpr bool bIsTexture2D = TIsSame<MetalTextureType, FMetalTexture2D>::Value;
    
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

FRHISamplerState* FMetalCoreInterface::RHICreateSamplerState(const FRHISamplerStateInitializer& Initializer)
{
    return dbg_new FMetalSamplerState();
}

FRHIVertexBuffer* FMetalCoreInterface::RHICreateVertexBuffer(const FRHIVertexBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalVertexBuffer>(Initializer);
}

FRHIIndexBuffer* FMetalCoreInterface::RHICreateIndexBuffer(const FRHIIndexBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalIndexBuffer>(Initializer);
}

FRHIGenericBuffer* FMetalCoreInterface::RHICreateGenericBuffer(const FRHIGenericBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalGenericBuffer>(Initializer);
}

FRHIConstantBuffer* FMetalCoreInterface::RHICreateConstantBuffer(const FRHIConstantBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalConstantBuffer>(Initializer);
}

template<typename MetalBufferType, typename InitializerType>
MetalBufferType* FMetalCoreInterface::CreateBuffer(const InitializerType& Initializer)
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

FRHIRayTracingScene* FMetalCoreInterface::RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
{
    return dbg_new FMetalRayTracingScene(GetDeviceContext(), Initializer);
}

FRHIRayTracingGeometry* FMetalCoreInterface::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
{
    return dbg_new FMetalRayTracingGeometry(Initializer);
}

FRHIShaderResourceView* FMetalCoreInterface::RHICreateShaderResourceView(const FRHITextureSRVInitializer& Initializer)
{
    return dbg_new FMetalShaderResourceView(GetDeviceContext(), Initializer.Texture);
}

FRHIShaderResourceView* FMetalCoreInterface::RHICreateShaderResourceView(const FRHIBufferSRVInitializer& Initializer)
{
    return dbg_new FMetalShaderResourceView(GetDeviceContext(), Initializer.Buffer);
}

FRHIUnorderedAccessView* FMetalCoreInterface::RHICreateUnorderedAccessView(const FRHITextureUAVInitializer& Initializer)
{
    return dbg_new FMetalUnorderedAccessView(GetDeviceContext(), Initializer.Texture);
}

FRHIUnorderedAccessView* FMetalCoreInterface::RHICreateUnorderedAccessView(const FRHIBufferUAVInitializer& Initializer)
{
    return dbg_new FMetalUnorderedAccessView(GetDeviceContext(), Initializer.Buffer);
}

FRHIComputeShader* FMetalCoreInterface::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalComputeShader(GetDeviceContext(), ShaderCode);
}

FRHIVertexShader* FMetalCoreInterface::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalVertexShader(GetDeviceContext(), ShaderCode);
}

FRHIHullShader* FMetalCoreInterface::RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIDomainShader* FMetalCoreInterface::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIGeometryShader* FMetalCoreInterface::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIMeshShader* FMetalCoreInterface::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIAmplificationShader* FMetalCoreInterface::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIPixelShader* FMetalCoreInterface::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalPixelShader(GetDeviceContext(), ShaderCode);
}

FRHIRayGenShader* FMetalCoreInterface::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalRayGenShader(GetDeviceContext(), ShaderCode);
}

FRHIRayAnyHitShader* FMetalCoreInterface::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalRayAnyHitShader(GetDeviceContext(), ShaderCode);
}

FRHIRayClosestHitShader* FMetalCoreInterface::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalRayClosestHitShader(GetDeviceContext(), ShaderCode);
}

FRHIRayMissShader* FMetalCoreInterface::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new CMetalRayMissShader(GetDeviceContext(), ShaderCode);
}

FRHIDepthStencilState* FMetalCoreInterface::RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& Initializer)
{
    return dbg_new FMetalDepthStencilState(GetDeviceContext(), Initializer);
}

FRHIRasterizerState* FMetalCoreInterface::RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer)
{
    return dbg_new FMetalRasterizerState(GetDeviceContext(), Initializer);
}

FRHIBlendState* FMetalCoreInterface::RHICreateBlendState(const FRHIBlendStateInitializer& Initializer)
{
    return dbg_new FMetalBlendState();
}

FRHIVertexInputLayout* FMetalCoreInterface::RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer)
{
    return dbg_new FMetalInputLayoutState(GetDeviceContext(), Initializer);
}

FRHIGraphicsPipelineState* FMetalCoreInterface::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    return dbg_new FMetalGraphicsPipelineState(GetDeviceContext(), Initializer);
}

FRHIComputePipelineState* FMetalCoreInterface::RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer)
{
    return dbg_new FMetalComputePipelineState();
}

FRHIRayTracingPipelineState* FMetalCoreInterface::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer)
{
    return dbg_new FMetalRayTracingPipelineState();
}

FRHITimestampQuery* FMetalCoreInterface::RHICreateTimestampQuery()
{
    return dbg_new FMetalTimestampQuery();
}

FRHIViewport* FMetalCoreInterface::RHICreateViewport(const FRHIViewportInitializer& Initializer)
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
    
    return dbg_new FMetalViewport(GetDeviceContext(), NewInitializer);
}

IRHICommandContext* FMetalCoreInterface::RHIGetDefaultCommandContext()
{
    return CommandContext;
}

FString FMetalCoreInterface::GetAdapterDescription() const
{
    return FString();
}

void FMetalCoreInterface::RHIQueryRayTracingSupport(FRayTracingSupport& OutSupport) const
{
    OutSupport = FRayTracingSupport();
}

void FMetalCoreInterface::RHIQueryShadingRateSupport(FShadingRateSupport& OutSupport) const
{
    OutSupport = FShadingRateSupport();
}

bool FMetalCoreInterface::RHIQueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

#pragma clang diagnostic pop
