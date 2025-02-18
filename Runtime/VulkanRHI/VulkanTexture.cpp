#include "Core/Templates/NumericLimits.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanViewport.h"
#include "VulkanRHI/VulkanCommandContext.h"

uint32 FVulkanTextureHelper::CalculateTextureRowPitch(VkFormat Format, uint32 Width)
{
    const bool bIsBlockCompressed = VkFormatIsBlockCompressed(Format);
    if (bIsBlockCompressed)
    {
        const uint32 BlockSize = GetVkFormatBlockSize(Format);
        CHECK(BlockSize != 0);
        
        Width = FMath::Max<uint32>(1, (Width + 3) / 4);
        return Width * BlockSize;
    }
    else
    {
        const uint32 PixelSize = GetVkFormatByteStride(Format);
        CHECK(PixelSize != 0);
        return Width * PixelSize;
    }
}

uint32 FVulkanTextureHelper::CalculateTextureNumRows(VkFormat Format, uint32 Height)
{
    const bool bIsBlockCompressed = VkFormatIsBlockCompressed(Format);
    return bIsBlockCompressed ? FMath::AlignUp<uint32>(1, (Height + 3) / 4) : Height;
}

uint64 FVulkanTextureHelper::CalculateTextureUploadSize(VkFormat Format, uint32 Width, uint32 Height)
{
    const bool bIsBlockCompressed = VkFormatIsBlockCompressed(Format);
    if (bIsBlockCompressed)
    {
        const uint32 BlockSize = GetVkFormatBlockSize(Format);
        CHECK(BlockSize != 0);
        
        Width  = FMath::Max<uint32>(1, (Width + 3) / 4);
        Height = FMath::Max<uint32>(1, (Height + 3) / 4);
        return Width * Height * BlockSize;
    }
    else
    {
        const uint32 PixelSize = GetVkFormatByteStride(Format);
        CHECK(PixelSize != 0);
        return Width * Height * PixelSize;
    }
}

FVulkanTexture* FVulkanTexture::ResourceCast(FRHITexture* Texture)
{
    FVulkanTexture* VulkanTexture = nullptr;
    if (Texture)
    {
        if (IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::Presentable))
        {
            VulkanTexture = static_cast<FVulkanBackBufferTexture*>(Texture);
        }
        else
        {
            VulkanTexture = static_cast<FVulkanTexture*>(Texture);
        }
    }
    
    return VulkanTexture;
}

FVulkanTexture* FVulkanTexture::ResourceCast(FVulkanCommandContext* InCommandContext, FRHITexture* Texture)
{
    FVulkanTexture* VulkanTexture = nullptr;
    if (Texture)
    {
        if (IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::Presentable))
        {
            FVulkanBackBufferTexture* BackBuffer = static_cast<FVulkanBackBufferTexture*>(Texture);
            VulkanTexture = BackBuffer->GetCurrentBackBufferTexture(InCommandContext);
        }
        else
        {
            VulkanTexture = static_cast<FVulkanTexture*>(Texture);
        }
    }

    return VulkanTexture;
}

FVulkanTexture::FVulkanTexture(FVulkanDevice* InDevice, const FRHITextureInfo& InTextureInfo)
    : FRHITexture(InTextureInfo)
    , FVulkanDeviceChild(InDevice)
    , Image(VK_NULL_HANDLE)
    , Format(VK_FORMAT_UNDEFINED)
    , MemoryAllocation()
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , ImageViews()
    , DebugName()
{
}

FVulkanTexture::~FVulkanTexture()
{
    DestroyImageViews();

    // Check allocation in order to determine if this is a BackBuffer
    if (MemoryAllocation.IsValid() && VULKAN_CHECK_HANDLE(Image))
    {
        FVulkanDevice* VulkanDevice = GetDevice();
        
        // Destroy the image
        vkDestroyImage(VulkanDevice->GetVkDevice(), Image, nullptr);
        Image = VK_NULL_HANDLE;

        // Free the memory
        FVulkanMemoryManager& MemoryManager = VulkanDevice->GetMemoryManager();
        MemoryManager.Free(MemoryAllocation);
    }
}

