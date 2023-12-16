#include "VulkanCommandContext.h"
#include "VulkanResourceViews.h"
#include "VulkanTexture.h"
#include "VulkanViewport.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FVulkanCommandContext::FVulkanCommandContext(FVulkanDevice* InDevice, FVulkanQueue* InCommandQueue)
    : FVulkanDeviceObject(InDevice)
    , Queue(MakeSharedRef<FVulkanQueue>(InCommandQueue))
    , CommandBuffer(InDevice, InCommandQueue->GetType())
    , ContextState(InDevice, *this)
{
}

FVulkanCommandContext::~FVulkanCommandContext()
{
    RHIFlush();
}

bool FVulkanCommandContext::Initialize()
{
    if (!CommandBuffer.Initialize(VK_COMMAND_BUFFER_LEVEL_PRIMARY))
    {
        return false;
    }

    if (!ContextState.Initialize())
    {
        VULKAN_ERROR("Failed to initialize ContextState");
        return false;
    }

    return true;
}

void FVulkanCommandContext::ImageLayoutTransitionBarrier(const FVulkanImageTransitionBarrier& TransitionBarrier)
{
    CommandBuffer.ImageLayoutTransitionBarrier(TransitionBarrier);
}

void FVulkanCommandContext::ObtainCommandBuffer()
{
    VULKAN_ERROR_COND(CommandBuffer.Begin(), "Failed to Begin CommandBuffer");

    // Clear the list of resources that are scheduled to be destroyed
    DiscardList.Clear();
    DiscardListVk.Clear();
}

void FVulkanCommandContext::FlushCommandBuffer()
{
    if (CommandBuffer.IsRecording())
    {
        VULKAN_ERROR_COND(CommandBuffer.End(), "Failed to End CommandBuffer");

        FVulkanCommandBuffer* SubmitCommandBuffer = &CommandBuffer;
        Queue->ExecuteCommandBuffer(&SubmitCommandBuffer, 1, CommandBuffer.GetFence());
    }
    
    ContextState.ResetStateForNewCommandBuffer();
}

void FVulkanCommandContext::RHIStartContext()
{
    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Lock to the thread that started the context
    CommandContextCS.Lock();

    // Reset the state
    ContextState.ResetState();

    // Retrieve a new CommandBuffer
    ObtainCommandBuffer();
}

void FVulkanCommandContext::RHIFinishContext()
{
    // Submit the CommandBuffer
    FlushCommandBuffer();
    
    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Unlock from the thread that started the context
    CommandContextCS.Unlock();
}

void FVulkanCommandContext::RHIBeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
{
    // TODO: Implement queries
}

void FVulkanCommandContext::RHIEndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)  
{
    // TODO: Implement queries
}

void FVulkanCommandContext::RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(RenderTargetView.Texture);
    VULKAN_ERROR_COND(VulkanTexture, "Trying to clear an RenderTargetView that is nullptr");
    
    if (FVulkanImageView* ImageView = VulkanTexture->GetOrCreateRenderTargetView(RenderTargetView))
    {
        VkClearColorValue VulkanClearColor;
        FMemory::Memcpy(VulkanClearColor.float32, ClearColor.Data(), sizeof(VulkanClearColor.float32));

        VkImageSubresourceRange SubresourceRange = ImageView->GetSubresourceRange();
        CommandBuffer.ClearColorImage(ImageView->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &VulkanClearColor, 1, &SubresourceRange);
    }
}

void FVulkanCommandContext::RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(DepthStencilView.Texture);
    VULKAN_ERROR_COND(VulkanTexture, "Trying to clear an DepthStencilView that is nullptr");

    if (FVulkanImageView* ImageView = VulkanTexture->GetOrCreateDepthStencilView(DepthStencilView))
    {
        VkClearDepthStencilValue DepthStenciLValue;
        DepthStenciLValue.depth   = Depth;
        DepthStenciLValue.stencil = Stencil;
        
        VkImageSubresourceRange SubresourceRange = ImageView->GetSubresourceRange();
        CommandBuffer.ClearDepthStencilImage(ImageView->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &DepthStenciLValue, 1, &SubresourceRange);
    }
}

