#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanSwapChain.h"

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

void FVulkanResourceView::OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage)
{
    CHECK(RelocatedResource == OwnerResource);

    if (!NewMemoryStorage)
    {
        OwnerResource = nullptr;
    }
}

void FVulkanResourceView::RegisterToResource(FVulkanResource* InOwner)
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

    Type                            = EType::StructuredBufferView;
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

    Type                       = EType::TypedBufferView;
    TypedBufferInfo.Buffer     = InBuffer;
    TypedBufferInfo.Format     = InFormat;
    TypedBufferInfo.Range      = (InRange == 0) ? VK_WHOLE_SIZE : InRange;
    TypedBufferInfo.ViewOffset = InOffset;
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

FVulkanShaderResourceViewRHI::FVulkanShaderResourceViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIShaderResourceView(InResource)
    , FVulkanResourceView(InDevice)
{
}

FRHIDescriptorHandle FVulkanShaderResourceViewRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

void* FVulkanShaderResourceViewRHI::GetRHINativeHandle() const
{
    return GetRHINativeHandleForType();
}

void FVulkanShaderResourceViewRHI::OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage)
{
    FVulkanResourceView::OnResourceRelocated(RelocatedResource, NewMemoryStorage);

    if (NewMemoryStorage)
    {
        if (Type == EType::ImageView)
        {
            FVulkanTextureRHI* VulkanTexture = static_cast<FVulkanTextureRHI*>(RelocatedResource);
            
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
            FVulkanBufferRHI* VulkanBuffer = static_cast<FVulkanBufferRHI*>(RelocatedResource);
            StructuredBufferInfo.Buffer = VulkanBuffer->GetBindVkBuffer();
            StructuredBufferInfo.Offset = VulkanBuffer->GetBindOffset() + StructuredBufferInfo.ViewOffset;
        }
        else if (Type == EType::TypedBufferView)
        {
            FVulkanBufferRHI* VulkanBuffer = static_cast<FVulkanBufferRHI*>(RelocatedResource);

            if (VULKAN_CHECK_HANDLE(TypedBufferInfo.BufferView))
            {
                vkDestroyBufferView(GetDevice()->GetVkDevice(), TypedBufferInfo.BufferView, nullptr);
                TypedBufferInfo.BufferView = VK_NULL_HANDLE;
            }

            const VkDeviceSize Offset = VulkanBuffer->GetBindOffset() + TypedBufferInfo.ViewOffset;
            InitializeTypedBufferView(VulkanBuffer->GetBindVkBuffer(), TypedBufferInfo.Format, Offset, TypedBufferInfo.Range);
        }
    }
}

