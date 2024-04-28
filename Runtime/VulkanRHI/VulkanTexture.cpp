#include "VulkanRHI.h"
#include "VulkanTexture.h"
#include "VulkanViewport.h"
#include "VulkanCommandContext.h"
#include "Core/Templates/NumericLimits.h"

FVulkanTexture::FVulkanTexture(FVulkanDevice* InDevice, const FRHITextureInfo& InTextureInfo)
    : FRHITexture(InTextureInfo)
    , FVulkanDeviceChild(InDevice)
    , Image(VK_NULL_HANDLE)
    , Format(VK_FORMAT_UNDEFINED)
    , MemoryAllocation()
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , RenderTargetViews()
    , DepthStencilViews()
    , DebugName()
{
}

FVulkanTexture::~FVulkanTexture()
{
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

bool FVulkanTexture::Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData)
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
    ImageCreateInfo.extent.width          = Info.Extent.x;
    ImageCreateInfo.extent.height         = Info.Extent.y;
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
        ImageCreateInfo.extent.depth = Info.Extent.z;
        ImageCreateInfo.arrayLayers  = 1;
    }
    else
    {
        ImageCreateInfo.extent.depth = 1;
        ImageCreateInfo.arrayLayers  = Info.NumArraySlices;
    }
    
    // Enable Texture-Cube views
    if (Info.IsTextureCube() || Info.IsTextureCubeArray())
    {
        ImageCreateInfo.flags       |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        ImageCreateInfo.arrayLayers  = Info.NumArraySlices * RHI_NUM_CUBE_FACES;
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
        FRHITextureSRVDesc ViewDesc;
        ViewDesc.Texture = this;
        ViewDesc.Format  = VulkanCastShaderResourceFormat(Info.Format);

        if (Info.IsTexture2D())
        {
            ViewDesc.NumMips       = static_cast<uint8>(Info.NumMipLevels);
            ViewDesc.FirstMipLevel = 0;
            ViewDesc.MinLODClamp   = 0.0f;
        }
        else if (Info.IsTexture2DArray())
        {
            ViewDesc.NumMips         = static_cast<uint8>(Info.NumMipLevels);
            ViewDesc.FirstMipLevel   = 0;
            ViewDesc.MinLODClamp     = 0.0f;
            ViewDesc.NumSlices       = static_cast<uint16>(Info.NumArraySlices);
            ViewDesc.FirstArraySlice = 0;
        }
        else if (Info.IsTextureCube())
        {
            ViewDesc.NumMips       = static_cast<uint8>(Info.NumMipLevels);
            ViewDesc.FirstMipLevel = 0;
            ViewDesc.MinLODClamp   = 0.0f;
        }
        else if (Info.IsTextureCubeArray())
        {
            ViewDesc.NumMips         = static_cast<uint8>(Info.NumMipLevels);
            ViewDesc.FirstMipLevel   = 0;
            ViewDesc.MinLODClamp     = 0.0f;
            ViewDesc.FirstArraySlice = 0;
            ViewDesc.NumSlices       = static_cast<uint16>(Info.NumArraySlices);
        }
        else if (Info.IsTexture3D())
        {
            ViewDesc.NumMips       = static_cast<uint8>(Info.NumMipLevels);
            ViewDesc.FirstMipLevel = 0;
            ViewDesc.MinLODClamp   = 0.0f;
        }
        else
        {
            VULKAN_ERROR("Unsupported resource dimension");
            return false;
        }

        FVulkanShaderResourceViewRef DefaultSRV = new FVulkanShaderResourceView(GetDevice(), this);
        if (!DefaultSRV->CreateTextureView(ViewDesc))
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
            FRHITextureUAVDesc ViewDesc;
            ViewDesc.Texture         = this;
            ViewDesc.Format          = Info.Format;
            ViewDesc.FirstArraySlice = 0;
            ViewDesc.MipLevel        = 0;
            ViewDesc.NumSlices       = static_cast<uint16>(Info.NumArraySlices);

            FVulkanUnorderedAccessViewRef DefaultUAV = new FVulkanUnorderedAccessView(GetDevice(), this);
            if (!DefaultUAV->CreateTextureView(ViewDesc))
            {
                return false;
            }

            UnorderedAccessView = DefaultUAV;
        }
    }
    
    if (InInitialData)
    {
        // TODO: Support other types than texture 2D
        FVulkanCommandContext* Context = FVulkanRHI::GetRHI()->ObtainCommandContext();
        Context->RHIStartContext();
        
        VkImageMemoryBarrier ImageBarrier;
        FMemory::Memzero(&ImageBarrier, sizeof(VkImageMemoryBarrier));

        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = Image;
        ImageBarrier.srcAccessMask                   = VK_ACCESS_NONE;
        ImageBarrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageCreateInfo.format);
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        VkPipelineStageFlags SrcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags DstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        Context->GetBarrierBatcher().AddImageMemoryBarrier(SrcStageMask, DstStageMask, 0, ImageBarrier);

        // Transfer all the miplevels
        uint32 Width  = Info.Extent.x;
        uint32 Height = Info.Extent.y;
        for (uint32 Index = 0; Index < Info.NumMipLevels; ++Index)
        {
            // TODO: This does not feel optimal
            if (IsBlockCompressed(Info.Format) && (Width % 4 != 0 || Height % 4 != 0))
            {
                break;
            }

            // If there is no data for this miplevel we break
            void* Data = InInitialData->GetMipData(Index);
            if (!Data)
            {
                break;
            }
            
            FTextureRegion2D TextureRegion(Width, Height);
            Context->RHIUpdateTexture2D(this, TextureRegion, Index, Data, static_cast<uint32>(InInitialData->GetMipRowPitch(Index)));

            Width  = Width / 2;
            Height = Height / 2;
        }

        // NOTE: Transition into InitialAccess
        Context->RHITransitionTexture(this, EResourceAccess::CopyDest, InInitialAccess);
        Context->RHIFinishContext();
    }
    else
    {
        // NOTE: Transition the texture into the expected ImageLayout
        FVulkanCommandContext* Context = FVulkanRHI::GetRHI()->ObtainCommandContext();
        Context->RHIStartContext();

        VkImageMemoryBarrier ImageBarrier;
        FMemory::Memzero(&ImageBarrier, sizeof(VkImageMemoryBarrier));

        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageBarrier.newLayout                       = ConvertResourceStateToImageLayout(InInitialAccess);
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = Image;
        ImageBarrier.srcAccessMask                   = VK_ACCESS_NONE;
        ImageBarrier.dstAccessMask                   = VK_ACCESS_NONE;
        ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageCreateInfo.format);
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        VkPipelineStageFlags SrcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags DstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        Context->GetBarrierBatcher().AddImageMemoryBarrier(SrcStageMask, DstStageMask, 0, ImageBarrier);

        Context->RHIFinishContext();
    }
    
    return true;
}

