#include "VulkanRHI.h"
#include "VulkanTexture.h"
#include "VulkanViewport.h"
#include "VulkanCommandContext.h"

FVulkanTexture::FVulkanTexture(FVulkanDevice* InDevice, const FRHITextureDesc& InDesc)
    : FRHITexture(InDesc)
    , FVulkanDeviceObject(InDevice)
    , Image(VK_NULL_HANDLE)
    , bIsImageOwner(false)
{
}

FVulkanTexture::~FVulkanTexture()
{
    if (bIsImageOwner && VULKAN_CHECK_HANDLE(Image))
    {
        vkDestroyImage(GetDevice()->GetVkDevice(), Image, nullptr);
        Image = VK_NULL_HANDLE;
    }
}

bool FVulkanTexture::Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData)
{
    VkSampleCountFlagBits SampleCount;
    switch (Desc.NumSamples)
    {
    case 1:  SampleCount = VK_SAMPLE_COUNT_1_BIT;  break;
    case 2:  SampleCount = VK_SAMPLE_COUNT_2_BIT;  break;
    case 4:  SampleCount = VK_SAMPLE_COUNT_4_BIT;  break;
    case 8:  SampleCount = VK_SAMPLE_COUNT_8_BIT;  break;
    case 16: SampleCount = VK_SAMPLE_COUNT_16_BIT; break;
    case 32: SampleCount = VK_SAMPLE_COUNT_32_BIT; break;
    case 64: SampleCount = VK_SAMPLE_COUNT_64_BIT; break;

    default:
        VULKAN_ERROR("Invalid SampleCount");
        return false;
    }
       
    VkImageCreateInfo ImageCreateInfo;
    FMemory::Memzero(&ImageCreateInfo);

    ImageCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.pNext                 = nullptr;
    ImageCreateInfo.flags                 = 0;
    ImageCreateInfo.imageType             = ConvertTextureDimension(Desc.Dimension);;
    ImageCreateInfo.extent.width          = Desc.Extent.x;
    ImageCreateInfo.extent.height         = Desc.Extent.y;
    ImageCreateInfo.mipLevels             = Desc.NumMipLevels;
    ImageCreateInfo.pQueueFamilyIndices   = nullptr;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples               = SampleCount;
    ImageCreateInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.usage                 = 0;
    ImageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    
    // NOTE: We store the format so that we have easy access to it later
    ImageCreateInfo.format = Format = ConvertFormat(Desc.Format);

    if (ImageCreateInfo.imageType == VK_IMAGE_TYPE_3D)
    {
        ImageCreateInfo.extent.depth = Desc.Extent.z;
        ImageCreateInfo.arrayLayers  = 1;
    }
    else
    {
        ImageCreateInfo.extent.depth = 1;
        ImageCreateInfo.arrayLayers  = Desc.NumArraySlices;
    }
    
    // Enable Texture-Cube views
    if (Desc.IsTextureCube() || Desc.IsTextureCubeArray())
    {
        ImageCreateInfo.flags       |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        ImageCreateInfo.arrayLayers  = Desc.NumArraySlices * kRHINumCubeFaces;
    }
    
    // TODO: Look into abstracting these flags
    ImageCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    if (Desc.IsRenderTarget())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (Desc.IsDepthStencil())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (Desc.IsShaderResource())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (Desc.IsUnorderedAccess())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    VkResult Result = vkCreateImage(GetDevice()->GetVkDevice(), &ImageCreateInfo, nullptr, &Image);
    VULKAN_CHECK_RESULT(Result, "Failed to create image");
    bIsImageOwner = true;
    
    // Check if the image should be allocated using a dedicated allocation
    bool bUseDedicatedAllocation = false;
    VkMemoryRequirements MemoryRequirements;
#if VK_KHR_get_memory_requirements2 && VK_KHR_dedicated_allocation
    if (FVulkanDedicatedAllocationKHR::IsEnabled())
    {
        VkMemoryRequirements2KHR MemoryRequirements2;
        FMemory::Memzero(&MemoryRequirements2);
        MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;

        VkMemoryDedicatedRequirementsKHR MemoryDedicatedRequirements;
        FMemory::Memzero(&MemoryDedicatedRequirements);
        MemoryDedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;

        // Add the proper pNext values in the structs
        FVulkanStructureHelper MemoryRequirements2Helper(MemoryRequirements2);
        MemoryRequirements2Helper.AddNext(MemoryDedicatedRequirements);

        VkImageMemoryRequirementsInfo2KHR ImageMemoryRequirementsInfo;
        ImageMemoryRequirementsInfo.sType  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR;
        ImageMemoryRequirementsInfo.pNext  = nullptr;
        ImageMemoryRequirementsInfo.image  = Image;
        
        vkGetImageMemoryRequirements2KHR(GetDevice()->GetVkDevice(), &ImageMemoryRequirementsInfo, &MemoryRequirements2);
        MemoryRequirements      = MemoryRequirements2.memoryRequirements;
        bUseDedicatedAllocation = (MemoryDedicatedRequirements.requiresDedicatedAllocation != VK_FALSE) || (MemoryDedicatedRequirements.prefersDedicatedAllocation != VK_FALSE);
    }
    else
#endif
    {
        vkGetImageMemoryRequirements(GetDevice()->GetVkDevice(), Image, &MemoryRequirements);
    }

    const int32 MemoryTypeIndex = GetDevice()->GetPhysicalDevice()->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VULKAN_CHECK(MemoryTypeIndex != UINT32_MAX, "No suitable memory type");

    VkMemoryAllocateInfo AllocateInfo;
    FMemory::Memzero(&AllocateInfo);

    FVulkanStructureHelper AllocationInfoHelper(AllocateInfo);
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
    AllocateInfo.allocationSize  = MemoryRequirements.size;

#if VK_KHR_dedicated_allocation
    VkMemoryDedicatedAllocateInfoKHR DedicatedAllocateInfo;
    FMemory::Memzero(&DedicatedAllocateInfo);

    DedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
    DedicatedAllocateInfo.image = Image;

    if (bUseDedicatedAllocation && FVulkanDedicatedAllocationKHR::IsEnabled())
    {
        VULKAN_INFO("Using dedicated allocation for Image");
        AllocationInfoHelper.AddNext(DedicatedAllocateInfo);
    }
#endif

    const bool bResult = GetDevice()->AllocateMemory(AllocateInfo, &DeviceMemory);
    VULKAN_CHECK(bResult, "Failed to allocate memory");

    Result = vkBindImageMemory(GetDevice()->GetVkDevice(), Image, DeviceMemory, 0);
    VULKAN_CHECK_RESULT(Result, "Failed to bind Image-DeviceMemory");


    if (InInitialData)
    {
        // TODO: Upload initial data
    }

    // NOTE: Transition the texture into the expected ImageLayout
    FVulkanCommandContext* Context = FVulkanRHI::GetRHI()->ObtainCommandContext();
    Context->RHIStartContext();

    FVulkanImageTransitionBarrier TransitionBarrier;
    TransitionBarrier.Image                           = Image;
    TransitionBarrier.PreviousLayout                  = VK_IMAGE_LAYOUT_UNDEFINED;
    TransitionBarrier.NewLayout                       = ConvertResourceStateToImageLayout(InInitialAccess);
    TransitionBarrier.DependencyFlags                 = 0;
    TransitionBarrier.SrcAccessMask                   = VK_ACCESS_NONE;
    TransitionBarrier.DstAccessMask                   = VK_ACCESS_NONE;
    TransitionBarrier.SrcStageMask                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    TransitionBarrier.DstStageMask                    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    TransitionBarrier.SubresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageCreateInfo.format);
    TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
    TransitionBarrier.SubresourceRange.baseMipLevel   = 0;
    TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    TransitionBarrier.SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    Context->ImageLayoutTransitionBarrier(TransitionBarrier);
    Context->RHIFinishContext();
    return true;
}

