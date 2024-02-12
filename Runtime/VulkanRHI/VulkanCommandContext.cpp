#include "VulkanCommandContext.h"
#include "VulkanResourceViews.h"
#include "VulkanTexture.h"
#include "VulkanViewport.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"

FVulkanCommandContext::FVulkanCommandContext(FVulkanDevice* InDevice, FVulkanQueue& InQueue)
    : FVulkanDeviceChild(InDevice)
    , Queue(InQueue)
    , CommandPool(nullptr)
    , CommandBuffer(nullptr)
    , CommandPacket(nullptr)
    , ContextState(InDevice, *this)
    , bIsRecording(false)
{
}

FVulkanCommandContext::~FVulkanCommandContext()
{
    RHIFlush();
}

bool FVulkanCommandContext::Initialize()
{
    if (!ContextState.Initialize())
    {
        VULKAN_ERROR("Failed to initialize ContextState");
        return false;
    }

    // TODO: Another solution for this but for now Transition default images here
    ObtainCommandBuffer();

    VkBuffer DefaultBuffer = ContextState.GetDescriptorSetCache().GetDefaultResources().NullBuffer;
    GetCommandBuffer()->FillBuffer(DefaultBuffer, 0, VULKAN_DEFAULT_BUFFER_NUM_BYTES, 0);

    VkImage DefaultImage = ContextState.GetDescriptorSetCache().GetDefaultResources().NullImage;

    FVulkanImageTransitionBarrier TransitionBarrier;
    TransitionBarrier.Image                           = DefaultImage;
    TransitionBarrier.DependencyFlags                 = 0;
    TransitionBarrier.PreviousLayout                  = VK_IMAGE_LAYOUT_UNDEFINED;
    TransitionBarrier.NewLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    TransitionBarrier.SrcAccessMask                   = VK_ACCESS_NONE;
    TransitionBarrier.DstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
    TransitionBarrier.SrcStageMask                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    TransitionBarrier.DstStageMask                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
    TransitionBarrier.SubresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(VK_FORMAT_R8G8B8A8_UNORM);
    TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
    TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    TransitionBarrier.SubresourceRange.baseMipLevel   = 0;
    TransitionBarrier.SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);

    VkBufferImageCopy BufferImageCopy;
    BufferImageCopy.bufferOffset                    = 0;
    BufferImageCopy.bufferRowLength                 = 0;
    BufferImageCopy.bufferImageHeight               = 0;
    BufferImageCopy.imageSubresource.aspectMask     = TransitionBarrier.SubresourceRange.aspectMask;
    BufferImageCopy.imageSubresource.mipLevel       = 0;
    BufferImageCopy.imageSubresource.baseArrayLayer = 0;
    BufferImageCopy.imageSubresource.layerCount     = 1;
    BufferImageCopy.imageOffset                     = { 0, 0, 0 };
    BufferImageCopy.imageExtent                     = { VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 1 };

    GetCommandBuffer()->CopyBufferToImage(DefaultBuffer, DefaultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);

    TransitionBarrier.PreviousLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    TransitionBarrier.NewLayout      = VK_IMAGE_LAYOUT_GENERAL;
    TransitionBarrier.SrcAccessMask  = VK_ACCESS_TRANSFER_WRITE_BIT;
    TransitionBarrier.DstAccessMask  = VK_ACCESS_TRANSFER_WRITE_BIT;
    TransitionBarrier.SrcStageMask   = VK_PIPELINE_STAGE_TRANSFER_BIT;
    TransitionBarrier.DstStageMask   = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);

    FinishCommandBuffer(true);
    return true;
}

void FVulkanCommandContext::RHIBeginFrame()
{
    FVulkanRHI::GetRHI()->RHIBeginFrame();
}

void FVulkanCommandContext::RHIEndFrame()
{
    FVulkanRHI::GetRHI()->RHIEndFrame();
}

