#include "VulkanResourceViews.h"
#include "VulkanDevice.h"

FVulkanImageView::FVulkanImageView(FVulkanDevice* InDevice)
    : FVulkanDeviceObject(InDevice)
    , Image(VK_NULL_HANDLE)
    , ImageView(VK_NULL_HANDLE)
    , Format(VK_FORMAT_UNDEFINED)
    , Flags(0)
{
}

FVulkanImageView::~FVulkanImageView()
{
    DestroyView();
}

bool FVulkanImageView::CreateView(VkImage InImage, VkImageViewType ViewType, VkFormat InFormat, VkImageViewCreateFlags InFlags, const VkImageSubresourceRange& InSubresourceRange)
{
    if (!VULKAN_CHECK_HANDLE(InImage))
    {
        VULKAN_ERROR("Image cannot be a NULL_HANDLE");
        return false;
    }
    if (InFormat == VK_FORMAT_UNDEFINED)
    {
        VULKAN_ERROR("Format cannot be a undefined");
        return false;
    }
    
    VkImageViewCreateInfo ImageViewCreateInfo;
    FMemory::Memzero(&ImageViewCreateInfo);

    ImageViewCreateInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.flags            = InFlags;
    ImageViewCreateInfo.format           = InFormat;
    ImageViewCreateInfo.image            = InImage;
    ImageViewCreateInfo.viewType         = ViewType;
    ImageViewCreateInfo.components.r     = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g     = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b     = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a     = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange = InSubresourceRange;

    VkResult Result = vkCreateImageView(GetDevice()->GetVkDevice(), &ImageViewCreateInfo, nullptr, &ImageView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkCreateImageView failed");
        return false;
    }
    else
    {
        Image            = InImage;
        SubresourceRange = InSubresourceRange;
        Flags            = InFlags;
        Format           = InFormat;
    }

    return true;
}

void FVulkanImageView::DestroyView()
{
    if (VULKAN_CHECK_HANDLE(ImageView))
    {
        // Ensure that the FrameBuffer gets released if it is using this ImageView
        GetDevice()->GetFramebufferCache().OnReleaseImageView(ImageView);

        // Destroy the actual view
        vkDestroyImageView(GetDevice()->GetVkDevice(), ImageView, nullptr);
        
        // Reset the view
        Image     = VK_NULL_HANDLE;
        ImageView = VK_NULL_HANDLE;
        Format    = VK_FORMAT_UNDEFINED;
    }
}


FVulkanShaderResourceView::FVulkanShaderResourceView(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIShaderResourceView(InResource)
    , FVulkanDeviceObject(InDevice)
    , ImageView(nullptr)
    , BufferInfo{VK_NULL_HANDLE, 0, 0}
{
}

bool FVulkanShaderResourceView::CreateTextureView(const FRHITextureSRVDesc& InDesc)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(InDesc.Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR("Texture cannot be nullptr");
        return false;
    }

    if (IsTypelessFormat(InDesc.Format))
    {
        VULKAN_ERROR("Cannot create a view of a typeless format");
        return false;
    }
    
    const VkFormat VulkanFormat = ConvertFormat(InDesc.Format);

    VkImageSubresourceRange SubresourceRange;
    SubresourceRange.aspectMask   = GetImageAspectFlagsFromFormat(VulkanFormat);
    SubresourceRange.baseMipLevel = InDesc.FirstMipLevel;
    SubresourceRange.levelCount   = InDesc.NumMips;
    
    if (IsTextureCube(VulkanTexture->GetDimension()))
    {
        SubresourceRange.baseArrayLayer = InDesc.FirstArraySlice * VULKAN_NUM_CUBE_FACES;
        SubresourceRange.layerCount     = FMath::Max<uint16>(InDesc.NumSlices, 1u) * VULKAN_NUM_CUBE_FACES;
    }
    else
    {
        SubresourceRange.baseArrayLayer = InDesc.FirstArraySlice;
        SubresourceRange.layerCount     = FMath::Max<uint16>(InDesc.NumSlices, 1u);
    }

    
    VkImageViewType VulkanImageType;
    switch(VulkanTexture->GetDimension())
    {
        case ETextureDimension::Texture2D:
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case ETextureDimension::Texture2DArray:
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        case ETextureDimension::TextureCube:
            VulkanImageType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case ETextureDimension::TextureCubeArray:
            VulkanImageType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            break;
        case ETextureDimension::Texture3D:
            VulkanImageType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        default:
            VulkanImageType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
            break;
    }
    
    
    ImageView = new FVulkanImageView(GetDevice());
    if (!ImageView->CreateView(VulkanTexture->GetVkImage(), VulkanImageType, VulkanFormat, 0, SubresourceRange))
    {
        return false;
    }
    else
    {
        ImageSubresourceRange = SubresourceRange;
    }
    
    return true;
}