bool FVulkanTexture::Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const IRHITextureData* InInitialData)
{
    const VkSampleCountFlagBits SampleCount = ConvertSampleCount(Info.NumSamples);
    if (SampleCount < VK_SAMPLE_COUNT_1_BIT)
    {
        VULKAN_ERROR("Invalid SampleCount");
        return false;
    }

    VkImageCreateInfo ImageCreateInfo;
    FMemory::Memzero(&ImageCreateInfo);

    ImageCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType             = ConvertTextureDimension(Info.Dimension);
    ImageCreateInfo.extent.width          = Info.Extent.X;
    ImageCreateInfo.extent.height         = Info.Extent.Y;
    ImageCreateInfo.mipLevels             = Info.NumMipLevels;
    ImageCreateInfo.pQueueFamilyIndices   = nullptr;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples               = SampleCount;
    ImageCreateInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    // NOTE: We store the format so that we have easy access to it later
    ImageCreateInfo.format = Format = ConvertFormat(Info.Format);
    if (IsTypelessFormat(Info.Format))
    {
        ImageCreateInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }

    if (ImageCreateInfo.imageType == VK_IMAGE_TYPE_3D)
    {
        ImageCreateInfo.arrayLayers  = 1;
        ImageCreateInfo.extent.depth = Info.Extent.Z;
    }
    else
    {
        ImageCreateInfo.arrayLayers  = Info.NumArraySlices;
        ImageCreateInfo.extent.depth = 1;
    }
    
    // Enable Texture-Cube views
    if (Info.IsTextureCube() || Info.IsTextureCubeArray())
    {
        ImageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        ImageCreateInfo.arrayLayers = Info.NumArraySlices * RHI_NUM_CUBE_FACES;
    }

    // TODO: Look into abstracting these flags
    ImageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (Info.IsRenderTarget())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (Info.IsDepthStencil())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (Info.IsShaderResource())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (Info.IsUnorderedAccess())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    VkResult Result = vkCreateImage(GetDevice()->GetVkDevice(), &ImageCreateInfo, nullptr, &Image);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create image");
        return false;
    }
    else
    {
        CreateInfo = ImageCreateInfo;
    }

    // NOTE: All textures are allocated as device local, maybe we want to move this into the AllocateImageMemory function
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const VkMemoryAllocateFlags AllocateFlags    = 0;

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateImageMemory(Image, MemoryProperties, AllocateFlags, GVulkanForceDedicatedImageAllocations, MemoryAllocation))
    {
        VULKAN_ERROR("Failed to allocate ImageMemory");
        return false;
    }

    {
        FRHITextureSRVInfo ViewInfo;
        ViewInfo.Texture = this;
        ViewInfo.Format  = VulkanCastShaderResourceFormat(Info.Format);

        if (Info.IsTexture2D())
        {
            ViewInfo.NumMips       = static_cast<uint8>(Info.NumMipLevels);
            ViewInfo.FirstMipLevel = 0;
            ViewInfo.MinLODClamp   = 0.0f;
        }
        else if (Info.IsTexture2DArray())
        {
            ViewInfo.NumMips         = static_cast<uint8>(Info.NumMipLevels);
            ViewInfo.FirstMipLevel   = 0;
            ViewInfo.MinLODClamp     = 0.0f;
            ViewInfo.NumSlices       = static_cast<uint16>(Info.NumArraySlices);
            ViewInfo.FirstArraySlice = 0;
        }
        else if (Info.IsTextureCube())
        {
            ViewInfo.NumMips       = static_cast<uint8>(Info.NumMipLevels);
            ViewInfo.FirstMipLevel = 0;
            ViewInfo.MinLODClamp   = 0.0f;
        }
        else if (Info.IsTextureCubeArray())
        {
            ViewInfo.NumMips         = static_cast<uint8>(Info.NumMipLevels);
            ViewInfo.FirstMipLevel   = 0;
            ViewInfo.MinLODClamp     = 0.0f;
            ViewInfo.FirstArraySlice = 0;
            ViewInfo.NumSlices       = static_cast<uint16>(Info.NumArraySlices);
        }
        else if (Info.IsTexture3D())
        {
            ViewInfo.NumMips       = static_cast<uint8>(Info.NumMipLevels);
            ViewInfo.FirstMipLevel = 0;
            ViewInfo.MinLODClamp   = 0.0f;
        }
        else
        {
            VULKAN_ERROR("Unsupported resource dimension");
            return false;
        }

        FVulkanShaderResourceViewRef DefaultSRV = new FVulkanShaderResourceView(GetDevice(), this);
        if (!DefaultSRV->InitializeTextureSRV(ViewInfo))
        {
            return false;
        }

        ShaderResourceView = DefaultSRV;
    }

    // TODO: Fix for other resources than Texture2D
    const bool bIsTexture2D = Info.IsTexture2D();
    if (bIsTexture2D)
    {
        if (Info.IsUnorderedAccess())
        {
            FRHITextureUAVInfo ViewInfo;
            ViewInfo.Texture         = this;
            ViewInfo.Format          = Info.Format;
            ViewInfo.FirstArraySlice = 0;
            ViewInfo.MipLevel        = 0;
            ViewInfo.NumSlices       = static_cast<uint16>(Info.NumArraySlices);

            FVulkanUnorderedAccessViewRef DefaultUAV = new FVulkanUnorderedAccessView(GetDevice(), this);
            if (!DefaultUAV->InitializeTextureUAV(ViewInfo))
            {
                return false;
            }

            UnorderedAccessView = DefaultUAV;
        }
    }
    
    CHECK(InCommandContext != nullptr);
    if (InInitialData)
    {
        // TODO: Support other types than texture 2D
        InCommandContext->RHIStartContext();
        
        VkImageMemoryBarrier2 ImageBarrier;
        FMemory::Memzero(&ImageBarrier);

        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = Image;
        ImageBarrier.srcAccessMask                   = VK_ACCESS_NONE;
        ImageBarrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
        ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageCreateInfo.format);
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        InCommandContext->GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);

        // Transfer all the miplevels
        uint32 Width  = Info.Extent.X;
        uint32 Height = Info.Extent.Y;
        for (uint32 Index = 0; Index < Info.NumMipLevels; ++Index)
        {
            // TODO: This does not feel optimal
            if (IsBlockCompressed(Info.Format) && (Width % 4 != 0 || Height % 4 != 0))
            {
                break;
            }

            // If there is no data for this mip-level we break
            void* Data = InInitialData->GetMipData(Index);
            if (!Data)
            {
                break;
            }
            
            FTextureRegion2D TextureRegion(Width, Height);
            InCommandContext->RHIUpdateTexture2D(this, TextureRegion, Index, Data, static_cast<uint32>(InInitialData->GetMipRowPitch(Index)));

            Width  = Width / 2;
            Height = Height / 2;
        }

        // NOTE: Transition into InitialAccess
        InCommandContext->RHITransitionTexture(this, FRHITextureTransition::Make(EResourceAccess::CopyDest, InInitialAccess));
        InCommandContext->RHIFinishContext();
    }
    else
    {
        // NOTE: Transition the texture into the expected ImageLayout
        InCommandContext->RHIStartContext();

        VkImageMemoryBarrier2 ImageBarrier;
        FMemory::Memzero(&ImageBarrier);

        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageBarrier.newLayout                       = ConvertResourceStateToImageLayout(InInitialAccess);
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = Image;
        ImageBarrier.srcAccessMask                   = VK_ACCESS_NONE;
        ImageBarrier.dstAccessMask                   = VK_ACCESS_NONE;
        ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageCreateInfo.format);
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        InCommandContext->GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);
        InCommandContext->RHIFinishContext();
    }
    
    return true;
}