void FVulkanCommandContext::ObtainCommandBuffer()
{
    // Retrieve a CommandPool if there are none
    if (!CommandPool)
    {
        CommandPool = Queue.ObtainCommandPool();
        CHECK(CommandPool != nullptr);
    }
    
    // At this point we cannot have a valid CommandBuffer
    CHECK(CommandBuffer == nullptr);
    CommandBuffer = CommandPool->CreateBuffer();
    CHECK(CommandBuffer != nullptr);
    
    // Begin to record to this CommandPool
    if (!CommandBuffer->Begin())
    {
        VULKAN_ERROR("Failed to Begin CommandBuffer");
        DEBUG_BREAK();
    }
    
    if (!CommandPacket)
    {
        CommandPacket = new FVulkanCommandPacket(GetDevice(), Queue);
    }

    // Clear all the DescriptorPools for reuse
    ContextState.ResetPendingDescriptorPools();
}

void FVulkanCommandContext::FinishCommandBuffer(bool bFlushPool)
{
    CHECK(CommandBuffer != nullptr);
    
    if (CommandBuffer->IsRecording())
    {
        if (!CommandBuffer->End())
        {
            VULKAN_ERROR("Failed to End CommandBuffer");
            DEBUG_BREAK();
        }

        if (bFlushPool)
        {
            GetCommandPacket().AddCommandPool(CommandPool);
            CommandPool = nullptr;
        }
        
        GetCommandPacket().AddCommandBuffer(CommandBuffer);
        CommandBuffer = nullptr;

        FVulkanRHI::GetRHI()->SubmitCommands(CommandPacket);
        CommandPacket = nullptr;
    }
    
    ContextState.ResetStateForNewCommandBuffer();
}

void FVulkanCommandContext::RHIStartContext()
{
    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Lock to the thread that started the context
    CommandContextCS.Lock();

    // Update state
    CHECK(bIsRecording == false);
    bIsRecording = true;

    // Reset the state
    ContextState.ResetState();

    // Process submitted commands
    FVulkanRHI::GetRHI()->ProcessPendingCommands();
    
    // Retrieve a new CommandBuffer
    ObtainCommandBuffer();
}

void FVulkanCommandContext::RHIFinishContext()
{
    // Submit the CommandBuffer
    FinishCommandBuffer(true);
    
    // Update state
    CHECK(bIsRecording == true);
    bIsRecording = false;

    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Unlock from the thread that started the context
    CommandContextCS.Unlock();
}

void FVulkanCommandContext::RHIBeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
{
    // TODO: Implement queries
    UNREFERENCED_VARIABLE(TimestampQuery);
    UNREFERENCED_VARIABLE(Index);
}

void FVulkanCommandContext::RHIEndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)  
{
    // TODO: Implement queries
    UNREFERENCED_VARIABLE(TimestampQuery);
    UNREFERENCED_VARIABLE(Index);
}

void FVulkanCommandContext::RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(RenderTargetView.Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR("Trying to clear an RenderTargetView that is nullptr");
        return;
    }
    
    if (FVulkanImageView* ImageView = VulkanTexture->GetOrCreateRenderTargetView(RenderTargetView))
    {
        // NOTE: Here the image is expected to be in a "RenderTargetState" so we need to transition it to TransferDst, we then need to transition back when the clear is done
        FVulkanImageTransitionBarrier TransitionBarrier;
        TransitionBarrier.Image                           = VulkanTexture->GetVkImage();
        TransitionBarrier.PreviousLayout                  = ConvertResourceStateToImageLayout(EResourceAccess::RenderTarget);
        TransitionBarrier.NewLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        TransitionBarrier.DependencyFlags                 = 0;
        TransitionBarrier.SrcAccessMask                   = ConvertResourceStateToAccessFlags(EResourceAccess::RenderTarget);
        TransitionBarrier.DstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        TransitionBarrier.SrcStageMask                    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        TransitionBarrier.DstStageMask                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
        TransitionBarrier.SubresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageView->GetVkFormat());
        TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
        TransitionBarrier.SubresourceRange.baseMipLevel   = 0;
        TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        TransitionBarrier.SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
        GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);

        VkClearColorValue VulkanClearColor;
        FMemory::Memcpy(VulkanClearColor.float32, ClearColor.Data(), sizeof(VulkanClearColor.float32));

        VkImageSubresourceRange SubresourceRange = ImageView->GetSubresourceRange();
        GetCommandBuffer()->ClearColorImage(ImageView->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &VulkanClearColor, 1, &SubresourceRange);
        
        // .. And transition back into "RenderTargetState"
        TransitionBarrier.PreviousLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        TransitionBarrier.NewLayout      = ConvertResourceStateToImageLayout(EResourceAccess::RenderTarget);
        TransitionBarrier.SrcAccessMask  = VK_ACCESS_TRANSFER_WRITE_BIT;
        TransitionBarrier.DstAccessMask  = ConvertResourceStateToAccessFlags(EResourceAccess::RenderTarget);
        TransitionBarrier.SrcStageMask   = VK_PIPELINE_STAGE_TRANSFER_BIT;
        TransitionBarrier.DstStageMask   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);
    }
}