bool FVulkanShaderResourceViewRHI::Initialize(const FRHIShaderResourceViewDesc& InDesc)
{
    if (InDesc.IsBufferSRV())
    {
		FVulkanBufferRHI* VulkanBuffer = FVulkanRHI::ResourceCast(InDesc.BufferSRV.Buffer);
		if (!VulkanBuffer)
		{
			VULKAN_ERROR_CRITICAL("Buffer cannot be nullptr");
			return false;
		}

		const VkBuffer Buffer = VulkanBuffer->GetBindVkBuffer();

		if (InDesc.BufferSRV.Format != EBufferSRVFormat::None)
		{
			VkFormat     VulkanFormat = VK_FORMAT_UNDEFINED;
			VkDeviceSize ElementSize  = 0;
			if (InDesc.BufferSRV.Format == EBufferSRVFormat::UInt32)
			{
				VulkanFormat = VK_FORMAT_R32_UINT;
				ElementSize  = sizeof(uint32);
			}

			const VkDeviceSize ViewOffset = ElementSize * InDesc.BufferSRV.FirstElement;
			const VkDeviceSize Range      = ElementSize * InDesc.BufferSRV.NumElements;
			const VkDeviceSize Offset     = VulkanBuffer->GetBindOffset() + ViewOffset;

			if (!InitializeTypedBufferView(Buffer, VulkanFormat, Offset, Range))
			{
				return false;
			}
		}
		else
		{
			const VkDeviceSize Stride     = VulkanBuffer->GetDesc().Stride;
			const VkDeviceSize ViewOffset = Stride * InDesc.BufferSRV.FirstElement;
			const VkDeviceSize Range      = Stride * InDesc.BufferSRV.NumElements;
			const VkDeviceSize Offset     = VulkanBuffer->GetBindOffset() + ViewOffset;

			if (!InitializeStructuredBufferView(Buffer, Offset, Range, ViewOffset))
			{
				return false;
			}
		}

		RegisterToResource(VulkanBuffer);
		return true;
    }
    else if (InDesc.IsTextureSRV())
    {
		FVulkanTextureRHI* VulkanTexture = FVulkanRHI::ResourceCast(InDesc.TextureSRV.Texture);
		if (!VulkanTexture)
		{
			VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
			return false;
		}

		if (IsTypelessFormat(InDesc.TextureSRV.Format))
		{
			VULKAN_ERROR_CRITICAL("Cannot create a view of a typeless format");
			return false;
		}

		VkImageViewType VulkanImageType;
		switch (VulkanTexture->GetDimension())
		{
			case ETextureDimension::Texture1D:
			{
				VulkanImageType = VK_IMAGE_VIEW_TYPE_1D;
				break;
			}
			case ETextureDimension::Texture1DArray:
			{
				VulkanImageType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
				break;
			}
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
			BaseArrayLayer = InDesc.TextureSRV.FirstArraySlice * RHI_NUM_CUBE_FACES;
			LayerCount     = Math::Max<uint16>(InDesc.TextureSRV.NumSlices, 1u) * RHI_NUM_CUBE_FACES;
		}
		else
		{
			BaseArrayLayer = InDesc.TextureSRV.FirstArraySlice;
			LayerCount     = Math::Max<uint16>(InDesc.TextureSRV.NumSlices, 1u);
		}

		const VkFormat           VulkanFormat     = VulkanTexture->GetVkFormat();
		const VkImage            Image            = VulkanTexture->GetVkImage();
		const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

		if (InitializeImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, InDesc.TextureSRV.FirstMipLevel, InDesc.TextureSRV.NumMips))
		{
			FString TextureDebugName;
			VulkanTexture->GetDebugName(TextureDebugName);
			if (!TextureDebugName.IsEmpty())
			{
				SetDebugName(TextureDebugName + " ImageView SRV");
			}

			RegisterToResource(VulkanTexture);
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

FVulkanUnorderedAccessViewRHI::FVulkanUnorderedAccessViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIUnorderedAccessView(InResource)
    , FVulkanResourceView(InDevice)
{
}

FRHIDescriptorHandle FVulkanUnorderedAccessViewRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

void* FVulkanUnorderedAccessViewRHI::GetRHINativeHandle() const
{
    return GetRHINativeHandleForType();
}

void FVulkanUnorderedAccessViewRHI::OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage)
{
    FVulkanResourceView::OnResourceRelocated(RelocatedResource, NewMemoryStorage);

    if (NewMemoryStorage)
    {
        if (Type == EType::ImageView)
        {
            FVulkanTextureRHI* VulkanTexture = static_cast<FVulkanTextureRHI*>(RelocatedResource);
            
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
            FVulkanBufferRHI* VulkanBuffer = static_cast<FVulkanBufferRHI*>(RelocatedResource);
            StructuredBufferInfo.Buffer = VulkanBuffer->GetBindVkBuffer();
            StructuredBufferInfo.Offset = VulkanBuffer->GetBindOffset() + StructuredBufferInfo.ViewOffset;
        }
        else if (Type == EType::TypedBufferView)
        {
            FVulkanBufferRHI* VulkanBuffer = static_cast<FVulkanBufferRHI*>(RelocatedResource);

            if (VULKAN_CHECK_HANDLE(TypedBufferInfo.BufferView))
            {
                vkDestroyBufferView(GetDevice()->GetVkDevice(), TypedBufferInfo.BufferView, nullptr);
                TypedBufferInfo.BufferView = VK_NULL_HANDLE;
            }

            const VkDeviceSize Offset = VulkanBuffer->GetBindOffset() + TypedBufferInfo.ViewOffset;
            InitializeTypedBufferView(VulkanBuffer->GetBindVkBuffer(), TypedBufferInfo.Format, Offset, TypedBufferInfo.Range);
        }
    }
}

bool FVulkanUnorderedAccessViewRHI::Initialize(const FRHIUnorderedAccessViewDesc& InDesc)
{
	if (InDesc.IsBufferUAV())
	{
		FVulkanBufferRHI* VulkanBuffer = FVulkanRHI::ResourceCast(InDesc.BufferUAV.Buffer);
		if (!VulkanBuffer)
		{
			VULKAN_ERROR_CRITICAL("Buffer cannot be nullptr");
			return false;
		}

		const VkBuffer Buffer = VulkanBuffer->GetBindVkBuffer();

		if (InDesc.BufferUAV.Format != EBufferUAVFormat::None)
		{
			VkFormat     VulkanFormat = VK_FORMAT_UNDEFINED;
			VkDeviceSize ElementSize  = 0;
			if (InDesc.BufferUAV.Format == EBufferUAVFormat::UInt32)
			{
				VulkanFormat = VK_FORMAT_R32_UINT;
				ElementSize  = sizeof(uint32);
			}

			const VkDeviceSize ViewOffset = ElementSize * InDesc.BufferUAV.FirstElement;
			const VkDeviceSize Range      = ElementSize * InDesc.BufferUAV.NumElements;
			const VkDeviceSize Offset     = VulkanBuffer->GetBindOffset() + ViewOffset;

			if (!InitializeTypedBufferView(Buffer, VulkanFormat, Offset, Range))
			{
				return false;
			}
		}
		else
		{
			const VkDeviceSize Stride     = VulkanBuffer->GetDesc().Stride;
			const VkDeviceSize ViewOffset = Stride * InDesc.BufferUAV.FirstElement;
			const VkDeviceSize Range      = Stride * InDesc.BufferUAV.NumElements;
			const VkDeviceSize Offset     = VulkanBuffer->GetBindOffset() + ViewOffset;

			if (!InitializeStructuredBufferView(Buffer, Offset, Range, ViewOffset))
			{
				return false;
			}
		}

		RegisterToResource(VulkanBuffer);
		return true;
	}
	else if (InDesc.IsTextureUAV())
	{
		FVulkanTextureRHI* VulkanTexture = FVulkanRHI::ResourceCast(InDesc.TextureUAV.Texture);
		if (!VulkanTexture)
		{
			VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
			return false;
		}

		if (IsTypelessFormat(InDesc.TextureUAV.Format))
		{
			VULKAN_ERROR_CRITICAL("Cannot create a view of a typeless format");
			return false;
		}

		VkImageViewType VulkanImageType;
		switch (VulkanTexture->GetDimension())
		{
			case ETextureDimension::Texture1D:
			{
				VulkanImageType = VK_IMAGE_VIEW_TYPE_1D;
				break;
			}
			
			case ETextureDimension::Texture1DArray:
			{
				VulkanImageType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
				break;
			}
			
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
			BaseArrayLayer = InDesc.TextureUAV.FirstArraySlice * RHI_NUM_CUBE_FACES;
			LayerCount     = Math::Max<uint16>(InDesc.TextureUAV.NumSlices, 1u) * RHI_NUM_CUBE_FACES;
		}
		else
		{
			BaseArrayLayer = InDesc.TextureUAV.FirstArraySlice;
			LayerCount     = Math::Max<uint16>(InDesc.TextureUAV.NumSlices, 1u);
		}

		const VkFormat           VulkanFormat     = VulkanTexture->GetVkFormat();
		const VkImage            Image            = VulkanTexture->GetVkImage();
		const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

		if (InitializeImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, InDesc.TextureUAV.MipLevel, 1u))
		{
			FString TextureDebugName;
			VulkanTexture->GetDebugName(TextureDebugName);
			if (!TextureDebugName.IsEmpty())
			{
				SetDebugName(TextureDebugName + " ImageView UAV");
			}

			RegisterToResource(VulkanTexture);
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

FVulkanRenderTargetViewRHI::FVulkanRenderTargetViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FVulkanRenderTargetViewBase(InResource)
    , FVulkanResourceView(InDevice)
{
}

void* FVulkanRenderTargetViewRHI::GetRHINativeHandle() const
{
    return GetRHINativeHandleForType();
}

FVulkanRenderTargetViewRHI* FVulkanRenderTargetViewRHI::GetRenderTargetViewInterface() const
{
    return const_cast<FVulkanRenderTargetViewRHI*>(this);
}

void FVulkanRenderTargetViewRHI::OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage)
{
    FVulkanResourceView::OnResourceRelocated(RelocatedResource, NewMemoryStorage);

    if (NewMemoryStorage)
    {
        FVulkanTextureRHI* VulkanTexture = static_cast<FVulkanTextureRHI*>(RelocatedResource);

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
        IncrementDescriptorVersion();
    }
}

bool FVulkanRenderTargetViewRHI::Initialize(const FRHIRenderTargetViewDesc& InDesc)
{
    FVulkanTextureRHI* VulkanTexture = FVulkanRHI::ResourceCast(InDesc.Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
        return false;
    }

    const VkImage Image = VulkanTexture->GetVkImage();
    if (!VULKAN_CHECK_HANDLE(Image))
    {
        VULKAN_WARNING("Texture does not have a valid Image");
        return false;
    }

    VkImageViewType VulkanImageType;
    switch (VulkanTexture->GetDimension())
    {
        case ETextureDimension::Texture1D:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_1D;
            break;
        }

        case ETextureDimension::Texture1DArray:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            break;
        }

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

    const VkFormat           VulkanFormat     = ConvertFormat(InDesc.Format);
    const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

    constexpr uint32 NumMipLevels = 1;
    const uint32 LayerCount       = Math::Max<uint16>(InDesc.NumArraySlices, 1u);
    const uint32 BaseArrayLayer   = InDesc.ArrayIndex;

    if (!InitializeImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, InDesc.MipLevel, NumMipLevels))
    {
        return false;
    }

    FString TextureDebugName;
    VulkanTexture->GetDebugName(TextureDebugName);
    if (!TextureDebugName.IsEmpty())
    {
        SetDebugName(TextureDebugName + " ImageView RTV");
    }

    RegisterToResource(VulkanTexture);
    return true;
}

