#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/CRC.h"
#include "Core/Platform/CriticalSection.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "RHI/RHITypes.h"

class FVulkanBufferRHI;
class FVulkanPipelineLayout;
class FVulkanResourceView;
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
            Memory::Memcmp(Resources.Data(), Other.Resources.Data(), Resources.SizeInBytes()) == 0 : false;
    }

    bool operator!=(const FVulkanDescriptorSetKey& Other) const
    {
        return !(*this == Other);
    }

    TArray<FBinding>      Resources;
    VkDescriptorSetLayout SetLayout;
    uint64                Hash;
};

template<>
struct THash<FVulkanDescriptorSetKey>
{
    static uint64 GetHash(const FVulkanDescriptorSetKey& Value)
    {
        return Value.Hash;
    }
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
            Memory::Memcmp(DescriptorSizes.Data(), Other.DescriptorSizes.Data(), DescriptorSizes.SizeInBytes()) == 0 : false;
    }

    bool operator!=(const FVulkanDescriptorPoolInfo& Other) const
    {
        return !(*this == Other);
    }

    VkDescriptorSetLayout   DescriptorSetLayout;
    TArray<FDescriptorSize> DescriptorSizes;
    uint64                  Hash;
};

template<>
struct THash<FVulkanDescriptorPoolInfo>
{
    static uint64 GetHash(const FVulkanDescriptorPoolInfo& Value)
    {
        return Value.Hash;
    }
};

class FVulkanDescriptorSetBuilder
{
public:
    FVulkanDescriptorSetBuilder()
        : DescriptorWrites(nullptr)
        , NumDescriptorWrites(0)
        , ImmutableBindingMask(0)
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
        Memory::Memzero(DescriptorSetKey.Resources.Data(), DescriptorSetKey.Resources.SizeInBytes());
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

    // Writes a dynamic uniform buffer descriptor with offset=0. The actual offset is passed
    // at bind time via vkCmdBindDescriptorSets, so the key only tracks buffer handle and range.
    void WriteDynamicUniformBuffer(int32 Binding, VkBuffer Buffer, VkDeviceSize Range)
    {
        CHECK(Binding < NumDescriptorWrites);
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

        VkDescriptorBufferInfo* pBufferInfo = const_cast<VkDescriptorBufferInfo*>(DescriptorWrites[Binding].pBufferInfo);
        CHECK(pBufferInfo != nullptr);

        const uint64 Resource = reinterpret_cast<uint64>(Buffer);
        if (DescriptorSetKey.Resources[Binding].Resource != Resource || DescriptorSetKey.Resources[Binding].Range != Range)
        {
            pBufferInfo->buffer = Buffer;
            pBufferInfo->offset = 0;
            pBufferInfo->range  = Range;

            DescriptorSetKey.Resources[Binding].Resource = Resource;
            DescriptorSetKey.Resources[Binding].Offset   = 0;
            DescriptorSetKey.Resources[Binding].Range    = Range;
            bKeyIsDirty = true;
        }
    }
    
    void WriteStorageBuffer(int32 Binding, VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Range)
    {
        CHECK(Binding < NumDescriptorWrites);
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        WriteBuffer(Binding, Buffer, Offset, Range);
    }

    void WriteUniformTexelBuffer(int32 Binding, VkBufferView BufferView)
    {
        CHECK(Binding < NumDescriptorWrites);
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
        WriteTexelBuffer(Binding, BufferView);
    }

    void WriteStorageTexelBuffer(int32 Binding, VkBufferView BufferView)
    {
        CHECK(Binding < NumDescriptorWrites);
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
        WriteTexelBuffer(Binding, BufferView);
    }
    