void FVulkanCommandContext::RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    // TODO: Implement when UAVs are implemented
}

void FVulkanCommandContext::RHIBeginRenderPass(const FRHIRenderPassDesc& RenderPassInitializer)
{
    VkClearValue ClearValues[FRHILimits::MaxRenderTargets + 1];
    FMemory::Memzero(ClearValues, sizeof(ClearValues));
    
    uint32 Width          = 0;
    uint32 Height         = 0;
    uint32 NumClearValues = 0;

    VkRenderPass  RenderPass  = VK_NULL_HANDLE;
    VkFramebuffer FrameBuffer = VK_NULL_HANDLE;

    if (RenderPassInitializer.DepthStencilView.Texture || RenderPassInitializer.NumRenderTargets)
    {
        // TODO: Verify that the samples are all the same
        uint8 NumSamples = 0;

        // Set extent to max so that we can use the min operator when going through the RenderTargets/DepthStencil
        Width  = TNumericLimits<uint32>::Max();
        Height = TNumericLimits<uint32>::Max();

        // Check for each render-target-texture
        FVulkanRenderPassKey RenderPassKey;
        RenderPassKey.NumRenderTargets = RenderPassInitializer.NumRenderTargets;
    
        FVulkanFramebufferKey FramebufferKey;
        for (uint32 Index = 0; Index < RenderPassInitializer.NumRenderTargets; Index++)
        {
            const FRHIRenderTargetView& RenderTargetView = RenderPassInitializer.RenderTargets[Index];
            if (FVulkanTexture* VulkanTexture = GetVulkanTexture(RenderTargetView.Texture))
            {
                Width      = FMath::Min<uint32>(VulkanTexture->GetWidth(), Width);
                Height     = FMath::Min<uint32>(VulkanTexture->GetHeight(), Height);
                NumSamples = FMath::Max<uint8>(static_cast<uint8>(VulkanTexture->GetNumSamples()), NumSamples);

                RenderPassKey.RenderTargetFormats[Index]             = RenderTargetView.Format;
                RenderPassKey.RenderTargetActions[Index].LoadAction  = RenderTargetView.LoadAction;
                RenderPassKey.RenderTargetActions[Index].StoreAction = RenderTargetView.StoreAction;
            
                VkClearValue& ClearValue = ClearValues[NumClearValues++];
                FMemory::Memcpy(ClearValue.color.float32, &RenderTargetView.ClearValue.r, sizeof(ClearValue.color.float32));

                // Get the image view
                if (FVulkanImageView* ImageView = VulkanTexture->GetOrCreateRenderTargetView(RenderTargetView))
                {
                    FramebufferKey.AttachmentViews[FramebufferKey.NumAttachmentViews++] = ImageView->GetVkImageView();
                }
            }
            else
            {
                CHECK(RenderTargetView.Texture == nullptr);
            }
        }
    
        // Check for depth-texture
        if (FVulkanTexture* VulkanTexture = GetVulkanTexture(RenderPassInitializer.DepthStencilView.Texture))
        {
            Width      = FMath::Min<uint32>(VulkanTexture->GetWidth(), Width);
            Height     = FMath::Min<uint32>(VulkanTexture->GetHeight(), Height);
            NumSamples = FMath::Max<uint8>(static_cast<uint8>(VulkanTexture->GetNumSamples()), NumSamples);

            const FRHIDepthStencilView& DepthStencilView = RenderPassInitializer.DepthStencilView;
            RenderPassKey.DepthStencilFormat              = DepthStencilView.Format;
            RenderPassKey.DepthStencilActions.LoadAction  = DepthStencilView.LoadAction;
            RenderPassKey.DepthStencilActions.StoreAction = DepthStencilView.StoreAction;
        
            VkClearValue& ClearValue = ClearValues[NumClearValues++];
            ClearValue.depthStencil.depth   = DepthStencilView.ClearValue.Depth;
            ClearValue.depthStencil.stencil = DepthStencilView.ClearValue.Stencil;

            // Get the image view
            if (FVulkanImageView* ImageView = VulkanTexture->GetOrCreateDepthStencilView(DepthStencilView))
            {
                FramebufferKey.AttachmentViews[FramebufferKey.NumAttachmentViews++] = ImageView->GetVkImageView();
            }
        }
        else
        {
            CHECK(RenderPassInitializer.DepthStencilView.Texture == nullptr);
        }

        // Retrieve or create a RenderPass
        RenderPassKey.NumSamples = NumSamples;

        RenderPass = GetDevice()->GetRenderPassCache().GetRenderPass(RenderPassKey);
        if (!VULKAN_CHECK_HANDLE(RenderPass))
        {
            DEBUG_BREAK();
        }
    
        // Retrieve or create a FrameBuffer
        FramebufferKey.RenderPass = RenderPass;

        CHECK(Width != TNumericLimits<uint32>::Max());
        FramebufferKey.Width = static_cast<uint16>(Width);

        CHECK(Height != TNumericLimits<uint32>::Max());
        FramebufferKey.Height = static_cast<uint16>(Height);

        FrameBuffer = GetDevice()->GetFramebufferCache().GetFramebuffer(FramebufferKey);
        if (!VULKAN_CHECK_HANDLE(FrameBuffer))
        {
            DEBUG_BREAK();
        }
    }
    
    // Begin the RenderPass
    VkRenderPassBeginInfo RenderPassBeginInfo;
    FMemory::Memzero(&RenderPassBeginInfo);
    
    RenderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass               = RenderPass;
    RenderPassBeginInfo.framebuffer              = FrameBuffer;
    RenderPassBeginInfo.renderArea.extent.width  = Width;
    RenderPassBeginInfo.renderArea.extent.height = Height;
    RenderPassBeginInfo.clearValueCount          = NumClearValues;
    RenderPassBeginInfo.pClearValues             = ClearValues;
    
    CommandBuffer.BeginRenderPass(&RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void FVulkanCommandContext::RHIEndRenderPass()  
{
    CommandBuffer.EndRenderPass();
}

void FVulkanCommandContext::RHISetViewport(const FRHIViewportRegion& ViewportRegion)
{
    VkViewport Viewport;
    Viewport.width    = ViewportRegion.Width;
    Viewport.height   = ViewportRegion.Height;
    Viewport.maxDepth = ViewportRegion.MaxDepth;
    Viewport.minDepth = ViewportRegion.MinDepth;
    Viewport.x        = ViewportRegion.PositionX;
    Viewport.y        = ViewportRegion.PositionY;
    
    ContextState.SetViewports(&Viewport, 1);
}

void FVulkanCommandContext::RHISetScissorRect(const FRHIScissorRegion& ScissorRegion)
{
    VkRect2D ScissorRect;
    ScissorRect.offset.x      = static_cast<int32_t>(ScissorRegion.PositionX);
    ScissorRect.extent.width  = static_cast<int32_t>(ScissorRegion.Width);
    ScissorRect.offset.y      = static_cast<int32_t>(ScissorRegion.PositionY);
    ScissorRect.extent.height = static_cast<int32_t>(ScissorRegion.Height);
    
    ContextState.SetScissorRects(&ScissorRect, 1);
}

void FVulkanCommandContext::RHISetBlendFactor(const FVector4& Color)
{
    ContextState.SetBlendFactor(Color.Data());
}

void FVulkanCommandContext::RHISetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 Index = 0; Index < InVertexBuffers.Size(); ++Index)
    {
        FVulkanBuffer* VulkanVertexBuffer = static_cast<FVulkanBuffer*>(InVertexBuffers[Index]);
        ContextState.SetVertexBuffer(VulkanVertexBuffer, BufferSlot + Index);
    }
}

