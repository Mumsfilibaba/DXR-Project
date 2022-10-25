#include "MetalInterface.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

FMetalInterface::FMetalInterface()
    : FRHIInterface(ERHIInstanceType::Metal)
    , CommandContext()
{ }

FMetalInterface::~FMetalInterface()
{
    SAFE_DELETE(CommandContext);
    SAFE_DELETE(DeviceContext);
}

bool FMetalInterface::Initialize()
{
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

FRHITexture* FMetalInterface::RHICreateTexture2D(const FRHITexture2DInitializer& Initializer)
{
    return CreateTexture<FMetalTexture2D>(Initializer);
}

FRHITexture* FMetalInterface::RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer)
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
    
    TSharedRef<MetalTextureType> NewMetalTexture = dbg_new MetalTextureType(GetDeviceContext(), Initializer);

    MTLTextureDescriptor* TextureDescriptor = [[MTLTextureDescriptor new] autorelease];
    TextureDescriptor.textureType               = GetMTLTextureType(NewMetalTexture.Get());
    TextureDescriptor.pixelFormat               = ConvertFormat(Initializer.Format);
    TextureDescriptor.usage                     = ConvertTextureFlags(Initializer.UsageFlags);
    TextureDescriptor.allowGPUOptimizedContents = NO;
    TextureDescriptor.swizzle                   = MTLTextureSwizzleChannelsMake(MTLTextureSwizzleRed, MTLTextureSwizzleGreen, MTLTextureSwizzleBlue, MTLTextureSwizzleAlpha);
    
    const FIntVector3 Extent = NewMetalTexture->GetExtent();
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
    TextureDescriptor.sampleCount        = NewMetalTexture->GetNumSamples();
    TextureDescriptor.resourceOptions    = MTLResourceCPUCacheModeWriteCombined;
    TextureDescriptor.cpuCacheMode       = MTLCPUCacheModeWriteCombined;
    TextureDescriptor.storageMode        = MTLStorageModePrivate;
    TextureDescriptor.hazardTrackingMode = MTLHazardTrackingModeDefault;
    
    id<MTLDevice>  Device = GetDeviceContext()->GetMTLDevice();
    id<MTLTexture> NewTexture = [Device newTextureWithDescriptor:TextureDescriptor];
    if (!NewTexture)
    {
        return nullptr;
    }
    
    NewMetalTexture->SetDrawableTexture(NewTexture);
    
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
                                  toTexture:NewTexture
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
    
    return NewMetalTexture.ReleaseOwnership();
}

FRHISamplerState* FMetalInterface::RHICreateSamplerState(const FRHISamplerStateInitializer& Initializer)
{
    return dbg_new FMetalSamplerState();
}

FRHIVertexBuffer* FMetalInterface::RHICreateBuffer(const FRHIVertexBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalVertexBuffer>(Initializer);
}

FRHIIndexBuffer* FMetalInterface::RHICreateBuffer(const FRHIIndexBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalIndexBuffer>(Initializer);
}

FRHIGenericBuffer* FMetalInterface::RHICreateBuffer(const FRHIGenericBufferInitializer& Initializer)
{
    return CreateBuffer<FMetalGenericBuffer>(Initializer);
}

FRHIConstantBuffer* FMetalInterface::RHICreateBuffer(const FRHIConstantBufferInitializer& Initializer)
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

FRHIRayTracingScene* FMetalInterface::RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& Initializer)
{
    return dbg_new FMetalRayTracingScene(GetDeviceContext(), Initializer);
}

FRHIRayTracingGeometry* FMetalInterface::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& Initializer)
{
    return dbg_new FMetalRayTracingGeometry(Initializer);
}

FRHIShaderResourceView* FMetalInterface::RHICreateShaderResourceView(const FRHITextureSRVDesc& Initializer)
{
    return dbg_new FMetalShaderResourceView(GetDeviceContext(), Initializer.Texture);
}

FRHIShaderResourceView* FMetalInterface::RHICreateShaderResourceView(const FRHIBufferSRVDesc& Initializer)
{
    return dbg_new FMetalShaderResourceView(GetDeviceContext(), Initializer.Buffer);
}

FRHIUnorderedAccessView* FMetalInterface::RHICreateUnorderedAccessView(const FRHITextureUAVDesc& Initializer)
{
    return dbg_new FMetalUnorderedAccessView(GetDeviceContext(), Initializer.Texture);
}

FRHIUnorderedAccessView* FMetalInterface::RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& Initializer)
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

FRHIDepthStencilState* FMetalInterface::RHICreateDepthStencilState(const FRHIDepthStencilStateDesc& Initializer)
{
    return dbg_new FMetalDepthStencilState(GetDeviceContext(), Initializer);
}

FRHIRasterizerState* FMetalInterface::RHICreateRasterizerState(const FRHIRasterizerStateDesc& Initializer)
{
    return dbg_new FMetalRasterizerState(GetDeviceContext(), Initializer);
}

FRHIBlendState* FMetalInterface::RHICreateBlendState(const FRHIBlendStateDesc& Initializer)
{
    return dbg_new FMetalBlendState();
}

FRHIVertexInputLayout* FMetalInterface::RHICreateVertexInputLayout(const FRHIVertexInputLayoutDesc& Initializer)
{
    return dbg_new FMetalInputLayoutState(GetDeviceContext(), Initializer);
}

FRHIGraphicsPipelineState* FMetalInterface::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& Initializer)
{
    return dbg_new FMetalGraphicsPipelineState(GetDeviceContext(), Initializer);
}

FRHIComputePipelineState* FMetalInterface::RHICreateComputePipelineState(const FRHIComputePipelineStateDesc& Initializer)
{
    return dbg_new FMetalComputePipelineState();
}

FRHIRayTracingPipelineState* FMetalInterface::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Initializer)
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
    ExecuteOnMainThread(^
    {
        Frame       = Window.frame;
        ContentRect = [Window contentRectForFrameRect:Window.frame];
    }, NSDefaultRunLoopMode, true);
    
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
