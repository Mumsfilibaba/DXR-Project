#pragma once
#include "Core/Templates/TypeHash.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "Core/Misc/CRC.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12SamplerState.h"

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
        FMemory::Memzero(BufferResources, sizeof(BufferResources));
        NumVertexBuffers = 0;
    }

    D3D12_VERTEX_BUFFER_VIEW VertexBuffers[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    FD3D12BufferRHI*         BufferResources[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    uint32                   NumVertexBuffers;
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
        BufferResource = nullptr;
    }

    D3D12_INDEX_BUFFER_VIEW IndexBuffer;
    FD3D12BufferRHI*        BufferResource;
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

    FD3D12RenderTargetViewRHI* RenderTargetViews[D3D12_MAX_RENDER_TARGET_COUNT];
    FD3D12DepthStencilViewRHI* DepthStencilView;
    uint32                     NumRenderTargets;
};

enum class ED3D12DescriptorState : uint8
{
    None                 = 0,
    ResourcesDirty       = (1 << 0),
    DescriptorTableDirty = (1 << 1)
};
ENUM_CLASS_OPERATORS(ED3D12DescriptorState)

struct FD3D12ResourceCache
{
    bool IsResourcesDirty(EShaderVisibility ShaderStage) const
    {
        return IsEnumFlagSet(DescriptorState[ShaderStage], ED3D12DescriptorState::ResourcesDirty);
    }

    bool IsDescriptorTableDirty(EShaderVisibility ShaderStage) const
    {
        return IsEnumFlagSet(DescriptorState[ShaderStage], ED3D12DescriptorState::DescriptorTableDirty);
    }

    void DirtyResources(EShaderVisibility ShaderStage)
    {
        DescriptorState[ShaderStage] |= ED3D12DescriptorState::ResourcesDirty;
    }

    void DirtyResourcesAll()
    {
        for (uint32 i = ShaderVisibility_All; i < ShaderVisibility_Count; i++)
        {
            DescriptorState[i] |= ED3D12DescriptorState::ResourcesDirty;
        }
    }

    void DirtyDescriptorTable(EShaderVisibility ShaderStage)
    {
        DescriptorState[ShaderStage] |= ED3D12DescriptorState::DescriptorTableDirty;
    }

    void DirtyDescriptorTableAll()
    {
        for (uint32 i = ShaderVisibility_All; i < ShaderVisibility_Count; i++)
        {
            DescriptorState[i] |= ED3D12DescriptorState::DescriptorTableDirty;
        }
    }

    void ClearResourcesDirty(EShaderVisibility ShaderStage)
    {
        DescriptorState[ShaderStage] &= ~ED3D12DescriptorState::ResourcesDirty;
    }

    void ClearDescriptorTableDirty(EShaderVisibility ShaderStage)
    {
        DescriptorState[ShaderStage] &= ~ED3D12DescriptorState::DescriptorTableDirty;
    }

    void ClearAll()
    {
        for (uint32 i = ShaderVisibility_All; i < ShaderVisibility_Count; i++)
        {
            DescriptorState[i] = ED3D12DescriptorState::None;
        }
    }

    ED3D12DescriptorState DescriptorState[ShaderVisibility_Count];
};

struct FD3D12ConstantBufferCache : public FD3D12ResourceCache
{
    FD3D12ConstantBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageViews = ResourceViews[Index];
            FMemory::Memzero(&StageViews, sizeof(StageViews));
            FMemory::Memzero(&ViewVersions[Index], sizeof(ViewVersions[Index]));
            NumBuffers[Index] = 0;
        }
    }

    FD3D12BufferRHI* ResourceViews[ShaderVisibility_Count][D3D12_DEFAULT_CONSTANT_BUFFER_COUNT];
    uint32           ViewVersions[ShaderVisibility_Count][D3D12_DEFAULT_CONSTANT_BUFFER_COUNT];
    uint8            NumBuffers[ShaderVisibility_Count];
};

struct FD3D12ShaderResourceViewCache : public FD3D12ResourceCache
{
    FD3D12ShaderResourceViewCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageViews = ResourceViews[Index];
            FMemory::Memzero(&StageViews, sizeof(StageViews));
            FMemory::Memzero(&ViewVersions[Index], sizeof(ViewVersions[Index]));
            NumViews[Index] = 0;
        }
    }

    FD3D12ShaderResourceViewRHI* ResourceViews[ShaderVisibility_Count][D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT];
    uint32                       ViewVersions[ShaderVisibility_Count][D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT];
    uint8                        NumViews[ShaderVisibility_Count];
};

