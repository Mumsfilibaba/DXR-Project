#pragma once
#include "VulkanDeviceChild.h"
#include "VulkanRefCounted.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/CRC.h"

class FVulkanPipelineLayout;
struct FVulkanDefaultResources;
struct FVulkanDescriptorRemappingInfo;

struct FVulkanDescriptorSetKey
{
    struct FBinding
    {
        uint64 Type;
        uint64 ResourceID;
    };
    
    FVulkanDescriptorSetKey()
        : Resources()
        , Hash(0)
    {
    }
    
    uint64 GenerateHash()
    {
        Hash = FCRC32::Generate(Resources.Data(), Resources.SizeInBytes());
        return Hash;
    }
    
    bool operator==(const FVulkanDescriptorSetKey& Other) const
    {
        return (Resources.Size() == Other.Resources.Size()) ? FMemory::Memcmp(Resources.Data(), Other.Resources.Data(), Resources.SizeInBytes()) == 0 : false;
    }
    
    bool operator!=(const FVulkanDescriptorSetKey& Other) const
    {
        return !(*this == Other);
    }
    
    friend uint64 HashType(const FVulkanDescriptorSetKey& Value)
    {
        return Value.Hash;
    }
    
    TArray<FBinding> Resources;
    uint64           Hash;
};

struct FVulkanDescriptorPoolInfo
{
    struct FDescriptorSize
    {
        FDescriptorSize() = default;

        FDescriptorSize(uint32 InType, uint32 InNumDescriptors)
            : Type(InType)
            , NumDescriptors(InNumDescriptors)
        {
        }

        uint32 Type;
        uint32 NumDescriptors;
    };

    FVulkanDescriptorPoolInfo()
        : DescriptorSetLayout(VK_NULL_HANDLE)
        , DescriptorSizes()
        , Hash(0)
    {
    }

    uint64 GenerateHash()
    {
        Hash = reinterpret_cast<uint64>(DescriptorSetLayout);
        HashCombine(Hash, FCRC32::Generate(DescriptorSizes.Data(), DescriptorSizes.SizeInBytes()));
        return Hash;
    }

    bool operator==(const FVulkanDescriptorPoolInfo& Other) const
    {
        return (DescriptorSizes.Size() == Other.DescriptorSizes.Size()) ? FMemory::Memcmp(DescriptorSizes.Data(), Other.DescriptorSizes.Data(), DescriptorSizes.SizeInBytes()) == 0 : false;
    }

    bool operator!=(const FVulkanDescriptorPoolInfo& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 HashType(const FVulkanDescriptorPoolInfo& Value)
    {
        return Value.Hash;
    }

    VkDescriptorSetLayout   DescriptorSetLayout;
    TArray<FDescriptorSize> DescriptorSizes;
    uint64 Hash;
};

class FVulkanDescriptorSetBuilder
{
public:
    FVulkanDescriptorSetBuilder()
        : DescriptorWrites(nullptr)
        , NumDescriptorWrites(0)
        , bKeyIsDirty(true)
    {
    }

    void SetupDescriptorWrites(VkWriteDescriptorSet* InDescriptorWrites, int32 InNumDescriptorWrites)
    {
        // Setup DescriptorWrites
        DescriptorWrites    = InDescriptorWrites;
        NumDescriptorWrites = InNumDescriptorWrites;
        
        // Allocate HashKey
        DescriptorSetKey.Resources.Resize(InNumDescriptorWrites);
        FMemory::Memzero(DescriptorSetKey.Resources.Data(), DescriptorSetKey.Resources.SizeInBytes());
        UpdateHash();
        
        // Initialize all the types
        for (int32 Index = 0; Index < NumDescriptorWrites; Index++)
        {
            DescriptorSetKey.Resources[Index].Type = DescriptorWrites[Index].descriptorType;
        }
    }
    
    void WriteSampledImage(int32 Binding, VkImageView ImageView, VkImageLayout ImageLayout)
    {
        CHECK(Binding < NumDescriptorWrites);
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
        WriteImage(Binding, ImageView, ImageLayout);
    }
    
