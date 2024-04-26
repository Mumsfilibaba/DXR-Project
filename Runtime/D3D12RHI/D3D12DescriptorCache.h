#pragma once
#include "D3D12Buffer.h"
#include "D3D12RootSignature.h"
#include "D3D12Descriptors.h"
#include "D3D12CommandList.h"
#include "D3D12ResourceViews.h"
#include "D3D12SamplerState.h"
#include "Core/Templates/TypeHash.h"

#if DEBUG_BUILD
    #define D3D12_BREAK_ON_HASH_COLLISION (1)
#else
    #define D3D12_BREAK_ON_HASH_COLLISION (0)
#endif

class FD3D12CommandContext;

struct FD3D12VertexBufferCache
{
    FD3D12VertexBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(VertexBuffers, sizeof(VertexBuffers));
        NumVertexBuffers = 0;
    }

    D3D12_VERTEX_BUFFER_VIEW VertexBuffers[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    uint32 NumVertexBuffers;
};

struct FD3D12IndexBufferCache
{
    FD3D12IndexBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(&IndexBuffer, sizeof(IndexBuffer));
    }

    D3D12_INDEX_BUFFER_VIEW IndexBuffer;
};

struct FD3D12RenderTargetCache
{
    FD3D12RenderTargetCache()
        : RenderTargetViews()
        , NumRenderTargets(0)
        , DepthStencilView(nullptr)
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(RenderTargetViews, sizeof(RenderTargetViews));
        NumRenderTargets = 0;
        DepthStencilView = nullptr;
    }

    FD3D12RenderTargetView* RenderTargetViews[D3D12_MAX_RENDER_TARGET_COUNT];
    FD3D12DepthStencilView* DepthStencilView;
    uint32                  NumRenderTargets;
};

struct FD3D12ResourceCache
{
    bool IsDirty(EShaderVisibility ShaderStage) const
    {
        return bDirty[ShaderStage];
    }

    void DirtyState(uint32 StartStage, uint32 EndStage)
    {
        CHECK(EndStage < ShaderVisibility_Count);

        for (uint32 Index = StartStage; Index < ShaderVisibility_Count; Index++)
        {
            bDirty[Index] = true;
        }
    }

    void DirtyStateAll()
    {
        for (uint32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            bDirty[Index] = true;
        }
    }

    bool bDirty[ShaderVisibility_Count];
};

struct FD3D12ConstantBufferCache : public FD3D12ResourceCache
{
    FD3D12ConstantBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyState(ShaderVisibility_All, ShaderVisibility_Pixel);

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageViews = ResourceViews[Index];
            FMemory::Memzero(&StageViews, sizeof(StageViews));
            NumBuffers[Index] = 0;
        }
    }

    FD3D12ConstantBufferView* ResourceViews[ShaderVisibility_Count][D3D12_DEFAULT_CONSTANT_BUFFER_COUNT];
    uint8 NumBuffers[ShaderVisibility_Count];
};

struct FD3D12ShaderResourceViewCache : public FD3D12ResourceCache
{
    FD3D12ShaderResourceViewCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyState(ShaderVisibility_All, ShaderVisibility_Pixel);

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageViews = ResourceViews[Index];
            FMemory::Memzero(&StageViews, sizeof(StageViews));
            NumViews[Index] = 0;
        }
    }

    FD3D12ShaderResourceView* ResourceViews[ShaderVisibility_Count][D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT];
    uint8 NumViews[ShaderVisibility_Count];
};

struct FD3D12UnorderedAccessViewCache : public FD3D12ResourceCache
{
    FD3D12UnorderedAccessViewCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyState(ShaderVisibility_All, ShaderVisibility_Pixel);

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageViews = ResourceViews[Index];
            FMemory::Memzero(&StageViews, sizeof(StageViews));
            NumViews[Index] = 0;
        }
    }

    FD3D12UnorderedAccessView* ResourceViews[ShaderVisibility_Count][D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT];
    uint8 NumViews[ShaderVisibility_Count];
};

