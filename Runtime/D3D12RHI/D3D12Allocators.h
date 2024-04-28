#pragma once
#include "D3D12DeviceChild.h"

struct FD3D12UploadAllocation
{
    TComPtr<ID3D12Resource> Resource;
    uint8* Memory         = nullptr;
    uint64 ResourceOffset = 0;
};

class FD3D12UploadHeapAllocator : public FD3D12DeviceChild
{
public:
    FD3D12UploadHeapAllocator(FD3D12Device* InDevice);
    virtual ~FD3D12UploadHeapAllocator();

    FD3D12UploadAllocation Allocate(uint64 Size, uint64 Alignment);
    void Release();

private:
    uint64                  BufferSize;
    uint64                  CurrentOffset;
    TComPtr<ID3D12Resource> Resource;
    uint8*                  MappedMemory;
    FCriticalSection        CriticalSection;
};