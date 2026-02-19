#include "Core/Templates/NumericLimits.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanCommandContext.h"

uint32 VulkanTextureHelper::CalculateTextureRowPitch(VkFormat Format, uint32 Width)
{
    const bool bIsBlockCompressed = VkFormatIsBlockCompressed(Format);
    if (bIsBlockCompressed)
    {
        const uint32 BlockSize = GetVkFormatBlockSize(Format);
        CHECK(BlockSize != 0);
        
        Width = Math::Max<uint32>(1, (Width + 3) / 4);
        return Width * BlockSize;
    }
    else
    {
        const uint32 PixelSize = GetVkFormatByteStride(Format);
        CHECK(PixelSize != 0);
        return Width * PixelSize;
    }
}

uint32 VulkanTextureHelper::CalculateTextureNumRows(VkFormat Format, uint32 Height)
{
    const bool bIsBlockCompressed = VkFormatIsBlockCompressed(Format);
    return bIsBlockCompressed ? Math::Max<uint32>(1, (Height + 3) / 4) : Height;
}

uint64 VulkanTextureHelper::CalculateTextureUploadSize(VkFormat Format, uint32 Width, uint32 Height)
{
    const bool bIsBlockCompressed = VkFormatIsBlockCompressed(Format);
    if (bIsBlockCompressed)
    {
        const uint32 BlockSize = GetVkFormatBlockSize(Format);
        CHECK(BlockSize != 0);
        
        Width  = Math::Max<uint32>(1, (Width + 3) / 4);
        Height = Math::Max<uint32>(1, (Height + 3) / 4);
        return Width * Height * BlockSize;
    }
    else
    {
        const uint32 PixelSize = GetVkFormatByteStride(Format);
        CHECK(PixelSize != 0);
        return Width * Height * PixelSize;
    }
}

FVulkanTextureRHI::FVulkanTextureRHI(FVulkanDevice* InDevice, const FRHITextureDesc& InTextureDesc)
    : FRHITexture(InTextureDesc)
    , FVulkanDeviceChild(InDevice)
    , DebugName()
    , Image(VK_NULL_HANDLE)
    , MemoryAllocation()
    , CreateInfo{}
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , ImageViews()
    , ImageViewMap()
{
}

FVulkanTextureRHI::~FVulkanTextureRHI()
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

