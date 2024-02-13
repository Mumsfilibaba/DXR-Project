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
            uint8 ShaderStage;
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
        , NumSetLayouts(0)
        , NumGlobalConstants(0)
    {
    }
    
    bool operator==(const FVulkanPipelineLayoutCreateInfo& Other) const
    {
        if (NumSetLayouts != Other.NumSetLayouts || NumGlobalConstants != Other.NumGlobalConstants)
        {
            return false;
        }
        
        for (int32 Index = 0; Index < NumSetLayouts; Index++)
        {
            if (StageSetLayouts[Index] != Other.StageSetLayouts[Index])
            {
                return false;
            }
        }
        
        return true;
    }
    
    bool operator!=(const FVulkanPipelineLayoutCreateInfo& Other) const
    {
        return !(*this == Other);
    }
    
    FDescriptorSetLayout StageSetLayouts[DESCRIPTOR_SET_STAGE_COUNT];
    uint8                NumSetLayouts;
    uint8                NumGlobalConstants;
};

inline uint64 HashType(const FVulkanPipelineLayoutCreateInfo& Value)
{
    uint64 Hash = Value.NumSetLayouts;
    HashCombine(Hash, Value.NumGlobalConstants);
    for (int32 Index = 0; Index < Value.NumSetLayouts; Index++)
    {
        HashCombine(Hash, Value.StageSetLayouts[Index].PackedLayout);
    }
    
    return Hash;
}


class FVulkanPipelineLayout : public FVulkanDeviceChild, public FVulkanRefCounted
{
public:
    FVulkanPipelineLayout(FVulkanDevice* InDevice);
    ~FVulkanPipelineLayout();

    bool Initialize(const FVulkanPipelineLayoutCreateInfo& LayoutCreateInfo);

    VkPipelineLayout GetVkPipelineLayout() const
    {
        return LayoutHandle;
    }

    VkDescriptorSetLayout GetVkDescriptorSetLayout(uint32 Index) const
    {
        return DescriptorSetLayoutHandles[Index];
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

    void ReleaseAll();
    
    TSharedRef<FVulkanPipelineLayout> GetOrCreateLayout(const FVulkanPipelineLayoutCreateInfo& LayoutCreateInfo);

private:
    TMap<FVulkanPipelineLayoutCreateInfo, TSharedRef<FVulkanPipelineLayout>> Layouts;
};