void FVulkanCommandContext::RHISetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    FVulkanBuffer* VulkanIndexBuffer = static_cast<FVulkanBuffer*>(IndexBuffer);
    ContextState.SetIndexBuffer(VulkanIndexBuffer, ConvertIndexFormat(IndexFormat));
}

void FVulkanCommandContext::RHISetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState)
{
    FVulkanGraphicsPipelineState* VulkanPipelineState = static_cast<FVulkanGraphicsPipelineState*>(PipelineState);
    ContextState.SetGraphicsPipelineState(VulkanPipelineState);
}

void FVulkanCommandContext::RHISetComputePipelineState(class FRHIComputePipelineState* PipelineState)  
{
    FVulkanComputePipelineState* VulkanPipelineState = static_cast<FVulkanComputePipelineState*>(PipelineState);
    ContextState.SetComputePipelineState(VulkanPipelineState);
}

void FVulkanCommandContext::RHISet32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);

    ContextState.SetPushConstants(reinterpret_cast<const uint32*>(Shader32BitConstants), Num32BitConstants);
}

void FVulkanCommandContext::RHISetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(ParameterIndex < VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);

    FVulkanShaderResourceView* VulkanShaderResourceView = static_cast<FVulkanShaderResourceView*>(ShaderResourceView);
    ContextState.SetSRV(VulkanShaderResourceView, VulkanShader->GetShaderVisibility(), ParameterIndex);
}