void FVulkanCommandContext::RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(DepthStencilView.Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR("Trying to clear an DepthStencilView that is nullptr");
        return;
    }

    if (FVulkanImageView* ImageView = VulkanTexture->GetOrCreateDepthStencilView(DepthStencilView))
    {
        // NOTE: Here the image is expected to be in a "DepthStencilState" so we need to transition it to TransferDst, we then need to transition back when the clear is done
        FVulkanImageTransitionBarrier TransitionBarrier;
        TransitionBarrier.Image                           = VulkanTexture->GetVkImage();
        TransitionBarrier.PreviousLayout                  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        TransitionBarrier.NewLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        TransitionBarrier.DependencyFlags                 = 0;
        TransitionBarrier.SrcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        TransitionBarrier.DstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        TransitionBarrier.SrcStageMask                    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        TransitionBarrier.DstStageMask                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
        TransitionBarrier.SubresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(ImageView->GetVkFormat());
        TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
        TransitionBarrier.SubresourceRange.baseMipLevel   = 0;
        TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        TransitionBarrier.SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
        GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);
        
        VkClearDepthStencilValue DepthStenciLValue;
        DepthStenciLValue.depth   = Depth;
        DepthStenciLValue.stencil = Stencil;
        
        VkImageSubresourceRange SubresourceRange = ImageView->GetSubresourceRange();
        GetCommandBuffer()->ClearDepthStencilImage(ImageView->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &DepthStenciLValue, 1, &SubresourceRange);
        
        // .. And transition back into "DepthStencilState"
        TransitionBarrier.PreviousLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        TransitionBarrier.NewLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        TransitionBarrier.SrcAccessMask  = VK_ACCESS_TRANSFER_WRITE_BIT;
        TransitionBarrier.DstAccessMask  = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        TransitionBarrier.SrcStageMask   = VK_PIPELINE_STAGE_TRANSFER_BIT;
        TransitionBarrier.DstStageMask   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);
    }
}