FVulkanResourceView* FVulkanTexture::GetOrCreateImageView(const FVulkanHashableImageView& ImageViewInfo)
{
    if (!VULKAN_CHECK_HANDLE(Image))
    {
        VULKAN_WARNING("Texture does not have a valid Image");
        return nullptr;
    } 

    // Check for existing view and control the format of the view
    const VkFormat VulkanFormat = ConvertFormat(static_cast<EFormat>(ImageViewInfo.Format));
    const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

    // Check for an existing view
    FVulkanResourceView* ExistingView = nullptr;
    if (ImageViewInfo.NumArraySlices > 1)
    {
        if (FVulkanResourceView** ImageView = ImageViewMap.Find(ImageViewInfo))
        {
            ExistingView = *ImageView;
        }
    }
    else
    {
        // Calculate the subresource for this view
        const uint32 Subresource = VulkanCalculateSubresource(ImageViewInfo.MipLevel, ImageViewInfo.ArrayIndex, 0, GetNumMipLevels(), GetNumArraySlices());
        if (Subresource < static_cast<uint32>(ImageViews.Size()))
        {
            ExistingView = ImageViews[Subresource];
        }
        else
        {
            ImageViews.Resize(Subresource + 1);
        }
    }

    if (ExistingView)
    {
        const FVulkanResourceView::FImageView& ExistingImageViewInfo = ExistingView->GetImageViewInfo();
        if (ExistingImageViewInfo.Format != VulkanFormat)
        {
            VULKAN_WARNING("A ImageView for this subresource already exists with another format");
        }

        return ExistingView;
    }

    // Get the ImageViewType
    VkImageViewType ImageViewType;
    switch (Info.Dimension)
    {
        case ETextureDimension::Texture2D:
        {
            ImageViewType = VK_IMAGE_VIEW_TYPE_2D;
            break;
        }
        case ETextureDimension::Texture2DArray:
        case ETextureDimension::TextureCube:
        case ETextureDimension::TextureCubeArray:
        {
            ImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        }
        case ETextureDimension::Texture3D:
        {
            ImageViewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }
        default:
        {
            ImageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
            break;
        }
    }

    // Number of mip-levels
    constexpr uint32 NumMipLevels = 1;

    // Create a new view
    FVulkanResourceView* NewImageView = new FVulkanResourceView(GetDevice());
    if (!NewImageView->InitializeAsImageView(Image, VulkanFormat, ImageViewType, ImageAspectFlags, ImageViewInfo.ArrayIndex, ImageViewInfo.NumArraySlices, ImageViewInfo.MipLevel, NumMipLevels))
    {
        return nullptr;
    }

    if (ImageViewInfo.NumArraySlices > 1)
    {
        ImageViewMap.Add(ImageViewInfo, NewImageView);
    }
    else
    {
        const uint32 Subresource = VulkanCalculateSubresource(ImageViewInfo.MipLevel, ImageViewInfo.ArrayIndex, 0, GetNumMipLevels(), GetNumArraySlices());
        ImageViews[Subresource] = NewImageView;
    }

    return NewImageView;
}