void FVulkanCommandContext::RHISetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(ParameterIndex + InShaderResourceViews.Size() <= VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);

    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        FVulkanShaderResourceView* VulkanShaderResourceView = static_cast<FVulkanShaderResourceView*>(InShaderResourceViews[Index]);
        ContextState.SetSRV(VulkanShaderResourceView, VulkanShader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FVulkanCommandContext::RHISetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(ParameterIndex < VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    FVulkanUnorderedAccessView* VulkanUnorderedAccessView = static_cast<FVulkanUnorderedAccessView*>(UnorderedAccessView);
    ContextState.SetUAV(VulkanUnorderedAccessView, VulkanShader->GetShaderVisibility(), ParameterIndex);
}

void FVulkanCommandContext::RHISetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(ParameterIndex + InUnorderedAccessViews.Size() <= VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        FVulkanUnorderedAccessView* VulkanUnorderedAccessView = static_cast<FVulkanUnorderedAccessView*>(InUnorderedAccessViews[Index]);
        ContextState.SetUAV(VulkanUnorderedAccessView, VulkanShader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FVulkanCommandContext::RHISetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(ParameterIndex < VULKAN_DEFAULT_CONSTANT_BUFFER_COUNT);

    FVulkanBuffer* VulkanConstantBuffer = static_cast<FVulkanBuffer*>(ConstantBuffer);
    ContextState.SetCBV(VulkanConstantBuffer, VulkanShader->GetShaderVisibility(), ParameterIndex);
}

void FVulkanCommandContext::RHISetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(ParameterIndex + InConstantBuffers.Size() <= VULKAN_DEFAULT_CONSTANT_BUFFER_COUNT);

    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        FVulkanBuffer* VulkanConstantBuffer = static_cast<FVulkanBuffer*>(InConstantBuffers[Index]);
        ContextState.SetCBV(VulkanConstantBuffer, VulkanShader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FVulkanCommandContext::RHISetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(ParameterIndex < VULKAN_DEFAULT_SAMPLER_STATE_COUNT);

    FVulkanSamplerState* VulkanSamplerState = static_cast<FVulkanSamplerState*>(SamplerState);
    ContextState.SetSampler(VulkanSamplerState, VulkanShader->GetShaderVisibility(), ParameterIndex);
}

void FVulkanCommandContext::RHISetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(ParameterIndex + InSamplerStates.Size() <= VULKAN_DEFAULT_SAMPLER_STATE_COUNT);

    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        FVulkanSamplerState* VulkanSamplerState = static_cast<FVulkanSamplerState*>(InSamplerStates[Index]);
        ContextState.SetSampler(VulkanSamplerState, VulkanShader->GetShaderVisibility(), ParameterIndex + Index);
    }
}