struct FD3D12UnorderedAccessViewCache : public FD3D12ResourceCache
{
    FD3D12UnorderedAccessViewCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageViews = ResourceViews[Index];
            FMemory::Memzero(&StageViews, sizeof(StageViews));
            FMemory::Memzero(&ViewVersions[Index], sizeof(ViewVersions[Index]));
            NumViews[Index] = 0;
        }
    }

    FD3D12UnorderedAccessViewRHI* ResourceViews[ShaderVisibility_Count][D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT];
    uint32                        ViewVersions[ShaderVisibility_Count][D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT];
    uint8                         NumViews[ShaderVisibility_Count];
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

    friend uint64 GetHashForType(const FD3D12UniqueSamplerTable& Table)
    {
        return CRC32::Generate(Table.UniqueIDs, sizeof(Table.UniqueIDs));
    }

    uint16 UniqueIDs[D3D12_DEFAULT_SAMPLER_STATE_COUNT];
};

struct FD3D12SamplerStateCache : public FD3D12ResourceCache
{
    FD3D12SamplerStateCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageSamplers = SamplerStates[Index];
            NumSamplers[Index] = 0;
            FMemory::Memzero(&StageSamplers, sizeof(StageSamplers));
        }
    }

    FD3D12SamplerStateRHI* SamplerStates[ShaderVisibility_Count][D3D12_DEFAULT_SAMPLER_STATE_COUNT];
    uint8                  NumSamplers[ShaderVisibility_Count];
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
        CHECK(StartStage < EndStage && EndStage < ShaderVisibility_Count);

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
        CHECK(Block != nullptr && InHandle <= Block->NumDescriptors);
        CurrentHandle = InHandle;
    }

    FORCEINLINE FD3D12DescriptorHeap* GetHeap() const
    {
        return Heap.Get();
    }

    FORCEINLINE uint32 GetBlockSize() const
    {
        return Block ? Block->NumDescriptors : 0;
    }

private:
    FD3D12CommandContext&        Context;
    FD3D12DescriptorHeapRef      Heap;
    FD3D12OnlineDescriptorBlock* Block;
    uint32                       CurrentHandle;
    bool                         bIsSamplerHeap;
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
    void InvalidateCachedSamplerTables();

    void SetRenderTargets(FD3D12RenderTargetCache& Cache);
    void SetVertexBuffers(FD3D12VertexBufferCache& VertexBuffers);
    void SetIndexBuffer(FD3D12IndexBufferCache& IndexBuffer);

    void PrepareCBVs(FD3D12ConstantBufferCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumCBVs, uint32& DescriptorHandleOffset);
    void PrepareSRVs(FD3D12ShaderResourceViewCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumSRVs, uint32& DescriptorHandleOffset);
    void PrepareUAVs(FD3D12UnorderedAccessViewCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumUAVs, uint32& DescriptorHandleOffset);
    void PrepareSamplers(FD3D12SamplerStateCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumSamplers, uint32& DescriptorHandleOffset);

    void BindCBVs(FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage);
    void BindSRVs(FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage);
    void BindUAVs(FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage);
    void BindSamplers(FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage);

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

    FORCEINLINE const FD3D12DefaultDescriptors& GetDefaultDescriptors() const
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
    FD3D12CommandContext&           Context;
    const FD3D12DefaultDescriptors& DefaultDescriptors;
    ID3D12DescriptorHeap*           CurrentDescriptorHeaps[2] = { nullptr, nullptr };
    FD3D12LocalDescriptorHeap       ResourceHeap;
    FD3D12LocalDescriptorHeap       SamplerHeap;
    FD3D12DescriptorHandleCache     ConstantBufferCache;
    FD3D12DescriptorHandleCache     ShaderResourceViewCache;
    FD3D12DescriptorHandleCache     UnorderedAccessViewCache;
    FD3D12DescriptorHandleCache     SamplerDescriptorHandles;
    FD3D12SamplerLookupTable        SamplerCache;
};
