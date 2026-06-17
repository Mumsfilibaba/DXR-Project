#pragma once
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12Fence.h"
#include "D3D12RHI/D3D12ResidencyManager.h"
#include "D3D12RHI/D3D12Query.h"

class FD3D12ComputePipelineStateRHI;

class FD3D12CommandAllocator : public FD3D12DeviceChild, FNonCopyable
{
public:
    FD3D12CommandAllocator(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12CommandAllocator();

    bool Initialize();
    bool Reset();

    ID3D12CommandAllocator* GetD3D12Allocator() const
    {
        return Allocator.Get();
    }

    void SetDebugName(const String& Name)
    {
        WString WideName = CharToWide(Name);
        Allocator->SetName(*WideName);
    }

    ED3D12CommandQueueType GetQueueType() const
    {
        return QueueType;
    }

private:
    ED3D12CommandQueueType          QueueType;
    TComPtr<ID3D12CommandAllocator> Allocator;
};

class FD3D12CommandAllocatorManager : public FD3D12DeviceChild, FNonCopyable
{
public:
    FD3D12CommandAllocatorManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12CommandAllocatorManager();

    FD3D12CommandAllocator* ObtainAllocator();
    void RecycleAllocator(FD3D12CommandAllocator* InAllocator);

    ED3D12CommandQueueType GetQueueType() const
    {
        return QueueType;
    }

private:
    ED3D12CommandQueueType const    QueueType;
    D3D12_COMMAND_LIST_TYPE         CommandListType;
    TQueue<FD3D12CommandAllocator*> AvailableAllocators;
    TArray<FD3D12CommandAllocator*> CommandAllocators;
    FCriticalSection                CommandAllocatorsCS;
};

class FD3D12CommandList : public FD3D12DeviceChild, FNonCopyable
{
    friend struct FD3D12Commands;

    template<typename CommandListInterfaceType>
    struct CommandList
    {
        CommandList(FD3D12CommandList* InCommandListParent, CommandListInterfaceType* InCommandListInterface)
            : CommandListParent(InCommandListParent)
            , CommandListInterface(InCommandListInterface)
        {    
        }

        CommandListInterfaceType* operator->()
        {
            CommandListParent->NumCommands++;
            return CommandListInterface;
        }

		bool IsValid() const
		{
			return CommandListInterface != nullptr;
		}

        FD3D12CommandList*        CommandListParent;
        CommandListInterfaceType* CommandListInterface;
    };

public:
    FD3D12CommandList(FD3D12Device* InDevice);
    ~FD3D12CommandList();
    
    bool Initialize(D3D12_COMMAND_LIST_TYPE Type, FD3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline);
    bool Reset(FD3D12CommandAllocator* Allocator);
    bool Close();

    void BeginQuery(const FD3D12Query& Query);
    void EndQuery(const FD3D12Query& Query);
    void InsertBeginTimestamp(FD3D12QueryAllocator& Allocator);
    void InsertEndTimestamp(FD3D12QueryAllocator& Allocator);

    FORCEINLINE bool IsReady() const
    {
        return bIsReady;
    }

    FORCEINLINE uint32 GetNumCommands() const
    {
        return NumCommands;
    }

    FORCEINLINE void SetDebugName(const String& Name)
    {
        WString WideName = CharToWide(Name);
        CmdList->SetName(*WideName);
    }

    FORCEINLINE CommandList<ID3D12GraphicsCommandList> operator->()
    {
        return CommandList(this, CmdList.Get());
    }

