#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanBuffer.h"

FVulkanResourceView::FVulkanResourceView(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Type(EType::None)
    , OwnerResource(nullptr)
    , DescriptorVersion(0)
{
    FMemory::Memzero(&ImageViewInfo);
}

FVulkanResourceView::~FVulkanResourceView()
{
    UnregisterFromResource();

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

void FVulkanResourceView::OnResourceRelocated(FVulkanGenericResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage)
{
    CHECK(RelocatedResource == OwnerResource);

    if (!NewMemoryStorage)
    {
        OwnerResource = nullptr;
    }
}

void FVulkanResourceView::RegisterWithResource(FVulkanGenericResource* InOwner)
{
    OwnerResource = InOwner;
    if (OwnerResource)
    {
        OwnerResource->AddResourceRelocatedListener(this);
    }
}

void FVulkanResourceView::UnregisterFromResource()
{
    if (OwnerResource)
    {
        OwnerResource->RemoveResourceRelocatedListener(this);
        OwnerResource = nullptr;
    }
}

bool FVulkanResourceView::InitializeImageView(VkImage InImage, VkFormat InFormat, VkImageViewType InImageViewType, VkImageAspectFlags InAspectMask, uint32 InBaseArrayLayer, uint32 InLayerCount, uint32 InBaseMipLevel, uint32 InLevelCount)
{
    if (!VULKAN_CHECK_HANDLE(InImage))
    {
        VULKAN_ERROR_CRITICAL("Image cannot be a NULL_HANDLE");
        return false;
    }
    
    if (InFormat == VK_FORMAT_UNDEFINED)
    {
        VULKAN_ERROR_CRITICAL("Format cannot be undefined");
        return false;
    }

    // Create a new view
    VkImageViewCreateInfo ImageViewCreateInfo = {};
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
        VULKAN_ERROR_CRITICAL("vkCreateImageView failed");
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

bool FVulkanResourceView::InitializeStructuredBufferView(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange, VkDeviceSize InViewOffset)
{
    if (!VULKAN_CHECK_HANDLE(InBuffer))
    {
        VULKAN_ERROR_CRITICAL("Buffer cannot be NULL");
        return false;
    }

    Type = EType::StructuredBufferView;
    StructuredBufferInfo.Buffer     = InBuffer;
    StructuredBufferInfo.Offset     = InOffset;
    StructuredBufferInfo.Range      = InRange;
    StructuredBufferInfo.ViewOffset = InViewOffset;
    return true;
}

bool FVulkanResourceView::InitializeTypedBufferView(VkBuffer InBuffer, VkFormat InFormat, VkDeviceSize InOffset, VkDeviceSize InRange)
{
    if (!VULKAN_CHECK_HANDLE(InBuffer))
    {
        VULKAN_ERROR_CRITICAL("Image cannot be a NULL_HANDLE");
        return false;
    }
    if (InFormat == VK_FORMAT_UNDEFINED)
    {
        VULKAN_ERROR_CRITICAL("Format cannot be undefined");
        return false;
    }

    VkBufferViewCreateInfo BufferViewCreateInfo = {};
    BufferViewCreateInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    BufferViewCreateInfo.buffer = InBuffer;
    BufferViewCreateInfo.format = InFormat;
    BufferViewCreateInfo.range  = (InRange == 0) ? VK_WHOLE_SIZE : InRange;
    BufferViewCreateInfo.offset = InOffset;

    VkResult Result = vkCreateBufferView(GetDevice()->GetVkDevice(), &BufferViewCreateInfo, nullptr, &TypedBufferInfo.BufferView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkCreateBufferView failed");
        return false;
    }

    Type = EType::TypedBufferView;
    TypedBufferInfo.Buffer = InBuffer;
    return true;
}

bool FVulkanResourceView::InitializeAccelerationStructureView(VkAccelerationStructureKHR InAccelerationStructure)
{
    if (!VULKAN_CHECK_HANDLE(InAccelerationStructure))
    {
        VULKAN_ERROR_CRITICAL("AccelerationStructure cannot be NULL");
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
            VulkanSetObjectName(GetDevice()->GetVkDevice(), InName.Data(), ImageViewInfo.ImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
        }
        else if (Type == EType::TypedBufferView)
        {
            VulkanSetObjectName(GetDevice()->GetVkDevice(), InName.Data(), TypedBufferInfo.BufferView, VK_OBJECT_TYPE_BUFFER_VIEW);
        }
    }
}

FVulkanShaderResourceView::FVulkanShaderResourceView(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIShaderResourceView(InResource)
    , FVulkanResourceView(InDevice)
{
}

void FVulkanShaderResourceView::OnResourceRelocated(FVulkanGenericResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage)
{
    FVulkanResourceView::OnResourceRelocated(RelocatedResource, NewMemoryStorage);

    if (NewMemoryStorage)
    {
        if (Type == EType::ImageView)
        {
            FVulkanTexture* VulkanTexture = static_cast<FVulkanTexture*>(RelocatedResource);
            
            if (VULKAN_CHECK_HANDLE(ImageViewInfo.ImageView))
            {
                GetDevice()->GetRenderPassCache().OnReleaseImageView(ImageViewInfo.ImageView);
                vkDestroyImageView(GetDevice()->GetVkDevice(), ImageViewInfo.ImageView, nullptr);
                ImageViewInfo.ImageView = VK_NULL_HANDLE;
            }

            InitializeImageView(
                VulkanTexture->GetVkImage(),
                ImageViewInfo.Format,
                ImageViewInfo.ImageViewType,
                ImageViewInfo.SubresourceRange.aspectMask,
                ImageViewInfo.SubresourceRange.baseArrayLayer,
                ImageViewInfo.SubresourceRange.layerCount,
                ImageViewInfo.SubresourceRange.baseMipLevel,
                ImageViewInfo.SubresourceRange.levelCount);
        }
        else if (Type == EType::StructuredBufferView)
        {
            FVulkanBuffer* VulkanBuffer = static_cast<FVulkanBuffer*>(RelocatedResource);
            StructuredBufferInfo.Buffer = VulkanBuffer->GetBindVkBuffer();
            StructuredBufferInfo.Offset = VulkanBuffer->GetBindOffset() + StructuredBufferInfo.ViewOffset;
        }
    }
}

bool FVulkanShaderResourceView::Initialize(const FRHIShaderResourceViewInfo& InInfo)
{
    if (InInfo.IsBufferSRV())
    {
		FVulkanBuffer* VulkanBuffer = FVulkanBuffer::Cast(InInfo.BufferSRV.Buffer);
		if (!VulkanBuffer)
		{
			VULKAN_ERROR_CRITICAL("Buffer cannot be nullptr");
			return false;
		}

		// TODO: Use typed buffers

		VkDeviceSize Stride = 0;
		if (InInfo.BufferSRV.Format == EBufferSRVFormat::None)
		{
			Stride = VulkanBuffer->GetInfo().Stride;
		}
		else if (InInfo.BufferSRV.Format == EBufferSRVFormat::UInt32)
		{
			Stride = sizeof(uint32);
		}

        const VkDeviceSize ViewOffset = Stride * InInfo.BufferSRV.FirstElement;
        const VkBuffer     Buffer     = VulkanBuffer->GetBindVkBuffer();
        const VkDeviceSize Offset     = VulkanBuffer->GetBindOffset() + ViewOffset;
		const VkDeviceSize Range      = Stride * InInfo.BufferSRV.NumElements;

		if (!InitializeStructuredBufferView(Buffer, Offset, Range, ViewOffset))
		{
			return false;
		}

		RegisterWithResource(VulkanBuffer);
		return true;
    }
    else if (InInfo.IsTextureSRV())
    {
		FVulkanTexture* VulkanTexture = FVulkanTexture::Cast(InInfo.TextureSRV.Texture);
		if (!VulkanTexture)
		{
			VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
			return false;
		}

		if (IsTypelessFormat(InInfo.TextureSRV.Format))
		{
			VULKAN_ERROR_CRITICAL("Cannot create a view of a typeless format");
			return false;
		}

		VkImageViewType VulkanImageType;
		switch (VulkanTexture->GetDimension())
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

		uint32 LayerCount;
		uint32 BaseArrayLayer;
		if (IsTextureCube(VulkanTexture->GetDimension()))
		{
			BaseArrayLayer = InInfo.TextureSRV.FirstArraySlice * RHI_NUM_CUBE_FACES;
			LayerCount     = Math::Max<uint16>(InInfo.TextureSRV.NumSlices, 1u) * RHI_NUM_CUBE_FACES;
		}
		else
		{
			BaseArrayLayer = InInfo.TextureSRV.FirstArraySlice;
			LayerCount     = Math::Max<uint16>(InInfo.TextureSRV.NumSlices, 1u);
		}

		// NOTE: We need to read the format from the texture, otherwise we need the MUTABLE flag on the texture
		const VkFormat           VulkanFormat     = VulkanTexture->GetVkFormat();
		const VkImage            Image            = VulkanTexture->GetVkImage();
		const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

		if (InitializeImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, InInfo.TextureSRV.FirstMipLevel, InInfo.TextureSRV.NumMips))
		{
			const FString TextureDebugName = VulkanTexture->GetDebugName();
			if (!TextureDebugName.IsEmpty())
			{
				SetDebugName(TextureDebugName + " ImageView SRV");
			}

			RegisterWithResource(VulkanTexture);
			return true;
		}
		else
		{
			return false;
		}
    }
    else
    {
        return false;
    }
}

FVulkanUnorderedAccessView::FVulkanUnorderedAccessView(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIUnorderedAccessView(InResource)
    , FVulkanResourceView(InDevice)
{
}

void FVulkanUnorderedAccessView::OnResourceRelocated(FVulkanGenericResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage)
{
    FVulkanResourceView::OnResourceRelocated(RelocatedResource, NewMemoryStorage);

    if (NewMemoryStorage)
    {
        if (Type == EType::ImageView)
        {
            FVulkanTexture* VulkanTexture = static_cast<FVulkanTexture*>(RelocatedResource);
            
            if (VULKAN_CHECK_HANDLE(ImageViewInfo.ImageView))
            {
                GetDevice()->GetRenderPassCache().OnReleaseImageView(ImageViewInfo.ImageView);
                vkDestroyImageView(GetDevice()->GetVkDevice(), ImageViewInfo.ImageView, nullptr);
                ImageViewInfo.ImageView = VK_NULL_HANDLE;
            }

            InitializeImageView(
                VulkanTexture->GetVkImage(),
                ImageViewInfo.Format,
                ImageViewInfo.ImageViewType,
                ImageViewInfo.SubresourceRange.aspectMask,
                ImageViewInfo.SubresourceRange.baseArrayLayer,
                ImageViewInfo.SubresourceRange.layerCount,
                ImageViewInfo.SubresourceRange.baseMipLevel,
                ImageViewInfo.SubresourceRange.levelCount);
        }
        else if (Type == EType::StructuredBufferView)
        {
            FVulkanBuffer* VulkanBuffer = static_cast<FVulkanBuffer*>(RelocatedResource);
            StructuredBufferInfo.Buffer = VulkanBuffer->GetBindVkBuffer();
            StructuredBufferInfo.Offset = VulkanBuffer->GetBindOffset() + StructuredBufferInfo.ViewOffset;
        }
    }
}

bool FVulkanUnorderedAccessView::Initialize(const FRHIUnorderedAccessViewInfo& InInfo)
{
	if (InInfo.IsBufferUAV())
	{
		FVulkanBuffer* VulkanBuffer = FVulkanBuffer::Cast(InInfo.BufferUAV.Buffer);
		if (!VulkanBuffer)
		{
			VULKAN_ERROR_CRITICAL("Buffer cannot be nullptr");
			return false;
		}

		// TODO: Use typed buffers

		VkDeviceSize Stride = 0;
		if (InInfo.BufferUAV.Format == EBufferUAVFormat::None)
		{
			Stride = VulkanBuffer->GetInfo().Stride;
		}
		else if (InInfo.BufferUAV.Format == EBufferUAVFormat::UInt32)
		{
			Stride = sizeof(uint32);
		}

		const VkDeviceSize ViewOffset = Stride * InInfo.BufferUAV.FirstElement;
		const VkBuffer     Buffer     = VulkanBuffer->GetBindVkBuffer();
		const VkDeviceSize Offset     = VulkanBuffer->GetBindOffset() + ViewOffset;
		const VkDeviceSize Range      = Stride * InInfo.BufferUAV.NumElements;

		if (!InitializeStructuredBufferView(Buffer, Offset, Range, ViewOffset))
		{
			return false;
		}

		RegisterWithResource(VulkanBuffer);
		return true;
	}
	else if (InInfo.IsTextureUAV())
	{
		FVulkanTexture* VulkanTexture = FVulkanTexture::Cast(InInfo.TextureUAV.Texture);
		if (!VulkanTexture)
		{
			VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
			return false;
		}

		if (IsTypelessFormat(InInfo.TextureUAV.Format))
		{
			VULKAN_ERROR_CRITICAL("Cannot create a view of a typeless format");
			return false;
		}

		VkImageViewType VulkanImageType;
		switch (VulkanTexture->GetDimension())
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

		uint32 LayerCount;
		uint32 BaseArrayLayer;
		if (IsTextureCube(VulkanTexture->GetDimension()))
		{
			BaseArrayLayer = InInfo.TextureUAV.FirstArraySlice * RHI_NUM_CUBE_FACES;
			LayerCount     = Math::Max<uint16>(InInfo.TextureUAV.NumSlices, 1u) * RHI_NUM_CUBE_FACES;
		}
		else
		{
			BaseArrayLayer = InInfo.TextureUAV.FirstArraySlice;
			LayerCount     = Math::Max<uint16>(InInfo.TextureUAV.NumSlices, 1u);
		}

		// NOTE: We need to read the format from the texture, otherwise we need the MUTABLE flag on the texture
		const VkFormat           VulkanFormat     = VulkanTexture->GetVkFormat();
		const VkImage            Image            = VulkanTexture->GetVkImage();
		const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

		if (InitializeImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, InInfo.TextureUAV.MipLevel, 1u))
		{
			const FString TextureDebugName = VulkanTexture->GetDebugName();
			if (!TextureDebugName.IsEmpty())
			{
				SetDebugName(TextureDebugName + " ImageView UAV");
			}

			RegisterWithResource(VulkanTexture);
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}
