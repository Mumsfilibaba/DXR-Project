#include "VulkanTexture.h"
#include "VulkanViewport.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTexture

CVulkanTexture::CVulkanTexture(CVulkanDevice* InDevice)
    : CVulkanDeviceObject(InDevice)
    , Image(VK_NULL_HANDLE)
    , bIsImageOwner(false)
{
}

CVulkanTexture::~CVulkanTexture()
{
    if (bIsImageOwner && VULKAN_CHECK_HANDLE(Image))
    {
        vkDestroyImage(GetDevice()->GetVkDevice(), Image, nullptr);
        Image = VK_NULL_HANDLE;
    }
}

bool CVulkanTexture::CreateImage(VkImageType ImageType, VkImageCreateFlags Flags, VkImageUsageFlags Usage, VkFormat Format, VkExtent2D Extent, uint32 DepthOrArraySize, uint32 NumMipLevels, uint32 NumSamples)
{
    VkSampleCountFlagBits SampleCount;
    switch (NumSamples)
    {
    case 1:  SampleCount = VK_SAMPLE_COUNT_1_BIT;  break;
    case 2:  SampleCount = VK_SAMPLE_COUNT_2_BIT;  break;
    case 4:  SampleCount = VK_SAMPLE_COUNT_4_BIT;  break;
    case 8:  SampleCount = VK_SAMPLE_COUNT_8_BIT;  break;
    case 16: SampleCount = VK_SAMPLE_COUNT_16_BIT; break;
    case 32: SampleCount = VK_SAMPLE_COUNT_32_BIT; break;
    case 64: SampleCount = VK_SAMPLE_COUNT_64_BIT; break;

    default:
        VULKAN_ERROR_ALWAYS("Invalid SampleCount");
        return false;
    }

    VkImageCreateInfo ImageCreateInfo;
    CMemory::Memzero(&ImageCreateInfo);

    ImageCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.pNext                 = nullptr;
    ImageCreateInfo.flags                 = Flags;
    ImageCreateInfo.imageType             = ImageType;
    ImageCreateInfo.extent.width          = Extent.width;
    ImageCreateInfo.extent.height         = Extent.height;
    ImageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.mipLevels             = NumMipLevels;
    ImageCreateInfo.pQueueFamilyIndices   = nullptr;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples               = SampleCount;
    ImageCreateInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.usage                 = Usage;
    ImageCreateInfo.format                = Format;

    if (ImageType == VK_IMAGE_TYPE_3D)
    {
        ImageCreateInfo.extent.depth = DepthOrArraySize;
        ImageCreateInfo.arrayLayers  = 1;
    }
    else
    {
        ImageCreateInfo.extent.depth = 1;
        ImageCreateInfo.arrayLayers  = DepthOrArraySize;
    }

    VkResult Result = vkCreateImage(GetDevice()->GetVkDevice(), &ImageCreateInfo, nullptr, &Image);
    VULKAN_CHECK_RESULT(Result, "Failed to create image");

    bIsImageOwner = true;
    
    bool bUseDedicatedAllocation = false;

    VkMemoryRequirements MemoryRequirements;
#if (VK_KHR_get_memory_requirements2) && (VK_KHR_dedicated_allocation)
    if (CVulkanDedicatedAllocationKHR::IsEnabled())
    {
        VkMemoryRequirements2KHR MemoryRequirements2;
        CMemory::Memzero(&MemoryRequirements2);

        MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;

        CVulkanStructureHelper MemoryRequirementsHelper(MemoryRequirements2);

        VkMemoryDedicatedRequirementsKHR MemoryDedicatedRequirements;
        CMemory::Memzero(&MemoryDedicatedRequirements);

        MemoryDedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;
        MemoryRequirementsHelper.AddNext(MemoryDedicatedRequirements);

        VkImageMemoryRequirementsInfo2KHR BufferMemoryRequirementsInfo;
        BufferMemoryRequirementsInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2_KHR;
        BufferMemoryRequirementsInfo.pNext  = nullptr;
        BufferMemoryRequirementsInfo.image  = Image;

        vkGetImageMemoryRequirements2KHR(GetDevice()->GetVkDevice(), &BufferMemoryRequirementsInfo, &MemoryRequirements2);
        MemoryRequirements = MemoryRequirements2.memoryRequirements;

        bUseDedicatedAllocation = (MemoryDedicatedRequirements.requiresDedicatedAllocation != VK_FALSE) || (MemoryDedicatedRequirements.prefersDedicatedAllocation != VK_FALSE);
    }
    else
#endif
    {
        vkGetImageMemoryRequirements(GetDevice()->GetVkDevice(), Image, &MemoryRequirements);
    }

    CVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

    const int32 MemoryTypeIndex = PhysicalDevice->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VULKAN_CHECK(MemoryTypeIndex != UINT32_MAX, "No suitable memory type");

    VkMemoryAllocateInfo AllocateInfo;
    CMemory::Memzero(&AllocateInfo);

    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
    AllocateInfo.allocationSize  = MemoryRequirements.size;

    CVulkanStructureHelper AllocationInfoHelper(AllocateInfo);

#if VK_KHR_dedicated_allocation
    VkMemoryDedicatedAllocateInfoKHR DedicatedAllocateInfo;
    CMemory::Memzero(&DedicatedAllocateInfo);

    DedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
    DedicatedAllocateInfo.image = Image;

    if (bUseDedicatedAllocation && CVulkanDedicatedAllocationKHR::IsEnabled())
    {
        VULKAN_INFO("Using dedicated allocation for Image");
        AllocationInfoHelper.AddNext(DedicatedAllocateInfo);
    }
#endif

    const bool bResult = GetDevice()->AllocateMemory(AllocateInfo, &DeviceMemory);
    VULKAN_CHECK(bResult, "Failed to allocate memory");

    Result = vkBindImageMemory(GetDevice()->GetVkDevice(), Image, DeviceMemory, 0);
    VULKAN_CHECK_RESULT(Result, "Failed to bind Image-DeviceMemory");
    
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTexture2D

CVulkanTexture2DRef CVulkanTexture2D::CreateTexture2D(CVulkanDevice* InDevice, ERHIFormat InFormat, uint32 InSizeX, uint32 InSizeY, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimalClearValue)
{
    CVulkanTexture2DRef NewTexture = dbg_new CVulkanTexture2D(InDevice, InFormat, InSizeX, InSizeY, InNumMips, InNumSamples, InFlags, InOptimalClearValue);
    if (NewTexture && NewTexture->Initialize())
    {
        return NewTexture;
    }

    return nullptr;
}

CVulkanTexture2D::CVulkanTexture2D(CVulkanDevice* InDevice, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMipLevels, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimalClearValue)
    : CRHITexture2D(InFormat, InWidth, InHeight, InNumMipLevels, InNumSamples, InFlags, InOptimalClearValue)
    , CVulkanTexture(InDevice)
    , RenderTargetView(dbg_new CVulkanRenderTargetView())
    , DepthStencilView(dbg_new CVulkanDepthStencilView())
    , UnorderedAccessView(dbg_new CVulkanUnorderedAccessView())
{
}

bool CVulkanTexture2D::Initialize()
{
    VkImageCreateFlags Flags;
    VkImageUsageFlags  Usage;
    VkFormat           Format;
    VkExtent2D         Extent = { GetWidth(), GetHeight() };

    const bool bResult = CreateImage(VK_IMAGE_TYPE_2D, Flags, Usage, Format, Extent, 1, GetNumMips(), GetNumSamples());
    if (!bResult)
    {
        VULKAN_ERROR_ALWAYS("Failed to create Texture2D");
        return true;
    }

    return false;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanBackBuffer

CVulkanBackBufferRef CVulkanBackBuffer::CreateBackBuffer(CVulkanDevice* InDevice, CVulkanViewport* InViewport, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumSamples)
{
    CVulkanBackBufferRef NewBackBuffer = dbg_new CVulkanBackBuffer(InDevice, InViewport, InFormat, InWidth, InHeight, InNumSamples);
    if (NewBackBuffer)
    {
        return NewBackBuffer;
    }

    return nullptr;
}

CVulkanBackBuffer::CVulkanBackBuffer(CVulkanDevice* InDevice, CVulkanViewport* InViewport, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumSamples)
    : CVulkanTexture2D(InDevice, InFormat, InWidth, InHeight, 1, InNumSamples, TextureFlag_RTV, SClearValue())
    , Viewport(MakeSharedRef<CVulkanViewport>(InViewport))
{
}

void CVulkanBackBuffer::AquireNextImage()
{
    CVulkanSwapChain* SwapChain = Viewport->GetSwapChain();
    
    Image = Viewport->GetImage(SwapChain->GetBufferIndex());
    Assert(Image != VK_NULL_HANDLE);
    
    CVulkanImageView* ImageView = Viewport->GetImageView(SwapChain->GetBufferIndex());
    Assert(ImageView != nullptr);
    
    Assert(RenderTargetView != nullptr);
    RenderTargetView->SetImageView(MakeSharedRef<CVulkanImageView>(ImageView));
}