bool FVulkanTextureRHI::Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const IRHITextureData* InInitialData)
{
    const VkSampleCountFlagBits SampleCount = ConvertSampleCount(Desc.NumSamples);
    if (SampleCount < VK_SAMPLE_COUNT_1_BIT)
    {
        VULKAN_ERROR_CRITICAL("Invalid SampleCount");
        return false;
    }

    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType             = ConvertTextureDimension(Desc.Dimension);
    ImageCreateInfo.format                = ConvertFormat(Desc.Format);
    ImageCreateInfo.extent.width          = Desc.Extent.X;
    ImageCreateInfo.extent.height         = Desc.Extent.Y;
    ImageCreateInfo.mipLevels             = Desc.NumMipLevels;
    ImageCreateInfo.pQueueFamilyIndices   = nullptr;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples               = SampleCount;
    ImageCreateInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    if (IsTypelessFormat(Desc.Format))
    {
        ImageCreateInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }

    if (ImageCreateInfo.imageType == VK_IMAGE_TYPE_3D)
    {
        ImageCreateInfo.arrayLayers  = 1;
        ImageCreateInfo.extent.depth = Desc.Extent.Z;
    }
    else
    {
        ImageCreateInfo.arrayLayers  = Desc.NumArraySlices;
        ImageCreateInfo.extent.depth = 1;
    }
    
    // Enable Texture-Cube views
    if (Desc.IsTextureCube() || Desc.IsTextureCubeArray())
    {
        ImageCreateInfo.flags      |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        ImageCreateInfo.arrayLayers = Desc.NumArraySlices * RHI_NUM_CUBE_FACES;
    }

    // Currently needed for most textures
    ImageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (Desc.IsRenderTarget())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (Desc.IsDepthStencil())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (Desc.IsShaderResourceTexture())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (Desc.IsUnorderedAccessTexture())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (Desc.IsShadingRateTexture())
    {
        ImageCreateInfo.usage |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
    }

    VkResult Result = vkCreateImage(GetDevice()->GetVkDevice(), &ImageCreateInfo, nullptr, &Image);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create image");
        return false;
    }
    else
    {
        CreateInfo = ImageCreateInfo;
    }

    // NOTE: All textures are allocated as device local, maybe we want to move this into the AllocateImageMemory function
    const VkMemoryAllocateFlags AllocateFlags    = 0;
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateImageMemory(Image, MemoryProperties, AllocateFlags, GVulkanForceDedicatedImageAllocations, MemoryAllocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate ImageMemory");
        return false;
    }

    {
        FRHIShaderResourceViewDesc ViewDesc;
        ViewDesc.Type               = FRHIShaderResourceViewDesc::EType::TextureSRV;
        ViewDesc.TextureSRV.Texture = this;
        ViewDesc.TextureSRV.Format  = VulkanCastShaderResourceFormat(Desc.Format);

        if (Desc.IsTexture2D() || Desc.IsTextureCube())
        {
            ViewDesc.TextureSRV.FirstMipLevel   = 0;
            ViewDesc.TextureSRV.NumMips         = static_cast<uint8>(Desc.NumMipLevels);
            ViewDesc.TextureSRV.MinLODClamp     = 0.0f;
            ViewDesc.TextureSRV.FirstArraySlice = 0;
            ViewDesc.TextureSRV.NumSlices       = 1;
        }
        else if (Desc.IsTexture2DArray() || Desc.IsTextureCubeArray() || Desc.IsTexture3D())
        {
            ViewDesc.TextureSRV.FirstMipLevel   = 0;
            ViewDesc.TextureSRV.NumMips         = static_cast<uint8>(Desc.NumMipLevels);
            ViewDesc.TextureSRV.MinLODClamp     = 0.0f;
            ViewDesc.TextureSRV.FirstArraySlice = 0;
            ViewDesc.TextureSRV.NumSlices       = static_cast<uint16>(Desc.NumArraySlices);
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Unsupported resource dimension");
            return false;
        }

        FVulkanShaderResourceViewRHIRef DefaultSRV = new FVulkanShaderResourceViewRHI(GetDevice(), this);
        if (!DefaultSRV->InitializeSRV(ViewDesc))
        {
            return false;
        }

        ShaderResourceView = DefaultSRV;
    }

    // TODO: Fix for other resources than Texture2D
    const bool bIsTexture2D = Desc.IsTexture2D();
    if (bIsTexture2D)
    {
        if (Desc.IsUnorderedAccessTexture())
        {
            FRHIUnorderedAccessViewDesc ViewDesc;
            ViewDesc.Type                       = FRHIUnorderedAccessViewDesc::EType::TextureUAV;
            ViewDesc.TextureUAV.Texture         = this;
            ViewDesc.TextureUAV.Format          = Desc.Format;
            ViewDesc.TextureUAV.FirstArraySlice = 0;
            ViewDesc.TextureUAV.MipLevel        = 0;
            ViewDesc.TextureUAV.NumSlices       = static_cast<uint16>(Desc.NumArraySlices);

            FVulkanUnorderedAccessViewRHIRef DefaultUAV = new FVulkanUnorderedAccessViewRHI(GetDevice(), this);
            if (!DefaultUAV->InitializeUAV(ViewDesc))
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
        InCommandContext->StartContext();
        
        VkImageMemoryBarrier2 ImageBarrier = {};
        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = Image;
        ImageBarrier.srcAccessMask                   = VK_ACCESS_2_NONE;
        ImageBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageCreateInfo.format);
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        InCommandContext->GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);

        // Transfer all the mip-levels
        uint32 Width  = Desc.Extent.X;
        uint32 Height = Desc.Extent.Y;

        for (uint32 Index = 0; Index < Desc.NumMipLevels; ++Index)
        {
            // TODO: This does not feel optimal
            if (IsBlockCompressed(Desc.Format) && (Width % 4 != 0 || Height % 4 != 0))
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
            InCommandContext->UpdateTexture2D(this, TextureRegion, Index, Data, static_cast<uint32>(InInitialData->GetMipRowPitch(Index)));

            Width  = Math::Max(1u, Width >> 1);
            Height = Math::Max(1u, Height >> 1);
        }

        // NOTE: Transition into InitialAccess
        InCommandContext->TransitionTexture(this, FRHITextureTransition::Make(EResourceAccess::CopyDest, InInitialAccess));
        InCommandContext->FinishContext();
    }
    else
    {
        InCommandContext->StartContext();

        const VkImageLayout FinalLayout = FVulkanRHI::ResourceStateToImageLayout(InInitialAccess);

        const bool bNeedsClear = (Desc.IsRenderTarget() || Desc.IsDepthStencil() || Desc.IsUnorderedAccessTexture()) && !MemoryAllocation.bIsDedicated;
        if (bNeedsClear)
        {
            // Transition to TRANSFER_DST so we can clear, then clear, then transition to initial access.
            VkImageMemoryBarrier2 ImageBarrier = {};
            ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
            ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.image                           = Image;
            ImageBarrier.srcAccessMask                   = VK_ACCESS_2_NONE;
            ImageBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageCreateInfo.format);
            ImageBarrier.subresourceRange.baseArrayLayer = 0;
            ImageBarrier.subresourceRange.baseMipLevel   = 0;
            ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
            ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

            InCommandContext->GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);
            InCommandContext->GetBarrierBatcher().FlushBarriers();

            VkImageSubresourceRange SubresourceRange = {};
            SubresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageCreateInfo.format);
            SubresourceRange.baseMipLevel   = 0;
            SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
            SubresourceRange.baseArrayLayer = 0;
            SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

            if (Desc.IsRenderTarget())
            {
                VkClearColorValue ClearColor = {};
                ClearColor.float32[0] = 0.0f;
                ClearColor.float32[1] = 0.0f;
                ClearColor.float32[2] = 0.0f;
                ClearColor.float32[3] = 1.0f;

                InCommandContext->GetCommandBuffer()->ClearColorImage(Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ClearColor, 1, &SubresourceRange);
            }
            else if (Desc.IsDepthStencil())
            {
                VkClearDepthStencilValue DepthStencilValue = {};
                DepthStencilValue.depth   = 1.0f;
                DepthStencilValue.stencil = 0;

                InCommandContext->GetCommandBuffer()->ClearDepthStencilImage(Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &DepthStencilValue, 1, &SubresourceRange);
            }
            else if (Desc.IsUnorderedAccessTexture())
            {
                VkClearColorValue ClearColor = {};
                ClearColor.float32[0] = 0.0f;
                ClearColor.float32[1] = 0.0f;
                ClearColor.float32[2] = 0.0f;
                ClearColor.float32[3] = 0.0f;

                InCommandContext->GetCommandBuffer()->ClearColorImage(Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ClearColor, 1, &SubresourceRange);
            }

            ImageBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            ImageBarrier.newLayout     = FinalLayout;
            ImageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            ImageBarrier.dstAccessMask = FVulkanRHI::ResourceStateToAccessFlags(InInitialAccess);
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            ImageBarrier.dstStageMask  = FVulkanRHI::ResourceStateToPipelineStageFlags(InInitialAccess);

            InCommandContext->GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);
        }
        else
        {
            VkImageMemoryBarrier2 ImageBarrier = {};
            ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
            ImageBarrier.newLayout                       = FinalLayout;
            ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.image                           = Image;
            ImageBarrier.srcAccessMask                   = VK_ACCESS_2_NONE;
            ImageBarrier.dstAccessMask                   = VK_ACCESS_2_NONE;
            ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageCreateInfo.format);
            ImageBarrier.subresourceRange.baseArrayLayer = 0;
            ImageBarrier.subresourceRange.baseMipLevel   = 0;
            ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
            ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

            InCommandContext->GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);
        }

        InCommandContext->FinishContext();
        InCommandContext->GetCommandQueue().WaitForCompletion();
    }
    
    return true;
}

