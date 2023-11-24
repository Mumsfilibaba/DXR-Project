#include "VulkanCommandContext.h"
#include "VulkanResourceViews.h"
#include "VulkanTexture.h"
#include "VulkanViewport.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FVulkanCommandContextState::FVulkanCommandContextState(FVulkanDevice* InDevice)
    : FVulkanDeviceObject(InDevice)
    , bIsReady(false)
    , bIsCapturing(false)
    , bIsRenderPassActive(false)
{
}

bool FVulkanCommandContextState::Initialize()
{
    // if (!DescriptorCache.Initialize())
    // {
    //    D3D12_ERROR("Failed to initialize DescriptorCache");
    //    return false;
    // }

    return true;
}

void FVulkanCommandContextState::ApplyGraphics(FVulkanCommandBuffer& CommandBuffer)
{
    // VertexBuffer and IndexBuffer
    if (Graphics.bBindVertexBuffers)
    {
        // DescriptorCache.SetVertexBuffers(Graphics.VBCache);
        CommandBuffer.BindVertexBuffers(0, Graphics.VBCache.NumVertexBuffers, Graphics.VBCache.VertexBuffers, Graphics.VBCache.VertexBufferOffsets);
        Graphics.bBindVertexBuffers = false;
    }

    if (Graphics.bBindIndexBuffer)
    {
        // DescriptorCache.SetIndexBuffer(Graphics.IBCache);
        CommandBuffer.BindIndexBuffer(Graphics.IBCache.IndexBuffer, Graphics.IBCache.Offset, Graphics.IBCache.IndexType);
        Graphics.bBindIndexBuffer = false;
    }

    // Scissor and Viewport
    if (Graphics.bBindViewports)
    {
        CommandBuffer.SetViewport(0, Graphics.NumViewports, Graphics.Viewports);
        Graphics.bBindViewports = false;
    }

    if (Graphics.bBindScissorRects)
    {
        CommandBuffer.SetScissor(0, Graphics.NumScissor, Graphics.ScissorRects);
        Graphics.bBindScissorRects = false;
    }

    // BlendFactor
    if (Graphics.bBindBlendFactor)
    {
        CommandBuffer.SetBlendConstants(Graphics.BlendFactor.Data());
        Graphics.bBindBlendFactor = false;
    }

    // Descriptors
    //DescriptorCache.PrepareGraphicsDescriptors(Batch, CurrentRootSignture, FD3D12PipelineStageMask::BasicGraphicsMask());

    // Constants
    if (Graphics.PipelineState)
    {
        // Pipeline
        if (Graphics.bBindPipeline)
        {
            VkPipeline Pipeline = Graphics.PipelineState->GetVkPipeline();
            CommandBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
            Graphics.bBindPipeline = false;
        }

        VkPipelineLayout PipelineLayout = Graphics.PipelineState->GetVkPipelineLayout();
        PushConstantsCache.Commit(CommandBuffer, PipelineLayout);
    }
}

void FVulkanCommandContextState::ApplyCompute(FVulkanCommandBuffer& CommandBuffer)
{

    // Descriptors
    // DescriptorCache.PrepareComputeDescriptors(Batch, CurrentRootSignture);

    // Constants
    if (Compute.PipelineState)
    {
        // Pipeline
        if (Compute.bBindPipeline)
        {
            VkPipeline Pipeline = Compute.PipelineState->GetVkPipeline();
            CommandBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
            Compute.bBindPipeline = false;
        }
        
        VkPipelineLayout PipelineLayout = Compute.PipelineState->GetVkPipelineLayout();
        PushConstantsCache.Commit(CommandBuffer, PipelineLayout);
    }
}

void FVulkanCommandContextState::ClearGraphics()
{
    Graphics.VBCache.Clear();
    Graphics.IBCache.Clear();

    Graphics.PipelineState.Reset();

    //Graphics.ShadingRateTexture.Reset();

    Graphics.bBindRenderTargets = true;
    Graphics.bBindBlendFactor   = true;
    Graphics.bBindPipeline      = true;
    Graphics.bBindVertexBuffers = true;
    Graphics.bBindIndexBuffer   = true;
    Graphics.bBindScissorRects  = true;
    Graphics.bBindViewports     = true;
}

void FVulkanCommandContextState::ClearCompute()
{
    Compute.PipelineState.Reset();
    Compute.bBindPipeline = true;
}

