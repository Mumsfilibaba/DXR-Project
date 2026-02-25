#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/CRC.h"
#include "Core/Platform/CriticalSection.h"
#include "VulkanRHI/VulkanRefCounted.h"
#include "VulkanRHI/VulkanDeviceChild.h"

class FVulkanBuffer;
class FVulkanPipelineLayout;
struct FVulkanDefaultResources;
struct FVulkanDescriptorRemappingInfo;

struct FVulkanDescriptorSetKey
{
    struct FBinding
    {
        uint64 Type;
        uint64 Resource;
        uint64 Offset;
        uint64 Range;
    };

    FVulkanDescriptorSetKey()
        : Resources()
        , SetLayout(VK_NULL_HANDLE)
        , Hash(0)
    {
    }

    uint64 GenerateHash()
    {
        Hash = reinterpret_cast<uint64>(SetLayout);
        HashCombine(Hash, CRC32::Generate(Resources.Data(), Resources.SizeInBytes()));
        return Hash;
    }

    bool operator==(const FVulkanDescriptorSetKey& Other) const
    {
        if (SetLayout != Other.SetLayout)
        {
            return false;
        }
        
        return Resources.Size() == Other.Resources.Size() ? 
            FMemory::Memcmp(Resources.Data(), Other.Resources.Data(), Resources.SizeInBytes()) == 0 : false;
    }

    bool operator!=(const FVulkanDescriptorSetKey& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FVulkanDescriptorSetKey& Value)
    {
        return Value.Hash;
    }
    