void FVulkanCommandContext::RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    FVulkanUnorderedAccessView* VulkanUnorderedAccessView = static_cast<FVulkanUnorderedAccessView*>(UnorderedAccessView);
    CHECK(VulkanUnorderedAccessView != nullptr);
    
    VkClearColorValue VulkanClearColor;
    FMemory::Memcpy(VulkanClearColor.float32, ClearColor.Data(), sizeof(VulkanClearColor.float32));

    if (VulkanUnorderedAccessView->HasImageView())
    {
        VkImageSubresourceRange SubresourceRange = VulkanUnorderedAccessView->GetImageSubresourceRange();
        GetCommandBuffer()->ClearColorImage(VulkanUnorderedAccessView->GetVkImage(), VK_IMAGE_LAYOUT_GENERAL, &VulkanClearColor, 1, &SubresourceRange);
    }
    else
    {
        // TODO: Clear UAV-Buffers
    }
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

    GetCommandBuffer()->BeginRenderPass(&RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void FVulkanCommandContext::RHIEndRenderPass()  
{
    GetCommandBuffer()->EndRenderPass();
}

void FVulkanCommandContext::RHISetViewport(const FRHIViewportRegion& ViewportRegion)
{
    static bool bNegativeViewport = true;
    
    VkViewport Viewport;
    if (bNegativeViewport)
    {
        Viewport.width    =  ViewportRegion.Width;
        Viewport.height   = -ViewportRegion.Height;
        Viewport.maxDepth =  ViewportRegion.MaxDepth;
        Viewport.minDepth =  ViewportRegion.MinDepth;
        Viewport.x        =  ViewportRegion.PositionX;
        Viewport.y        =  ViewportRegion.Height - ViewportRegion.PositionY;
    }
    else
    {
        Viewport.width    = ViewportRegion.Width;
        Viewport.height   = ViewportRegion.Height;
        Viewport.maxDepth = ViewportRegion.MaxDepth;
        Viewport.minDepth = ViewportRegion.MinDepth;
        Viewport.x        = ViewportRegion.PositionX;
        Viewport.y        = ViewportRegion.PositionY;
    }
    
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
        
        GetCommandBuffer()->CopyBuffer(Allocation.Buffer->GetVkBuffer(), VulkanBuffer->GetVkBuffer(), 1, &BufferCopy);
        FVulkanRHI::GetRHI()->GetDeletionQueue().Emplace(Allocation.Buffer.Get());
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

    GetCommandBuffer()->CopyBufferToImage(Allocation.Buffer->GetVkBuffer(), VulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);
    FVulkanRHI::GetRHI()->GetDeletionQueue().Emplace(Allocation.Buffer.Get());
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
    
    GetCommandBuffer()->ResolveImage(SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageResolve);
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
    
    GetCommandBuffer()->CopyBuffer(SrcVulkanBuffer->GetVkBuffer(), DstVulkanBuffer->GetVkBuffer(), 1, &BufferCopy);
}

void FVulkanCommandContext::RHICopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FVulkanTexture* SrcVulkanTexture = GetVulkanTexture(Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTexture* DstVulkanTexture = GetVulkanTexture(Dst);
    CHECK(DstVulkanTexture != nullptr);
    
    CHECK(SrcVulkanTexture->GetWidth()        == DstVulkanTexture->GetWidth());
    CHECK(SrcVulkanTexture->GetHeight()       == DstVulkanTexture->GetHeight());
    CHECK(SrcVulkanTexture->GetDepth()        == DstVulkanTexture->GetDepth());
    CHECK(SrcVulkanTexture->GetNumMipLevels() == DstVulkanTexture->GetNumMipLevels());
    CHECK(SrcVulkanTexture->GetDimension()    == DstVulkanTexture->GetDimension());
    
    constexpr uint32 MaxCopies = 15;
    VkImageCopy ImageCopies[MaxCopies];
    
    const FRHITextureDesc Desc = DstVulkanTexture->GetDesc();
    for (uint32 MipLevel = 0; MipLevel < Desc.NumMipLevels; MipLevel++)
    {
        VkImageCopy& ImageCopy = ImageCopies[MipLevel];
        FMemory::Memzero(&ImageCopy, sizeof(ImageCopy));
    
        ImageCopy.extent.width                  = FMath::Max<uint32>(Desc.Extent.x >> MipLevel, 1u);
        ImageCopy.extent.height                 = FMath::Max<uint32>(Desc.Extent.y >> MipLevel, 1u);
        ImageCopy.extent.depth                  = FMath::Max<uint32>(Desc.Extent.z >> MipLevel, 1u);
        ImageCopy.srcSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
        ImageCopy.srcSubresource.mipLevel       = MipLevel;
        ImageCopy.srcSubresource.baseArrayLayer = 0;
        ImageCopy.dstSubresource.aspectMask     = GetImageAspectFlagsFromFormat(DstVulkanTexture->GetVkFormat());
        ImageCopy.dstSubresource.mipLevel       = MipLevel;
        ImageCopy.dstSubresource.baseArrayLayer = 0;

        // NOTE: We want to copy the full function
        if (IsTextureCube(DstVulkanTexture->GetDimension()))
        {
            ImageCopy.srcSubresource.layerCount = SrcVulkanTexture->GetNumArraySlices() * VULKAN_NUM_CUBE_FACES;
            ImageCopy.dstSubresource.layerCount = DstVulkanTexture->GetNumArraySlices() * VULKAN_NUM_CUBE_FACES;
        }
        else
        {
            ImageCopy.srcSubresource.layerCount = SrcVulkanTexture->GetNumArraySlices();
            ImageCopy.dstSubresource.layerCount = DstVulkanTexture->GetNumArraySlices();
        }
    }
    
    GetCommandBuffer()->CopyImage(SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Desc.NumMipLevels, ImageCopies);
}

void FVulkanCommandContext::RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc)
{
    FVulkanTexture* SrcVulkanTexture = GetVulkanTexture(Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTexture* DstVulkanTexture = GetVulkanTexture(Dst);
    CHECK(DstVulkanTexture != nullptr);
    
    constexpr uint32 MaxCopies = 15;
    VkImageCopy ImageCopy[MaxCopies];
    
    uint32_t NumArrayLayers    = 0;
    uint32_t DstBaseArrayLayer = 0;
    uint32_t SrcBaseArrayLayer = 0;
    if (IsTextureCube(SrcVulkanTexture->GetDimension()))
    {
        SrcBaseArrayLayer = CopyDesc.SrcArraySlice  * VULKAN_NUM_CUBE_FACES;
        NumArrayLayers    = CopyDesc.NumArraySlices * VULKAN_NUM_CUBE_FACES;
    }
    else
    {
        SrcBaseArrayLayer = CopyDesc.SrcArraySlice;
        NumArrayLayers    = CopyDesc.NumArraySlices;
    }
    
    if (IsTextureCube(DstVulkanTexture->GetDimension()))
    {
        DstBaseArrayLayer = CopyDesc.DstArraySlice  * VULKAN_NUM_CUBE_FACES;
        NumArrayLayers    = FMath::Max(CopyDesc.NumArraySlices * VULKAN_NUM_CUBE_FACES, NumArrayLayers);
    }
    else
    {
        DstBaseArrayLayer = CopyDesc.DstArraySlice;
        NumArrayLayers    = FMath::Max(CopyDesc.NumArraySlices * VULKAN_NUM_CUBE_FACES, NumArrayLayers);
    }
    
    // We copy each layer seperatly due to MoltenVK seems to be acting weird when doing all layers seperatly
        for (uint32 ArrayLayer = 0; ArrayLayer < NumArrayLayers; ArrayLayer++)
    {
        for (uint32 MipLevel = 0; MipLevel < CopyDesc.NumMipLevels; MipLevel++)
        {
            VkImageCopy& CopyInfo = ImageCopy[MipLevel];
            FMemory::Memzero(&CopyInfo, sizeof(CopyInfo));
            
            // Describe the source subresource
            CopyInfo.srcSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
            CopyInfo.srcSubresource.mipLevel       = CopyDesc.SrcMipSlice + MipLevel;
            CopyInfo.srcOffset.x                   = CopyDesc.SrcPosition.x >> MipLevel;
            CopyInfo.srcOffset.y                   = CopyDesc.SrcPosition.y >> MipLevel;
            CopyInfo.srcOffset.z                   = CopyDesc.SrcPosition.z >> MipLevel;
            CopyInfo.srcSubresource.baseArrayLayer = SrcBaseArrayLayer + ArrayLayer;
            CopyInfo.srcSubresource.layerCount     = 1;
            
            // Describe the destination subresource
            CopyInfo.dstSubresource.aspectMask     = GetImageAspectFlagsFromFormat(DstVulkanTexture->GetVkFormat());
            CopyInfo.dstSubresource.mipLevel       = CopyDesc.DstMipSlice + MipLevel;
            CopyInfo.dstOffset.x                   = CopyDesc.DstPosition.x >> MipLevel;
            CopyInfo.dstOffset.y                   = CopyDesc.DstPosition.y >> MipLevel;
            CopyInfo.dstOffset.z                   = CopyDesc.DstPosition.z >> MipLevel;
            CopyInfo.dstSubresource.baseArrayLayer = DstBaseArrayLayer + ArrayLayer;
            CopyInfo.dstSubresource.layerCount     = 1;
            
            // Size of this mipslice
            CopyInfo.extent.width  = FMath::Max(CopyDesc.Size.x >> MipLevel, 1);
            CopyInfo.extent.height = FMath::Max(CopyDesc.Size.y >> MipLevel, 1);
            CopyInfo.extent.depth  = FMath::Max(CopyDesc.Size.z >> MipLevel, 1);
        }
        
        GetCommandBuffer()->CopyImage(SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, CopyDesc.NumMipLevels, ImageCopy);
    }
}

void FVulkanCommandContext::RHIDestroyResource(FRHIResource* Resource)  
{
    if (Resource)
    {
        FVulkanRHI::GetRHI()->GetDeletionQueue().Emplace(Resource);
    }
}

void FVulkanCommandContext::RHIDiscardContents(FRHITexture* Resource)
{
    UNREFERENCED_VARIABLE(Resource);
}

void FVulkanCommandContext::RHIBuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, FRHIBuffer* VertexBuffer, uint32 NumVertices, FRHIBuffer* IndexBuffer, uint32 NumIndices, EIndexFormat IndexFormat, bool bUpdate)
{
    UNREFERENCED_VARIABLE(RayTracingGeometry);
    UNREFERENCED_VARIABLE(VertexBuffer);
    UNREFERENCED_VARIABLE(NumVertices);
    UNREFERENCED_VARIABLE(IndexBuffer);
    UNREFERENCED_VARIABLE(NumIndices);
    UNREFERENCED_VARIABLE(IndexFormat);
    UNREFERENCED_VARIABLE(bUpdate);
}

