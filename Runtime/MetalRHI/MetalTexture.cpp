#include "MetalTexture.h"
#include "MetalViewport.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FMetalTexture::FMetalTexture(FMetalDeviceContext* InDeviceContext, const FRHITextureDesc& InDesc)
    : FRHITexture(InDesc)
    , FMetalObject(InDeviceContext)
    , Texture(nil)
    , Viewport(nullptr)
    , ShaderResourceView(nullptr)
{
}

FMetalTexture::~FMetalTexture()
{
    NSSafeRelease(Texture);
}

bool FMetalTexture::Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData)
{
    SCOPED_AUTORELEASE_POOL();

    MTLTextureDescriptor* TextureDescriptor = [[MTLTextureDescriptor new] autorelease];
    TextureDescriptor.textureType               = GetMTLTextureType(Desc.Dimension, Desc.IsMultisampled());
    TextureDescriptor.pixelFormat               = ConvertFormat(Desc.Format);
    TextureDescriptor.usage                     = ConvertTextureFlags(Desc.UsageFlags);
    TextureDescriptor.allowGPUOptimizedContents = NO;
    TextureDescriptor.swizzle                   = MTLTextureSwizzleChannelsMake(MTLTextureSwizzleRed, MTLTextureSwizzleGreen, MTLTextureSwizzleBlue, MTLTextureSwizzleAlpha);
    
    TextureDescriptor.width  = Desc.Extent.x;
    TextureDescriptor.height = Desc.Extent.y;
    
    if (Desc.IsTexture3D())
    {
        TextureDescriptor.depth       = Desc.Extent.z;
        TextureDescriptor.arrayLength = 1;
    }
    else
    {
        TextureDescriptor.depth       = 1;
        TextureDescriptor.arrayLength = FMath::Max(Desc.Extent.z, 1);
    }
    
    TextureDescriptor.mipmapLevelCount   = Desc.NumMipLevels;
    TextureDescriptor.sampleCount        = Desc.NumSamples;
    TextureDescriptor.resourceOptions    = MTLResourceCPUCacheModeWriteCombined;
    TextureDescriptor.cpuCacheMode       = MTLCPUCacheModeWriteCombined;
    TextureDescriptor.storageMode        = MTLStorageModePrivate;
    TextureDescriptor.hazardTrackingMode = MTLHazardTrackingModeDefault;
    
    id<MTLDevice> Device = GetDeviceContext()->GetMTLDevice();
    CHECK(Device != nil);

    id<MTLTexture> NewTexture = [Device newTextureWithDescriptor:TextureDescriptor];
    if (!NewTexture)
    {
        return false;
    }
    
    SetDrawableTexture(NewTexture);
    
    // TODO: Fix upload for other resources than Texture2D
    if (Desc.IsTexture2D())
    {
        if (InInitialData)
        {
            @autoreleasepool
            {
                id<MTLCommandQueue>       CommandQueue  = GetDeviceContext()->GetMTLCommandQueue();
                id<MTLCommandBuffer>      CommandBuffer = [CommandQueue commandBuffer];
                id<MTLBlitCommandEncoder> CopyEncoder   = [CommandBuffer blitCommandEncoder];

                // TODO: Handle uploadbuffers differently
                // Calculate total size of upload buffer
                uint64 TotalTextureSize = 0;
                for (uint32 Index = 0; Index < Desc.NumMipLevels; ++Index)
                {
                    TotalTextureSize += InInitialData->GetMipSlicePitch(Index);
                }
                
                id<MTLBuffer> StagingBuffer = [Device newBufferWithLength:TotalTextureSize options:MTLResourceCPUCacheModeDefaultCache];

                // Transfer all the mip-levels
                uint32 Width        = Desc.Extent.x;
                uint32 Height       = Desc.Extent.y;
                uint64 SourceOffset = 0;
                for (uint32 Index = 0; Index < Desc.NumMipLevels; ++Index)
                {
                    MTLRegion Region;
                    Region.origin = { 0, 0, 0 };
                    Region.size   = { NSUInteger(Width), NSUInteger(Height), 1 };
                    
                    const NSUInteger BytesPerRow = NSUInteger(InInitialData->GetMipRowPitch(Index));
                    const NSUInteger SlicePitch  = NSUInteger(InInitialData->GetMipSlicePitch(Index));
                    
                    [CopyEncoder copyFromBuffer:StagingBuffer
                                sourceOffset:SourceOffset
                            sourceBytesPerRow:BytesPerRow
                            sourceBytesPerImage:0
                                    sourceSize:Region.size
                                    toTexture:NewTexture
                            destinationSlice:0
                            destinationLevel:Index
                            destinationOrigin:Region.origin];
                    
                    Width        = Width / 2;
                    Height       = Height / 2;
                    SourceOffset = SourceOffset + InInitialData->GetMipSlicePitch(Index);
                }

                [CopyEncoder endEncoding];

                // TODO: we do not want to wait here
                [CommandBuffer commit];
                [CommandBuffer waitUntilCompleted];
            
                [StagingBuffer release];
            }
        }
    }

    return true;
}

void FMetalTexture::SetName(const FString& InName)
{
    @autoreleasepool
    {
        id<MTLTexture> TextureHandle = GetMTLTexture();
        if (TextureHandle)
        {
            TextureHandle.label = InName.GetNSString();
        }
    }
}

FString FMetalTexture::GetName() const
{
    FString Result;
    
    @autoreleasepool
    {
        id<MTLTexture> TextureHandle = GetMTLTexture();
        if (TextureHandle)
        {
            Result = FString(TextureHandle.label);
        }
    }
    
    return Result;
}

id<MTLTexture> FMetalTexture::GetMTLTexture() const
{
    // Need to get the texture from the viewport
    if (Viewport)
    {   
        return Viewport->GetDrawableTexture();
    }
    else
    {
        return Texture;
    }
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