    TArray<FBinding>      Resources;
    VkDescriptorSetLayout SetLayout;
    uint64                Hash;
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
        HashCombine(Hash, CRC32::Generate(DescriptorSizes.Data(), DescriptorSizes.SizeInBytes()));
        return Hash;
    }

    bool operator==(const FVulkanDescriptorPoolInfo& Other) const
    {
        if (DescriptorSetLayout != Other.DescriptorSetLayout)
        {
            return false;
        }

        return DescriptorSizes.Size() == Other.DescriptorSizes.Size() ? 
            FMemory::Memcmp(DescriptorSizes.Data(), Other.DescriptorSizes.Data(), DescriptorSizes.SizeInBytes()) == 0 : false;
    }

    bool operator!=(const FVulkanDescriptorPoolInfo& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FVulkanDescriptorPoolInfo& Value)
    {
        return Value.Hash;
    }

    VkDescriptorSetLayout   DescriptorSetLayout;
    TArray<FDescriptorSize> DescriptorSizes;
    uint64                  Hash;
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

    void SetupDescriptorWrites(VkDescriptorSetLayout SetLayout, VkWriteDescriptorSet* InDescriptorWrites, int32 InNumDescriptorWrites)
    {
        // Setup DescriptorWrites
        DescriptorWrites    = InDescriptorWrites;
        NumDescriptorWrites = InNumDescriptorWrites;

        // Allocate HashKey
        DescriptorSetKey.Resources.Resize(InNumDescriptorWrites);
        FMemory::Memzero(DescriptorSetKey.Resources.Data(), DescriptorSetKey.Resources.SizeInBytes());
        DescriptorSetKey.SetLayout = SetLayout;

        // Initialize all the types
        for (int32 Index = 0; Index < NumDescriptorWrites; Index++)
        {
            DescriptorSetKey.Resources[Index].Type = DescriptorWrites[Index].descriptorType;
        }

        bKeyIsDirty = true;
        UpdateHash();
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
        
        const uint64 Resource = reinterpret_cast<uint64>(Sampler);
        if (DescriptorSetKey.Resources[Binding].Resource != Resource)
        {
            pImageInfo->sampler     = Sampler;
            pImageInfo->imageView   = VK_NULL_HANDLE;
            pImageInfo->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            DescriptorSetKey.Resources[Binding].Resource = Resource;
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

    void ClearDirtyKey()
    {
        bKeyIsDirty = false;
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
        
        const uint64 Resource = reinterpret_cast<uint64>(Buffer);
        if (DescriptorSetKey.Resources[Binding].Resource != Resource || DescriptorSetKey.Resources[Binding].Offset != Offset || DescriptorSetKey.Resources[Binding].Range != Range)
        {
            pBufferInfo->buffer = Buffer;
            pBufferInfo->offset = Offset;
            pBufferInfo->range  = Range;

            DescriptorSetKey.Resources[Binding].Resource = Resource;
            DescriptorSetKey.Resources[Binding].Offset   = Offset;
            DescriptorSetKey.Resources[Binding].Range    = Range;
            bKeyIsDirty = true;
        }
    }
    
    void WriteImage(int32 Binding, VkImageView ImageView, VkImageLayout ImageLayout)
    {
        VkDescriptorImageInfo* pImageInfo = const_cast<VkDescriptorImageInfo*>(DescriptorWrites[Binding].pImageInfo);
        CHECK(pImageInfo != nullptr);
        
        const uint64 Resource = reinterpret_cast<uint64>(ImageView);
        if (DescriptorSetKey.Resources[Binding].Resource != Resource)
        {
            pImageInfo->sampler     = VK_NULL_HANDLE;
            pImageInfo->imageView   = ImageView;
            pImageInfo->imageLayout = ImageLayout;
            DescriptorSetKey.Resources[Binding].Resource = Resource;
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

class FVulkanDescriptorState : public FVulkanDeviceChild, FNonCopyable
{
public:
    FVulkanDescriptorState(FVulkanDevice* InDevice, FVulkanPipelineLayout* InLayout, const FVulkanDefaultResources& InDefaultResources);
    ~FVulkanDescriptorState() = default;

    void SetSRV(class FVulkanShaderResourceView* ShaderResourceView, uint32 DescriptorSetIndex, uint32 BindingIndex);
    void SetUAV(class FVulkanUnorderedAccessView* UnorderedAccessView, uint32 DescriptorSetIndex, uint32 BindingIndex);
    void SetUniformBuffer(class FVulkanBuffer* UniformBuffer, uint32 DescriptorSetIndex, uint32 BindingIndex);
    void SetSampler(class FVulkanSamplerState* SamplerState, uint32 DescriptorSetIndex, uint32 BindingIndex);

    void UpdateDescriptorSets(class FVulkanTransientDescriptorAllocator* TransientAllocator);
    void Reset();

    inline void BindGraphicsDescriptorSets(class FVulkanCommandBuffer& CommandBuffer)
    {
        BindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    }

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
    uint64                              DescriptorSetVersion;
};

class FVulkanDescriptorPool : public FVulkanDeviceChild, FNonCopyable
{
public:
    FVulkanDescriptorPool(FVulkanDevice* InDevice);
    ~FVulkanDescriptorPool();
    
    bool Initialize(const FVulkanDescriptorPoolInfo& PoolInfo, int32 MaxDescriptorSetCount);
    bool AllocateDescriptorSet(const VkDescriptorSetAllocateInfo& DescriptorSetAllocateInfo, VkDescriptorSet* OutDescriptorSets);
    void Reset();

    bool CanAllocateDescriptorSet() const { return NumDescriptorSets > 0; }
    bool CanRecycle() const { return LiveDescriptorSets == 0; }

    void IncrementLive() { LiveDescriptorSets++; }
    void DecrementLive() { LiveDescriptorSets--; }

private:
    VkDescriptorPool DescriptorPool;
    int32            MaxDescriptorSets;
    int32            NumDescriptorSets;
    int32            LiveDescriptorSets;
};

class FVulkanDescriptorPoolManager : public FVulkanDeviceChild
{
    struct FFreePool
    {
        FVulkanDescriptorPool* Pool;
        uint64                 ReturnedFrame;
    };

public:
    FVulkanDescriptorPoolManager(FVulkanDevice* InDevice);
    ~FVulkanDescriptorPoolManager();

    FVulkanDescriptorPool* AcquirePool(const FVulkanDescriptorPoolInfo& PoolInfo);
    void ReleasePool(const FVulkanDescriptorPoolInfo& PoolInfo, FVulkanDescriptorPool* Pool);
    void EvictUnusedPools();

private:
    static constexpr uint64 MinUnusedFrames = 8;

    TMap<FVulkanDescriptorPoolInfo, TArray<FFreePool>> FreePools;
    FCriticalSection                                   PoolCS;
    uint64                                             CurrentFrame;
};

class FVulkanTransientDescriptorAllocator : public FVulkanDeviceChild
{
    struct FPoolSet
    {
        FVulkanDescriptorPool*         ActivePool = nullptr;
        TArray<FVulkanDescriptorPool*> UsedPools;
    };

public:
    FVulkanTransientDescriptorAllocator(FVulkanDevice* InDevice, FVulkanDescriptorPoolManager& InPoolManager);
    ~FVulkanTransientDescriptorAllocator();

    bool AllocateDescriptorSet(const FVulkanDescriptorPoolInfo& PoolInfo, FVulkanDescriptorSetBuilder& DSBuilder, VkDescriptorSet& OutDescriptorSet);
    void FlushPools(struct FVulkanCommands& Commands);

    uint64 GetDescriptorSetVersion() const
    {
        return DescriptorSetVersion;
    }

private:
    FVulkanDescriptorPoolManager&              PoolManager;
    TMap<FVulkanDescriptorPoolInfo, FPoolSet>  PoolSets;
    uint64                                     DescriptorSetVersion;
};

class FVulkanDescriptorSetCache : public FVulkanDeviceChild
{
    class FCachedPool : public FVulkanDeviceChild
    {
    public:
        FCachedPool(FVulkanDevice* InDevice, const FVulkanDescriptorPoolInfo& InPoolInfo);
        ~FCachedPool();

        bool AllocateDescriptorSet(VkDescriptorSetLayout SetLayout, VkDescriptorSet& OutDescriptorSet, FVulkanDescriptorPool** OutPool);

    private:
        FVulkanDescriptorPool*         CurrentDescriptorPool;
        TArray<FVulkanDescriptorPool*> DescriptorPools;
        FVulkanDescriptorPoolInfo      PoolInfo;
    };

    struct FCachedDescriptorSet
    {
        VkDescriptorSet        DescriptorSet = VK_NULL_HANDLE;
        uint64                 LastUsedFrame = 0;
        FVulkanDescriptorPool* OwnerPool     = nullptr;
    };

public:
    FVulkanDescriptorSetCache(FVulkanDevice* InDevice);
    ~FVulkanDescriptorSetCache();

    bool FindOrCreateDescriptorSet(const FVulkanDescriptorPoolInfo& PoolInfo, FVulkanDescriptorSetBuilder& DSBuilder, VkDescriptorSet& OutDescriptorSet);
    void EvictStaleDescriptorSets(uint64 InFramesInFlight);

private:
    static constexpr uint64 MinUnusedFrames = 2;

    TMap<FVulkanDescriptorPoolInfo, FCachedPool*>       Caches;
    TMap<FVulkanDescriptorSetKey, FCachedDescriptorSet> DescriptorSets;
    FCriticalSection                                    CacheCS;
    uint64                                              CurrentFrame;
};