void FVulkanCommandContext::RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
{
    UNREFERENCED_VARIABLE(RayTracingScene);
    UNREFERENCED_VARIABLE(Instances);
    UNREFERENCED_VARIABLE(bUpdate);
}

void FVulkanCommandContext::RHISetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources)
{
    UNREFERENCED_VARIABLE(RayTracingScene);
    UNREFERENCED_VARIABLE(PipelineState);
    UNREFERENCED_VARIABLE(GlobalResource);
    UNREFERENCED_VARIABLE(RayGenLocalResources);
    UNREFERENCED_VARIABLE(MissLocalResources);
    UNREFERENCED_VARIABLE(HitGroupResources);
    UNREFERENCED_VARIABLE(NumHitGroupResources);
}

void FVulkanCommandContext::RHIGenerateMips(FRHITexture* Texture)
{
    FVulkanTexture* VulkanTexture = static_cast<FVulkanTexture*>(Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR("Texture cannot be nullptr");
        return;
    }
    
    // Only support ColorFormats
    VkFormat VulkanFormat = VulkanTexture->GetVkFormat();
    CHECK(GetImageAspectFlagsFromFormat(VulkanFormat) == VK_IMAGE_ASPECT_COLOR_BIT);
    
    // TODO: Fix for devices that do NOT support linear filtering
    constexpr VkImageCreateFlags NecessaryFeatureFlags = VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;
    VkFormatProperties FormatProperties = GetDevice()->GetPhysicalDevice()->GetFormatProperties(VulkanFormat);
    if ((FormatProperties.optimalTilingFeatures & NecessaryFeatureFlags) != NecessaryFeatureFlags)
    {
        VULKAN_ERROR("Device does not support generating miplevels at the moment");
        return;
    }

    FRHITextureDesc TextureDesc = VulkanTexture->GetDesc();
    const uint32 MipLevelCount = TextureDesc.NumMipLevels;
    if (MipLevelCount < 2)
    {
        VULKAN_ERROR("MipLevels must be more than one in order to generate any Mips");
        return;
    }
    
    constexpr VkImageCreateFlags NecessaryUsageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkImageCreateInfo ImageCreateInfo = VulkanTexture->GetVkImageCreateInfo();
    if ((ImageCreateInfo.usage & NecessaryUsageFlags) != NecessaryUsageFlags)
    {
        VULKAN_ERROR("Input texture is missing the VK_IMAGE_USAGE_TRANSFER_SRC_BIT and VK_IMAGE_USAGE_TRANSFER_DST_BIT flags");
        return;
    }

    VkImage    VulkanImage       = VulkanTexture->GetVkImage();
    VkExtent2D SourceExtent      = { static_cast<uint32>(TextureDesc.Extent.x), static_cast<uint32>(TextureDesc.Extent.y) };
    VkExtent2D DestinationExtent = {};
    
    // Setup TransitionBarrier since these parameters will be the same for all MipLevels
    FVulkanImageTransitionBarrier TransitionBarrier;
    TransitionBarrier.PreviousLayout                  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    TransitionBarrier.NewLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    TransitionBarrier.Image                           = VulkanImage;
    TransitionBarrier.DependencyFlags                 = 0;
    TransitionBarrier.SrcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
    TransitionBarrier.DstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
    TransitionBarrier.SrcStageMask                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
    TransitionBarrier.DstStageMask                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
    TransitionBarrier.SubresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
    TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    TransitionBarrier.SubresourceRange.levelCount     = 1;

    const uint32 NumArrayLayers = IsTextureCube(VulkanTexture->GetDimension()) ? TextureDesc.NumArraySlices * VULKAN_NUM_CUBE_FACES : TextureDesc.NumArraySlices;
    for (uint32 Index = 1; Index < MipLevelCount; Index++)
    {
        DestinationExtent = { FMath::Max(SourceExtent.width / 2U, 1u), FMath::Max(SourceExtent.height / 2U, 1U) };

        // Transition this the source MipLevel into correct layout
        TransitionBarrier.SubresourceRange.baseMipLevel = Index - 1;
        GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);

        VkImageBlit ImageBlit;
        FMemory::Memzero(&ImageBlit);
        
        ImageBlit.srcOffsets[0]                  = { 0, 0, 0 };
        ImageBlit.srcOffsets[1]                  = { static_cast<int32>(SourceExtent.width), static_cast<int32>(SourceExtent.height), 1 };
        ImageBlit.srcSubresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageBlit.srcSubresource.mipLevel        = Index - 1;
        ImageBlit.srcSubresource.baseArrayLayer  = 0;
        ImageBlit.srcSubresource.layerCount      = NumArrayLayers;
        ImageBlit.dstOffsets[0]                  = { 0, 0, 0 };
        ImageBlit.dstOffsets[1]                  = { static_cast<int32>(DestinationExtent.width), static_cast<int32>(DestinationExtent.height), 1 };
        ImageBlit.dstSubresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageBlit.dstSubresource.mipLevel        = Index;
        ImageBlit.dstSubresource.baseArrayLayer  = 0;
        ImageBlit.dstSubresource.layerCount      = NumArrayLayers;

        GetCommandBuffer()->BlitImage(VulkanImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VulkanImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageBlit, VK_FILTER_LINEAR);
        SourceExtent = DestinationExtent;
    }

    TransitionBarrier.SubresourceRange.baseMipLevel = MipLevelCount - 1;
    GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);
    
    // Insert a final TransitionBarrier from Source to Destination since the texture is expected to be in CopyDest
    TransitionBarrier.PreviousLayout                  = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    TransitionBarrier.NewLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    TransitionBarrier.Image                           = VulkanImage;
    TransitionBarrier.DependencyFlags                 = 0;
    TransitionBarrier.SrcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
    TransitionBarrier.DstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
    TransitionBarrier.SrcStageMask                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
    TransitionBarrier.DstStageMask                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
    TransitionBarrier.SubresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
    TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    TransitionBarrier.SubresourceRange.baseMipLevel   = 0;
    TransitionBarrier.SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
    
    GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);
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
    
        GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);
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

    GetCommandBuffer()->BufferMemoryPipelineBarrier(BufferBarrier);
}

