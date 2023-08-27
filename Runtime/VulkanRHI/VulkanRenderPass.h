//#pragma once
//#include "VulkanDeviceObject.h"
//#include "Core/Containers/Map.h"
//
//struct FVulkanRenderPassCacheKey
//{
//    FVulkanRenderPassCacheKey();
//
//    uint64 GetHash() const;
//    
//    bool operator==(const FVulkanRenderPassCacheKey& Other) const;
//
//    bool operator!=(const FVulkanRenderPassCacheKey& Other) const
//    {
//        return !(*this == Other);
//    }
//
//    EFormat DepthStencilFormat;
//    EFormat RenderTargetFormats[LAMBDA_MAX_RENDERTARGET_COUNT];
//    uint32  NumRenderTargets;
//    uint32  SampleCount;
//    uint64  Hash;
//};
//
//
//struct VKNRenderPassCacheKeyHash
//{
//    size_t operator()(const VKNRenderPassCacheKey& key) const
//    {
//        return key.GetHash();
//    }
//};
//
//
//class FVulkanRenderPassCache : public FVulkanDeviceObject
//{
//public:
//    FVulkanRenderPassCache(FVulkanDevice* InDevice);
//    ~FVulkanRenderPassCache();
//
//    VkRenderPass GetRenderPass(const FVulkanRenderPassCacheKey& key);
//    
//    void ReleaseAll();
//
//private:
//    TMap<VKNRenderPassCacheKey, VkRenderPass, VKNRenderPassCacheKeyHash> RenderPasses;
//};