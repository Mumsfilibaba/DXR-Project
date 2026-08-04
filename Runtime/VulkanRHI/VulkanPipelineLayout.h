#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/CRC.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanShader.h"

struct EResourceType
{
    enum Type : uint8
    {
        SRV = 0,
        UAV,
        UniformBuffer,
        Sampler,
    };
};

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
        
        if (Memory::Memcmp(Bindings.Data(), Other.Bindings.Data(), Bindings.SizeInBytes()) != 0)
        {
            return false;
        }
        
        if (ImmutableSamplers.Size() != Other.ImmutableSamplers.Size())
        {
            return false;
        }
        
        if (ImmutableSamplers.Size() > 0 && Memory::Memcmp(ImmutableSamplers.Data(), Other.ImmutableSamplers.Data(), ImmutableSamplers.SizeInBytes()) != 0)
        {
            return false;
        }

        return true;
    }
    
    bool operator!=(const FVulkanDescriptorSetLayoutInfo& Other) const
    {
        return !(*this == Other);
    }
    
    TArray<VkDescriptorSetLayoutBinding> Bindings;
    TArray<VkSampler>                    ImmutableSamplers; // Parallel to Bindings; VK_NULL_HANDLE for non-immutable
    uint64                               Hash;
};

template<>
struct THash<FVulkanDescriptorSetLayoutInfo>
{
    static uint64 GetHash(const FVulkanDescriptorSetLayoutInfo& Value)
    {
        return Value.Hash;
    }
};

struct FVulkanDescriptorRemappingInfo
{
    struct FRemappingInfo
    {
        EVulkanBindingType::Type BindingType;
        uint8                    BindingIndex;
        EVulkanNullImageViewType NullViewType;
        uint8                    Padding;
        uint16                   OriginalBindingIndex;
    };

    static_assert(sizeof(FRemappingInfo) == 6, "FRemappingInfo must not contain implicit padding, it is hashed and compared as raw bytes");

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
            Memory::Memcmp(RemappingInfo.Data(), Other.RemappingInfo.Data(), RemappingInfo.SizeInBytes()) == 0 : false;
    }

    bool operator!=(const FVulkanDescriptorRemappingInfo& Other) const
    {
        return !(*this == Other);
    }

    TArray<FRemappingInfo> RemappingInfo;
    uint64                 Hash;
#if VULKAN_ENABLE_BINDING_DEBUG_NAMES
    TArray<String>         DebugNames;
#endif
};

template<>
struct THash<FVulkanDescriptorRemappingInfo>
{
    static uint64 GetHash(const FVulkanDescriptorRemappingInfo& Value)
    {
        return Value.Hash;
    }
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
        , bAnyStageUsesBindless(false)
    {
    }

    // Add info for a new DescriptorSet based on the ShaderInfo from a certain shader
    void AddSetForStage(VkShaderStageFlagBits ShaderStage, const FVulkanShaderInfo& ShaderInfo);

    // Merge a stage's bindings into a single shared descriptor set (set 0). Used by ray tracing, where all
    // stages share one set and each binding's stageFlags must include every stage that uses it.
    void MergeSetForStage(VkShaderStageFlagBits ShaderStage, const FVulkanShaderInfo& ShaderInfo);

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
        HashCombine(Hash, bAnyStageUsesBindless ? 1ull : 0ull);

        for (int32 Index = 0; Index < SetLayoutInfos.Size(); Index++)
        {
            FVulkanDescriptorSetLayoutInfo& SetLayoutInfo = SetLayoutInfos[Index];
            SetLayoutInfo.GenerateHash();
            HashCombine(Hash, ::THash<FVulkanDescriptorSetLayoutInfo>::GetHash(SetLayoutInfo));

            FVulkanDescriptorRemappingInfo& SetLayoutRemap = SetLayoutRemappings[Index];
            SetLayoutRemap.GenerateHash();
            HashCombine(Hash, ::THash<FVulkanDescriptorRemappingInfo>::GetHash(SetLayoutRemap));
        }

        return Hash;
    }
    
    bool operator==(const FVulkanPipelineLayoutInfo& Other) const
    {
        if (SetLayoutRemappings.Size() != Other.SetLayoutRemappings.Size() || 
            SetLayoutInfos.Size() != Other.SetLayoutInfos.Size() || 
            ConstantsInfo != Other.ConstantsInfo ||
            bAnyStageUsesBindless != Other.bAnyStageUsesBindless)
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
    
    TArray<FVulkanDescriptorRemappingInfo> SetLayoutRemappings; // Information that is needed when building the resource-map
    TArray<FVulkanDescriptorSetLayoutInfo> SetLayoutInfos;      // The actual information for the DescriptorSetLayouts
    FPushConstantsInfo                     ConstantsInfo;       // Information about global push constants in the pipeline
    uint64                                 Hash;
    bool                                   bAnyStageUsesBindless;
};

template<>
struct THash<FVulkanPipelineLayoutInfo>
{
    static uint64 GetHash(const FVulkanPipelineLayoutInfo& Value)
    {
        return Value.Hash;
    }
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

    bool GetDescriptorBinding(EShaderVisibility::Type ShaderStage, EResourceType::Type ResourceType, int32 ResourceIndex, uint32& OutDescriptorSetIndex, uint32& OutBinding);
    bool GetDescriptorSetIndex(EShaderVisibility::Type ShaderStage, uint32& OutDescriptorSetIndex);
    bool GetRemappedBinding(EShaderVisibility::Type ShaderStage, EVulkanBindingType::Type BindingType, uint16 OriginalBindingIndex, uint32& OutBinding) const;

    VkPipelineLayout GetVkPipelineLayout() const
    {
        return LayoutHandle;
    }

    VkDescriptorSetLayout GetVkDescriptorSetLayout(int32 RegularSetIndex) const
    {
        return RegularSetLayoutHandles[RegularSetIndex];
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

    uint32 GetRegularSetCount() const
    {
        return RegularSetCount;
    }

    bool HasBindlessSet() const
    {
        return bHasBindlessSet;
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
    TArray<VkDescriptorSetLayout>          RegularSetLayoutHandles;
    VkDescriptorSetLayout                  BindlessSetLayoutHandle;
    TArray<FVulkanDescriptorRemappingInfo> SetLayoutRemappings;
    TArray<uint32>                         DynamicOffsetCounts;
    uint32                                 TotalDynamicOffsets;
    FPushConstantsInfo                     ConstantsInfo;
    FStageDescriptorMap                    DescriptorBindMap[EShaderVisibility::Count];
    uint32                                 RegularSetCount;
    bool                                   bHasBindlessSet;
#if VULKAN_STORE_DEBUG_NAMES
    String                                 DebugName;
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