void FVulkanCommandContext::RHIUnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(Texture);
    if (!VulkanTexture)
    {
        VULKAN_WARNING("Texture is nullptr");
        return;
    }

    FVulkanImageTransitionBarrier TransitionBarrier;
    TransitionBarrier.PreviousLayout                  = VK_IMAGE_LAYOUT_GENERAL;
    TransitionBarrier.NewLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    TransitionBarrier.Image                           = VulkanTexture->GetVkImage();
    TransitionBarrier.DependencyFlags                 = 0;
    TransitionBarrier.SrcAccessMask                   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    TransitionBarrier.DstAccessMask                   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    TransitionBarrier.SrcStageMask                    = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    TransitionBarrier.DstStageMask                    = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    TransitionBarrier.SubresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(VulkanTexture->GetVkFormat());
    TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
    TransitionBarrier.SubresourceRange.baseMipLevel   = 0;
    TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    TransitionBarrier.SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);
}

void FVulkanCommandContext::RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer)   
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(Buffer);
    if (!VulkanBuffer)
    {
        VULKAN_WARNING("Buffer is nullptr");
        return;
    }
    
    FVulkanBufferBarrier BufferBarrier;
    BufferBarrier.SrcStageMask    = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    BufferBarrier.DstStageMask    = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    BufferBarrier.DependencyFlags = 0;
    BufferBarrier.Buffer          = VulkanBuffer->GetVkBuffer();
    BufferBarrier.SrcAccessMask   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    BufferBarrier.DstAccessMask   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    BufferBarrier.Offset          = 0;
    BufferBarrier.Size            = VK_WHOLE_SIZE;

    GetCommandBuffer()->BufferMemoryPipelineBarrier(BufferBarrier);
}