void FVulkanTexture::SetName(const FString& InName)
{
    FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.GetCString(), Image, VK_OBJECT_TYPE_IMAGE);
}

FString FVulkanTexture::GetName() const
{
    return FString();
}


FVulkanBackBuffer::FVulkanBackBuffer(FVulkanDevice* InDevice, FVulkanViewport* InViewport, const FRHITextureDesc& InDesc)
    : FVulkanTexture(InDevice, InDesc)
    , Viewport(MakeSharedRef<FVulkanViewport>(InViewport))
{
    // There will always be atleast one RenderTargetView with the BackBuffer
    RenderTargetViews.Emplace(new FVulkanRenderTargetView());
}

void FVulkanBackBuffer::AquireNextImage()
{
    // NOTE: The format has to be updated here, in-case the format has changed when the Swapchain is recreated
    FVulkanSwapChain* SwapChain = Viewport->GetSwapChain();
    Format = SwapChain->GetVkSurfaceFormat().format;

    // Update the image
    Image = Viewport->GetImage(SwapChain->GetBufferIndex());
    CHECK(Image != VK_NULL_HANDLE);
    
    // Update the ImageView
    FVulkanImageView* ImageView = Viewport->GetImageView(SwapChain->GetBufferIndex());
    CHECK(ImageView != nullptr);
    
    FVulkanRenderTargetViewRef RenderTargetView = RenderTargetViews[0];
    CHECK(RenderTargetView != nullptr);
    RenderTargetView->SetImageView(MakeSharedRef<FVulkanImageView>(ImageView));
}
