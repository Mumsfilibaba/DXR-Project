#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "RHI/RHIRayTracing.h"

FVulkanResourceView::FVulkanResourceView(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Type(EType::None)
    , OwnerResource(nullptr)
    , DescriptorVersion(0)
    , BindlessHandle()
    , bBindlessIsWritable(false)
{
    Memory::Memzero(&ImageViewInfo);
}

FVulkanResourceView::~FVulkanResourceView()
{
    UnregisterFromResource();
    FreeBindlessHandle();

    if (Type == EType::ImageView)
    {
        if (VULKAN_CHECK_HANDLE(ImageViewInfo.ImageView))
        {
        #if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
            GetDevice()->GetRenderPassCache().OnReleaseImageView(ImageViewInfo.ImageView);
        #endif
            vkDestroyImageView(GetDevice()->GetVkDevice(), ImageViewInfo.ImageView, nullptr);
        }

        ImageViewInfo.Image            = VK_NULL_HANDLE;
        ImageViewInfo.ImageView        = VK_NULL_HANDLE;
        ImageViewInfo.Format           = VK_FORMAT_UNDEFINED;
        ImageViewInfo.Flags            = 0;
        ImageViewInfo.SubresourceRange = { };
    }
    else if (Type == EType::TypedBufferView)
    {
        if (VULKAN_CHECK_HANDLE(TypedBufferInfo.BufferView))
        {
            vkDestroyBufferView(GetDevice()->GetVkDevice(), TypedBufferInfo.BufferView, nullptr);
        }

        TypedBufferInfo.Buffer     = VK_NULL_HANDLE;
        TypedBufferInfo.BufferView = VK_NULL_HANDLE;
    }
    else if (Type == EType::StructuredBufferView)
    {
        StructuredBufferInfo.Buffer = VK_NULL_HANDLE;
        StructuredBufferInfo.Offset = 0;
        StructuredBufferInfo.Range  = 0;
    }
    else if (Type == EType::AccelerationStructureView)
    {
        AccelerationStructureInfo.AccelerationStructure = VK_NULL_HANDLE;
    }
}

void FVulkanResourceView::OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryLocation* NewMemoryLocation)
{
    CHECK(RelocatedResource == OwnerResource);

    if (!NewMemoryLocation)
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

    Type                                          = EType::ImageView;
    ImageViewInfo.Image                           = InImage;
    ImageViewInfo.Format                          = InFormat;
    ImageViewInfo.ImageViewType                   = InImageViewType;
    ImageViewInfo.SubresourceRange.aspectMask     = InAspectMask;
    ImageViewInfo.SubresourceRange.baseArrayLayer = InBaseArrayLayer;
    ImageViewInfo.SubresourceRange.layerCount     = InLayerCount;
    ImageViewInfo.SubresourceRange.baseMipLevel   = InBaseMipLevel;
    ImageViewInfo.SubresourceRange.levelCount     = InLevelCount;
    
    IncrementDescriptorVersion();
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

    IncrementDescriptorVersion();
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

    IncrementDescriptorVersion();
    return true;
}

bool FVulkanResourceView::InitializeAccelerationStructureView(VkAccelerationStructureKHR InAccelerationStructure)
{
    if (!VULKAN_CHECK_HANDLE(InAccelerationStructure))
    {
        VULKAN_ERROR_CRITICAL("AccelerationStructure cannot be NULL");
        return false;
    }

    Type                                            = EType::AccelerationStructureView;
    AccelerationStructureInfo.AccelerationStructure = InAccelerationStructure;

    IncrementDescriptorVersion();
    return true;
}