    void WriteAccelerationStructure(int32 Binding, VkAccelerationStructureKHR AccelerationStructure)
    {
        CHECK(Binding < NumDescriptorWrites);
        CHECK(DescriptorWrites[Binding].descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

        VkWriteDescriptorSetAccelerationStructureKHR* AccelerationStructureInfo = const_cast<VkWriteDescriptorSetAccelerationStructureKHR*>(
            reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(DescriptorWrites[Binding].pNext));
        CHECK(AccelerationStructureInfo != nullptr);
        
        VkAccelerationStructureKHR* AccelerationStructureHandles = const_cast<VkAccelerationStructureKHR*>(AccelerationStructureInfo->pAccelerationStructures);
        CHECK(AccelerationStructureHandles != nullptr);

        const uint64 Resource = reinterpret_cast<uint64>(AccelerationStructure);
        if (DescriptorSetKey.Resources[Binding].Resource != Resource)
        {
            AccelerationStructureHandles[0] = AccelerationStructure;
            DescriptorSetKey.Resources[Binding].Resource = Resource;
            bKeyIsDirty = true;
        }
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

    void MarkBindingAsImmutable(int32 BindingIndex)
    {
        CHECK(BindingIndex < 32);
        ImmutableBindingMask |= (1u << BindingIndex);
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
        if (!DescriptorWrites || NumDescriptorWrites == 0)
        {
            return;
        }

        if (ImmutableBindingMask == 0)
        {
            vkUpdateDescriptorSets(Device, NumDescriptorWrites, DescriptorWrites, 0, nullptr);
        }
        else
        {
            VkWriteDescriptorSet FilteredWrites[32];
            int32 FilteredCount = 0;
            for (int32 i = 0; i < NumDescriptorWrites; i++)
            {
                if (i >= 32 || (ImmutableBindingMask & (1u << i)) == 0)
                {
                    FilteredWrites[FilteredCount++] = DescriptorWrites[i];
                }
            }

            if (FilteredCount > 0)
            {
                vkUpdateDescriptorSets(Device, FilteredCount, FilteredWrites, 0, nullptr);
            }
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

    void WriteTexelBuffer(int32 Binding, VkBufferView BufferView)
    {
        CHECK(DescriptorWrites[Binding].pTexelBufferView != nullptr);

        const uint64 Resource = reinterpret_cast<uint64>(BufferView);
        if (DescriptorSetKey.Resources[Binding].Resource != Resource)
        {
            VkBufferView* pTexelBufferView = const_cast<VkBufferView*>(DescriptorWrites[Binding].pTexelBufferView);
            *pTexelBufferView = BufferView;

            DescriptorSetKey.Resources[Binding].Resource = Resource;
            bKeyIsDirty = true;
        }
    }
        
    FVulkanDescriptorSetKey DescriptorSetKey;
    VkWriteDescriptorSet*   DescriptorWrites;
    int32                   NumDescriptorWrites;
    uint32                  ImmutableBindingMask;
    bool                    bKeyIsDirty;
};

struct FVulkanDescriptorWrites
{
    TArray<VkWriteDescriptorSet>                         DescriptorWrites;
    TArray<VkDescriptorBufferInfo>                       DescriptorBufferInfos;
    TArray<VkDescriptorImageInfo>                        DescriptorImageInfos;
    TArray<VkBufferView>                                 DescriptorTexelBufferViews;
    TArray<VkWriteDescriptorSetAccelerationStructureKHR> DescriptorAccelerationStructureInfos;
    TArray<VkAccelerationStructureKHR>                   DescriptorAccelerationStructures;
};

enum class EVulkanDescriptorDirtyFlags : uint8
{
    None               = 0,
    ResourcesDirty     = (1 << 0),
    DescriptorSetDirty = (1 << 1),
};

ENUM_CLASS_OPERATORS(EVulkanDescriptorDirtyFlags)

class FVulkanDescriptorState : public FVulkanDeviceChild, FNonCopyable
{
public:
    FVulkanDescriptorState(FVulkanDevice* InDevice, FVulkanPipelineLayout* InLayout, const FVulkanDefaultResources& InDefaultResources);
    ~FVulkanDescriptorState() = default;

    void SetSRV(class FVulkanShaderResourceViewRHI* ShaderResourceView, uint32 DescriptorSetIndex, uint32 BindingIndex);
    void SetUAV(class FVulkanUnorderedAccessViewRHI* UnorderedAccessView, uint32 DescriptorSetIndex, uint32 BindingIndex);
    void SetUniformBuffer(class FVulkanBufferRHI* UniformBuffer, uint32 DescriptorSetIndex, uint32 BindingIndex);
    void SetSampler(class FVulkanSamplerStateRHI* SamplerState, uint32 DescriptorSetIndex, uint32 BindingIndex);

    void TransitionBoundResources(class FVulkanCommandContext& Context);

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

    bool IsResourcesDirty() const
    {
        return IsEnumFlagSet(DirtyFlags, EVulkanDescriptorDirtyFlags::ResourcesDirty);
    }

    bool IsDescriptorSetDirty() const
    {
        return IsEnumFlagSet(DirtyFlags, EVulkanDescriptorDirtyFlags::DescriptorSetDirty);
    }

    void DirtyResources()
    {
        DirtyFlags |= EVulkanDescriptorDirtyFlags::ResourcesDirty | EVulkanDescriptorDirtyFlags::DescriptorSetDirty;
    }

    void DirtyDescriptorSet()
    {
        DirtyFlags |= EVulkanDescriptorDirtyFlags::DescriptorSetDirty;
    }

    void ClearResourcesDirty()
    {
        DirtyFlags &= ~EVulkanDescriptorDirtyFlags::ResourcesDirty;
    }

    void ClearDescriptorSetDirty()
    {
        DirtyFlags &= ~EVulkanDescriptorDirtyFlags::DescriptorSetDirty;
    }

private:
    
    // Binds all the DescriptorSets that we want to bind
    void BindDescriptorSets(class FVulkanCommandBuffer& CommandBuffer, VkPipelineBindPoint BindPoint);

    // Resets a particular bind point with null-descriptors to ensure that there is a valid resource bound
    void ResetDescriptorBinding(uint32 DescriptorSetIndex, uint32 BindingIndex);

    FVulkanPipelineLayout*               Layout;
    const FVulkanDefaultResources&       DefaultResources;
    TArray<VkDescriptorSet>              DescriptorSetHandles;
    TArray<FVulkanDescriptorWrites>      DescriptorSetWrites;
    TArray<FVulkanDescriptorSetBuilder>  DescriptorSetBuilders;
    TArray<FVulkanDescriptorPoolInfo>    DescriptorPoolInfos;
    uint64                               DescriptorSetVersion;
    TArray<TArray<FVulkanResourceView*>> BoundResourceViews;
    EVulkanDescriptorDirtyFlags          DirtyFlags = EVulkanDescriptorDirtyFlags::None;
    // Flat array of dynamic offsets passed directly to vkCmdBindDescriptorSets,
    // ordered by (set, binding). Indexed via DynamicOffsetBasePerSet + BindingToDynamicIndex.
    TArray<uint32>                       DynamicOffsets;
    // Per-set base index into DynamicOffsets
    TArray<uint32>                       DynamicOffsetBasePerSet;
    // Per-set mapping from binding index to dynamic offset slot (-1 if not dynamic)
    TArray<TArray<int32>>                BindingToDynamicIndex;
    bool                                 bDynamicOffsetsDirty;
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

    const FVulkanDescriptorPoolInfo& GetPoolInfo() const { return PoolInfo; }

private:
    FVulkanDescriptorPoolInfo PoolInfo;
    VkDescriptorPool          DescriptorPool;
    int32                     MaxDescriptorSets;
    int32                     NumDescriptorSets;
    int32                     LiveDescriptorSets;
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
    void ReleasePool(FVulkanDescriptorPool* Pool);
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
    void FlushPools();

    uint64 GetDescriptorSetVersion() const
    {
        return DescriptorSetVersion;
    }

private:
    FVulkanDescriptorPoolManager&              PoolManager;
    TMap<FVulkanDescriptorPoolInfo, FPoolSet>  PoolSets;
    uint64                                     DescriptorSetVersion;
};

struct FVulkanPendingBindlessWrite
{
    FVulkanPendingBindlessWrite()
        : Binding(0)
        , ArraySlot(0)
        , DescriptorType(VK_DESCRIPTOR_TYPE_MAX_ENUM)
        , Image{}
    {
    }
    
    uint32                     Binding                     = 0;
    uint32                     ArraySlot                   = 0;
    VkDescriptorType           DescriptorType              = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    VkAccelerationStructureKHR AccelerationStructureHandle = VK_NULL_HANDLE;

    union
    {
        VkDescriptorImageInfo                        Image;
        VkDescriptorBufferInfo                       Buffer;
        VkBufferView                                 TexelBuffer;
        VkWriteDescriptorSetAccelerationStructureKHR AccelerationStructure;
    };
};

class VULKANRHI_API FVulkanBindlessDescriptorManager : public FVulkanDeviceChild
{
public:
    FVulkanBindlessDescriptorManager(FVulkanDevice* InDevice);
    ~FVulkanBindlessDescriptorManager();

    bool Initialize();
    void Release();

    NODISCARD FRHIDescriptorHandle Allocate(EDescriptorType InType);
    void Free(FRHIDescriptorHandle Handle);

    void EnqueueImageWrite(FRHIDescriptorHandle Handle, VkImageView ImageView, VkImageLayout ImageLayout, VkDescriptorType DescriptorType);
    void EnqueueBufferWrite(FRHIDescriptorHandle Handle, VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Range, VkDescriptorType DescriptorType);
    void EnqueueTexelBufferWrite(FRHIDescriptorHandle Handle, VkBufferView BufferView, VkDescriptorType DescriptorType);
    void EnqueueAccelerationStructureWrite(FRHIDescriptorHandle Handle, VkAccelerationStructureKHR AccelerationStructure);
    void EnqueueSamplerWrite(FRHIDescriptorHandle Handle, VkSampler Sampler);

    void Flush();

    NODISCARD FORCEINLINE bool IsEnabled() const
    {
        return bIsEnabled;
    }

    NODISCARD FORCEINLINE VkDescriptorSetLayout GetLayout() const
    {
        return SetLayout;
    }

    NODISCARD FORCEINLINE VkDescriptorSet GetDescriptorSet() const
    {
        return DescriptorSet;
    }

    NODISCARD FORCEINLINE uint32 GetResourceCapacity() const
    {
        return ResourceCapacity;
    }

    NODISCARD FORCEINLINE uint32 GetSamplerCapacity() const
    {
        return SamplerCapacity;
    }

private:
    void RecycleSlot(FRHIDescriptorHandle Handle);

    bool                                bIsEnabled;
    VkDescriptorPool                    DescriptorPool;
    VkDescriptorSetLayout               SetLayout;
    VkDescriptorSet                     DescriptorSet;
    uint32                              ResourceCapacity;
    uint32                              SamplerCapacity;
    uint32                              NextFreshResourceSlot;
    uint32                              NextFreshSamplerSlot;
    TArray<uint32>                      FreeResourceStack;
    TArray<uint32>                      FreeSamplerStack;
    FCriticalSection                    AllocCS;
    TArray<FVulkanPendingBindlessWrite> PendingWrites;
    FCriticalSection                    PendingWritesCS;
};

class FVulkanDescriptorSetCache : public FVulkanDeviceChild
{
    struct FCachedDescriptorSet
    {
        VkDescriptorSet        DescriptorSet = VK_NULL_HANDLE;
        uint64                 LastUsedFrame = 0;
        FVulkanDescriptorPool* OwnerPool     = nullptr;
    };

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