void FVulkanCommandContextState::SetVertexBuffer(FVulkanBuffer* VertexBuffer, uint32 Slot)
{
    VkBuffer CurrentBuffer = Graphics.VBCache.VertexBuffers[Slot];
    VkBuffer NewBuffer = VertexBuffer ? VertexBuffer->GetVkBuffer() : VK_NULL_HANDLE;
    if (VertexBuffer)
    {
        Graphics.VBCache.VertexBuffers[Slot]       = NewBuffer;
        Graphics.VBCache.VertexBufferOffsets[Slot] = 0;
    }
    else
    {
        Graphics.VBCache.VertexBuffers[Slot]       = VK_NULL_HANDLE;
        Graphics.VBCache.VertexBufferOffsets[Slot] = 0;
    }

    if (NewBuffer != CurrentBuffer)
    {
        Graphics.bBindVertexBuffers = true;
    }
}

void FVulkanCommandContextState::SetIndexBuffer(FVulkanBuffer* IndexBuffer, VkIndexType IndexFormat)
{
    VkBuffer CurrentBuffer = Graphics.IBCache.IndexBuffer;
    VkBuffer NewBuffer = IndexBuffer ? IndexBuffer->GetVkBuffer() : VK_NULL_HANDLE;
    if (IndexBuffer)
    {
        Graphics.IBCache.IndexType   = IndexFormat;
        Graphics.IBCache.IndexBuffer = NewBuffer;
        Graphics.IBCache.Offset      = 0;
    }
    else
    {
        Graphics.IBCache.Clear();
    }

    if (NewBuffer != CurrentBuffer)
    {
        Graphics.bBindIndexBuffer = true;
    }
}


FVulkanCommandContext::FVulkanCommandContext(FVulkanDevice* InDevice, FVulkanQueue* InCommandQueue)
    : FVulkanDeviceObject(InDevice)
    , Queue(MakeSharedRef<FVulkanQueue>(InCommandQueue))
    , CommandBuffer(InDevice, InCommandQueue->GetType())
    , ContextState(InDevice)
{
}

FVulkanCommandContext::~FVulkanCommandContext()
{
}

bool FVulkanCommandContext::Initialize()
{
    if (!CommandBuffer.Initialize(VK_COMMAND_BUFFER_LEVEL_PRIMARY))
    {
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

        FVulkanCommandBuffer* SubmitCommandBuffer = GetCommandBuffer();
        Queue->ExecuteCommandBuffer(&SubmitCommandBuffer, 1, CommandBuffer.GetFence());
    }
    
    ContextState.ClearAll();
}

void FVulkanCommandContext::RHIStartContext()
{
    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Lock to the thread that started the context
    CommandContextCS.Lock();
    ObtainCommandBuffer();
}

void FVulkanCommandContext::RHIFinishContext()
{
    FlushCommandBuffer();
    
    // TODO: Remove lock, the command context itself should only be used from a single thread
    // Unlock from the thread that started the context
    CommandContextCS.Unlock();
}

void FVulkanCommandContext::RHIBeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
{
}

void FVulkanCommandContext::RHIEndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)  
{
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
}

