#pragma once
#include "VulkanDeviceChild.h"
#include "VulkanRefCounted.h"
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
    
    void Reset()
    {
        FMemory::Memzero(Resources.Data(), Resources.SizeInBytes());
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

class FVulkanDescriptorSetBuilder
{
public:
    FVulkanDescriptorSetBuilder();
    ~FVulkanDescriptorSetBuilder() = default;
    
    void SetupDescriptorWrites(const FVulkanDescriptorRemappingInfo& SetRemappingInfo);

    void WriteSampledImage(int32 Binding, VkImageView ImageView, VkImageLayout ImageLayout)
    {
        CHECK(Binding < DescriptorWrites.Size());
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
        WriteImage(Binding, ImageView, ImageLayout);
    }
    
    void WriteStorageImage(int32 Binding, VkImageView ImageView, VkImageLayout ImageLayout)
    {
        CHECK(Binding < DescriptorWrites.Size());
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        WriteImage(Binding, ImageView, ImageLayout);
    }
    
    void WriteUniformBuffer(int32 Binding, VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Range)
    {
        CHECK(Binding < DescriptorWrites.Size());
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        WriteBuffer(Binding, Buffer, Offset, Range);
    }
    
    void WriteStorageBuffer(int32 Binding, VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Range)
    {
        CHECK(Binding < DescriptorWrites.Size());
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        WriteBuffer(Binding, Buffer, Offset, Range);
    }
    
    void WriteSampler(int32 Binding, VkSampler Sampler)
    {
        CHECK(Binding < DescriptorWrites.Size());
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
        for (VkWriteDescriptorSet& CurrentWrite : DescriptorWrites)
        {
            CurrentWrite.dstSet = DescriptorSet;
        }
    }
    
    void UpdateDescriptorSet(VkDevice Device)
    {
        if (!DescriptorWrites.IsEmpty())
        {
            vkUpdateDescriptorSets(Device, DescriptorWrites.Size(), DescriptorWrites.Data(), 0, nullptr);
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

    VkDescriptorType GetDescriptorType(int32 BindingIndex) const
    {
        CHECK(BindingIndex < DescriptorWrites.Size());
        return DescriptorWrites[BindingIndex].descriptorType;
    }

    const FVulkanDescriptorSetKey& GetKey() const
    {
        return DescriptorSetKey;
    }

    const VkWriteDescriptorSet* GetDescriptorWrites() const
    {
        return DescriptorWrites.Data();
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
        
    TArray<VkWriteDescriptorSet>   DescriptorWrites;
    TArray<VkDescriptorBufferInfo> DescriptorBufferInfos;
    TArray<VkDescriptorImageInfo>  DescriptorImageInfos;
    FVulkanDescriptorSetKey        DescriptorSetKey;
    bool                           bKeyIsDirty;
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

    // This function creates or retrieves handles for all descriptorsets
    void UpdateDescriptorSets();

    // This function binds all descriptorsets to the graphics-pipeline
    inline void BindGraphicsDescriptorSets(class FVulkanCommandBuffer& CommandBuffer)
    {
        BindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    }

    // This function binds all descriptorsets to the compute-pipeline
    inline void BindComputeDescriptorSets(class FVulkanCommandBuffer& CommandBuffer)
    {
        BindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);
    }

private:
    // Binds all the descriptorsets that we want to bind
    void BindDescriptorSets(class FVulkanCommandBuffer& CommandBuffer, VkPipelineBindPoint BindPoint);

    // Resets a particular bind point with nulldescriptors to ensure that there is a valid resource bound
    void ResetDescriptorBinding(uint32 DescriptorSetIndex, uint32 BindingIndex);

    FVulkanPipelineLayout*              Layout;
    TArray<VkDescriptorSet>             DescriptorSetHandles;
    TArray<FVulkanDescriptorSetBuilder> DescriptorSetBuilders;
    const FVulkanDefaultResources&      DefaultResources;
};

class FVulkanDescriptorPool : public FVulkanDeviceChild
{
public:
    FVulkanDescriptorPool(const FVulkanDescriptorPool&) = delete;
    FVulkanDescriptorPool& operator=(const FVulkanDescriptorPool&) = delete;

    FVulkanDescriptorPool(FVulkanDevice* InDevice);
    ~FVulkanDescriptorPool();
    
    bool Initialize();
    
    bool AllocateDescriptorSet(VkDescriptorSetLayout DescriptorSetLayout, VkDescriptorSet& OuDescriptorSet);
    
    void Reset();

private:
    VkDescriptorPool DescriptorPool;
};

class FVulkanDescriptorPoolManager : public FVulkanDeviceChild
{
public:
    FVulkanDescriptorPoolManager(FVulkanDevice* InDevice);
    ~FVulkanDescriptorPoolManager();
    
    FVulkanDescriptorPool* ObtainPool();
    void RecyclePool(FVulkanDescriptorPool* InDescriptorPool);
    
    void ReleaseAll();
    
private:
    TArray<FVulkanDescriptorPool*> DescriptorPools;
    FCriticalSection               DescriptorPoolsCS;
};