void FVulkanCommandContext::RHIUpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)     
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(Dst);
    if (!VulkanBuffer)
    {
        VULKAN_WARNING("Buffer is nullptr");
        return;
    }
    
    if (IsEnumFlagSet(VulkanBuffer->GetFlags(), EBufferUsageFlags::Dynamic))
    {
        VkDevice       NativeDevice = GetDevice()->GetVkDevice();
        VkDeviceMemory DeviceMemory = VulkanBuffer->GetVkDeviceMemory();
        uint8*         BufferData   = nullptr;
        
        // Align the size
        // TODO: Setup offset and size
        // const VkDeviceSize AlignedSize = FMath::AlignUp<VkDeviceSize>(BufferRegion.Size, 0x100);

        // Map buffer memory
        VkResult Result = vkMapMemory(NativeDevice, DeviceMemory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&BufferData));
        if (VULKAN_FAILED(Result) || !BufferData)
        {
            VULKAN_ERROR("Failed to map buffer memory");
            return;
        }

        // Copy over relevant data
        FMemory::Memcpy(BufferData + BufferRegion.Offset, SrcData, BufferRegion.Size);
        
        // Flush memory ranges
        VkMappedMemoryRange MappedMemoryRange;
        FMemory::Memzero(&MappedMemoryRange);
        
        MappedMemoryRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        MappedMemoryRange.memory = DeviceMemory;
        MappedMemoryRange.offset = 0;
        MappedMemoryRange.size   = VK_WHOLE_SIZE;
        
        Result = vkFlushMappedMemoryRanges(NativeDevice, 1, &MappedMemoryRange);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("Failed to flush buffer memory");
            return;
        }
        
        // Unmap memory
        vkUnmapMemory(NativeDevice, DeviceMemory);
    }
    else
    {
        FVulkanUploadAllocation Allocation = GetDevice()->GetUploadHeap().Allocate(BufferRegion.Size, 1);
        CHECK(Allocation.Memory != nullptr);
        FMemory::Memcpy(Allocation.Memory, SrcData, BufferRegion.Size);
        
        VkBufferCopy BufferCopy;
        BufferCopy.srcOffset = Allocation.Offset;
        BufferCopy.dstOffset = BufferRegion.Offset;
        BufferCopy.size      = BufferRegion.Size;
        
        CommandBuffer.CopyBuffer(Allocation.Buffer->GetVkBuffer(), VulkanBuffer->GetVkBuffer(), 1, &BufferCopy);
        DiscardListVk.Add(Allocation.Buffer);
    }
}

void FVulkanCommandContext::RHIUpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) 
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(Dst);
    if (!VulkanTexture)
    {
        VULKAN_WARNING("Texture is nullptr");
        return;
    }
    
    const VkFormat Format = VulkanTexture->GetVkFormat();
    const uint64 RequiredSize = FVulkanTextureHelper::CalculateTextureUploadSize(Format, TextureRegion.Width, TextureRegion.Height);
    
    // TODO: Check if there exists a Vulkan macro for this
    const uint64 Alignment = 256;

    FVulkanUploadAllocation Allocation = GetDevice()->GetUploadHeap().Allocate(RequiredSize, Alignment);
    CHECK(Allocation.Memory != nullptr);

    const uint8* Source = reinterpret_cast<const uint8*>(SrcData);
    CHECK(Source != nullptr);
    
    const uint32 RowPitch = FVulkanTextureHelper::CalculateTextureRowPitch(Format, TextureRegion.Width);
    const uint32 NumRows  = FVulkanTextureHelper::CalculateTextureNumRows(Format, TextureRegion.Height);
    for (uint64 y = 0; y < NumRows; y++)
    {
        FMemory::Memcpy(Allocation.Memory, Source, SrcRowPitch);
        Source            += SrcRowPitch;
        Allocation.Memory += RowPitch;
    }
    
    VkBufferImageCopy BufferImageCopy;
    BufferImageCopy.bufferOffset                    = Allocation.Offset;
    BufferImageCopy.bufferRowLength                 = 0;
    BufferImageCopy.bufferImageHeight               = 0;
    BufferImageCopy.imageSubresource.aspectMask     = GetImageAspectFlagsFromFormat(Format);
    BufferImageCopy.imageSubresource.mipLevel       = MipLevel;
    BufferImageCopy.imageSubresource.baseArrayLayer = 0;
    BufferImageCopy.imageSubresource.layerCount     = 1;
    BufferImageCopy.imageOffset                     = { 0, 0, 0 };
    BufferImageCopy.imageExtent                     = { TextureRegion.Width, TextureRegion.Height, 1 };

    CommandBuffer.CopyBufferToImage(Allocation.Buffer->GetVkBuffer(), VulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);
    DiscardListVk.Add(Allocation.Buffer);
}

