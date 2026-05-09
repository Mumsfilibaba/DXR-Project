#include "Core/Templates/NumericLimits.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "RHI/RHIStats.h"

uint32 VkCalculateTextureRowPitch(VkFormat Format, uint32 Width)
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

uint32 VkCalculateTextureNumRows(VkFormat Format, uint32 Height)
{
    const bool bIsBlockCompressed = VkFormatIsBlockCompressed(Format);
    return bIsBlockCompressed ? Math::Max<uint32>(1, (Height + 3) / 4) : Height;
}

uint64 VkCalculateTextureUploadSize(VkFormat Format, uint32 Width, uint32 Height)
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
    : FVulkanTextureBase(InTextureDesc)
    , FVulkanResource(InDevice)
    , DebugName()
    , Image(VK_NULL_HANDLE)
    , CreateInfo{}
    , ShaderResourceView(nullptr)
    , UnorderedAccessView(nullptr)
    , RenderTargetView(nullptr)
    , DepthStencilView(nullptr)
{
}

FVulkanTextureRHI* FVulkanTextureRHI::GetTextureInterface() const
{
    return const_cast<FVulkanTextureRHI*>(this);
}

void* FVulkanTextureRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(GetVkImage());
}

FRHIShaderResourceView* FVulkanTextureRHI::GetShaderResourceView() const
{
    return ShaderResourceView.Get();
}

FRHIUnorderedAccessView* FVulkanTextureRHI::GetUnorderedAccessView() const
{
    return UnorderedAccessView.Get();
}

FRHIRenderTargetView* FVulkanTextureRHI::GetRenderTargetView() const
{
    return RenderTargetView.Get();
}

FRHIDepthStencilView* FVulkanTextureRHI::GetDepthStencilView() const
{
    return DepthStencilView.Get();
}

FRHIDescriptorHandle FVulkanTextureRHI::GetBindlessUAVHandle() const
{
    return FRHIDescriptorHandle();
}

FRHIDescriptorHandle FVulkanTextureRHI::GetBindlessSRVHandle() const
{
    return FRHIDescriptorHandle();
}