    void WriteStorageImage(int32 Binding, VkImageView ImageView, VkImageLayout ImageLayout)
    {
        CHECK(Binding < NumDescriptorWrites);
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        WriteImage(Binding, ImageView, ImageLayout);
    }
    
    void WriteUniformBuffer(int32 Binding, VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Range)
    {
        CHECK(Binding < NumDescriptorWrites);
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        WriteBuffer(Binding, Buffer, Offset, Range);
    }
    
    void WriteStorageBuffer(int32 Binding, VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Range)
    {
        CHECK(Binding < NumDescriptorWrites);
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        WriteBuffer(Binding, Buffer, Offset, Range);
    }
    
    void WriteSampler(int32 Binding, VkSampler Sampler)
    {
        CHECK(Binding < NumDescriptorWrites);
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER);
        
        VkDescriptorImageInfo* pImageInfo = const_cast<VkDescriptorImageInfo*>(DescriptorWrites[Binding].pImageInfo);
        CHECK(pImageInfo != nullptr);
        
        const uint64 ResourceID = reinterpret_cast<uint64>(Sampler);
        if (DescriptorSetKey.Resources[Binding].ResourceID != ResourceID)
        {
            pImageInfo->sampler     = Sampler;
            pImageInfo->imageView   = VK_NULL_HANDLE;
            pImageInfo->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            DescriptorSetKey.Resources[Binding].ResourceID = ResourceID;
            bKeyIsDirty = true;
        }
    }

    void SetDescriptorSet(VkDescriptorSet DescriptorSet)
    {
        for (int32 Index = 0; Index < NumDescriptorWrites; Index++)
        {
            DescriptorWrites[Index].dstSet = DescriptorSet;
        }
    }
    
    void UpdateDescriptorSet(VkDevice Device)
    {
        if (DescriptorWrites)
        {
            vkUpdateDescriptorSets(Device, NumDescriptorWrites, DescriptorWrites, 0, nullptr);
        }
    }
    
    void UpdateHash()
    {
        if (bKeyIsDirty)
        {
            DescriptorSetKey.GenerateHash();
            bKeyIsDirty = false;
        }
    }
    
    bool IsKeyDirty() const
    {
        return bKeyIsDirty;
    }

    const FVulkanDescriptorSetKey& GetKey() const
    {
        return DescriptorSetKey;
    }
    
private:
    void WriteBuffer(int32 Binding, VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Range)
    {
        VkDescriptorBufferInfo* pBufferInfo = const_cast<VkDescriptorBufferInfo*>(DescriptorWrites[Binding].pBufferInfo);
        CHECK(pBufferInfo != nullptr);
        
        const uint64 ResourceID = reinterpret_cast<uint64>(Buffer);
        if (DescriptorSetKey.Resources[Binding].ResourceID != ResourceID)
        {
            pBufferInfo->buffer = Buffer;
            pBufferInfo->offset = Offset;
            pBufferInfo->range  = Range;
            DescriptorSetKey.Resources[Binding].ResourceID = ResourceID;
            bKeyIsDirty = true;
        }
    }
    
    void WriteImage(int32 Binding, VkImageView ImageView, VkImageLayout ImageLayout)
    {
        VkDescriptorImageInfo* pImageInfo = const_cast<VkDescriptorImageInfo*>(DescriptorWrites[Binding].pImageInfo);
        CHECK(pImageInfo != nullptr);
        
        const uint64 ResourceID = reinterpret_cast<uint64>(ImageView);
        if (DescriptorSetKey.Resources[Binding].ResourceID != ResourceID)
        {
            pImageInfo->sampler     = VK_NULL_HANDLE;
            pImageInfo->imageView   = ImageView;
            pImageInfo->imageLayout = ImageLayout;
            DescriptorSetKey.Resources[Binding].ResourceID = ResourceID;
            bKeyIsDirty = true;
        }
    }
        