void FVulkanCommandContext::RHIResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FVulkanTexture* SrcVulkanTexture = GetVulkanTexture(Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTexture* DstVulkanTexture = GetVulkanTexture(Dst);
    CHECK(DstVulkanTexture != nullptr);
    
    CHECK(SrcVulkanTexture->GetWidth()  == DstVulkanTexture->GetWidth());
    CHECK(SrcVulkanTexture->GetHeight() == DstVulkanTexture->GetHeight());
    CHECK(SrcVulkanTexture->GetDepth()  == DstVulkanTexture->GetDepth());
    
    VkImageResolve ImageResolve;
    FMemory::Memzero(&ImageResolve);
    
    ImageResolve.srcSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
    ImageResolve.srcSubresource.mipLevel       = 0;
    ImageResolve.srcSubresource.baseArrayLayer = 0;
    ImageResolve.srcSubresource.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageResolve.dstSubresource.aspectMask     = GetImageAspectFlagsFromFormat(DstVulkanTexture->GetVkFormat());
    ImageResolve.dstSubresource.mipLevel       = 0;
    ImageResolve.dstSubresource.baseArrayLayer = 0;
    ImageResolve.dstSubresource.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageResolve.extent.width                  = DstVulkanTexture->GetWidth();
    ImageResolve.extent.height                 = DstVulkanTexture->GetHeight();
    ImageResolve.extent.depth                  = DstVulkanTexture->GetDepth();
    
    CommandBuffer.ResolveImage(SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageResolve);
}

void FVulkanCommandContext::RHICopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    FVulkanBuffer* SrcVulkanBuffer = GetVulkanBuffer(Src);
    CHECK(SrcVulkanBuffer != nullptr);
    
    FVulkanBuffer* DstVulkanBuffer = GetVulkanBuffer(Dst);
    CHECK(DstVulkanBuffer != nullptr);

    VkBufferCopy BufferCopy;
    BufferCopy.srcOffset = CopyDesc.SrcOffset;
    BufferCopy.dstOffset = CopyDesc.DstOffset;
    BufferCopy.size      = CopyDesc.Size;
    
    CommandBuffer.CopyBuffer(SrcVulkanBuffer->GetVkBuffer(), DstVulkanBuffer->GetVkBuffer(), 1, &BufferCopy);
}

void FVulkanCommandContext::RHICopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FVulkanTexture* SrcVulkanTexture = GetVulkanTexture(Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTexture* DstVulkanTexture = GetVulkanTexture(Dst);
    CHECK(DstVulkanTexture != nullptr);
    
    CHECK(SrcVulkanTexture->GetWidth()  == DstVulkanTexture->GetWidth());
    CHECK(SrcVulkanTexture->GetHeight() == DstVulkanTexture->GetHeight());
    CHECK(SrcVulkanTexture->GetDepth()  == DstVulkanTexture->GetDepth());
    
    VkImageCopy ImageCopy;
    FMemory::Memzero(&ImageCopy);
    
    ImageCopy.srcSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
    ImageCopy.srcSubresource.mipLevel       = 0;
    ImageCopy.srcSubresource.baseArrayLayer = 0;
    ImageCopy.srcSubresource.layerCount     = SrcVulkanTexture->GetNumArraySlices();
    ImageCopy.dstSubresource.aspectMask     = GetImageAspectFlagsFromFormat(DstVulkanTexture->GetVkFormat());
    ImageCopy.dstSubresource.mipLevel       = 0;
    ImageCopy.dstSubresource.baseArrayLayer = 0;
    ImageCopy.dstSubresource.layerCount     = DstVulkanTexture->GetNumArraySlices();
    ImageCopy.extent.width                  = DstVulkanTexture->GetWidth();
    ImageCopy.extent.height                 = DstVulkanTexture->GetHeight();
    
    if (DstVulkanTexture->GetDimension() == ETextureDimension::Texture3D)
    {
        ImageCopy.extent.depth = DstVulkanTexture->GetDepth();
    }
    else
    {
        ImageCopy.extent.depth = 1;
    }
    
    CommandBuffer.CopyImage(SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageCopy);
}

