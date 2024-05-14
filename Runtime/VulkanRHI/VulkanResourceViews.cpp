#include "VulkanResourceViews.h"
#include "VulkanDevice.h"

FVulkanResourceView::FVulkanResourceView(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Type(EType::None)
{
    FMemory::Memzero(&ImageViewInfo);
}

FVulkanResourceView::~FVulkanResourceView()
{
    if (Type == EType::ImageView)
    {
        // Destroy the actual view
        if (VULKAN_CHECK_HANDLE(ImageViewInfo.ImageView))
        {
            GetDevice()->GetRenderPassCache().OnReleaseImageView(ImageViewInfo.ImageView);
            vkDestroyImageView(GetDevice()->GetVkDevice(), ImageViewInfo.ImageView, nullptr);
        }

        // Reset the view
        ImageViewInfo.Image            = VK_NULL_HANDLE;
        ImageViewInfo.ImageView        = VK_NULL_HANDLE;
        ImageViewInfo.Format           = VK_FORMAT_UNDEFINED;
        ImageViewInfo.Flags            = 0;
        ImageViewInfo.SubresourceRange = { };
    }
    else if (Type == EType::TypedBufferView)
    {
        // Destroy the actual view
        if (VULKAN_CHECK_HANDLE(TypedBufferInfo.BufferView))
        {
            vkDestroyBufferView(GetDevice()->GetVkDevice(), TypedBufferInfo.BufferView, nullptr);
        }

        // Reset the view
        TypedBufferInfo.Buffer     = VK_NULL_HANDLE;
        TypedBufferInfo.BufferView = VK_NULL_HANDLE;
    }
    else if (Type == EType::StructuredBufferView)
    {
        // Reset the view
        StructuredBufferInfo.Buffer = VK_NULL_HANDLE;
        StructuredBufferInfo.Offset = 0;
        StructuredBufferInfo.Range  = 0;
    }
    else if (Type == EType::AccelerationStructureView)
    {
        // Reset the view
        AccelerationStructureInfo.AccelerationStructure = VK_NULL_HANDLE;
    }
}

bool FVulkanResourceView::InitializeAsImageView(VkImage InImage, VkFormat InFormat, VkImageViewType InImageViewType, VkImageAspectFlags InAspectMask, uint32 InBaseArrayLayer, uint32 InLayerCount, uint32 InBaseMipLevel, uint32 InLevelCount)
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

    // Create a new view
    VkImageViewCreateInfo ImageViewCreateInfo;
    FMemory::Memzero(&ImageViewCreateInfo);

    ImageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.image                           = InImage;
    ImageViewCreateInfo.format                          = InFormat;
    ImageViewCreateInfo.viewType                        = InImageViewType;
    ImageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange.aspectMask     = InAspectMask;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = InBaseArrayLayer;
    ImageViewCreateInfo.subresourceRange.layerCount     = InLayerCount;
    ImageViewCreateInfo.subresourceRange.baseMipLevel   = InBaseMipLevel;
    ImageViewCreateInfo.subresourceRange.levelCount     = InLevelCount;

    VkResult Result = vkCreateImageView(GetDevice()->GetVkDevice(), &ImageViewCreateInfo, nullptr, &ImageViewInfo.ImageView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkCreateImageView failed");
        return false;
    }

    Type = EType::ImageView;
    ImageViewInfo.Image                           = InImage;
    ImageViewInfo.Format                          = InFormat;
    ImageViewInfo.ImageViewType                   = InImageViewType;
    ImageViewInfo.SubresourceRange.aspectMask     = InAspectMask;
    ImageViewInfo.SubresourceRange.baseArrayLayer = InBaseArrayLayer;
    ImageViewInfo.SubresourceRange.layerCount     = InLayerCount;
    ImageViewInfo.SubresourceRange.baseMipLevel   = InBaseMipLevel;
    ImageViewInfo.SubresourceRange.levelCount     = InLevelCount;
    return true;
}

bool FVulkanResourceView::InitializeAsStructuredBufferView(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange)
{
    if (!VULKAN_CHECK_HANDLE(InBuffer))
    {
        VULKAN_ERROR("Buffer cannot be NULL");
        return false;
    }

    Type = EType::StructuredBufferView;
    StructuredBufferInfo.Buffer = InBuffer;
    StructuredBufferInfo.Offset = InOffset;
    StructuredBufferInfo.Range  = InRange;
    return true;
}

