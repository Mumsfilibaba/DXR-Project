//VKNRenderPassCacheKey::VKNRenderPassCacheKey()
//    : Hash(0),
//    NumRenderTargets(0),
//    SampleCount(0),
//    DepthStencilFormat(FORMAT_UNKNOWN),
//    RenderTargetFormats()
//{
//    for (uint32 i = 0; i < LAMBDA_MAX_RENDERTARGET_COUNT; i++)
//        RenderTargetFormats[i] = FORMAT_UNKNOWN;
//}
//
//
//bool VKNRenderPassCacheKey::operator==(const VKNRenderPassCacheKey& other) const
//{
//    if (GetHash() != other.GetHash() || NumRenderTargets != other.NumRenderTargets ||
//        SampleCount != other.SampleCount || DepthStencilFormat != other.DepthStencilFormat)
//    {
//        return false;
//    }
//
//    for (uint32 i = 0; i < NumRenderTargets; i++)
//    {
//        if (RenderTargetFormats[i] != other.RenderTargetFormats[i])
//            return false;
//    }
//
//    return true;
//}
//
//
//std::size_t VKNRenderPassCacheKey::GetHash() const
//{
//    if (Hash == 0)
//    {
//        HashCombine<uint32>(Hash, SampleCount);
//        HashCombine<uint32>(Hash, DepthStencilFormat);
//        for (uint32 i = 0; i < NumRenderTargets; i++)
//            HashCombine<uint32>(Hash, RenderTargetFormats[i]);
//    }
//
//    return Hash;
//}
//
////-------------------
////VKNRenderPassCache
////-------------------
//
//VKNRenderPassCache* VKNRenderPassCache::s_pInstance = nullptr;
//
//VKNRenderPassCache::VKNRenderPassCache(VKNDevice* pVkDevice)
//    : m_pDevice(pVkDevice),
//    m_RenderPasses()
//{
//    LAMBDA_ASSERT(s_pInstance == nullptr);
//    s_pInstance = this;
//}
//
//
//VKNRenderPassCache::~VKNRenderPassCache()
//{
//    if (s_pInstance == this)
//        s_pInstance = nullptr;
//
//    ReleaseAll();
//}
//
//
//VkRenderPass VKNRenderPassCache::GetRenderPass(const VKNRenderPassCacheKey& key)
//{
//    //Check if the renderpass exists
//    auto pass = m_RenderPasses.find(key);
//    if (pass != m_RenderPasses.end())
//        return pass->second;
//
//    //Setup color attachments
//    std::vector<VkAttachmentReference>      colorAttachentRefs;
//    std::vector<VkAttachmentDescription>    attachments;
//
//    //Number of samples (MSAA)
//    VkSampleCountFlagBits sampleCount = ConvertSampleCount(key.SampleCount);
//
//    //Setup colorattachments
//    for (uint32 i = 0; i < key.NumRenderTargets; i++)
//    {
//        //Setup attachments
//        VkAttachmentDescription colorAttachment = {};
//        colorAttachment.flags			= 0;
//        colorAttachment.format			= ConvertFormat(key.RenderTargetFormats[i]);
//        colorAttachment.samples			= sampleCount;
//        colorAttachment.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
//        colorAttachment.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
//        colorAttachment.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//        colorAttachment.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
//        colorAttachment.initialLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//        colorAttachment.finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//        attachments.emplace_back(colorAttachment);
//
//        VkAttachmentReference colorAttachmentRef = {};
//        colorAttachmentRef.attachment	= i;
//        colorAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//        colorAttachentRefs.emplace_back(colorAttachmentRef);
//    }
//
//    //Describe subpass
//    VkSubpassDescription subpass = {};
//    subpass.flags					= 0;
//    subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
//    subpass.colorAttachmentCount	= uint32(colorAttachentRefs.size());
//    subpass.pColorAttachments		= colorAttachentRefs.data();
//    subpass.preserveAttachmentCount = 0;
//    subpass.pPreserveAttachments	= nullptr;
//    subpass.inputAttachmentCount	= 0;
//    subpass.pInputAttachments		= nullptr;
//    subpass.pResolveAttachments		= nullptr;
//
//    //Setup depthstencil
//    VkAttachmentReference depthAttachmentRef = {};
//    if (key.DepthStencilFormat == FORMAT_UNKNOWN)
//    {
//        subpass.pDepthStencilAttachment = nullptr;
//    }
//    else
//    {
//        VkAttachmentDescription depthAttachment = {};
//        depthAttachment.flags			= 0;
//        depthAttachment.format			= ConvertFormat(key.DepthStencilFormat);
//        depthAttachment.samples			= sampleCount;
//        depthAttachment.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
//        depthAttachment.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
//        depthAttachment.stencilLoadOp	= depthAttachment.loadOp;
//        depthAttachment.stencilStoreOp	= depthAttachment.storeOp;
//        depthAttachment.initialLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//        depthAttachment.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//        attachments.emplace_back(depthAttachment);
//
//        depthAttachmentRef.attachment	= uint32(attachments.size() - 1);
//        depthAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//        subpass.pDepthStencilAttachment = &depthAttachmentRef;
//    }
//
//
//    //Create renderpass
//    VkRenderPassCreateInfo renderPassInfo = {};
//    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//    renderPassInfo.flags = 0;
//    renderPassInfo.pNext = nullptr;
//    renderPassInfo.attachmentCount = uint32(attachments.size());
//    renderPassInfo.pAttachments = attachments.data();
//    renderPassInfo.subpassCount = 1;
//    renderPassInfo.pSubpasses = &subpass;
//    renderPassInfo.pDependencies = nullptr;
//
//    VkRenderPass renderPass = VK_NULL_HANDLE;
//    if (vkCreateRenderPass(m_pDevice->GetVkDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
//    {
//        LOG_DEBUG_ERROR("[Vulkan] Failed to create renderpass\n");
//        return VK_NULL_HANDLE;
//    }
//    else
//    {
//        LOG_DEBUG_INFO("[Vulkan] Created new renderpass\n");
//
//        m_RenderPasses.insert(std::pair<VKNRenderPassCacheKey, VkRenderPass>(key, renderPass));
//        return renderPass;
//    }
//}
//
//
//void VKNRenderPassCache::ReleaseAll()
//{
//    VKNFramebufferCache& cache = VKNFramebufferCache::Get();
//
//    //Destroy all renderpasses
//    for (auto& pass : m_RenderPasses)
//    {
//        //Release all framebuffers with this renderpass
//        cache.OnReleaseRenderPass(pass.second);
//
//        //Safely destroy this renderpass
//        if (pass.second != VK_NULL_HANDLE)
//            m_pDevice->SafeReleaseVkResource(pass.second);
//    }
//
//    //Clear all
//    m_RenderPasses.clear();
//}
//
//
//VKNRenderPassCache& VKNRenderPassCache::Get()
//{
//    LAMBDA_ASSERT(s_pInstance != nullptr);
//    return *s_pInstance;
//}