FRHIDescriptorHandle FVulkanResourceView::EnsureBindlessHandle(EDescriptorType InType, bool bWritable) const
{
    FVulkanBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();
    if (!BindlessManager || !BindlessManager->IsEnabled())
    {
        return FRHIDescriptorHandle();
    }

    if (BindlessHandle.IsValid())
    {
        return BindlessHandle;
    }

    if (Type == EType::None)
    {
        // The view has not been initialized yet -- nothing to mirror.
        return FRHIDescriptorHandle();
    }

    BindlessHandle = BindlessManager->Allocate(InType);
    if (!BindlessHandle.IsValid())
    {
        return FRHIDescriptorHandle();
    }

    bBindlessIsWritable = bWritable;
    switch (Type)
    {
        case EType::ImageView:
        {
            const VkImageLayout Layout = bWritable
                ? VK_IMAGE_LAYOUT_GENERAL
                : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            const VkDescriptorType DescriptorType = bWritable
                ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

            BindlessManager->EnqueueImageWrite(BindlessHandle, ImageViewInfo.ImageView, Layout, DescriptorType);
            break;
        }

        case EType::StructuredBufferView:
        {
            BindlessManager->EnqueueBufferWrite(BindlessHandle, StructuredBufferInfo.Buffer,
                StructuredBufferInfo.Offset, StructuredBufferInfo.Range, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            break;
        }

        case EType::TypedBufferView:
        {
            const VkDescriptorType DescriptorType = bWritable
                ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
                : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

            BindlessManager->EnqueueTexelBufferWrite(BindlessHandle, TypedBufferInfo.BufferView, DescriptorType);
            break;
        }
        
        case EType::AccelerationStructureView:
        {
            BindlessManager->EnqueueAccelerationStructureWrite(BindlessHandle, AccelerationStructureInfo.AccelerationStructure);
            break;
        }

        default:
        {
            break;
        }
    }

    return BindlessHandle;
}

void FVulkanResourceView::RefreshBindlessIfBound()
{
    if (!BindlessHandle.IsValid())
    {
        return;
    }

    FVulkanBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();
    if (!BindlessManager || !BindlessManager->IsEnabled())
    {
        return;
    }

    switch (Type)
    {
        case EType::ImageView:
        {
            const VkImageLayout Layout = bBindlessIsWritable
                ? VK_IMAGE_LAYOUT_GENERAL
                : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            const VkDescriptorType DescriptorType = bBindlessIsWritable
                ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

            BindlessManager->EnqueueImageWrite(BindlessHandle, ImageViewInfo.ImageView, Layout, DescriptorType);
            break;
        }
        
        case EType::StructuredBufferView:
        {
            BindlessManager->EnqueueBufferWrite(BindlessHandle, StructuredBufferInfo.Buffer,
                StructuredBufferInfo.Offset, StructuredBufferInfo.Range, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            break;
        }
        
        case EType::TypedBufferView:
        {
            const VkDescriptorType DescriptorType = bBindlessIsWritable
                ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
                : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

            BindlessManager->EnqueueTexelBufferWrite(BindlessHandle, TypedBufferInfo.BufferView, DescriptorType);
            break;
        }
        
        case EType::AccelerationStructureView:
        {
            BindlessManager->EnqueueAccelerationStructureWrite(BindlessHandle, AccelerationStructureInfo.AccelerationStructure);
            break;
        }

        default:
        {
            break;
        }
    }
}

void FVulkanResourceView::FreeBindlessHandle()
{
    if (!BindlessHandle.IsValid())
    {
        return;
    }

    if (FVulkanBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager())
    {
        BindlessManager->Free(BindlessHandle);
    }

    BindlessHandle      = FRHIDescriptorHandle();
    bBindlessIsWritable = false;
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

FVulkanShaderResourceViewRHI::FVulkanShaderResourceViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc)
    : FRHIShaderResourceView(InResource, InRHIDesc)
    , FVulkanResourceView(InDevice)
{
}

FRHIDescriptorHandle FVulkanShaderResourceViewRHI::GetBindlessHandle() const
{
    return EnsureBindlessHandle(EDescriptorType::ShaderResource, /*bWritable=*/ false);
}

void* FVulkanShaderResourceViewRHI::GetRHINativeHandle() const
{
    return GetRHINativeHandleForType();
}

void FVulkanShaderResourceViewRHI::OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryLocation* NewMemoryLocation)
{
    FVulkanResourceView::OnResourceRelocated(RelocatedResource, NewMemoryLocation);

    if (NewMemoryLocation)
    {
        if (Type == EType::ImageView)
        {
            FVulkanTextureRHI* VulkanTexture = static_cast<FVulkanTextureRHI*>(RelocatedResource);
            if (VULKAN_CHECK_HANDLE(ImageViewInfo.ImageView))
            {
            #if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
                GetDevice()->GetRenderPassCache().OnReleaseImageView(ImageViewInfo.ImageView);
            #endif
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
            IncrementDescriptorVersion();
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

bool FVulkanShaderResourceViewRHI::Initialize(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
{
    if (InDesc.IsBufferSRV())
    {
        FVulkanBufferRHI* VulkanBuffer = FVulkanDeviceRHI::ResourceCast(static_cast<FRHIBuffer*>(InResource));
        if (!VulkanBuffer)
        {
            VULKAN_ERROR_CRITICAL("Buffer cannot be nullptr");
            return false;
        }

        const auto& BufferDesc = InDesc.Buffer;
        if (BufferDesc.Type == EBufferViewType::ByteAddress)
        {
            const VkFormat     VulkanFormat = VK_FORMAT_R32_UINT;
            const VkDeviceSize ElementSize  = sizeof(uint32);
            const VkDeviceSize ViewOffset   = ElementSize * BufferDesc.FirstElement;
            const VkDeviceSize Range        = ElementSize * BufferDesc.NumElements;
            const VkDeviceSize Offset       = VulkanBuffer->GetBindOffset() + ViewOffset;

            if (!InitializeTypedBufferView(VulkanBuffer->GetBindVkBuffer(), VulkanFormat, Offset, Range))
            {
                return false;
            }
        }
        else
        {
            const VkDeviceSize Stride     = VulkanBuffer->GetDesc().Stride;
            const VkDeviceSize ViewOffset = Stride * BufferDesc.FirstElement;
            const VkDeviceSize Range      = Stride * BufferDesc.NumElements;
            const VkDeviceSize Offset     = VulkanBuffer->GetBindOffset() + ViewOffset;

            if (!InitializeStructuredBufferView(VulkanBuffer->GetBindVkBuffer(), Offset, Range, ViewOffset))
            {
                return false;
            }
        }

        RegisterToResource(VulkanBuffer);
        return true;
    }
    else if (InDesc.IsTextureSRV())
    {
        FVulkanTextureRHI* VulkanTexture = FVulkanDeviceRHI::ResourceCast(static_cast<FRHITexture*>(InResource));
        if (!VulkanTexture)
        {
            VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
            return false;
        }

        VkImageViewType  VulkanImageType  = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        EFormat          ViewFormat       = EFormat::Unknown;
        uint8            FirstMipLevel    = 0;
        uint8            NumMips          = 1;
        uint32           BaseArrayLayer   = 0;
        uint32           LayerCount       = 1;

        switch (InDesc.ViewDimension)
        {
            case EViewDimension::Texture1D:
            {
                const auto& TextureDesc = InDesc.Texture1D;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_1D;
                ViewFormat      = TextureDesc.Format;
                FirstMipLevel   = TextureDesc.FirstMipLevel;
                NumMips         = TextureDesc.NumMips;
                break;
            }

            case EViewDimension::Texture1DArray:
            {
                const auto& TextureDesc = InDesc.Texture1DArray;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                ViewFormat      = TextureDesc.Format;
                FirstMipLevel   = TextureDesc.FirstMipLevel;
                NumMips         = TextureDesc.NumMips;
                BaseArrayLayer  = TextureDesc.FirstArraySlice;
                LayerCount      = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                break;
            }

            case EViewDimension::Texture2D:
            {
                const auto& TextureDesc = InDesc.Texture2D;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_2D;
                ViewFormat      = TextureDesc.Format;
                FirstMipLevel   = TextureDesc.FirstMipLevel;
                NumMips         = TextureDesc.NumMips;
                break;
            }

            case EViewDimension::Texture2DArray:
            {
                const auto& TextureDesc = InDesc.Texture2DArray;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                ViewFormat      = TextureDesc.Format;
                FirstMipLevel   = TextureDesc.FirstMipLevel;
                NumMips         = TextureDesc.NumMips;
                BaseArrayLayer  = TextureDesc.FirstArraySlice;
                LayerCount      = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                break;
            }

            case EViewDimension::TextureCube:
            {
                const auto& TextureDesc = InDesc.TextureCube;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_CUBE;
                ViewFormat      = TextureDesc.Format;
                FirstMipLevel   = TextureDesc.FirstMipLevel;
                NumMips         = TextureDesc.NumMips;
                BaseArrayLayer  = 0;
                LayerCount      = RHI_NUM_CUBE_FACES;
                break;
            }

            case EViewDimension::TextureCubeArray:
            {
                const auto& TextureDesc = InDesc.TextureCubeArray;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                ViewFormat      = TextureDesc.Format;
                FirstMipLevel   = TextureDesc.FirstMipLevel;
                NumMips         = TextureDesc.NumMips;
                BaseArrayLayer  = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
                LayerCount      = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, Math::Max<uint16>(TextureDesc.NumCubes, 1u));
                break;
            }

            case EViewDimension::Texture3D:
            {
                const auto& TextureDesc = InDesc.Texture3D;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_3D;
                ViewFormat      = TextureDesc.Format;
                FirstMipLevel   = TextureDesc.FirstMipLevel;
                NumMips         = TextureDesc.NumMips;
                break;
            }

            default:
            {
                VULKAN_ERROR_CRITICAL("Unsupported texture ViewDimension for SRV");
                return false;
            }
        }

        if (IsTypelessFormat(ViewFormat))
        {
            VULKAN_ERROR_CRITICAL("Cannot create a view of a typeless format");
            return false;
        }

        const VkImage            Image            = VulkanTexture->GetVkImage();
        const VkFormat           ImageFormat      = VulkanTexture->GetVkFormat();
        const VkFormat           VulkanFormat     = ConvertFormat(ViewFormat);
        const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

        if (ImageFormat != VK_FORMAT_UNDEFINED && ImageFormat != VulkanFormat)
        {
            if (!IsFormatInCompatibilityClass(ImageFormat, VulkanFormat))
            {
                VULKAN_ERROR_CRITICAL("Cannot create SRV with format '%s' on image with format '%s' (different compatibility class)",
                    ToString(VulkanFormat), ToString(ImageFormat));
                return false;
            }

            if ((VulkanTexture->GetVkImageCreateInfo().flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) == 0)
            {
                VULKAN_ERROR_CRITICAL("Cannot create SRV with format '%s' on non-mutable image with format '%s' (declare the texture as a typeless EFormat to allow cross-format views)",
                    ToString(VulkanFormat), ToString(ImageFormat));
                return false;
            }
        }

        if (InitializeImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, FirstMipLevel, NumMips))
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
    else if (InDesc.IsAccelerationStructureSRV())
    {
        FRHISceneAccelerationStructure* SceneAS = static_cast<FRHISceneAccelerationStructure*>(InResource);
        if (!SceneAS)
        {
            VULKAN_ERROR_CRITICAL("AccelerationStructure cannot be nullptr");
            return false;
        }

        VkAccelerationStructureKHR Handle = reinterpret_cast<VkAccelerationStructureKHR>(SceneAS->GetRHINativeResource());
        return InitializeAccelerationStructureView(Handle);
    }
    else
    {
        return false;
    }
}

FVulkanUnorderedAccessViewRHI::FVulkanUnorderedAccessViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc)
    : FVulkanUnorderedAccessViewBase(InResource, InRHIDesc)
    , FVulkanResourceView(InDevice)
{
}

FVulkanUnorderedAccessViewRHI* FVulkanUnorderedAccessViewRHI::GetUnorderedAccessViewInterface() const
{
    return const_cast<FVulkanUnorderedAccessViewRHI*>(this);
}

FRHIDescriptorHandle FVulkanUnorderedAccessViewRHI::GetBindlessHandle() const
{
    return EnsureBindlessHandle(EDescriptorType::UnorderedAccess, /*bWritable=*/ true);
}

void* FVulkanUnorderedAccessViewRHI::GetRHINativeHandle() const
{
    return GetRHINativeHandleForType();
}

void FVulkanUnorderedAccessViewRHI::OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryLocation* NewMemoryLocation)
{
    FVulkanResourceView::OnResourceRelocated(RelocatedResource, NewMemoryLocation);

    if (NewMemoryLocation)
    {
        if (Type == EType::ImageView)
        {
            FVulkanTextureRHI* VulkanTexture = static_cast<FVulkanTextureRHI*>(RelocatedResource);
            if (VULKAN_CHECK_HANDLE(ImageViewInfo.ImageView))
            {
            #if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
                GetDevice()->GetRenderPassCache().OnReleaseImageView(ImageViewInfo.ImageView);
            #endif
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
            IncrementDescriptorVersion();
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

bool FVulkanUnorderedAccessViewRHI::Initialize(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
{
    if (InDesc.IsBufferUAV())
    {
        FVulkanBufferRHI* VulkanBuffer = FVulkanDeviceRHI::ResourceCast(static_cast<FRHIBuffer*>(InResource));
        if (!VulkanBuffer)
        {
            VULKAN_ERROR_CRITICAL("Buffer cannot be nullptr");
            return false;
        }

        const auto& BufferDesc = InDesc.Buffer;
        if (BufferDesc.Type == EBufferViewType::ByteAddress)
        {
            const VkFormat     VulkanFormat = VK_FORMAT_R32_UINT;
            const VkDeviceSize ElementSize  = sizeof(uint32);
            const VkDeviceSize ViewOffset   = ElementSize * BufferDesc.FirstElement;
            const VkDeviceSize Range        = ElementSize * BufferDesc.NumElements;
            const VkDeviceSize Offset       = VulkanBuffer->GetBindOffset() + ViewOffset;

            if (!InitializeTypedBufferView(VulkanBuffer->GetBindVkBuffer(), VulkanFormat, Offset, Range))
            {
                return false;
            }
        }
        else
        {
            const VkDeviceSize Stride     = VulkanBuffer->GetDesc().Stride;
            const VkDeviceSize ViewOffset = Stride * BufferDesc.FirstElement;
            const VkDeviceSize Range      = Stride * BufferDesc.NumElements;
            const VkDeviceSize Offset     = VulkanBuffer->GetBindOffset() + ViewOffset;

            if (!InitializeStructuredBufferView(VulkanBuffer->GetBindVkBuffer(), Offset, Range, ViewOffset))
            {
                return false;
            }
        }

        RegisterToResource(VulkanBuffer);
        return true;
    }
    else if (InDesc.IsTextureUAV())
    {
        FVulkanTextureRHI* VulkanTexture = FVulkanDeviceRHI::ResourceCast(static_cast<FRHITexture*>(InResource));
        if (!VulkanTexture)
        {
            VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
            return false;
        }

        VkImageViewType VulkanImageType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        EFormat         ViewFormat      = EFormat::Unknown;
        uint8           MipLevel        = 0;
        uint32          BaseArrayLayer  = 0;
        uint32          LayerCount      = 1;

        switch (InDesc.ViewDimension)
        {
            case EViewDimension::Texture1D:
            {
                const auto& TextureDesc = InDesc.Texture1D;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_1D;
                ViewFormat      = TextureDesc.Format;
                MipLevel        = TextureDesc.MipLevel;
                break;
            }

            case EViewDimension::Texture1DArray:
            {
                const auto& TextureDesc = InDesc.Texture1DArray;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                ViewFormat      = TextureDesc.Format;
                MipLevel        = TextureDesc.MipLevel;
                BaseArrayLayer  = TextureDesc.FirstArraySlice;
                LayerCount      = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                break;
            }

            case EViewDimension::Texture2D:
            {
                const auto& TextureDesc = InDesc.Texture2D;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_2D;
                ViewFormat      = TextureDesc.Format;
                MipLevel        = TextureDesc.MipLevel;
                break;
            }

            case EViewDimension::Texture2DArray:
            {
                const auto& TextureDesc = InDesc.Texture2DArray;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                ViewFormat      = TextureDesc.Format;
                MipLevel        = TextureDesc.MipLevel;
                BaseArrayLayer  = TextureDesc.FirstArraySlice;
                LayerCount      = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                break;
            }

            case EViewDimension::TextureCube:
            {
                const auto& TextureDesc = InDesc.TextureCube;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                ViewFormat      = TextureDesc.Format;
                MipLevel        = TextureDesc.MipLevel;
                BaseArrayLayer  = 0;
                LayerCount      = RHI_NUM_CUBE_FACES;
                break;
            }

            case EViewDimension::TextureCubeArray:
            {
                const auto& TextureDesc = InDesc.TextureCubeArray;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                ViewFormat      = TextureDesc.Format;
                MipLevel        = TextureDesc.MipLevel;
                BaseArrayLayer  = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
                LayerCount      = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, Math::Max<uint16>(TextureDesc.NumCubes, 1u));
                break;
            }

            case EViewDimension::Texture3D:
            {
                const auto& TextureDesc = InDesc.Texture3D;
                VulkanImageType = VK_IMAGE_VIEW_TYPE_3D;
                ViewFormat      = TextureDesc.Format;
                MipLevel        = TextureDesc.MipLevel;
                BaseArrayLayer  = TextureDesc.FirstWSlice;
                LayerCount      = Math::Max<uint16>(TextureDesc.WSize, 1u);
                break;
            }

            default:
            {
                VULKAN_ERROR_CRITICAL("Unsupported texture ViewDimension for UAV");
                return false;
            }
        }

        if (IsTypelessFormat(ViewFormat))
        {
            VULKAN_ERROR_CRITICAL("Cannot create a view of a typeless format");
            return false;
        }

        const VkImage            Image            = VulkanTexture->GetVkImage();
        const VkFormat           ImageFormat      = VulkanTexture->GetVkFormat();
        const VkFormat           VulkanFormat     = ConvertFormat(ViewFormat);
        const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

        if (ImageFormat != VK_FORMAT_UNDEFINED && ImageFormat != VulkanFormat)
        {
            if (!IsFormatInCompatibilityClass(ImageFormat, VulkanFormat))
            {
                VULKAN_ERROR_CRITICAL("Cannot create UAV with format '%s' on image with format '%s' (different compatibility class)",
                    ToString(VulkanFormat), ToString(ImageFormat));
                return false;
            }

            if ((VulkanTexture->GetVkImageCreateInfo().flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) == 0)
            {
                VULKAN_ERROR_CRITICAL("Cannot create UAV with format '%s' on non-mutable image with format '%s' (declare the texture as a typeless EFormat to allow cross-format views)",
                    ToString(VulkanFormat), ToString(ImageFormat));
                return false;
            }
        }

        if (InitializeImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, MipLevel, 1u))
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

FVulkanRenderTargetViewRHI::FVulkanRenderTargetViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource, const FRHIRenderTargetViewDesc& InRHIDesc)
    : FVulkanRenderTargetViewBase(InResource, InRHIDesc)
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

void FVulkanRenderTargetViewRHI::OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryLocation* NewMemoryLocation)
{
    FVulkanResourceView::OnResourceRelocated(RelocatedResource, NewMemoryLocation);

    if (NewMemoryLocation)
    {
        FVulkanTextureRHI* VulkanTexture = static_cast<FVulkanTextureRHI*>(RelocatedResource);

        if (VULKAN_CHECK_HANDLE(ImageViewInfo.ImageView))
        {
        #if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
            GetDevice()->GetRenderPassCache().OnReleaseImageView(ImageViewInfo.ImageView);
        #endif
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

bool FVulkanRenderTargetViewRHI::Initialize(FRHITexture* InTexture, const FRHIRenderTargetViewDesc& InDesc)
{
    FVulkanTextureRHI* VulkanTexture = FVulkanDeviceRHI::ResourceCast(InTexture);
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

    VkImageViewType VulkanImageType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    EFormat         ViewFormat      = EFormat::Unknown;
    uint8           MipLevel        = 0;
    uint32          BaseArrayLayer  = 0;
    uint32          LayerCount      = 1;

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
        {
            const auto& TextureDesc = InDesc.Texture1D;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_1D;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            break;
        }

        case EViewDimension::Texture1DArray:
        {
            const auto& TextureDesc = InDesc.Texture1DArray;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            BaseArrayLayer  = TextureDesc.FirstArraySlice;
            LayerCount      = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            break;
        }

        case EViewDimension::Texture2D:
        {
            const auto& TextureDesc = InDesc.Texture2D;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            break;
        }

        case EViewDimension::Texture2DArray:
        {
            const auto& TextureDesc = InDesc.Texture2DArray;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            BaseArrayLayer  = TextureDesc.FirstArraySlice;
            LayerCount      = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            break;
        }

        case EViewDimension::TextureCube:
        {
            const auto& TextureDesc = InDesc.TextureCube;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            BaseArrayLayer  = 0;
            LayerCount      = RHI_NUM_CUBE_FACES;
            break;
        }

        case EViewDimension::TextureCubeArray:
        {
            const auto& TextureDesc = InDesc.TextureCubeArray;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            BaseArrayLayer  = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
            LayerCount      = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, Math::Max<uint16>(TextureDesc.NumCubes, 1u));
            break;
        }

        case EViewDimension::Texture3D:
        {
            const auto& TextureDesc = InDesc.Texture3D;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_3D;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            BaseArrayLayer  = TextureDesc.FirstWSlice;
            LayerCount      = Math::Max<uint16>(TextureDesc.WSize, 1u);
            break;
        }

        default:
        {
            VULKAN_ERROR_CRITICAL("Unsupported ViewDimension for RTV");
            return false;
        }
    }

    const VkFormat           VulkanFormat     = ConvertFormat(ViewFormat);
    const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

    if (!InitializeImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, MipLevel, /*NumMips=*/1u))
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

FVulkanDepthStencilViewRHI::FVulkanDepthStencilViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource, const FRHIDepthStencilViewDesc& InRHIDesc)
    : FRHIDepthStencilView(InResource, InRHIDesc)
    , FVulkanResourceView(InDevice)
    , Flags(EDepthStencilViewFlags::None)
    , bHasStencil(false)
{
}

void* FVulkanDepthStencilViewRHI::GetRHINativeHandle() const
{
    return GetRHINativeHandleForType();
}

void FVulkanDepthStencilViewRHI::OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryLocation* NewMemoryLocation)
{
    FVulkanResourceView::OnResourceRelocated(RelocatedResource, NewMemoryLocation);

    if (NewMemoryLocation)
    {
        FVulkanTextureRHI* VulkanTexture = static_cast<FVulkanTextureRHI*>(RelocatedResource);

        if (VULKAN_CHECK_HANDLE(ImageViewInfo.ImageView))
        {
        #if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
            GetDevice()->GetRenderPassCache().OnReleaseImageView(ImageViewInfo.ImageView);
        #endif
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

bool FVulkanDepthStencilViewRHI::Initialize(FRHITexture* InTexture, const FRHIDepthStencilViewDesc& InDesc)
{
    FVulkanTextureRHI* VulkanTexture = FVulkanDeviceRHI::ResourceCast(InTexture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
        return false;
    }

    Flags       = InDesc.Flags;
    bHasStencil = InDesc.HasStencilFormat();

    const VkImage Image = VulkanTexture->GetVkImage();
    if (!VULKAN_CHECK_HANDLE(Image))
    {
        VULKAN_WARNING("Texture does not have a valid Image");
        return false;
    }

    VkImageViewType VulkanImageType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    EFormat         ViewFormat      = EFormat::Unknown;
    uint8           MipLevel        = 0;
    uint32          BaseArrayLayer  = 0;
    uint32          LayerCount      = 1;

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
        {
            const auto& TextureDesc = InDesc.Texture1D;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_1D;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            break;
        }
        
        case EViewDimension::Texture1DArray:
        {
            const auto& TextureDesc = InDesc.Texture1DArray;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            BaseArrayLayer  = TextureDesc.FirstArraySlice;
            LayerCount      = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            break;
        }

        case EViewDimension::Texture2D:
        {
            const auto& TextureDesc = InDesc.Texture2D;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            break;
        }

        case EViewDimension::Texture2DArray:
        {
            const auto& TextureDesc = InDesc.Texture2DArray;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            BaseArrayLayer  = TextureDesc.FirstArraySlice;
            LayerCount      = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            break;
        }

        case EViewDimension::TextureCube:
        {
            const auto& TextureDesc = InDesc.TextureCube;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            BaseArrayLayer  = 0;
            LayerCount      = RHI_NUM_CUBE_FACES;
            break;
        }
        
        case EViewDimension::TextureCubeArray:
        {
            const auto& TextureDesc = InDesc.TextureCubeArray;
            VulkanImageType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            ViewFormat      = TextureDesc.Format;
            MipLevel        = TextureDesc.MipLevel;
            BaseArrayLayer  = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
            LayerCount      = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, Math::Max<uint16>(TextureDesc.NumCubes, 1u));
            break;
        }

        default:
        {
            VULKAN_ERROR_CRITICAL("Unsupported ViewDimension for DSV");
            return false;
        }
    }

    const VkFormat           VulkanFormat     = ConvertFormat(ViewFormat);
    const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

    if (!InitializeImageView(Image, VulkanFormat, VulkanImageType, ImageAspectFlags, BaseArrayLayer, LayerCount, MipLevel, /*NumMips=*/1u))
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
