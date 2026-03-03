#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/CRC.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanShader.h"

struct FVulkanDescriptorSetLayoutInfo
{
    FVulkanDescriptorSetLayoutInfo()
        : Bindings()
        , Hash(0)
    {
    }

    uint64 GenerateHash()
    {
        Hash = CRC32::Generate(Bindings.Data(), Bindings.SizeInBytes());
        return Hash;
    }
    
    bool operator==(const FVulkanDescriptorSetLayoutInfo& Other) const
    {
        return (Bindings.Size() == Other.Bindings.Size()) ? FMemory::Memcmp(Bindings.Data(), Other.Bindings.Data(), Bindings.SizeInBytes()) == 0 : false;
    }
    
    bool operator!=(const FVulkanDescriptorSetLayoutInfo& Other) const
    {
        return !(*this == Other);
    }
    
    friend uint64 GetHashForType(const FVulkanDescriptorSetLayoutInfo& Value)
    {
        return Value.Hash;
    }
    
    TArray<VkDescriptorSetLayoutBinding> Bindings;
    uint64                               Hash;
};

struct FVulkanDescriptorRemappingInfo
{
    struct FRemappingInfo
    {
        EVulkanBindingType BindingType;
        uint8        BindingIndex;
        uint16       OriginalBindingIndex;
    };

    FVulkanDescriptorRemappingInfo()
        : RemappingInfo()
        , Hash(0)
    {
    }

    uint64 GenerateHash()
    {
        Hash = CRC32::Generate(RemappingInfo.Data(), RemappingInfo.SizeInBytes());
        return Hash;
    }

    bool operator==(const FVulkanDescriptorRemappingInfo& Other) const
    {
        return (RemappingInfo.Size() == Other.RemappingInfo.Size()) ? FMemory::Memcmp(RemappingInfo.Data(), Other.RemappingInfo.Data(), RemappingInfo.SizeInBytes()) == 0 : false;
    }

    bool operator!=(const FVulkanDescriptorRemappingInfo& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FVulkanDescriptorRemappingInfo& Value)
    {
        return Value.Hash;
    }

    TArray<FRemappingInfo> RemappingInfo;
    uint64                 Hash;
};

struct FPushConstantsInfo
{
    bool operator==(const FPushConstantsInfo& Other) const = default;
    
    uint32             NumConstants = 0;
    VkShaderStageFlags StageFlags   = 0;
};

struct FVulkanPipelineLayoutInfo
{
    FVulkanPipelineLayoutInfo()
        : SetLayoutInfos()
        , ConstantsInfo()
        , Hash(0)
    {
    }

    // Add info for a new DescriptorSet based on the ShaderInfo from a certain shader
    void AddSetForStage(VkShaderStageFlagBits ShaderStage, const FVulkanShaderInfo& ShaderInfo);

    // Promotes VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER bindings to VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
    // when the pipeline layout's user data DWORD budget allows. Must be called before GenerateHash().
    void PromoteUniformBuffersToDynamic();
    
    // Update constants based on the ShaderInfo
    void UpdateConstantsForStage(VkShaderStageFlagBits ShaderStage, const FVulkanShaderInfo& ShaderInfo)
    {
        if (ShaderInfo.NumPushConstants)
        {
            ConstantsInfo.StageFlags  |= ShaderStage;
            ConstantsInfo.NumConstants = Math::Max<uint32>(ConstantsInfo.NumConstants, ShaderInfo.NumPushConstants);
        }
    }
    
    uint64 GenerateHash()
    {
        Hash = ConstantsInfo.NumConstants;
        HashCombine(Hash, ConstantsInfo.StageFlags);

        for (int32 Index = 0; Index < SetLayoutInfos.Size(); Index++)
        {
            FVulkanDescriptorSetLayoutInfo& SetLayoutInfo = SetLayoutInfos[Index];
            SetLayoutInfo.GenerateHash();
            HashCombine(Hash, GetHashForType(SetLayoutInfo));

            FVulkanDescriptorRemappingInfo& SetLayoutRemap = SetLayoutRemappings[Index];
            SetLayoutRemap.GenerateHash();
            HashCombine(Hash, GetHashForType(SetLayoutRemap));
        }

        return Hash;
    }
    
    bool operator==(const FVulkanPipelineLayoutInfo& Other) const
    {
        if (SetLayoutRemappings.Size() != Other.SetLayoutRemappings.Size() || SetLayoutInfos.Size() != Other.SetLayoutInfos.Size() || ConstantsInfo != Other.ConstantsInfo)
        {
            return false;
        }
        
        for (int32 Index = 0; Index < SetLayoutRemappings.Size(); Index++)
        {
            if (SetLayoutRemappings[Index] != Other.SetLayoutRemappings[Index])
            {
                return false;
            }
        }

        for (int32 Index = 0; Index < SetLayoutInfos.Size(); Index++)
        {
            if (SetLayoutInfos[Index] != Other.SetLayoutInfos[Index])
            {
                return false;
            }
        }
        
        return true;
    }
    