bool FVulkanResourceView::InitializeAsTypedBufferView(VkBuffer InBuffer, VkFormat InFormat, VkDeviceSize InOffset, VkDeviceSize InRange)
{
    if (!VULKAN_CHECK_HANDLE(InBuffer))
    {
        VULKAN_ERROR("Image cannot be a NULL_HANDLE");
        return false;
    }
    if (InFormat == VK_FORMAT_UNDEFINED)
    {
        VULKAN_ERROR("Format cannot be a undefined");
        return false;
    }

    VkBufferViewCreateInfo BufferViewCreateInfo;
    FMemory::Memzero(&BufferViewCreateInfo);

    BufferViewCreateInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    BufferViewCreateInfo.buffer = InBuffer;
    BufferViewCreateInfo.format = InFormat;
    BufferViewCreateInfo.range  = InRange;
    BufferViewCreateInfo.offset = InOffset;

    VkResult Result = vkCreateBufferView(GetDevice()->GetVkDevice(), &BufferViewCreateInfo, nullptr, &TypedBufferInfo.BufferView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkCreateBufferView failed");
        return false;
    }

    Type = EType::TypedBufferView;
    TypedBufferInfo.Buffer = InBuffer;
    return true;
}

bool FVulkanResourceView::InitializeAsAccelerationStructureView(VkAccelerationStructureKHR InAccelerationStructure)
{
    if (!VULKAN_CHECK_HANDLE(InAccelerationStructure))
    {
        VULKAN_ERROR("AccelerationStructure cannot be NULL");
        return false;
    }

    Type = EType::AccelerationStructureView;
    AccelerationStructureInfo.AccelerationStructure = InAccelerationStructure;
    return true;
}

void FVulkanResourceView::SetDebugName(const FString& InName)
{
    if (!InName.IsEmpty())
    {
        if (Type == EType::ImageView)
        {
            FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.Data(), ImageViewInfo.ImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
        }
        else if (Type == EType::TypedBufferView)
        {
            FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.Data(), TypedBufferInfo.BufferView, VK_OBJECT_TYPE_BUFFER_VIEW);
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
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D;
            break;
        }
        case ETextureDimension::Texture2DArray:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        }
        case ETextureDimension::TextureCube:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        }
        case ETextureDimension::TextureCubeArray:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            break;
        }
        case ETextureDimension::Texture3D:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }
        default:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
            break;
        }
    }

    uint32 BaseArrayLayer;
    uint32 LayerCount;
    if (IsTextureCube(VulkanTexture->GetDimension()))
    {
        BaseArrayLayer = InDesc.FirstArraySlice * RHI_NUM_CUBE_FACES;
        LayerCount = FMath::Max<uint16>(InDesc.NumSlices, 1u) * RHI_NUM_CUBE_FACES;
    }
    else
    {
        BaseArrayLayer = InDesc.FirstArraySlice;
        LayerCount = FMath::Max<uint16>(InDesc.NumSlices, 1u);
    }

    const VkImage Image = VulkanTexture->GetVkImage();
    const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);
    if (InitializeAsImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, InDesc.FirstMipLevel, InDesc.NumMips))
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

    if (!InitializeAsStructuredBufferView(Buffer, Offset, Range))
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
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D;
            break;
        }
        case ETextureDimension::Texture2DArray:
        case ETextureDimension::TextureCube:
        case ETextureDimension::TextureCubeArray:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        }
        case ETextureDimension::Texture3D:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }
        default:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
            break;
        }
    }

    uint32 BaseArrayLayer;
    uint32 LayerCount;
    if (IsTextureCube(VulkanTexture->GetDimension()))
    {
        BaseArrayLayer = InDesc.FirstArraySlice * RHI_NUM_CUBE_FACES;
        LayerCount = RHI_NUM_CUBE_FACES;
    }
    else
    {
        BaseArrayLayer = InDesc.FirstArraySlice;
        LayerCount = 1u;
    }

    const VkImage Image = VulkanTexture->GetVkImage();
    const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);
    if (InitializeAsImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, InDesc.MipLevel, 1u))
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

    if (!InitializeAsStructuredBufferView(Buffer, Offset, Range))
    {
        return false;
    }

    return true;
}
