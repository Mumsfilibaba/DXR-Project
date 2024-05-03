#include "VulkanResourceViews.h"
#include "VulkanDevice.h"

FVulkanResourceView::FVulkanResourceView(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Type(EType::None)
{
}

FVulkanResourceView::~FVulkanResourceView()
{
    if (Type == EType::ImageView)
    {
        if (VULKAN_CHECK_HANDLE(ImageInfo.ImageView))
        {
            // Ensure that the FrameBuffer gets released if it is using this ImageView
            GetDevice()->GetRenderPassCache().OnReleaseImageView(ImageInfo.ImageView);

            // Destroy the actual view
            vkDestroyImageView(GetDevice()->GetVkDevice(), ImageInfo.ImageView, nullptr);
            
            // Reset the view
            ImageInfo.Image            = VK_NULL_HANDLE;
            ImageInfo.ImageView        = VK_NULL_HANDLE;
            ImageInfo.Format           = VK_FORMAT_UNDEFINED;
            ImageInfo.Flags            = 0;
            ImageInfo.SubresourceRange = { };
        }
    }
    else if (Type == EType::Buffer)
    {
        BufferInfo.Buffer = VK_NULL_HANDLE;
        BufferInfo.Offset = 0;
        BufferInfo.Range  = 0;
    }
}

bool FVulkanResourceView::CreateImageView(const VkImageViewCreateInfo& CreateInfo)
{
    if (!VULKAN_CHECK_HANDLE(CreateInfo.image))
    {
        VULKAN_ERROR("Image cannot be a NULL_HANDLE");
        return false;
    }
    if (CreateInfo.format == VK_FORMAT_UNDEFINED)
    {
        VULKAN_ERROR("Format cannot be a undefined");
        return false;
    }

    VkResult Result = vkCreateImageView(GetDevice()->GetVkDevice(), &CreateInfo, nullptr, &ImageInfo.ImageView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkCreateImageView failed");
        return false;
    }

    Type = EType::ImageView;
    ImageInfo.Image            = CreateInfo.image;
    ImageInfo.SubresourceRange = CreateInfo.subresourceRange;
    ImageInfo.Flags            = CreateInfo.flags;
    ImageInfo.Format           = CreateInfo.format;
    ImageInfo.ImageViewType    = CreateInfo.viewType;
    return true;
}

bool FVulkanResourceView::CreateBufferView(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange)
{
    if (!VULKAN_CHECK_HANDLE(InBuffer))
    {
        VULKAN_ERROR("Buffer cannot be NULL");
        return false;
    }

    Type = EType::Buffer;
    BufferInfo.Buffer = InBuffer;
    BufferInfo.Offset = InOffset;
    BufferInfo.Range  = InRange;
    return true;
}

void FVulkanResourceView::SetDebugName(const FString& InName)
{
    if (Type == EType::ImageView)
    {
        if (!InName.IsEmpty())
        {
            FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.Data(), ImageInfo.ImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
}

FVulkanShaderResourceView::FVulkanShaderResourceView(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIShaderResourceView(InResource)
    , FVulkanResourceView(InDevice)
{
}

bool FVulkanShaderResourceView::InitializeTextureSRV(const FRHITextureSRVDesc& InDesc)
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

    VkImageViewCreateInfo ImageViewCreateInfo;
    FMemory::Memzero(&ImageViewCreateInfo);

    ImageViewCreateInfo.sType                         = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.flags                         = 0;
    ImageViewCreateInfo.format                        = VulkanFormat;
    ImageViewCreateInfo.image                         = VulkanTexture->GetVkImage();
    ImageViewCreateInfo.viewType                      = VulkanImageType;
    ImageViewCreateInfo.components.r                  = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g                  = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b                  = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a                  = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange.aspectMask   = GetImageAspectFlagsFromFormat(VulkanFormat);
    ImageViewCreateInfo.subresourceRange.baseMipLevel = InDesc.FirstMipLevel;
    ImageViewCreateInfo.subresourceRange.levelCount   = InDesc.NumMips;
    
    if (IsTextureCube(VulkanTexture->GetDimension()))
    {
        ImageViewCreateInfo.subresourceRange.baseArrayLayer = InDesc.FirstArraySlice * RHI_NUM_CUBE_FACES;
        ImageViewCreateInfo.subresourceRange.layerCount     = FMath::Max<uint16>(InDesc.NumSlices, 1u) * RHI_NUM_CUBE_FACES;
    }
    else
    {
        ImageViewCreateInfo.subresourceRange.baseArrayLayer = InDesc.FirstArraySlice;
        ImageViewCreateInfo.subresourceRange.layerCount     = FMath::Max<uint16>(InDesc.NumSlices, 1u);
    }

    if (CreateImageView(ImageViewCreateInfo))
    {
        const FString TextureDebugName = VulkanTexture->GetDebugName();
        if (!TextureDebugName.IsEmpty())
        {
            SetDebugName(TextureDebugName + " ImageView SRV");
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool FVulkanShaderResourceView::InitializeBufferSRV(const FRHIBufferSRVDesc& InDesc)
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(InDesc.Buffer);
    if (!VulkanBuffer)
    {
        VULKAN_ERROR("Buffer cannot be nullptr");
        return false;
    }

    // TODO: Use typed buffers
    
    VkDeviceSize Stride = 0;
    if (InDesc.Format == EBufferSRVFormat::None)
    {
        Stride = VulkanBuffer->GetStride();
    }
    else if (InDesc.Format == EBufferSRVFormat::Uint32)
    {
        Stride = sizeof(uint32);
    }

    VkBuffer     Buffer = VulkanBuffer->GetVkBuffer();
    VkDeviceSize Offset = Stride * InDesc.FirstElement;
    VkDeviceSize Range  = Stride * InDesc.NumElements;

    if (!CreateBufferView(Buffer, Offset, Range))
    {
        return false;
    }

    return true;
}

FVulkanUnorderedAccessView::FVulkanUnorderedAccessView(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIUnorderedAccessView(InResource)
    , FVulkanResourceView(InDevice)
{
}

bool FVulkanUnorderedAccessView::InitializeTextureUAV(const FRHITextureUAVDesc& InDesc)
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

    VkImageViewCreateInfo ImageViewCreateInfo;
    FMemory::Memzero(&ImageViewCreateInfo);

    ImageViewCreateInfo.sType                         = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.flags                         = 0;
    ImageViewCreateInfo.format                        = VulkanFormat;
    ImageViewCreateInfo.image                         = VulkanTexture->GetVkImage();
    ImageViewCreateInfo.viewType                      = VulkanImageType;
    ImageViewCreateInfo.components.r                  = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g                  = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b                  = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a                  = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange.aspectMask   = GetImageAspectFlagsFromFormat(VulkanFormat);
    ImageViewCreateInfo.subresourceRange.baseMipLevel = InDesc.MipLevel;
    ImageViewCreateInfo.subresourceRange.levelCount   = 1u;
    
    if (IsTextureCube(VulkanTexture->GetDimension()))
    {
        ImageViewCreateInfo.subresourceRange.baseArrayLayer = InDesc.FirstArraySlice * RHI_NUM_CUBE_FACES;
        ImageViewCreateInfo.subresourceRange.layerCount     = RHI_NUM_CUBE_FACES;
    }
    else
    {
        ImageViewCreateInfo.subresourceRange.baseArrayLayer = InDesc.FirstArraySlice;
        ImageViewCreateInfo.subresourceRange.layerCount     = 1u;
    }
    
    if (CreateImageView(ImageViewCreateInfo))
    {
        const FString TextureDebugName = VulkanTexture->GetDebugName();
        if (!TextureDebugName.IsEmpty())
        {
            SetDebugName(TextureDebugName + " ImageView UAV");
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool FVulkanUnorderedAccessView::InitializeBufferUAV(const FRHIBufferUAVDesc& InDesc)
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(InDesc.Buffer);
    if (!VulkanBuffer)
    {
        VULKAN_ERROR("Buffer cannot be nullptr");
        return false;
    }

    // TODO: Use typed buffers
    
    VkDeviceSize Stride = 0;
    if (InDesc.Format == EBufferUAVFormat::None)
    {
        Stride = VulkanBuffer->GetStride();
    }
    else if (InDesc.Format == EBufferUAVFormat::Uint32)
    {
        Stride = sizeof(uint32);
    }

    VkBuffer     Buffer = VulkanBuffer->GetVkBuffer();
    VkDeviceSize Offset = Stride * InDesc.FirstElement;
    VkDeviceSize Range  = Stride * InDesc.NumElements;

    if (!CreateBufferView(Buffer, Offset, Range))
    {
        return false;
    }

    return true;
}
