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
        , ImmutableSamplers()
        , Hash(0)
    {
    }

    uint64 GenerateHash()
    {
        Hash = CRC32::Generate(Bindings.Data(), Bindings.SizeInBytes());
        
        if (ImmutableSamplers.Size() > 0)
        {
            HashCombine(Hash, CRC32::Generate(ImmutableSamplers.Data(), ImmutableSamplers.SizeInBytes()));
        }

        return Hash;
    }
    
    bool operator==(const FVulkanDescriptorSetLayoutInfo& Other) const
    {
        if (Bindings.Size() != Other.Bindings.Size())
        {
            return false;
        }
        
        if (FMemory::Memcmp(Bindings.Data(), Other.Bindings.Data(), Bindings.SizeInBytes()) != 0)
        {
            return false;
        }
        
        if (ImmutableSamplers.Size() != Other.ImmutableSamplers.Size())
        {
            return false;
        }
        
        if (ImmutableSamplers.Size() > 0 && FMemory::Memcmp(ImmutableSamplers.Data(), Other.ImmutableSamplers.Data(), ImmutableSamplers.SizeInBytes()) != 0)
        {
            return false;
        }

        return true;
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
    TArray<VkSampler>                    ImmutableSamplers; // Parallel to Bindings; VK_NULL_HANDLE for non-immutable
    uint64                               Hash;
};

struct FVulkanDescriptorRemappingInfo
{
    struct FRemappingInfo
    {
        EVulkanBindingType BindingType;
        uint8              BindingIndex;
        uint16             OriginalBindingIndex;
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
        return (RemappingInfo.Size() == Other.RemappingInfo.Size()) ? 
            FMemory::Memcmp(RemappingInfo.Data(), Other.RemappingInfo.Data(), RemappingInfo.SizeInBytes()) == 0 : false;
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
#if VULKAN_ENABLE_BINDING_DEBUG_NAMES
    TArray<FString>        DebugNames;
#endif
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

    // Promotes VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER bindings to VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC. Must be called before GenerateHash().
    void PromoteUniformBuffersToDynamic();

    // Converts RHI static sampler info into VkSampler handles and assigns them as immutable samplers
    // on the matching descriptor set layout bindings. Must be called after all AddSetForStage calls and before GenerateHash().
    void ApplyImmutableSamplers(FVulkanDevice* Device, const TArrayView<const struct FRHIStaticSamplerInfo>& StaticSamplers);
    
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
        if (SetLayoutRemappings.Size() != Other.SetLayoutRemappings.Size() || 
            SetLayoutInfos.Size() != Other.SetLayoutInfos.Size() || 
            ConstantsInfo != Other.ConstantsInfo)
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
    
    TArray<FVulkanDescriptorRemappingInfo> SetLayoutRemappings; // Information that is needed when building the resource-map
    TArray<FVulkanDescriptorSetLayoutInfo> SetLayoutInfos;      // The actual information for the DescriptorSetLayouts
    FPushConstantsInfo                     ConstantsInfo;       // Information about global push constants in the pipeline
    uint64                                 Hash;
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

#if VULKAN_ENABLE_BINDING_DEBUG_NAMES
    const CHAR* GetBindingDebugName(int32 SetIndex, int32 BindingIndex) const
    {
        if (SetIndex < SetLayoutRemappings.Size() && BindingIndex < SetLayoutRemappings[SetIndex].DebugNames.Size())
        {
            return SetLayoutRemappings[SetIndex].DebugNames[BindingIndex].Data();
        }

        return "";
    }
#endif
    
private:
    void SetupResourceMapping(const FVulkanPipelineLayoutInfo& LayoutInfo);

    VkPipelineLayout                       LayoutHandle;
    TArray<VkDescriptorSetLayout>          SetLayoutHandles;
    TArray<FVulkanDescriptorRemappingInfo> SetLayoutRemappings;
    TArray<uint32>                         DynamicOffsetCounts;
    uint32                                 TotalDynamicOffsets;
    FPushConstantsInfo                     ConstantsInfo;
    FStageDescriptorMap                    DescriptorBindMap[ShaderVisibility_Count];
#if VULKAN_STORE_DEBUG_NAMES
    FString                                DebugName;
#endif
};

class FVulkanPipelineLayoutManager : public FVulkanDeviceChild
{
public:
    FVulkanPipelineLayoutManager(FVulkanDevice* InDevice);
    ~FVulkanPipelineLayoutManager();

    FVulkanPipelineLayout* FindOrCreateLayout(const FVulkanPipelineLayoutInfo& LayoutInfo);
    VkDescriptorSetLayout  FindOrCreateSetLayouts(const FVulkanDescriptorSetLayoutInfo& SetLayoutInfo);

private:
    TMap<FVulkanPipelineLayoutInfo, FVulkanPipelineLayout*>     Layouts;
    FCriticalSection                                            LayoutsCS;
    TMap<FVulkanDescriptorSetLayoutInfo, VkDescriptorSetLayout> SetLayouts;
    FCriticalSection                                            SetLayoutsCS;
};