FVulkanDepthStencilViewRHI::FVulkanDepthStencilViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIDepthStencilView(InResource)
    , FVulkanResourceView(InDevice)
{
}

void* FVulkanDepthStencilViewRHI::GetRHINativeHandle() const
{
    return GetRHINativeHandleForType();
}

void FVulkanDepthStencilViewRHI::OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage)
{
    FVulkanResourceView::OnResourceRelocated(RelocatedResource, NewMemoryStorage);

    if (NewMemoryStorage)
    {
        FVulkanTextureRHI* VulkanTexture = static_cast<FVulkanTextureRHI*>(RelocatedResource);

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
        IncrementDescriptorVersion();
    }
}

bool FVulkanDepthStencilViewRHI::Initialize(const FRHIDepthStencilViewDesc& InDesc)
{
    FVulkanTextureRHI* VulkanTexture = FVulkanRHI::ResourceCast(InDesc.Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
        return false;
    }

    const VkImage Image = VulkanTexture->GetVkImage();
    if (!VULKAN_CHECK_HANDLE(Image))
    {
        VULKAN_WARNING("Texture does not have a valid Image");
        return false;
    }

    VkImageViewType VulkanImageType;
    switch (VulkanTexture->GetDimension())
    {
        case ETextureDimension::Texture1D:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_1D;
            break;
        }

        case ETextureDimension::Texture1DArray:
        {
            VulkanImageType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            break;
        }

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

    const VkFormat           VulkanFormat     = ConvertFormat(InDesc.Format);
    const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

    constexpr uint32 NumMipLevels = 1;
    const uint32 LayerCount       = Math::Max<uint16>(InDesc.NumArraySlices, 1u);
    const uint32 BaseArrayLayer   = InDesc.ArrayIndex;

    if (!InitializeImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, InDesc.MipLevel, NumMipLevels))
    {
        return false;
    }

    FString TextureDebugName;
    VulkanTexture->GetDebugName(TextureDebugName);
    if (!TextureDebugName.IsEmpty())
    {
        SetDebugName(TextureDebugName + " ImageView DSV");
    }

    RegisterToResource(VulkanTexture);
    return true;
}

FVulkanBackBufferProxyRenderTargetViewRHI::FVulkanBackBufferProxyRenderTargetViewRHI(FVulkanSwapChainRHI* InSwapChain, FVulkanBackBufferProxyTextureRHI* InProxyTexture)
    : FVulkanRenderTargetViewBase(InProxyTexture)
    , SwapChain(InSwapChain)
{
}

FVulkanBackBufferProxyRenderTargetViewRHI::~FVulkanBackBufferProxyRenderTargetViewRHI()
{
    SwapChain = nullptr;
}

FVulkanRenderTargetViewRHI* FVulkanBackBufferProxyRenderTargetViewRHI::GetRenderTargetViewInterface() const
{
    return SwapChain ? SwapChain->GetCurrentBackBufferRenderTargetView() : nullptr;
}

void* FVulkanBackBufferProxyRenderTargetViewRHI::GetRHINativeHandle() const
{
    FVulkanRenderTargetViewRHI* CurrentRTV = GetRenderTargetViewInterface();
    return CurrentRTV ? CurrentRTV->GetRHINativeHandle() : nullptr;
}
