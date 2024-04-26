#pragma once
#include "D3D12DeviceChild.h"
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/AsyncTask.h"
#include "Core/Threading/Runnable.h"
#include "Core/Threading/AtomicInt.h"

class FD3D12OnlineDescriptorHeap;
struct FD3D12OnlineDescriptorBlock;

struct FD3D12DeferredObject
{
    // Go through an array of deferred resources and delete them
    static void ProcessItems(const TArray<FD3D12DeferredObject>& Items);

    enum class EType
    {
        D3DResource           = 1,
        Resource              = 2,
        RHIResource           = 3,
        OnlineDescriptorBlock = 4,
    };

    FD3D12DeferredObject(FRHIResource* InResource)
        : Type(EType::RHIResource)
        , RHIResource{InResource}
    {
        CHECK(InResource != nullptr);
        InResource->AddRef();
    }

    FD3D12DeferredObject(ID3D12Resource* InResource)
        : Type(EType::D3DResource)
        , D3DResource{InResource}
    {
        CHECK(InResource != nullptr);
        InResource->AddRef();
    }

    FD3D12DeferredObject(FD3D12Resource* InResource)
        : Type(EType::Resource)
        , Resource{InResource}
    {
        CHECK(InResource != nullptr);
        InResource->AddRef();
    }

    FD3D12DeferredObject(FD3D12OnlineDescriptorHeap* InHeap, FD3D12OnlineDescriptorBlock* InBlock)
        : Type(EType::OnlineDescriptorBlock)
        , OnlineDescriptorBlock{InHeap, InBlock}
    {
        CHECK(InHeap != nullptr);
        CHECK(InBlock != nullptr);
    }

    EType Type;
    union
    {
        struct 
        {
            FD3D12OnlineDescriptorHeap*  Heap;
            FD3D12OnlineDescriptorBlock* Block;
        } OnlineDescriptorBlock;

        FRHIResource*   RHIResource;
        FD3D12Resource* Resource;
        ID3D12Resource* D3DResource;
    };
};