struct FD3D12ShaderConstantsCache
{
    FD3D12ShaderConstantsCache()
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(Constants, sizeof(Constants));
        NumConstants = 0;
    }

    uint32 Constants[D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT];
    uint32 NumConstants;
};

struct FD3D12UniqueSamplerTable
{
    FD3D12UniqueSamplerTable()
    {
        Reset();
    }

    void Reset()
    {
        FMemory::Memzero(UniqueIDs, sizeof(UniqueIDs));
    }

    bool operator==(const FD3D12UniqueSamplerTable& Other) const
    {
        return FMemory::Memcmp(UniqueIDs, Other.UniqueIDs, sizeof(UniqueIDs)) == 0;
    }

    bool operator!=(const FD3D12UniqueSamplerTable& Other) const
    {
        return FMemory::Memcmp(UniqueIDs, Other.UniqueIDs, sizeof(UniqueIDs)) != 0;
    }

    uint16 UniqueIDs[D3D12_DEFAULT_SAMPLER_STATE_COUNT];
};

// TODO: Add CRC32 here
inline uint64 GetHashForType(const FD3D12UniqueSamplerTable& Table)
{
    return HashIntegers<uint16, D3D12_DEFAULT_SAMPLER_STATE_COUNT>(Table.UniqueIDs);
}


struct FD3D12SamplerStateCache : public FD3D12ResourceCache
{
    FD3D12SamplerStateCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyState(ShaderVisibility_All, ShaderVisibility_Pixel);

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageSamplers = SamplerStates[Index];
            NumSamplers[Index] = D3D12_DEFAULT_SAMPLER_STATE_COUNT;
            FMemory::Memzero(&StageSamplers, sizeof(StageSamplers));
        }
    }

    FD3D12SamplerState* SamplerStates[ShaderVisibility_Count][D3D12_DEFAULT_SAMPLER_STATE_COUNT];
    uint8               NumSamplers[ShaderVisibility_Count];
};

template<typename KeyType, typename ValueType>
class FD3D12LookupTable
{
public:
    FD3D12LookupTable(int32 NumEntries)
        : Table(NumEntries)
    {
        Clear();
    }

    ValueType* Find(const KeyType& Key)
    {
        const int32 Index = GetHashedIndex(Key);
        if (Table[Index].bHasValue && Key == Table[Index].Key)
        {
            return AddressOf(Table[Index].Value);
        }

        return nullptr;
    }

    void Insert(const ValueType& Value, const KeyType& Key)
    {
        const int32 Index = GetHashedIndex(Key);

        FEntry& Entry = Table[Index];
    #if D3D12_BREAK_ON_HASH_COLLISION
        if (Entry.bHasValue && Entry.Key != Key)
        {
            DEBUG_BREAK();
        }
    #endif

        // Always insert a entry at this hashed index
        Entry.Value     = Value;
        Entry.Key       = Key;
        Entry.bHasValue = true;
    }

    void Clear()
    {
        FMemory::Memzero(Table.Data(), Table.SizeInBytes());
    }

private:
    int32 GetHashedIndex(const KeyType& Entry) const
    {
        const uint64 Hash  = GetHashForType(Entry);
        const uint64 Index = Hash % Table.Size();
        return static_cast<int32>(Index);
    }

    struct FEntry
    {
        KeyType   Key;
        ValueType Value;
        bool      bHasValue;
    };

    TArray<FEntry> Table;
};

using FD3D12SamplerLookupTable = FD3D12LookupTable<FD3D12UniqueSamplerTable, D3D12_GPU_DESCRIPTOR_HANDLE>;


struct FD3D12DescriptorHandleCache
{
    FD3D12DescriptorHandleCache()
    {
        ClearAll();
    }

    void Clear(uint32 StartStage, uint32 EndStage)
    {
        CHECK(EndStage < ShaderVisibility_Count);

        for (uint32 Index = StartStage; Index < EndStage; Index++)
        {
            Handles[Index] = { 0 };
        }
    }

    void ClearAll()
    {
        FMemory::Memzero(Handles, sizeof(Handles));
    }

    D3D12_GPU_DESCRIPTOR_HANDLE Handles[ShaderVisibility_Count];
};