    FVulkanDescriptorSetKey DescriptorSetKey;
    VkWriteDescriptorSet*   DescriptorWrites;
    int32                   NumDescriptorWrites;
    bool                    bKeyIsDirty;
};

struct FVulkanDescriptorWrites
{
    TArray<VkWriteDescriptorSet>   DescriptorWrites;
    TArray<VkDescriptorBufferInfo> DescriptorBufferInfos;
    TArray<VkDescriptorImageInfo>  DescriptorImageInfos;
};

class FVulkanDescriptorState : public FVulkanDeviceChild
{
public:
    FVulkanDescriptorState(FVulkanDevice* InDevice, FVulkanPipelineLayout* InLayout, const FVulkanDefaultResources& InDefaultResources);
    ~FVulkanDescriptorState() = default;

    // Binds a ShaderResourceView to a binding, mapping from register to binding needs to be done here
    void SetSRV(class FVulkanShaderResourceView* ShaderResourceView, uint32 DescriptorSetIndex, uint32 BindingIndex);

    // Binds a UnorderedAccessView to a binding, mapping from register to binding needs to be done here
    void SetUAV(class FVulkanUnorderedAccessView* UnorderedAccessView, uint32 DescriptorSetIndex, uint32 BindingIndex);

    // Binds a UniformBuffer to a binding, mapping from register to binding needs to be done here
    void SetUniform(class FVulkanBuffer* UniformBuffer, uint32 DescriptorSetIndex, uint32 BindingIndex);

    // Binds a SamplerState to a binding, mapping from register to binding needs to be done here  
    void SetSampler(class FVulkanSamplerState* SamplerState, uint32 DescriptorSetIndex, uint32 BindingIndex);

    // This function creates or retrieves handles for all DescriptorSets
    void UpdateDescriptorSets();
    
    // Resets the state and puts default resources into all bindings
    void Reset();

    // This function binds all DescriptorSets to the graphics-pipeline
    inline void BindGraphicsDescriptorSets(class FVulkanCommandBuffer& CommandBuffer)
    {
        BindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    }

    // This function binds all DescriptorSets to the compute-pipeline
    inline void BindComputeDescriptorSets(class FVulkanCommandBuffer& CommandBuffer)
    {
        BindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);
    }

    FVulkanPipelineLayout* GetLayout() const
    {
        return Layout;
    }

private:
    
    // Binds all the DescriptorSets that we want to bind
    void BindDescriptorSets(class FVulkanCommandBuffer& CommandBuffer, VkPipelineBindPoint BindPoint);

    // Resets a particular bind point with null-descriptors to ensure that there is a valid resource bound
    void ResetDescriptorBinding(uint32 DescriptorSetIndex, uint32 BindingIndex);

    FVulkanPipelineLayout*              Layout;
    TArray<VkDescriptorSet>             DescriptorSetHandles;
    TArray<FVulkanDescriptorWrites>     DescriptorSetWrites;
    TArray<FVulkanDescriptorSetBuilder> DescriptorSetBuilders;
    TArray<FVulkanDescriptorPoolInfo>   DescriptorPoolInfos;
    const FVulkanDefaultResources&      DefaultResources;
};

class FVulkanDescriptorPool : public FVulkanDeviceChild
{
public:
    FVulkanDescriptorPool(const FVulkanDescriptorPool&) = delete;
    FVulkanDescriptorPool& operator=(const FVulkanDescriptorPool&) = delete;

    FVulkanDescriptorPool(FVulkanDevice* InDevice);
    ~FVulkanDescriptorPool();
    
    bool Initialize(const FVulkanDescriptorPoolInfo& PoolInfo);

    bool AllocateDescriptorSet(const VkDescriptorSetAllocateInfo& DescriptorSetAllocateInfo, VkDescriptorSet* OutDescriptorSets);
    
    void Reset();

    inline bool CanAllocateDescriptorSet()
    {
        return NumDescriptorSets > 0;
    }

private:
    VkDescriptorPool DescriptorPool;
    int32           MaxDescriptorSets;
    int32           NumDescriptorSets;
};