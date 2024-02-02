#pragma once
#include "VulkanDeviceChild.h"
#include "VulkanShader.h"
#include "Core/Containers/Map.h"

#define DESCRIPTOR_SET_STAGE_COUNT (5)

struct FDescriptorSetLayout
{
    FDescriptorSetLayout()
        : PackedLayout(0)
    {
    }
    
    bool operator==(const FDescriptorSetLayout& Other) const
    {
        return PackedLayout == Other.PackedLayout;
    }
    
    bool operator!=(const FDescriptorSetLayout& Other) const
    {
        return PackedLayout != Other.PackedLayout;
    }
    
    union
    {
        struct
        {
            uint8 NumUniformBuffers;
            uint8 NumImages;
            uint8 NumSamplers;
            uint8 NumStorageBuffers;
            uint8 NumStorageImages;
        };
        
        uint64 PackedLayout;
    };
};

struct FVulkanPipelineLayoutCreateInfo
{
    FVulkanPipelineLayoutCreateInfo()
        : StageSetLayouts()
        , NumGlobalConstants(0)
    {
    }
    
    FDescriptorSetLayout StageSetLayouts[DESCRIPTOR_SET_STAGE_COUNT];
    uint8                NumGlobalConstants;
};

class FVulkanPipelineLayout : public FVulkanDeviceChild
{
public:
    FVulkanPipelineLayout(FVulkanDevice* InDevice);
    ~FVulkanPipelineLayout();

    bool Initialize(const FVulkanPipelineLayoutCreateInfo& LayoutCreateInfo);

    FORCEINLINE VkDescriptorSetLayout GetVkDescriptorSetLayout(uint32 Index) const
    {
        return DescriptorSetLayoutHandles[Index];
    }

    FORCEINLINE VkPipelineLayout GetVkPipelineLayout() const 
    {
        return LayoutHandle;
    }

private:
    VkPipelineLayout              LayoutHandle;
    TArray<VkDescriptorSetLayout> DescriptorSetLayoutHandles;
};

class FVulkanPipelineLayoutManager : public FVulkanDeviceChild
{
public:
    FVulkanPipelineLayoutManager(FVulkanDevice* InDevice);
    ~FVulkanPipelineLayoutManager();

    FVulkanPipelineLayout* GetOrCreateLayout(const FVulkanPipelineLayoutCreateInfo& LayoutCreateInfo);

private:
    // TMap<FVulkanPipelineLayoutCreateInfo, TSharedRef<FVulkanPipelineLayout>> Layouts;
};