FVulkanTextureRHI::~FVulkanTextureRHI()
{
#if VULKAN_ENABLE_STATS
    const int64 AllocatedSize = static_cast<int64>(MemoryLocation.GetSize());
    if (AllocatedSize > 0)
    {
        if (Desc.IsRenderTarget() || Desc.IsDepthStencil())
        {
            STAT_SUBTRACT(STAT_RHI_RenderTargetMemory, AllocatedSize);
        }
        else
        {
            STAT_SUBTRACT(STAT_RHI_TextureMemory, AllocatedSize);
        }
    }
#endif

    if (MemoryLocation.IsValid() && VULKAN_CHECK_HANDLE(Image))
    {
        vkDestroyImage(GetDevice()->GetVkDevice(), Image, nullptr);
        Image = VK_NULL_HANDLE;
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
        ImageCreateInfo.arrayLayers  = RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices);
        ImageCreateInfo.extent.depth = 1;
    }

    // Enable Texture-Cube views
    if (Desc.IsTextureCube() || Desc.IsTextureCubeArray())
    {
        ImageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    // TODO: Look into abstracting these flags
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

    VkImageFormatListCreateInfo FormatListInfo = {};

    TArray<VkFormat> ViewFormats;
    if ((ImageCreateInfo.flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) != 0)
    {
        ViewFormats = GetVulkanFormatCompatibilityClass(ImageCreateInfo.format);

        if (ViewFormats.Size() > 0)
        {
            FormatListInfo.sType           = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
            FormatListInfo.viewFormatCount = static_cast<uint32>(ViewFormats.Size());
            FormatListInfo.pViewFormats    = ViewFormats.Data();
            FormatListInfo.pNext           = ImageCreateInfo.pNext;
            ImageCreateInfo.pNext          = &FormatListInfo;
        }
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

    const VkMemoryAllocateFlags AllocateFlags    = 0;
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateImageMemory(Image, ImageCreateInfo, MemoryProperties, AllocateFlags, MemoryLocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate ImageMemory (Width=%u, Height=%u, Format=%u, Usage=0x%x)", 
            ImageCreateInfo.extent.width, ImageCreateInfo.extent.height, ImageCreateInfo.format, ImageCreateInfo.usage);
        return false;
    }

    VkResult BindResult = vkBindImageMemory(GetDevice()->GetVkDevice(), Image, MemoryLocation.GetMemory(), MemoryLocation.GetMemoryOffset());
    if (VULKAN_FAILED(BindResult))
    {
        VULKAN_ERROR_CRITICAL("Failed to bind ImageMemory");
        return false;
    }

    if (!Desc.IsNoDefaultSRV())
    {
        const EFormat  SRVFormat      = Desc.Format;
        const uint8    NumMipLevels   = static_cast<uint8>(Desc.NumMipLevels);
        const uint16   NumArraySlices = static_cast<uint16>(Desc.NumArraySlices);

        FRHIShaderResourceViewDesc ViewDesc;
        if (Desc.IsTexture1D())
        {
            ViewDesc = FRHIShaderResourceViewDesc::CreateTexture1D(SRVFormat, 0, NumMipLevels);
        }
        else if (Desc.IsTexture1DArray())
        {
            ViewDesc = FRHIShaderResourceViewDesc::CreateTexture1DArray(SRVFormat, 0, NumMipLevels, 0, NumArraySlices);
        }
        else if (Desc.IsTexture2D())
        {
            ViewDesc = FRHIShaderResourceViewDesc::CreateTexture2D(SRVFormat, 0, NumMipLevels);
        }
        else if (Desc.IsTexture2DArray())
        {
            ViewDesc = FRHIShaderResourceViewDesc::CreateTexture2DArray(SRVFormat, 0, NumMipLevels, 0, NumArraySlices);
        }
        else if (Desc.IsTextureCube())
        {
            ViewDesc = FRHIShaderResourceViewDesc::CreateTextureCube(SRVFormat, 0, NumMipLevels);
        }
        else if (Desc.IsTextureCubeArray())
        {
            ViewDesc = FRHIShaderResourceViewDesc::CreateTextureCubeArray(SRVFormat, 0, NumMipLevels, 0, NumArraySlices);
        }
        else if (Desc.IsTexture3D())
        {
            ViewDesc = FRHIShaderResourceViewDesc::CreateTexture3D(SRVFormat, 0, NumMipLevels);
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Unsupported resource dimension for default SRV");
            CHECK(false);
            return false;
        }

        FVulkanShaderResourceViewRHIRef DefaultSRV = new FVulkanShaderResourceViewRHI(GetDevice(), this);
        if (!DefaultSRV->Initialize(this, ViewDesc))
        {
            return false;
        }

        ShaderResourceView = DefaultSRV;
    }

    if (Desc.IsUnorderedAccessTexture() && !Desc.IsNoDefaultUAV())
    {
        const EFormat UAVFormat = Desc.Format;

        FRHIUnorderedAccessViewDesc ViewDesc;
        if (Desc.IsTexture1D())
        {
            ViewDesc = FRHIUnorderedAccessViewDesc::CreateTexture1D(UAVFormat, 0);
        }
        else if (Desc.IsTexture1DArray())
        {
            ViewDesc = FRHIUnorderedAccessViewDesc::CreateTexture1DArray(UAVFormat, 0, 0, static_cast<uint16>(Desc.NumArraySlices));
        }
        else if (Desc.IsTexture2D())
        {
            ViewDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(UAVFormat, 0);
        }
        else if (Desc.IsTexture2DArray())
        {
            ViewDesc = FRHIUnorderedAccessViewDesc::CreateTexture2DArray(UAVFormat, 0, 0, static_cast<uint16>(Desc.NumArraySlices));
        }
        else if (Desc.IsTextureCube() || Desc.IsTextureCubeArray())
        {
            ViewDesc = FRHIUnorderedAccessViewDesc::CreateTexture2DArray(UAVFormat, 0, 0, static_cast<uint16>(RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices)));
        }
        else if (Desc.IsTexture3D())
        {
            ViewDesc = FRHIUnorderedAccessViewDesc::CreateTexture3D(UAVFormat, 0, 0, static_cast<uint16>(Desc.Extent.Z));
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Unsupported resource dimension for default UAV");
            CHECK(false);
            return false;
        }

        FVulkanUnorderedAccessViewRHIRef DefaultUAV = new FVulkanUnorderedAccessViewRHI(GetDevice(), this);
        if (!DefaultUAV->Initialize(this, ViewDesc))
        {
            return false;
        }

        UnorderedAccessView = DefaultUAV;
    }

    const uint16 FullResourceSliceCount = Desc.IsTexture3D()
        ? static_cast<uint16>(Desc.NumArraySlices)
        : static_cast<uint16>(RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices));

    if (Desc.IsRenderTarget() && !Desc.IsNoDefaultRTV())
    {
        FRHIRenderTargetViewDesc ViewDesc;
        if (Desc.IsTexture1D())
        {
            ViewDesc = FRHIRenderTargetViewDesc::CreateTexture1D(Desc.Format, 0);
        }
        else if (Desc.IsTexture1DArray())
        {
            ViewDesc = FRHIRenderTargetViewDesc::CreateTexture1DArray(Desc.Format, 0, 0, FullResourceSliceCount);
        }
        else if (Desc.IsTexture2D())
        {
            ViewDesc = FRHIRenderTargetViewDesc::CreateTexture2D(Desc.Format, 0);
        }
        else if (Desc.IsTexture2DArray() || Desc.IsTextureCube() || Desc.IsTextureCubeArray())
        {
            ViewDesc = FRHIRenderTargetViewDesc::CreateTexture2DArray(Desc.Format, 0, 0, FullResourceSliceCount);
        }
        else if (Desc.IsTexture3D())
        {
            ViewDesc = FRHIRenderTargetViewDesc::CreateTexture3D(Desc.Format, 0, 0, static_cast<uint16>(Desc.Extent.Z));
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Unsupported resource dimension for default RTV");
            CHECK(false);
            return false;
        }

        FVulkanRenderTargetViewRHIRef DefaultRTV = new FVulkanRenderTargetViewRHI(GetDevice(), this);
        if (!DefaultRTV->Initialize(this, ViewDesc))
        {
            return false;
        }

        RenderTargetView = DefaultRTV;
    }

    if (Desc.IsDepthStencil() && !Desc.IsNoDefaultDSV())
    {
        const EFormat DSVFormat = Desc.ClearValue.Format != EFormat::Unknown ? Desc.ClearValue.Format : Desc.Format;

        FRHIDepthStencilViewDesc ViewDesc;
        if (Desc.IsTexture1D())
        {
            ViewDesc = FRHIDepthStencilViewDesc::CreateTexture1D(DSVFormat, 0);
        }
        else if (Desc.IsTexture1DArray())
        {
            ViewDesc = FRHIDepthStencilViewDesc::CreateTexture1DArray(DSVFormat, 0, 0, FullResourceSliceCount);
        }
        else if (Desc.IsTexture2D())
        {
            ViewDesc = FRHIDepthStencilViewDesc::CreateTexture2D(DSVFormat, 0);
        }
        else if (Desc.IsTexture2DArray() || Desc.IsTextureCube() || Desc.IsTextureCubeArray())
        {
            ViewDesc = FRHIDepthStencilViewDesc::CreateTexture2DArray(DSVFormat, 0, 0, FullResourceSliceCount);
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Unsupported resource dimension for default DSV");
            CHECK(false);
            return false;
        }

        FVulkanDepthStencilViewRHIRef DefaultDSV = new FVulkanDepthStencilViewRHI(GetDevice(), this);
        if (!DefaultDSV->Initialize(this, ViewDesc))
        {
            return false;
        }

        DepthStencilView = DefaultDSV;
    }

    CHECK(InCommandContext != nullptr);
    if (InInitialData)
    {
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

        const uint32   NumArrayLayers = ImageCreateInfo.arrayLayers;
        const VkFormat Format         = ImageCreateInfo.format;
        const uint64   Alignment      = InCommandContext->GetDevice()->GetPhysicalDevice()->GetProperties().limits.optimalBufferCopyOffsetAlignment;

        uint32 Width  = Desc.Extent.X;
        uint32 Height = Desc.Extent.Y;
        uint32 Depth  = Desc.IsTexture3D() ? Desc.Extent.Z : 1;

        for (uint32 MipIndex = 0; MipIndex < Desc.NumMipLevels; ++MipIndex)
        {
            void* MipData = InInitialData->GetMipData(MipIndex);
            if (!MipData)
            {
                break;
            }

            const uint32 SrcRowPitch   = static_cast<uint32>(InInitialData->GetMipRowPitch(MipIndex));
            const int64  SrcSlicePitch = InInitialData->GetMipSlicePitch(MipIndex);

            const uint32 RowPitch = VkCalculateTextureRowPitch(Format, Width);
            const uint32 NumRows  = VkCalculateTextureNumRows(Format, Height);

            for (uint32 ArrayLayer = 0; ArrayLayer < NumArrayLayers; ++ArrayLayer)
            {
                const uint64 SliceSize = static_cast<uint64>(RowPitch) * NumRows * Depth;

                FVulkanMemoryLocation UploadLocation(InCommandContext->GetDevice());
                uint8* UploadMemory = static_cast<uint8*>(InCommandContext->GetDevice()->GetMemoryManager().AllocateUploadMemory(
                    SliceSize, 
                    Alignment, 
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                    UploadLocation));
                CHECK(UploadMemory != nullptr);

                const uint8* Source = reinterpret_cast<const uint8*>(MipData) + ArrayLayer * SrcSlicePitch;
                for (uint32 DepthSlice = 0; DepthSlice < Depth; ++DepthSlice)
                {
                    for (uint32 y = 0; y < NumRows; y++)
                    {
                        FMemory::Memcpy(UploadMemory, Source, RowPitch);
                        Source       += SrcRowPitch;
                        UploadMemory += RowPitch;
                    }
                }

                VkBufferImageCopy BufferImageCopy = {};
                BufferImageCopy.bufferOffset                    = UploadLocation.GetBufferOffset();
                BufferImageCopy.bufferRowLength                 = 0;
                BufferImageCopy.bufferImageHeight               = 0;
                BufferImageCopy.imageSubresource.aspectMask     = GetImageAspectFlagsFromFormat(Format);
                BufferImageCopy.imageSubresource.mipLevel       = MipIndex;
                BufferImageCopy.imageSubresource.baseArrayLayer = ArrayLayer;
                BufferImageCopy.imageSubresource.layerCount     = 1;
                BufferImageCopy.imageOffset                     = { 0, 0, 0 };
                BufferImageCopy.imageExtent                     = { Width, Height, Depth };

                InCommandContext->GetBarrierBatcher().FlushBarriers(InCommandContext->GetCommandBuffer());
                
                InCommandContext->GetCommandBuffer()->CopyBufferToImage(
                    UploadLocation.GetBackingBuffer(), 
                    Image, 
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                    1, 
                    &BufferImageCopy);

                if (Desc.IsTexture3D())
                {
                    break;
                }
            }

            Width  = Math::Max(1u, Width >> 1);
            Height = Math::Max(1u, Height >> 1);
            Depth  = Math::Max(1u, Depth >> 1);
        }

        InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::CopyDest, InInitialAccess));
        InCommandContext->FinishContext();
    }
    else
    {
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
        InCommandContext->GetBarrierBatcher().FlushBarriers(InCommandContext->GetCommandBuffer());

        VkImageSubresourceRange SubresourceRange = {};
        SubresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageCreateInfo.format);
        SubresourceRange.baseArrayLayer = 0;
        SubresourceRange.baseMipLevel   = 0;
        SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        if (Desc.IsDepthStencil())
        {
            VkClearDepthStencilValue ClearDS = {};
            if (Desc.ClearValue.IsDepthStencilValue())
            {
                const FDepthStencilValue& DS = Desc.ClearValue.AsDepthStencil();
                ClearDS.depth   = DS.Depth;
                ClearDS.stencil = static_cast<uint8>(DS.Stencil);
            }
            else
            {
                ClearDS.depth   = 1.0f;
                ClearDS.stencil = 0;
            }

            InCommandContext->GetCommandBuffer()->ClearDepthStencilImage(
                Image, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                &ClearDS, 
                1, 
                &SubresourceRange);
        }
        else if (!VkFormatIsBlockCompressed(ImageCreateInfo.format))
        {
            VkClearColorValue ClearColor = {};
            if (Desc.ClearValue.IsColorValue())
            {
                const FFloatColor& Color = Desc.ClearValue.AsColor();
                ClearColor.float32[0] = Color.R;
                ClearColor.float32[1] = Color.G;
                ClearColor.float32[2] = Color.B;
                ClearColor.float32[3] = Color.A;
            }

            InCommandContext->GetCommandBuffer()->ClearColorImage(
                Image, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                &ClearColor, 
                1, 
                &SubresourceRange);
        }

        InCommandContext->TransitionTextureState(this, FRHITextureTransition::Make(EResourceAccess::CopyDest, InInitialAccess));
        InCommandContext->FinishContext();
    }

    const VkImageLayout InitialLayout = FVulkanRHI::ResourceStateToImageLayout(InInitialAccess);
    const uint32 NumSubresources = ImageCreateInfo.mipLevels * ImageCreateInfo.arrayLayers;
    ImageLayoutState.SetImageLayout(InitialLayout);
    ImageLayoutState.Initialize(Math::Max(NumSubresources, 1u));

    {
        constexpr ETextureUsageFlags WriteMask = 
            ETextureUsageFlags::RenderTarget | 
            ETextureUsageFlags::DepthStencil | 
            ETextureUsageFlags::UnorderedAccessTexture | 
            ETextureUsageFlags::Presentable;

        if ((Desc.UsageFlags & WriteMask) == ETextureUsageFlags::None && IsEnumFlagSet(Desc.UsageFlags, ETextureUsageFlags::ShaderResourceTexture))
        {
            ImageLayoutState.SetDefaultLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    return true;
}

void FVulkanTextureRHI::SetVkImage(VkImage InImage)
{
    Image = InImage;

    if (CreateInfo.format == VK_FORMAT_UNDEFINED)
    {
        CreateInfo.format      = ConvertFormat(Desc.Format);
        CreateInfo.mipLevels   = Desc.NumMipLevels;
        CreateInfo.arrayLayers = Desc.NumArraySlices;
        CreateInfo.extent      = { static_cast<uint32>(Desc.Extent.X), static_cast<uint32>(Desc.Extent.Y), Math::Max(static_cast<uint32>(Desc.Extent.Z), 1u) };
    }

    const uint32 NumSubresources = CreateInfo.mipLevels * CreateInfo.arrayLayers;
    ImageLayoutState.Initialize(Math::Max(NumSubresources, 1u));
    ImageLayoutState.SetImageLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void FVulkanTextureRHI::SetDebugName(const FString& InName)
{
    if (VULKAN_CHECK_HANDLE(Image))
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), *InName, Image, VK_OBJECT_TYPE_IMAGE);
    #if VULKAN_STORE_DEBUG_NAMES
        DebugName = InName;
    #endif
    }
}

void FVulkanTextureRHI::GetDebugName(FString& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}
