#pragma once
#include "VulkanDeviceChild.h"
#include "VulkanShader.h"
#include "Core/Containers/Map.h"

#define DESCRIPTOR_SET_STAGE_COUNT (5)

struct FDescriptorSetLayout
{
    FDescriptorSetLayout()
        : ShaderVisibility(ShaderVisibility_Vertex)
        , Bindings()
    {
    }
    
    bool operator==(const FDescriptorSetLayout& Other) const
    {
        if (Bindings.Size() != Other.Bindings.Size() || ShaderVisibility != Other.ShaderVisibility)
            return false;
        
        for (int32 Index = 0; Index < Bindings.Size(); Index++)
        {
            if (Bindings[Index] != Other.Bindings[Index])
                return false;
        }
        
        return true;
    }
    
    bool operator!=(const FDescriptorSetLayout& Other) const
    {
        return !(*this == Other);
    }
    
    friend uint64 HashType(const FDescriptorSetLayout& Value)
    {
        uint64 Hash = Value.ShaderVisibility;
        for (const FVulkanShaderBinding& Binding : Value.Bindings)
            HashCombine(Hash, Binding.EncodedBinding);
        return Hash;
    }
    
    EShaderVisibility ShaderVisibility;
    TArray<FVulkanShaderBinding> Bindings;
};

struct FVulkanPipelineLayoutCreateInfo
{
    FVulkanPipelineLayoutCreateInfo()
        : StageSetLayouts()
        , NumGlobalConstants(0)
    {
    }
    
    bool operator==(const FVulkanPipelineLayoutCreateInfo& Other) const
    {
        if (StageSetLayouts.Size() != Other.StageSetLayouts.Size() || NumGlobalConstants != Other.NumGlobalConstants)
            return false;
        
        for (int32 Index = 0; Index < StageSetLayouts.Size(); Index++)
        {
            if (StageSetLayouts[Index] != Other.StageSetLayouts[Index])
                return false;
        }
        
        return true;
    }
    
    bool operator!=(const FVulkanPipelineLayoutCreateInfo& Other) const
    {
        return !(*this == Other);
    }
    
    friend uint64 HashType(const FVulkanPipelineLayoutCreateInfo& Value)
    {
        uint64 Hash = Value.NumGlobalConstants;
        for (const FDescriptorSetLayout& SetLayout : Value.StageSetLayouts)
            HashCombine(Hash, HashType(SetLayout));
        return Hash;
    }

    TArray<FDescriptorSetLayout> StageSetLayouts;
    uint8                        NumGlobalConstants;
};

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

    VkDescriptorSetLayout GetVkDescriptorSetLayout(EShaderVisibility ShaderVisibility) const
    {
        return DescriptorSetLayoutHandles[ShaderVisibility];
    }
    
    uint32 GetNumPushConstants() const
    {
        return NumPushConstants;
    }

private:
    VkPipelineLayout      LayoutHandle;
    VkDescriptorSetLayout DescriptorSetLayoutHandles[ShaderVisibility_Count];
    uint32                NumPushConstants;
};

class FVulkanPipelineLayoutManager : public FVulkanDeviceChild
{
public:
    FVulkanPipelineLayoutManager(FVulkanDevice* InDevice);
    ~FVulkanPipelineLayoutManager();

    bool Initialize();
    void Release();

    TSharedRef<FVulkanPipelineLayout> CreateLayout(const FVulkanPipelineLayoutCreateInfo& LayoutCreateInfo);

    VkDescriptorSetLayout GetDefaultSetLayout() const
    {
        return DefaultSetLayout;
    }

private:
    VkDescriptorSetLayout DefaultSetLayout;
    TMap<FVulkanPipelineLayoutCreateInfo, TSharedRef<FVulkanPipelineLayout>> Layouts;
    FCriticalSection LayoutsCS;
};