bool FVulkanShaderResourceView::CreateBufferView(const FRHIBufferSRVDesc& InDesc)
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(InDesc.Buffer);
    if (!VulkanBuffer)
    {
        VULKAN_ERROR("Buffer cannot be nullptr");
        return false;
    }

    VkDeviceSize Stride = 0;
    if (InDesc.Format == EBufferSRVFormat::None)
    {
        Stride = VulkanBuffer->GetStride();
    }
    else if (InDesc.Format == EBufferSRVFormat::Uint32)
    {
        Stride = sizeof(uint32);
    }

    BufferInfo.buffer = VulkanBuffer->GetVkBuffer();
    BufferInfo.offset = Stride * InDesc.FirstElement;
    BufferInfo.range  = Stride * InDesc.NumElements;
    return true;
}


FVulkanUnorderedAccessView::FVulkanUnorderedAccessView(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIUnorderedAccessView(InResource)
    , FVulkanDeviceObject(InDevice)
    , ImageView(nullptr)
    , BufferInfo{VK_NULL_HANDLE, 0, 0}
{
}

bool FVulkanUnorderedAccessView::CreateTextureView(const FRHITextureUAVDesc& InDesc)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(InDesc.Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR("Texture cannot be nullptr");
        return false;
    }

    if (IsTypelessFormat(InDesc.Format))
    {
        VULKAN_ERROR("Cannot create a view of a typeless format");
        return false;
    }
    
    const VkFormat VulkanFormat = ConvertFormat(InDesc.Format);

    VkImageSubresourceRange SubresourceRange;
    SubresourceRange.aspectMask   = GetImageAspectFlagsFromFormat(VulkanFormat);
    SubresourceRange.baseMipLevel = InDesc.MipLevel;
    SubresourceRange.levelCount   = 1u;
    
    if (IsTextureCube(VulkanTexture->GetDimension()))
    {
        SubresourceRange.baseArrayLayer = InDesc.FirstArraySlice * VULKAN_NUM_CUBE_FACES;
        SubresourceRange.layerCount     = VULKAN_NUM_CUBE_FACES;
    }
    else
    {
        SubresourceRange.baseArrayLayer = InDesc.FirstArraySlice;
        SubresourceRange.layerCount     = 1u;
    }

    
    VkImageViewType VulkanImageType;
    switch(VulkanTexture->GetDimension())
    {
        case ETextureDimension::Texture2D:
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case ETextureDimension::Texture2DArray:
        case ETextureDimension::TextureCube:
        case ETextureDimension::TextureCubeArray:
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        case ETextureDimension::Texture3D:
            VulkanImageType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        default:
            VulkanImageType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
            break;
    }
    
    
    ImageView = new FVulkanImageView(GetDevice());
    if (!ImageView->CreateView(VulkanTexture->GetVkImage(), VulkanImageType, VulkanFormat, 0, SubresourceRange))
    {
        return false;
    }
    else
    {
        ImageSubresourceRange = SubresourceRange;
    }
    
    return true;
}

bool FVulkanUnorderedAccessView::CreateBufferView(const FRHIBufferUAVDesc& InDesc)
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(InDesc.Buffer);
    if (!VulkanBuffer)
    {
        VULKAN_ERROR("Buffer cannot be nullptr");
        return false;
    }

    VkDeviceSize Stride = 0;
    if (InDesc.Format == EBufferUAVFormat::None)
    {
        Stride = VulkanBuffer->GetStride();
    }
    else if (InDesc.Format == EBufferUAVFormat::Uint32)
    {
        Stride = sizeof(uint32);
    }

    BufferInfo.buffer = VulkanBuffer->GetVkBuffer();
    BufferInfo.offset = Stride * InDesc.FirstElement;
    BufferInfo.range  = Stride * InDesc.NumElements;
    return true;
}