void FVulkanCommandContext::RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc)
{
    FVulkanTexture* SrcVulkanTexture = GetVulkanTexture(Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTexture* DstVulkanTexture = GetVulkanTexture(Dst);
    CHECK(DstVulkanTexture != nullptr);
    
    constexpr uint32 MaxCopies = 15;
    VkImageCopy ImageCopy[MaxCopies];
    
    for (uint32 MipLevel = 0; MipLevel < CopyDesc.NumMipLevels; MipLevel++)
    {
        VkImageCopy& CopyInfo = ImageCopy[MipLevel];
        FMemory::Memzero(&CopyInfo, sizeof(CopyInfo));
        
        // Describe the source subresource
        CopyInfo.srcSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
        CopyInfo.srcSubresource.mipLevel       = CopyDesc.SrcMipSlice + MipLevel;
        CopyInfo.srcSubresource.baseArrayLayer = CopyDesc.SrcArraySlice;
        CopyInfo.srcSubresource.layerCount     = CopyDesc.NumArraySlices;
        CopyInfo.srcOffset.x                   = CopyDesc.SrcPosition.x >> MipLevel;
        CopyInfo.srcOffset.y                   = CopyDesc.SrcPosition.y >> MipLevel;
        CopyInfo.srcOffset.z                   = CopyDesc.SrcPosition.z >> MipLevel;
        
        // Describe the destination subresource
        CopyInfo.dstSubresource.aspectMask     = GetImageAspectFlagsFromFormat(DstVulkanTexture->GetVkFormat());
        CopyInfo.dstSubresource.mipLevel       = CopyDesc.DstMipSlice + MipLevel;
        CopyInfo.dstSubresource.baseArrayLayer = CopyDesc.DstArraySlice;
        CopyInfo.dstSubresource.layerCount     = CopyDesc.NumArraySlices;
        CopyInfo.dstOffset.x                   = CopyDesc.DstPosition.x >> MipLevel;
        CopyInfo.dstOffset.y                   = CopyDesc.DstPosition.y >> MipLevel;
        CopyInfo.dstOffset.z                   = CopyDesc.DstPosition.z >> MipLevel;
        
        // Size of thie mipslice
        CopyInfo.extent.width  = FMath::Max(CopyDesc.Size.x >> MipLevel, 1);
        CopyInfo.extent.height = FMath::Max(CopyDesc.Size.y >> MipLevel, 1);
        CopyInfo.extent.depth  = FMath::Max(CopyDesc.Size.z >> MipLevel, 1);
    }
    
    CommandBuffer.CopyImage(SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, CopyDesc.NumMipLevels, ImageCopy);
}

void FVulkanCommandContext::RHIDestroyResource(IRefCounted* Resource)  
{
    if (Resource)
    {
        DiscardList.Add(MakeSharedRef<IRefCounted>(Resource));
    }
}

void FVulkanCommandContext::RHIDiscardContents(FRHITexture* Resource)
{
}

void FVulkanCommandContext::RHIBuildRayTracingGeometry(
    FRHIRayTracingGeometry* RayTracingGeometry,
    FRHIBuffer*             VertexBuffer,
    uint32                  NumVertices,
    FRHIBuffer*             IndexBuffer,
    uint32                  NumIndices,
    EIndexFormat            IndexFormat,
    bool                    bUpdate)
{
}

void FVulkanCommandContext::RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
{
}

void FVulkanCommandContext::RHISetRayTracingBindings(
    FRHIRayTracingScene*              RayTracingScene,
    FRHIRayTracingPipelineState*      PipelineState,
    const FRayTracingShaderResources* GlobalResource,
    const FRayTracingShaderResources* RayGenLocalResources,
    const FRayTracingShaderResources* MissLocalResources,
    const FRayTracingShaderResources* HitGroupResources,
    uint32                            NumHitGroupResources)
{
}

void FVulkanCommandContext::RHIGenerateMips(FRHITexture* Texture)
{
    // TODO: Implement this
}

void FVulkanCommandContext::RHITransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(Texture);
    if (!VulkanTexture)
    {
        VULKAN_WARNING("Texture is nullptr");
        return;
    }

    const VkImageLayout NewLayout      = ConvertResourceStateToImageLayout(AfterState);
    const VkImageLayout PreviousLayout = ConvertResourceStateToImageLayout(BeforeState);
    if (NewLayout != PreviousLayout)
    {
        FVulkanImageTransitionBarrier TransitionBarrier;
        TransitionBarrier.PreviousLayout                  = PreviousLayout;
        TransitionBarrier.NewLayout                       = NewLayout;
        TransitionBarrier.Image                           = VulkanTexture->GetVkImage();
        TransitionBarrier.DependencyFlags                 = 0;
        TransitionBarrier.SrcAccessMask                   = ConvertResourceStateToAccessFlags(BeforeState);
        TransitionBarrier.DstAccessMask                   = ConvertResourceStateToAccessFlags(AfterState);
        TransitionBarrier.SrcStageMask                    = ConvertResourceStateToPipelineStageFlags(BeforeState);
        TransitionBarrier.DstStageMask                    = ConvertResourceStateToPipelineStageFlags(AfterState);
        TransitionBarrier.SubresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(VulkanTexture->GetVkFormat());
        TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
        TransitionBarrier.SubresourceRange.baseMipLevel   = 0;
        TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        TransitionBarrier.SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
    
        ImageLayoutTransitionBarrier(TransitionBarrier);
    }
}