class FD3D12LocalDescriptorHeap : public FD3D12DeviceChild
{
public:
    FD3D12LocalDescriptorHeap(FD3D12Device* InDevice, FD3D12CommandContext& InContext, bool bInSamplers);
    ~FD3D12LocalDescriptorHeap() = default;

    bool Initialize();
    uint32 AllocateHandles(uint32 NumHandles);
    bool Realloc();
    bool HasSpace(uint32 NumHandles) const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(int32 Index) const { return Heap->GetCPUHandle(Index); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int32 Index) const { return Heap->GetGPUHandle(Index); }

    void SetCurrentHandle(uint32 InHandle)
    {
        CHECK(Block != nullptr && InHandle < Block->NumDescriptors);
        CurrentHandle = InHandle;
    }

    FORCEINLINE FD3D12DescriptorHeap* GetHeap() const
    {
        return Heap.Get();
    }

private:
    FD3D12CommandContext&        Context;
    FD3D12DescriptorHeapRef      Heap;
    FD3D12OnlineDescriptorBlock* Block;
    uint32                       CurrentHandle;
    bool                         bSamplers;
};

struct FD3D12DefaultDescriptors
{
    FD3D12ConstantBufferViewRef  DefaultCBV;
    FD3D12ShaderResourceViewRef  DefaultSRV;
    FD3D12UnorderedAccessViewRef DefaultUAV;
    FD3D12RenderTargetViewRef    DefaultRTV;
    FD3D12SamplerStateRef        DefaultSampler;
};

class FD3D12DescriptorCache : public FD3D12DeviceChild
{
public:
    FD3D12DescriptorCache(FD3D12Device* InDevice, FD3D12CommandContext& InContext);
    ~FD3D12DescriptorCache() = default;

    bool Initialize();
    void DirtyState();
    void DirtyStateSamplers();
    void DirtyStateResources();

    void SetRenderTargets(FD3D12RenderTargetCache& Cache);
    void SetVertexBuffers(FD3D12VertexBufferCache& VertexBuffers);
    void SetIndexBuffer(FD3D12IndexBufferCache& IndexBuffer);
    void SetSRVs(FD3D12ShaderResourceViewCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumSRVs, uint32& DescriptorHandleOffset);
    void SetUAVs(FD3D12UnorderedAccessViewCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumUAVs, uint32& DescriptorHandleOffset);
    void SetCBVs(FD3D12ConstantBufferCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumCBVs, uint32& DescriptorHandleOffset);
    void SetSamplers(FD3D12SamplerStateCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumSamplers, uint32& DescriptorHandleOffset);
    void SetDescriptorHeaps();

    FORCEINLINE void DirtyDescriptorHeaps()
    {
        CurrentDescriptorHeaps[0] = nullptr;
        CurrentDescriptorHeaps[1] = nullptr;
    }

    FORCEINLINE FD3D12CommandContext& GetContext() const
    {
        return Context;
    }

    FORCEINLINE FD3D12DefaultDescriptors& GetDefaultDescriptors()
    {
        return DefaultDescriptors;
    }

    FORCEINLINE FD3D12LocalDescriptorHeap& GetResourceHeap()
    {
        return ResourceHeap;
    }

    FORCEINLINE FD3D12LocalDescriptorHeap& GetSamplerHeap()
    {
        return SamplerHeap;
    }

private:
    FD3D12CommandContext&       Context;
    FD3D12DefaultDescriptors    DefaultDescriptors;

    ID3D12DescriptorHeap*       CurrentDescriptorHeaps[2] = { nullptr, nullptr };
    FD3D12LocalDescriptorHeap   ResourceHeap;
    FD3D12LocalDescriptorHeap   SamplerHeap;

    FD3D12DescriptorHandleCache ConstantBufferCache;
    FD3D12DescriptorHandleCache ShaderResourceViewCache;
    FD3D12DescriptorHandleCache UnorderedAccessViewCache;

    FD3D12DescriptorHandleCache SamplerDescriptorHandles;
    FD3D12SamplerLookupTable    SamplerCache;
};