    FORCEINLINE CommandList<ID3D12GraphicsCommandList> GetGraphicsCommandList()
    {
        return CommandList(this, CmdList.Get());
    }

#if D3D12_USE_ID3D12COMMANDLIST_1
    FORCEINLINE CommandList<ID3D12GraphicsCommandList1> GetGraphicsCommandList1()
    {
        return CommandList(this, CmdList1.Get());
    }
#endif

#if D3D12_USE_ID3D12COMMANDLIST_2
    FORCEINLINE CommandList<ID3D12GraphicsCommandList2> GetGraphicsCommandList2()
    {
        return CommandList(this, CmdList2.Get());
    }
#endif

#if D3D12_USE_ID3D12COMMANDLIST_3
    FORCEINLINE CommandList<ID3D12GraphicsCommandList3> GetGraphicsCommandList3()
    {
        return CommandList(this, CmdList3.Get());
    }
#endif

#if D3D12_USE_ID3D12COMMANDLIST_4
    FORCEINLINE CommandList<ID3D12GraphicsCommandList4> GetGraphicsCommandList4()
    {
        return CommandList(this, CmdList4.Get());
    }
#endif

#if D3D12_USE_ID3D12COMMANDLIST_5
    FORCEINLINE CommandList<ID3D12GraphicsCommandList5> GetGraphicsCommandList5()
    {
        return CommandList(this, CmdList5.Get());
    }
#endif

#if D3D12_USE_ID3D12COMMANDLIST_6
    FORCEINLINE CommandList<ID3D12GraphicsCommandList6> GetGraphicsCommandList6()
    {
        return CommandList(this, CmdList6.Get());
    }
#endif

#if D3D12_USE_ID3D12COMMANDLIST_7
    FORCEINLINE CommandList<ID3D12GraphicsCommandList7> GetGraphicsCommandList7()
    {
        return CommandList(this, CmdList7.Get());
    }
#endif

#if D3D12_USE_ID3D12COMMANDLIST_8
    FORCEINLINE CommandList<ID3D12GraphicsCommandList8> GetGraphicsCommandList8()
    {
        return CommandList(this, CmdList8.Get());
    }
#endif

#if D3D12_USE_ID3D12COMMANDLIST_9
    FORCEINLINE CommandList<ID3D12GraphicsCommandList9> GetGraphicsCommandList9()
    {
        return CommandList(this, CmdList9.Get());
    }
#endif

#if D3D12_USE_ID3D12COMMANDLIST_10
    FORCEINLINE CommandList<ID3D12GraphicsCommandList10> GetGraphicsCommandList10()
    {
        return CommandList(this, CmdList10.Get());
    }
#endif

    FORCEINLINE ID3D12CommandList* GetCommandList() const
    {
        return CmdList.Get();
    }

    FORCEINLINE void UpdateResidency(FD3D12ResidencyHandle* Handle)
    {
        ResidencySet.Insert(Handle);
    }

    FORCEINLINE FD3D12ResidencySet& GetResidencySet()
    {
        return ResidencySet;
    }

private:
    FD3D12Query                          BeginTimestamp;
    FD3D12Query                          EndTimestamp;
    TArray<FD3D12Query>                  TimestampQueries;
    TArray<FD3D12Query>                  OcclusionQueries;
    TArray<FD3D12Query>                  PipelineStatsQueries;

    TComPtr<ID3D12GraphicsCommandList>   CmdList;
#if D3D12_USE_ID3D12COMMANDLIST_1
    TComPtr<ID3D12GraphicsCommandList1>  CmdList1;
#endif
#if D3D12_USE_ID3D12COMMANDLIST_2
    TComPtr<ID3D12GraphicsCommandList2>  CmdList2;
#endif
#if D3D12_USE_ID3D12COMMANDLIST_3
    TComPtr<ID3D12GraphicsCommandList3>  CmdList3;
#endif
#if D3D12_USE_ID3D12COMMANDLIST_4
    TComPtr<ID3D12GraphicsCommandList4>  CmdList4;
#endif
#if D3D12_USE_ID3D12COMMANDLIST_5
    TComPtr<ID3D12GraphicsCommandList5>  CmdList5;
#endif
#if D3D12_USE_ID3D12COMMANDLIST_6
    TComPtr<ID3D12GraphicsCommandList6>  CmdList6;
#endif
#if D3D12_USE_ID3D12COMMANDLIST_7
    TComPtr<ID3D12GraphicsCommandList7>  CmdList7;
#endif
#if D3D12_USE_ID3D12COMMANDLIST_8
    TComPtr<ID3D12GraphicsCommandList8>  CmdList8;
#endif
#if D3D12_USE_ID3D12COMMANDLIST_9
    TComPtr<ID3D12GraphicsCommandList9>  CmdList9;
#endif
#if D3D12_USE_ID3D12COMMANDLIST_10
    TComPtr<ID3D12GraphicsCommandList10> CmdList10;
#endif
    FD3D12ResidencySet                   ResidencySet;
    uint32                               NumCommands;
    bool                                 bIsReady;
};