FVulkanImageView* FVulkanTexture::GetOrCreateRenderTargetView(const FRHIRenderTargetView& RenderTargetView)
{
    if (!VULKAN_CHECK_HANDLE(Image))
    {
        VULKAN_WARNING("Texture does not have a valid Image");
        return nullptr;
    }
    
    // Calculate the subresource for this view
    const uint32 Subresource = VulkanCalculateSubresource(RenderTargetView.MipLevel, RenderTargetView.ArrayIndex, 0, GetNumMipLevels(), GetNumArraySlices());
    
    // Check for existing view and control the format of the view
    const VkFormat VulkanFormat = ConvertFormat(RenderTargetView.Format);
    if (Subresource < static_cast<uint32>(RenderTargetViews.Size()))
    {
        if (FVulkanImageView* ExistingView = RenderTargetViews[Subresource].Get())
        {
            VULKAN_WARNING_COND(ExistingView->GetVkFormat() == VulkanFormat, "A RenderTargetView for this subresource already exists with another format");
            return ExistingView;
        }
    }
    else
    {
        RenderTargetViews.Resize(Subresource + 1);
    }
    
    // Create a new view
    VkImageSubresourceRange SubresourceRange;
    SubresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    SubresourceRange.baseArrayLayer = RenderTargetView.ArrayIndex;
    SubresourceRange.layerCount     = 1;
    SubresourceRange.baseMipLevel   = RenderTargetView.MipLevel;
    SubresourceRange.levelCount     = 1;

    FVulkanImageViewRef NewImageView = new FVulkanImageView(GetDevice());
    if (!NewImageView->CreateView(Image, VK_IMAGE_VIEW_TYPE_2D, VulkanFormat, 0, SubresourceRange))
    {
        return nullptr;
    }
    
    RenderTargetViews[Subresource] = NewImageView;
    return NewImageView.Get();
}

FVulkanImageView* FVulkanTexture::GetOrCreateDepthStencilView(const FRHIDepthStencilView& DepthStencilView)
{
    if (!VULKAN_CHECK_HANDLE(Image))
    {
        VULKAN_WARNING("Texture does not have a valid Image");
        return nullptr;
    } 
    
    // Calculate the subresource for this view
    const uint32 Subresource = VulkanCalculateSubresource(DepthStencilView.MipLevel, DepthStencilView.ArrayIndex, 0, GetNumMipLevels(), GetNumArraySlices());
    
    // Check for existing view and control the format of the view
    const VkFormat VulkanFormat = ConvertFormat(DepthStencilView.Format);
    if (Subresource < static_cast<uint32>(DepthStencilViews.Size()))
    {
        if (FVulkanImageView* ExistingView = DepthStencilViews[Subresource].Get())
        {
            VULKAN_WARNING_COND(ExistingView->GetVkFormat() == VulkanFormat, "A RenderTargetView for this subresource already exists with another format");
            return ExistingView;
        }
    }
    else
    {
        DepthStencilViews.Resize(Subresource + 1);
    }
    
    // Create a new view
    VkImageSubresourceRange SubresourceRange;
    SubresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    SubresourceRange.baseArrayLayer = DepthStencilView.ArrayIndex;
    SubresourceRange.layerCount     = 1;
    SubresourceRange.baseMipLevel   = DepthStencilView.MipLevel;
    SubresourceRange.levelCount     = 1;

    FVulkanImageViewRef NewImageView = new FVulkanImageView(GetDevice());
    if (!NewImageView->CreateView(Image, VK_IMAGE_VIEW_TYPE_2D, VulkanFormat, 0, SubresourceRange))
    {
        return nullptr;
    }
    
    DepthStencilViews[Subresource] = NewImageView;
    return NewImageView.Get();
}

void FVulkanTexture::SetVkImage(VkImage InImage)
{
    Image = InImage;
    RenderTargetViews.Clear();
    DepthStencilViews.Clear();

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
        FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.GetCString(), Image, VK_OBJECT_TYPE_IMAGE);
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
    Info.Extent.x = InWidth;
    Info.Extent.y = InHeight;
    
    const uint32 NumBackBuffers = Viewport->GetNumBackBuffers();
    for (uint32 Index = 0; Index < NumBackBuffers; Index++)
    {
        FVulkanTexture* BackBuffer = Viewport->GetBackBufferFromIndex(Index);
        BackBuffer->Resize(InWidth, InHeight);
    }
}

FVulkanTexture* FVulkanBackBufferTexture::GetCurrentBackBufferTexture()
{
    return Viewport ? Viewport->GetCurrentBackBuffer() : nullptr;
}


// FVulkanTextureHelper

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