FVulkanResourceView* FVulkanTextureRHI::GetOrCreateImageView(const FVulkanHashableImageView& ImageViewDesc)
{
    if (!VULKAN_CHECK_HANDLE(Image))
    {
        VULKAN_WARNING("Texture does not have a valid Image");
        return nullptr;
    } 

    // Check for existing view and control the format of the view
    const VkFormat VulkanFormat = ConvertFormat(static_cast<EFormat>(ImageViewDesc.Format));
    const VkImageAspectFlags ImageAspectFlags = GetImageAspectFlagsFromFormat(VulkanFormat);

    // Check for an existing view
    FVulkanResourceView* ExistingView = nullptr;
    if (ImageViewDesc.NumArraySlices > 1)
    {
        if (FVulkanResourceView** ImageView = ImageViewMap.Find(ImageViewDesc))
        {
            ExistingView = *ImageView;
        }
    }
    else
    {
        // Calculate the subresource for this view
        const uint32 Subresource = VulkanCalculateSubresource(ImageViewDesc.MipLevel, ImageViewDesc.ArrayIndex, 0, GetNumMipLevels(), GetNumArraySlices());
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
        const FVulkanResourceView::FImageView& ExistingImageViewDesc = ExistingView->GetImageViewDesc();
        if (ExistingImageViewDesc.Format != VulkanFormat)
        {
            VULKAN_WARNING("A ImageView for this subresource already exists with another format");
        }

        return ExistingView;
    }

    // Get the ImageViewType
    VkImageViewType ImageViewType;
    switch (Desc.Dimension)
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
    if (!NewImageView->InitializeAsImageView(Image, VulkanFormat, ImageViewType, ImageAspectFlags, ImageViewDesc.ArrayIndex, ImageViewDesc.NumArraySlices, ImageViewDesc.MipLevel, NumMipLevels))
    {
        return nullptr;
    }

    if (ImageViewDesc.NumArraySlices > 1)
    {
        ImageViewMap.Add(ImageViewDesc, NewImageView);
    }
    else
    {
        const uint32 Subresource = VulkanCalculateSubresource(ImageViewDesc.MipLevel, ImageViewDesc.ArrayIndex, 0, GetNumMipLevels(), GetNumArraySlices());
        ImageViews[Subresource] = NewImageView;
    }

    return NewImageView;
}

