#pragma once
#include "D3D12DeviceChild.h"
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/AsyncTask.h"
#include "Core/Threading/ThreadInterface.h"
#include "Core/Threading/AtomicInt.h"

class FD3D12OnlineDescriptorHeap;
struct FD3D12OnlineDescriptorBlock;

struct FD3D12DeferredObject
{
    enum class EDeferredObjectType
    {
        D3DResource           = 1,
        Resource              = 2,
        RHIResource           = 3,
        OnlineDescriptorBlock = 4,
    };

    FD3D12DeferredObject() = default;

    FD3D12DeferredObject(uint64 InFenceValue, IRefCounted* InResource)
        : Type(EDeferredObjectType::RHIResource)
        , FenceValue(InFenceValue)
        , RHIResource{InResource}
    {
        CHECK(InResource != nullptr);
        InResource->AddRef();
    }

    FD3D12DeferredObject(uint64 InFenceValue, ID3D12Resource* InResource)
        : Type(EDeferredObjectType::D3DResource)
        , FenceValue(InFenceValue)
        , D3DResource{InResource}
    {
        CHECK(InResource != nullptr);
        InResource->AddRef();
    }

    FD3D12DeferredObject(uint64 InFenceValue, FD3D12Resource* InResource)
        : Type(EDeferredObjectType::Resource)
        , FenceValue(InFenceValue)
        , Resource{InResource}
    {
        CHECK(InResource != nullptr);
        InResource->AddRef();
    }

    FD3D12DeferredObject(uint64 InFenceValue, FD3D12OnlineDescriptorHeap* InHeap, FD3D12OnlineDescriptorBlock* InBlock)
        : Type(EDeferredObjectType::OnlineDescriptorBlock)
        , FenceValue(InFenceValue)
        , OnlineDescriptorBlock{InHeap, InBlock}
    {
        CHECK(InHeap != nullptr);
        CHECK(InBlock != nullptr);
    }

    EDeferredObjectType Type;
    uint64 FenceValue;

    union
    {
        struct 
        {
            FD3D12OnlineDescriptorHeap*  Heap;
            FD3D12OnlineDescriptorBlock* Block;
        } OnlineDescriptorBlock;

        IRefCounted*    RHIResource;
        FD3D12Resource* Resource;
        ID3D12Resource* D3DResource;
    };
};

class FD3D12DeferredDeletionQueue : public FThreadInterface, public FD3D12DeviceChild
{
public:
    FD3D12DeferredDeletionQueue(FD3D12Device* InDevice);
    ~FD3D12DeferredDeletionQueue() = default;

    bool Initialize();

    virtual bool Start() override final;

    virtual int32 Run() override final;

    virtual void Stop() override final;

    void WaitForOutstandingTasks() const
    {
        while (!Tasks.IsEmpty())
        {
            FPlatformThreadMisc::Pause();
        }
    }

    template<typename... ArgTypes>
    void DeferDeletion(uint64 InFenceValue, ArgTypes&&... Args)
    {
        FD3D12DeferredObject NewObject(InFenceValue, ::Forward<ArgTypes>(Args)...);
        Tasks.Emplace(InFenceValue, ::Forward<ArgTypes>(Args)...);
    }

private:
    TSharedRef<FGenericThread>                     Thread;
    TQueue<FD3D12DeferredObject, EQueueType::MPSC> Tasks;
    bool                                           bIsRunning;
};