void FVulkanCommandContext::RHIBeginRenderPass(const FRHIRenderPassDesc& RenderPassInitializer)
{
    // TODO: Verify that the samples are all the same
    uint8  NumSamples = 0;
    uint32 Width      = 0;
    uint32 Height     = 0;
        
    // Collect ClearValues
    uint32 NumClearValues = 0;
    VkClearValue ClearValues[FRHILimits::MaxRenderTargets + 1];
    FMemory::Memzero(ClearValues, sizeof(ClearValues));
    
    // Check for each render-target-texture
    FVulkanRenderPassKey RenderPassKey;
    RenderPassKey.NumRenderTargets = RenderPassInitializer.NumRenderTargets;
    
    FVulkanFramebufferKey FramebufferKey;
    for (uint32 Index = 0; Index < RenderPassInitializer.NumRenderTargets; Index++)
    {
        const FRHIRenderTargetView& RenderTargetView = RenderPassInitializer.RenderTargets[Index];
        if (FVulkanTexture* VulkanTexture = GetVulkanTexture(RenderTargetView.Texture))
        {
            Width      = FMath::Max<int32>(VulkanTexture->GetWidth(), Width);
            Height     = FMath::Max<int32>(VulkanTexture->GetHeight(), Height);
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
    }
    
    // Check for depth-texture
    if (FVulkanTexture* VulkanTexture = GetVulkanTexture(RenderPassInitializer.DepthStencilView.Texture))
    {
        Width      = FMath::Max<int32>(VulkanTexture->GetWidth(), Width);
        Height     = FMath::Max<int32>(VulkanTexture->GetHeight(), Height);
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

    // Retrieve or create a RenderPass
    RenderPassKey.NumSamples = NumSamples;

    VkRenderPass RenderPass = GetDevice()->GetRenderPassCache().GetRenderPass(RenderPassKey);
    if (!VULKAN_CHECK_HANDLE(RenderPass))
    {
        DEBUG_BREAK();
    }
    
    // Retrieve or create a FrameBuffer
    FramebufferKey.RenderPass = RenderPass;
    FramebufferKey.Width      = static_cast<uint16>(Width);
    FramebufferKey.Height     = static_cast<uint16>(Height);

    VkFramebuffer FrameBuffer =  GetDevice()->GetFramebufferCache().GetFramebuffer(FramebufferKey);
    if (!VULKAN_CHECK_HANDLE(FrameBuffer))
    {
        DEBUG_BREAK();
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
    VkViewport& Viewport = ContextState.Graphics.Viewports[0];
    Viewport.width    = ViewportRegion.Width;
    Viewport.height   = ViewportRegion.Height;
    Viewport.maxDepth = ViewportRegion.MaxDepth;
    Viewport.minDepth = ViewportRegion.MinDepth;
    Viewport.x        = ViewportRegion.PositionX;
    Viewport.y        = ViewportRegion.PositionY;
    
    ContextState.Graphics.NumViewports   = 1;
    ContextState.Graphics.bBindViewports = true;
}

void FVulkanCommandContext::RHISetScissorRect(const FRHIScissorRegion& ScissorRegion)
{
    VkRect2D& ScissorRect = ContextState.Graphics.ScissorRects[0];
    ScissorRect.offset.x      = static_cast<int32_t>(ScissorRegion.PositionX);
    ScissorRect.extent.width  = static_cast<int32_t>(ScissorRegion.Width);
    ScissorRect.offset.y      = static_cast<int32_t>(ScissorRegion.PositionY);
    ScissorRect.extent.height = static_cast<int32_t>(ScissorRegion.Height);
    
    ContextState.Graphics.NumScissor        = 1;
    ContextState.Graphics.bBindScissorRects = true;
}

void FVulkanCommandContext::RHISetBlendFactor(const FVector4& Color)
{
    ContextState.Graphics.BlendFactor      = Color;
    ContextState.Graphics.bBindBlendFactor = true;
}

void FVulkanCommandContext::RHISetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 Index = 0; Index < InVertexBuffers.Size(); ++Index)
    {
        FVulkanBuffer* VulkanVertexBuffer = static_cast<FVulkanBuffer*>(InVertexBuffers[Index]);
        ContextState.SetVertexBuffer(VulkanVertexBuffer, BufferSlot + Index);
    }

    ContextState.Graphics.VBCache.NumVertexBuffers = FMath::Max(ContextState.Graphics.VBCache.NumVertexBuffers, BufferSlot + InVertexBuffers.Size());
}

void FVulkanCommandContext::RHISetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    FVulkanBuffer* VulkanIndexBuffer = static_cast<FVulkanBuffer*>(IndexBuffer);
    ContextState.SetIndexBuffer(VulkanIndexBuffer, ConvertIndexFormat(IndexFormat));
}

void FVulkanCommandContext::RHISetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState)
{
    FVulkanGraphicsPipelineState* VulkanPipelineState = static_cast<FVulkanGraphicsPipelineState*>(PipelineState);
    if (ContextState.Graphics.PipelineState != VulkanPipelineState)
    {
        ContextState.Graphics.PipelineState = MakeSharedRef<FVulkanGraphicsPipelineState>(VulkanPipelineState);
        ContextState.Graphics.bBindPipeline = true;
    }
}

void FVulkanCommandContext::RHISetComputePipelineState(class FRHIComputePipelineState* PipelineState)  
{
    FVulkanComputePipelineState* VulkanPipelineState = static_cast<FVulkanComputePipelineState*>(PipelineState);
    if (ContextState.Compute.PipelineState != VulkanPipelineState)
    {
        ContextState.Compute.PipelineState = MakeSharedRef<FVulkanComputePipelineState>(VulkanPipelineState);
        ContextState.Compute.bBindPipeline = true;
    }
}

void FVulkanCommandContext::RHISet32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
    UNREFERENCED_VARIABLE(Shader);
    ContextState.PushConstantsCache.SetPushConstants(reinterpret_cast<const uint32*>(Shader32BitConstants), Num32BitConstants);
}

void FVulkanCommandContext::RHISetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> ShaderResourceViews, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> UnorderedAccessViews, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> ConstantBuffers, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> SamplerStates, uint32 ParameterIndex)
{
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
}

void FVulkanCommandContext::RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer)   
{
}

void FVulkanCommandContext::RHIDraw(uint32 VertexCount, uint32 StartVertexLocation)
{
    ContextState.ApplyGraphics(CommandBuffer);
}

void FVulkanCommandContext::RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    ContextState.ApplyGraphics(CommandBuffer);
}

void FVulkanCommandContext::RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    ContextState.ApplyGraphics(CommandBuffer);
}

void FVulkanCommandContext::RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    ContextState.ApplyGraphics(CommandBuffer);
}

void FVulkanCommandContext::RHIDispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    ContextState.ApplyCompute(CommandBuffer);
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
