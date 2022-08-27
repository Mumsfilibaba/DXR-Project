#include "MetalInterface.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalInterface

FMetalInterface::FMetalInterface()
	: FRHIInterface(ERHIInstanceType::Metal)
	, CommandContext()
{ }

FMetalInterface::~FMetalInterface()
{
	SAFE_DELETE(CommandContext);
	SAFE_DELETE(DeviceContext);
}

FMetalInterface* FMetalInterface::CreateMetalCoreInterface()
{
	return dbg_new FMetalInterface();
}

bool FMetalInterface::Initialize(bool bEnableDebug)
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

FRHITexture2D* FMetalInterface::RHICreateTexture2D(const FRHITexture2DInitializer& Initializer)
{
    return CreateTexture<FMetalTexture2D>(Initializer);
}

FRHITexture2DArray* FMetalInterface::RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer)
{
    return CreateTexture<FMetalTexture2DArray>(Initializer);
}

FRHITextureCube* FMetalInterface::RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer)
{
    return CreateTexture<FMetalTextureCube>(Initializer);
}

FRHITextureCubeArray* FMetalInterface::RHICreateTextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer)
{
    return CreateTexture<FMetalTextureCubeArray>(Initializer);
}

FRHITexture3D* FMetalInterface::RHICreateTexture3D(const FRHITexture3DInitializer& Initializer)
{
    return CreateTexture<FMetalTexture3D>(Initializer);
}

template<typename MetalTextureType, typename InitializerType>
MetalTextureType* FMetalInterface::CreateTexture(const InitializerType& Initializer)
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
            
                [StagingBuffer release];
            }
        }
    }
    
    return NewTexture.ReleaseOwnership();
}

FRHISamplerState* FMetalInterface::RHICreateSamplerState(const FRHISamplerStateInitializer& Initializer)
{
    return dbg_new FMetalSamplerState();
}

FRHIVertexBuffer* FMetalInterface::RHICreateVertexBuffer(const FRHIVertexBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalVertexBuffer>(Initializer);
}

FRHIIndexBuffer* FMetalInterface::RHICreateIndexBuffer(const FRHIIndexBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalIndexBuffer>(Initializer);
}

FRHIGenericBuffer* FMetalInterface::RHICreateGenericBuffer(const FRHIGenericBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalGenericBuffer>(Initializer);
}

FRHIConstantBuffer* FMetalInterface::RHICreateConstantBuffer(const FRHIConstantBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalConstantBuffer, FRHIConstantBufferInitializer, kConstantBufferAlignment>(Initializer);
}

template<typename MetalBufferType, typename InitializerType, const uint32 BufferAlignment>
MetalBufferType* FMetalInterface::CreateBuffer(const InitializerType& Initializer)
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
    
    const auto BufferLength = NMath::AlignUp(NewBuffer->GetSize(), BufferAlignment);
    
    id<MTLDevice> Device       = GetDeviceContext()->GetMTLDevice();
    id<MTLBuffer> NewMTLBuffer = [Device newBufferWithLength:BufferLength options:ResourceOptions];
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
            
                [StagingBuffer release];
            }
        }
    }
    
    return NewBuffer.ReleaseOwnership();
}

FRHIRayTracingScene* FMetalInterface::RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
{
    return dbg_new FMetalRayTracingScene(GetDeviceContext(), Initializer);
}

FRHIRayTracingGeometry* FMetalInterface::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
{
    return dbg_new FMetalRayTracingGeometry(Initializer);
}

FRHIShaderResourceView* FMetalInterface::RHICreateShaderResourceView(const FRHITextureSRVInitializer& Initializer)
{
    return dbg_new FMetalShaderResourceView(GetDeviceContext(), Initializer.Texture);
}

FRHIShaderResourceView* FMetalInterface::RHICreateShaderResourceView(const FRHIBufferSRVInitializer& Initializer)
{
    return dbg_new FMetalShaderResourceView(GetDeviceContext(), Initializer.Buffer);
}

FRHIUnorderedAccessView* FMetalInterface::RHICreateUnorderedAccessView(const FRHITextureUAVInitializer& Initializer)
{
    return dbg_new FMetalUnorderedAccessView(GetDeviceContext(), Initializer.Texture);
}

FRHIUnorderedAccessView* FMetalInterface::RHICreateUnorderedAccessView(const FRHIBufferUAVInitializer& Initializer)
{
    return dbg_new FMetalUnorderedAccessView(GetDeviceContext(), Initializer.Buffer);
}

FRHIComputeShader* FMetalInterface::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalComputeShader(GetDeviceContext(), ShaderCode);
}

FRHIVertexShader* FMetalInterface::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalVertexShader(GetDeviceContext(), ShaderCode);
}

FRHIHullShader* FMetalInterface::RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIDomainShader* FMetalInterface::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIGeometryShader* FMetalInterface::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIMeshShader* FMetalInterface::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIAmplificationShader* FMetalInterface::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIPixelShader* FMetalInterface::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalPixelShader(GetDeviceContext(), ShaderCode);
}

FRHIRayGenShader* FMetalInterface::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalRayGenShader(GetDeviceContext(), ShaderCode);
}

FRHIRayAnyHitShader* FMetalInterface::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalRayAnyHitShader(GetDeviceContext(), ShaderCode);
}

FRHIRayClosestHitShader* FMetalInterface::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalRayClosestHitShader(GetDeviceContext(), ShaderCode);
}

FRHIRayMissShader* FMetalInterface::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return dbg_new FMetalRayMissShader(GetDeviceContext(), ShaderCode);
}

FRHIDepthStencilState* FMetalInterface::RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& Initializer)
{
    return dbg_new FMetalDepthStencilState(GetDeviceContext(), Initializer);
}

FRHIRasterizerState* FMetalInterface::RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer)
{
    return dbg_new FMetalRasterizerState(GetDeviceContext(), Initializer);
}

FRHIBlendState* FMetalInterface::RHICreateBlendState(const FRHIBlendStateInitializer& Initializer)
{
    return dbg_new FMetalBlendState();
}

FRHIVertexInputLayout* FMetalInterface::RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer)
{
    return dbg_new FMetalInputLayoutState(GetDeviceContext(), Initializer);
}

FRHIGraphicsPipelineState* FMetalInterface::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    return dbg_new FMetalGraphicsPipelineState(GetDeviceContext(), Initializer);
}

FRHIComputePipelineState* FMetalInterface::RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer)
{
    return dbg_new FMetalComputePipelineState();
}

FRHIRayTracingPipelineState* FMetalInterface::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer)
{
    return dbg_new FMetalRayTracingPipelineState();
}

FRHITimestampQuery* FMetalInterface::RHICreateTimestampQuery()
{
    return dbg_new FMetalTimestampQuery();
}

FRHIViewport* FMetalInterface::RHICreateViewport(const FRHIViewportInitializer& Initializer)
{
    FCocoaWindow* Window = (FCocoaWindow*)Initializer.WindowHandle;
    
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

IRHICommandContext* FMetalInterface::RHIGetDefaultCommandContext()
{
    return CommandContext;
}

FString FMetalInterface::GetAdapterDescription() const
{
    return FString();
}

void FMetalInterface::RHIQueryRayTracingSupport(FRayTracingSupport& OutSupport) const
{
    OutSupport = FRayTracingSupport();
}

void FMetalInterface::RHIQueryShadingRateSupport(FShadingRateSupport& OutSupport) const
{
    OutSupport = FShadingRateSupport();
}

bool FMetalInterface::RHIQueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

#pragma clang diagnostic pop
