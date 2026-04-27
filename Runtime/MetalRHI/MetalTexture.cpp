#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalSwapChain.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FMetalTextureRHI::FMetalTextureRHI(FMetalDevice* InDevice, const FRHITextureDesc& InTextureDesc)
    : FRHITexture(InTextureDesc)
    , FMetalDeviceChild(InDevice)
    , Texture(nil)
    , SwapChain(nullptr)
    , ShaderResourceView(nullptr)
    , RenderTargetView(nullptr)
    , DepthStencilView(nullptr)
{
}

FMetalTextureRHI::~FMetalTextureRHI()
{
    [Texture release];
}

void* FMetalTextureRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(GetMTLTexture());
}

FRHIShaderResourceView* FMetalTextureRHI::GetShaderResourceView() const
{
    return ShaderResourceView.Get();
}

FRHIUnorderedAccessView* FMetalTextureRHI::GetUnorderedAccessView() const
{
    return nullptr;
}

FRHIRenderTargetView* FMetalTextureRHI::GetRenderTargetView() const
{
    return RenderTargetView.Get();
}

FRHIDepthStencilView* FMetalTextureRHI::GetDepthStencilView() const
{
    return DepthStencilView.Get();
}

FRHIDescriptorHandle FMetalTextureRHI::GetBindlessUAVHandle() const
{
    return FRHIDescriptorHandle();
}

FRHIDescriptorHandle FMetalTextureRHI::GetBindlessSRVHandle() const
{
    return FRHIDescriptorHandle();
}

bool FMetalTextureRHI::Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData)
{
    SCOPED_AUTORELEASE_POOL();

    MTLTextureDescriptor* TextureDescriptor = [[MTLTextureDescriptor new] autorelease];
    TextureDescriptor.textureType               = GetMTLTextureType(Desc.Dimension, Desc.IsMultisampled());
    TextureDescriptor.pixelFormat               = ConvertFormat(Desc.Format);
    TextureDescriptor.usage                     = ConvertTextureFlags(Desc.UsageFlags);
    TextureDescriptor.allowGPUOptimizedContents = NO;
    TextureDescriptor.swizzle                   = MTLTextureSwizzleChannelsMake(MTLTextureSwizzleRed, MTLTextureSwizzleGreen, MTLTextureSwizzleBlue, MTLTextureSwizzleAlpha);
    TextureDescriptor.mipmapLevelCount          = Desc.NumMipLevels;
    TextureDescriptor.sampleCount               = Desc.NumSamples;
    TextureDescriptor.resourceOptions           = MTLResourceCPUCacheModeWriteCombined;
    TextureDescriptor.cpuCacheMode              = MTLCPUCacheModeWriteCombined;
    TextureDescriptor.storageMode               = MTLStorageModePrivate;
    TextureDescriptor.hazardTrackingMode        = MTLHazardTrackingModeDefault;
    TextureDescriptor.width                     = Desc.Extent.X;
    TextureDescriptor.height                    = Desc.Extent.Y;
    
    if (Desc.IsTexture3D())
    {
        TextureDescriptor.depth       = Desc.Extent.Z;
        TextureDescriptor.arrayLength = 1;
    }
    else
    {
        TextureDescriptor.depth       = 1;
        TextureDescriptor.arrayLength = Math::Max(Desc.Extent.Z, 1);
    }
    
    id<MTLDevice>  Device = GetDevice()->GetMTLDevice();
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
                id<MTLCommandQueue>       CommandQueue  = GetDevice()->GetMTLCommandQueue();
                id<MTLCommandBuffer>      CommandBuffer = [CommandQueue commandBuffer];
                id<MTLBlitCommandEncoder> CopyEncoder   = [CommandBuffer blitCommandEncoder];

                // TODO: Handle uploadbuffers differently
                
                // Calculate total size of upload buffer
                uint64 TotalTextureSize = 0;
                for (uint32 Index = 0; Index < Desc.NumMipLevels; ++Index)
                {
                    TotalTextureSize += InInitialData->GetMipSlicePitch(Index);
                }
                
                // Create a staginbuffer and get the data-pointer for it
                id<MTLBuffer> StagingBuffer = [Device newBufferWithLength:TotalTextureSize options:MTLResourceCPUCacheModeDefaultCache];
                uint8* StagingBufferContents = reinterpret_cast<uint8*>(StagingBuffer.contents);
                
                // Transfer all the mip-levels
                uint32 Width        = Desc.Extent.X;
                uint32 Height       = Desc.Extent.Y;
                uint64 SourceOffset = 0;
                for (uint32 Index = 0; Index < Desc.NumMipLevels; ++Index)
                {
                    // TODO: This does not feel optimal
                    if (IsBlockCompressed(Desc.Format) && ((Width % 4 != 0) || (Height % 4 != 0)))
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

    if (Desc.IsRenderTarget() && !Desc.IsNoDefaultRTV())
    {
        RenderTargetView = new FMetalRenderTargetViewRHI(GetDevice(), FRHIRenderTargetViewDesc(this));
    }

    if (Desc.IsDepthStencil() && !Desc.IsNoDefaultDSV())
    {
        DepthStencilView = new FMetalDepthStencilViewRHI(GetDevice(), FRHIDepthStencilViewDesc(this));
    }

    return true;
}

void FMetalTextureRHI::SetDebugName(const FString& InName)
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

void FMetalTextureRHI::GetDebugName(FString& OutDebugName) const
{
    OutDebugName.Clear();

    @autoreleasepool
    {
        id<MTLTexture> TextureHandle = GetMTLTexture();
        if (TextureHandle)
        {
            OutDebugName = FString(TextureHandle.label);
        }
    }
}

id<MTLTexture> FMetalTextureRHI::GetMTLTexture() const
{
    // Need to get the texture from the viewport
    if (SwapChain)
    {   
        return SwapChain->GetDrawableTexture();
    }
    else
    {
        return Texture;
    }
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