void FVulkanCommandContext::RHITransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)   
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(Buffer);
    if (!VulkanBuffer)
    {
        VULKAN_WARNING("Buffer is nullptr");
        return;
    }

    FVulkanBufferBarrier BufferBarrier;
    BufferBarrier.SrcStageMask    = ConvertResourceStateToPipelineStageFlags(BeforeState);
    BufferBarrier.DstStageMask    = ConvertResourceStateToPipelineStageFlags(AfterState);
    BufferBarrier.DependencyFlags = 0;
    BufferBarrier.Buffer          = VulkanBuffer->GetVkBuffer();
    BufferBarrier.SrcAccessMask   = ConvertResourceStateToAccessFlags(BeforeState);
    BufferBarrier.DstAccessMask   = ConvertResourceStateToAccessFlags(AfterState);
    BufferBarrier.Offset          = 0;
    BufferBarrier.Size            = VK_WHOLE_SIZE;

    CommandBuffer.BufferMemoryPipelineBarrier(BufferBarrier);
}

void FVulkanCommandContext::RHIUnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    // TODO: Check what type of barrier is needed here
}

void FVulkanCommandContext::RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer)   
{
    // TODO: Check what type of barrier is needed here
}

void FVulkanCommandContext::RHIDraw(uint32 VertexCount, uint32 StartVertexLocation)
{
    ContextState.BindGraphicsStates();
}

void FVulkanCommandContext::RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    ContextState.BindGraphicsStates();
}

void FVulkanCommandContext::RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    ContextState.BindGraphicsStates();
}

void FVulkanCommandContext::RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    ContextState.BindGraphicsStates();
}

void FVulkanCommandContext::RHIDispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    ContextState.BindComputeState();
    // CommandBuffer.Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
}

void FVulkanCommandContext::RHIDispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
}

void FVulkanCommandContext::RHIPresentViewport(FRHIViewport* Viewport, bool bVerticalSync)
{
    FlushCommandBuffer();

    FVulkanViewport* VulkanViewport = static_cast<FVulkanViewport*>(Viewport);
    VulkanViewport->Present(bVerticalSync);
}

void FVulkanCommandContext::RHIResizeViewport(FRHIViewport* Viewport, uint32 Width, uint32 Height)
{
    FVulkanViewport* VulkanViewport = static_cast<FVulkanViewport*>(Viewport);
    VulkanViewport->Resize(Width, Height);
}

void FVulkanCommandContext::RHIClearState()
{
    RHIFlush();
}

void FVulkanCommandContext::RHIFlush()
{
    FlushCommandBuffer();

    Queue->WaitForCompletion();
}

void FVulkanCommandContext::RHIInsertMarker(const FStringView& Message)
{
}

void FVulkanCommandContext::RHIBeginExternalCapture()
{
    // TODO: Investigate the probability of this
}

void FVulkanCommandContext::RHIEndExternalCapture()  
{
    // TODO: Investigate the probability of this
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