    bool operator!=(const FVulkanPipelineLayoutInfo& Other) const
    {
        return !(*this == Other);
    }
    
    friend uint64 GetHashForType(const FVulkanPipelineLayoutInfo& Value)
    {
        return Value.Hash;
    }
    
    // This information is needed when building the resource-map
    TArray<FVulkanDescriptorRemappingInfo> SetLayoutRemappings;
    
    // This contains the actual information for the DescriptorSetLayouts
    TArray<FVulkanDescriptorSetLayoutInfo> SetLayoutInfos;
    
    // This structure contains information about global push constants in the pipeline
    FPushConstantsInfo ConstantsInfo;
    uint64             Hash;
};

enum EResourceType
{
    ResourceType_SRV = 0,
    ResourceType_UAV,
    ResourceType_UniformBuffer,
    ResourceType_Sampler,
};

struct FStageDescriptorMap
{
    // DescriptorSetIndex for this shader-stage
    uint8 DescriptorSetIndex = UINT8_MAX;
    
    // Mappings from register to binding
    uint8 SRVMappings[VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT];
    uint8 UAVMappings[VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT];
    uint8 UniformMappings[VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT];
    uint8 SamplerMappings[VULKAN_DEFAULT_SAMPLER_STATE_COUNT];
};

class FVulkanPipelineLayout : public FVulkanDeviceChild
{
public:
    FVulkanPipelineLayout(FVulkanDevice* InDevice);
    ~FVulkanPipelineLayout();

    bool Initialize(const FVulkanPipelineLayoutInfo& LayoutInfo);
    void SetDebugName(const CHAR* InName);

    bool GetDescriptorBinding(EShaderVisibility ShaderStage, EResourceType ResourceType, int32 ResourceIndex, uint32& OutDescriptorSetIndex, uint32& OutBinding);
    bool GetDescriptorSetIndex(EShaderVisibility ShaderStage, uint32& OutDescriptorSetIndex);

    VkPipelineLayout GetVkPipelineLayout() const
    {
        return LayoutHandle;
    }

    VkDescriptorSetLayout GetVkDescriptorSetLayout(int32 DescriptorSetIndex) const
    {
        return SetLayoutHandles[DescriptorSetIndex];
    }

    const FVulkanDescriptorRemappingInfo& GetDescriptorRemappingInfo(int32 DescriptorSetIndex) const
    {
        return SetLayoutRemappings[DescriptorSetIndex];
    }
    
    const TArray<FVulkanDescriptorRemappingInfo>& GetDescriptorRemappingInfos() const
    {
        return SetLayoutRemappings;
    }

    const FPushConstantsInfo& GetConstantsInfo() const
    {
        return ConstantsInfo;
    }

    uint32 GetDynamicOffsetCount(int32 DescriptorSetIndex) const
    {
        return DynamicOffsetCounts[DescriptorSetIndex];
    }

    uint32 GetTotalDynamicOffsetCount() const
    {
        return TotalDynamicOffsets;
    }
    
private:
    void SetupResourceMapping(const FVulkanPipelineLayoutInfo& LayoutInfo);

    VkPipelineLayout                       LayoutHandle;
    TArray<VkDescriptorSetLayout>          SetLayoutHandles;
    TArray<FVulkanDescriptorRemappingInfo> SetLayoutRemappings;
    TArray<uint32>                         DynamicOffsetCounts;
    uint32                                 TotalDynamicOffsets;
    FPushConstantsInfo                     ConstantsInfo;
    FStageDescriptorMap                    DescriptorBindMap[ShaderVisibility_Count];
    FString                                DebugName;
};

class FVulkanPipelineLayoutManager : public FVulkanDeviceChild
{
public:
    FVulkanPipelineLayoutManager(FVulkanDevice* InDevice);
    ~FVulkanPipelineLayoutManager();

    FVulkanPipelineLayout* FindOrCreateLayout(const FVulkanPipelineLayoutInfo& LayoutInfo);
    VkDescriptorSetLayout FindOrCreateSetLayouts(const FVulkanDescriptorSetLayoutInfo& SetLayoutInfo);

private:
    TMap<FVulkanPipelineLayoutInfo, FVulkanPipelineLayout*>     Layouts;
    FCriticalSection                                            LayoutsCS;
    TMap<FVulkanDescriptorSetLayoutInfo, VkDescriptorSetLayout> SetLayouts;
    FCriticalSection                                            SetLayoutsCS;
};