void FVulkanTexture::DestroyImageViews()
{
    for (FVulkanResourceView* ImageView : ImageViews)
    {
        delete ImageView;
    }

    for (auto ImageViewPair : ImageViewMap)
    {
        delete ImageViewPair.Second;
    }

    ImageViews.Clear();
    ImageViewMap.Clear();
}

void FVulkanTexture::SetVkImage(VkImage InImage)
{
    DestroyImageViews();
    Image = InImage;

    // NOTE: Use the format in the description to set the native format if it is not set yet
    // this should only happen for BackBuffers
    if (Format == VK_FORMAT_UNDEFINED)
    {
        Format = ConvertFormat(Info.Format);
    }
}

void FVulkanTexture::SetDebugName(const FString& InName)
{
    if (VULKAN_CHECK_HANDLE(Image))
    {
        FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), *InName, Image, VK_OBJECT_TYPE_IMAGE);
        DebugName = InName;
    }
}

FString FVulkanTexture::GetDebugName() const
{
    return DebugName;
}


FVulkanBackBufferTexture::FVulkanBackBufferTexture(FVulkanDevice* InDevice, FVulkanViewport* InViewport, const FRHITextureInfo& InTextureInfo)
    : FVulkanTexture(InDevice, InTextureInfo)
    , Viewport(InViewport)
{
}

FVulkanBackBufferTexture::~FVulkanBackBufferTexture()
{
    Viewport = nullptr;
}

void FVulkanBackBufferTexture::ResizeBackBuffer(int32 InWidth, int32 InHeight)
{
    Info.Extent.X = InWidth;
    Info.Extent.Y = InHeight;
    
    const uint32 NumBackBuffers = Viewport->GetNumBackBuffers();
    for (uint32 Index = 0; Index < NumBackBuffers; Index++)
    {
        FVulkanTexture* BackBuffer = Viewport->GetBackBufferFromIndex(Index);
        BackBuffer->Resize(InWidth, InHeight);
        BackBuffer->DestroyImageViews();
    }
}

FVulkanTexture* FVulkanBackBufferTexture::GetCurrentBackBufferTexture(FVulkanCommandContext* InCommandContext)
{
    return Viewport ? Viewport->GetCurrentBackBuffer(InCommandContext) : nullptr;
}