void FVulkanCommandContext::RHIDraw(uint32 VertexCount, uint32 StartVertexLocation)
{
    ContextState.BindGraphicsStates();
    GetCommandBuffer()->Draw(VertexCount, 1, StartVertexLocation, 0);
}

void FVulkanCommandContext::RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    ContextState.BindGraphicsStates();
    GetCommandBuffer()->DrawIndexed(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void FVulkanCommandContext::RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    ContextState.BindGraphicsStates();
    GetCommandBuffer()->Draw(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FVulkanCommandContext::RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    ContextState.BindGraphicsStates();
    GetCommandBuffer()->DrawIndexed(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FVulkanCommandContext::RHIDispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    ContextState.BindComputeState();
    GetCommandBuffer()->Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
}

void FVulkanCommandContext::RHIDispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
    // TODO: Implement Vulkan RT
    UNREFERENCED_VARIABLE(InScene);
    UNREFERENCED_VARIABLE(InPipelineState);
    UNREFERENCED_VARIABLE(InWidth);
    UNREFERENCED_VARIABLE(InHeight);
    UNREFERENCED_VARIABLE(InDepth);
}

void FVulkanCommandContext::RHIPresentViewport(FRHIViewport* Viewport, bool bVerticalSync)
{
    FinishCommandBuffer(false);

    FVulkanViewport* VulkanViewport = static_cast<FVulkanViewport*>(Viewport);
    VulkanViewport->Present(bVerticalSync);

    ObtainCommandBuffer();
}

void FVulkanCommandContext::RHIResizeViewport(FRHIViewport* Viewport, uint32 Width, uint32 Height)
{
    FVulkanViewport* VulkanViewport = static_cast<FVulkanViewport*>(Viewport);
    VulkanViewport->Resize(Width, Height);
}

void FVulkanCommandContext::RHIClearState()
{
    SCOPED_LOCK(CommandContextCS);

    if (CommandBuffer)
    {
        FinishCommandBuffer(true);
    }
    
    Queue.WaitForCompletion();
    
    // After work is finished we can clear the state
    ContextState.ResetState();
}

void FVulkanCommandContext::RHIFlush()
{
    SCOPED_LOCK(CommandContextCS);

    if (CommandBuffer)
    {
        FinishCommandBuffer(true);
    }

    Queue.WaitForCompletion();
}

void FVulkanCommandContext::RHIInsertMarker(const FStringView& Message)
{
#if VK_EXT_debug_utils
    if (FVulkanDebugUtilsEXT::IsEnabled())
    {
        VkDebugUtilsLabelEXT DebugUtilsLabel;
        FMemory::Memzero(&DebugUtilsLabel);
        
        DebugUtilsLabel.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        DebugUtilsLabel.pLabelName = Message.GetCString();
        DebugUtilsLabel.color[0]   = 0.0f;
        DebugUtilsLabel.color[1]   = 0.0f;
        DebugUtilsLabel.color[2]   = 0.0f;
        DebugUtilsLabel.color[3]   = 1.0f;
        
        GetCommandBuffer()->InsertDebugUtilsLabel(&DebugUtilsLabel);
    }
#endif
}

void FVulkanCommandContext::RHIBeginExternalCapture()
{
    // TODO: Investigate the probability of this
}

void FVulkanCommandContext::RHIEndExternalCapture()  
{
    // TODO: Investigate the probability of this
}