void FVulkanTextureRHI::DestroyImageViews()
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

void FVulkanTextureRHI::SetVkImage(VkImage InImage)
{
    DestroyImageViews();
    Image = InImage;

    // NOTE: Use the format in the description to set the native format if it is not set yet this should only happen for BackBuffers
    if (CreateDesc.format == VK_FORMAT_UNDEFINED)
    {
        CreateDesc.format = ConvertFormat(Desc.Format);
    }
}

void FVulkanTextureRHI::SetDebugName(const FString& InName)
{
    if (VULKAN_CHECK_HANDLE(Image))
    {
        VulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), *InName, Image, VK_OBJECT_TYPE_IMAGE);
        DebugName = InName;
    }
}

FString FVulkanTextureRHI::GetDebugName() const
{
    return DebugName;
}

void FVulkanTextureRHI::EnableStateTracking(EResourceAccess InitialState)
{
    if (!ImageLayoutState)
    {
        FVulkanImageLayoutState::FImageState State;
        State.Layout = FVulkanRHI::ResourceStateToImageLayout(InitialState);
        State.Access = FVulkanRHI::ResourceStateToAccessFlags(InitialState);
        State.Stage  = FVulkanRHI::ResourceStateToPipelineStageFlags(InitialState);

        uint32 ArrayCount = GetNumArraySlices();
        if (IsTextureCube(GetDimension()))
        {
            ArrayCount *= RHI_NUM_CUBE_FACES;
        }

        ImageLayoutState = MakeUniquePtr<FVulkanImageLayoutState>(State, GetNumMipLevels(), ArrayCount);
    }
}

void FVulkanTextureRHI::DisableStateTracking(FVulkanCommandContext* CommandContext)
{
    if (!ImageLayoutState)
    {
        return;
    }

    if (CommandContext)
    {
        CommandContext->RequireTextureState(this, FRHIRequiredTextureState::Make(EResourceAccess::Common));
    }

    ImageLayoutState.Reset();
}


FVulkanBackBufferTexture::FVulkanBackBufferTexture(FVulkanDevice* InDevice, FVulkanSwapChainRHI* InSwapChain, const FRHITextureDesc& InTextureDesc)
    : FVulkanTextureRHI(InDevice, InTextureDesc)
    , SwapChain(InSwapChain)
{
}

FVulkanBackBufferTexture::~FVulkanBackBufferTexture()
{
    SwapChain = nullptr;
}

void FVulkanBackBufferTexture::ResizeBackBuffer(int32 InWidth, int32 InHeight)
{
    Desc.Extent.X = InWidth;
    Desc.Extent.Y = InHeight;
    
    const uint32 NumBackBuffers = SwapChain->GetNumBackBuffers();
    for (uint32 Index = 0; Index < NumBackBuffers; Index++)
    {
        FVulkanTextureRHI* BackBuffer = SwapChain->GetBackBufferFromIndex(Index);
        BackBuffer->Resize(InWidth, InHeight);
        BackBuffer->DestroyImageViews();
    }
}

FVulkanTextureRHI* FVulkanBackBufferTexture::GetCurrentBackBufferTexture(FVulkanCommandContext* InCommandContext)
{
    return SwapChain ? SwapChain->GetCurrentBackBuffer(InCommandContext) : nullptr;
}
