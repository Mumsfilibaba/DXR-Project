#include "MetalTexture.h"
#include "MetalViewport.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FMetalTexture::FMetalTexture(FMetalDeviceContext* InDeviceContext, const FRHITextureInfo& InTextureInfo)
    : FRHITexture(InTextureInfo)
    , FMetalDeviceChild(InDeviceContext)
    , Texture(nil)
    , Viewport(nullptr)
    , ShaderResourceView(nullptr)
{
}

FMetalTexture::~FMetalTexture()
{
    [Texture release];
}

bool FMetalTexture::Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData)
{
    SCOPED_AUTORELEASE_POOL();

    MTLTextureDescriptor* TextureDescriptor = [[MTLTextureDescriptor new] autorelease];
    TextureDescriptor.textureType               = GetMTLTextureType(Info.Dimension, Info.IsMultisampled());
    TextureDescriptor.pixelFormat               = ConvertFormat(Info.Format);
    TextureDescriptor.usage                     = ConvertTextureFlags(Info.UsageFlags);
    TextureDescriptor.allowGPUOptimizedContents = NO;
    TextureDescriptor.swizzle                   = MTLTextureSwizzleChannelsMake(MTLTextureSwizzleRed, MTLTextureSwizzleGreen, MTLTextureSwizzleBlue, MTLTextureSwizzleAlpha);
    TextureDescriptor.mipmapLevelCount          = Info.NumMipLevels;
    TextureDescriptor.sampleCount               = Info.NumSamples;
    TextureDescriptor.resourceOptions           = MTLResourceCPUCacheModeWriteCombined;
    TextureDescriptor.cpuCacheMode              = MTLCPUCacheModeWriteCombined;
    TextureDescriptor.storageMode               = MTLStorageModePrivate;
    TextureDescriptor.hazardTrackingMode        = MTLHazardTrackingModeDefault;
    TextureDescriptor.width                     = Info.Extent.X;
    TextureDescriptor.height                    = Info.Extent.Y;
    
    if (Info.IsTexture3D())
    {
        TextureDescriptor.depth       = Info.Extent.Z;
        TextureDescriptor.arrayLength = 1;
    }
    else
    {
        TextureDescriptor.depth       = 1;
        TextureDescriptor.arrayLength = FMath::Max(Info.Extent.Z, 1);
    }
    
    id<MTLDevice>  Device = GetDeviceContext()->GetMTLDevice();
    id<MTLTexture> NewTexture = [Device newTextureWithDescriptor:TextureDescriptor];
    if (!NewTexture)
    {
        return false;
    }
    
    SetDrawableTexture(NewTexture);
    
    // TODO: Fix upload for other resources than Texture2D
    if (Info.IsTexture2D())
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
                for (uint32 Index = 0; Index < Info.NumMipLevels; ++Index)
                {
                    TotalTextureSize += InInitialData->GetMipSlicePitch(Index);
                }
                
                // Create a staginbuffer and get the data-pointer for it
                id<MTLBuffer> StagingBuffer = [Device newBufferWithLength:TotalTextureSize options:MTLResourceCPUCacheModeDefaultCache];
                uint8* StagingBufferContents = reinterpret_cast<uint8*>(StagingBuffer.contents);
                
                // Transfer all the mip-levels
                uint32 Width        = Info.Extent.X;
                uint32 Height       = Info.Extent.Y;
                uint64 SourceOffset = 0;
                for (uint32 Index = 0; Index < Info.NumMipLevels; ++Index)
                {
                    // TODO: This does not feel optimal
                    if (IsBlockCompressed(Info.Format) && ((Width % 4 != 0) || (Height % 4 != 0)))
                    {
                        break;
                    }

                    MTLRegion Region;
                    Region.origin = { 0, 0, 0 };
                    Region.size   = { NSUInteger(Width), NSUInteger(Height), 1 };
                    
                    const NSUInteger BytesPerRow = NSUInteger(InInitialData->GetMipRowPitch(Index));
                    const NSUInteger SlicePitch  = NSUInteger(InInitialData->GetMipSlicePitch(Index));
                    
                    // Set the data in the stagingbuffer
                    FMemory::Memcpy(StagingBufferContents + SourceOffset, InInitialData->GetMipData(Index), SlicePitch);
                    
                    // Perform copy of the staginbuffer into the GPU memory
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
                    SourceOffset = SourceOffset + SlicePitch;
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

void FMetalTexture::SetDebugName(const FString& InName)
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

FString FMetalTexture::GetDebugName() const
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
