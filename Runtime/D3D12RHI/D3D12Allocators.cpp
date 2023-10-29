#include "D3D12Allocators.h"
#include "Core/Misc/ConsoleManager.h"

TAutoConsoleVariable<int32> CVarMaxStagingAllocationSize(
    "D3D12RHI.MaxStagingAllocationSize",
    "The maximum size for a resource that uses a shared staging-buffer (MB)",
    8);

FD3D12UploadHeapAllocator::FD3D12UploadHeapAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , BufferSize(0)
    , CurrentOffset(0)
    , Resource(nullptr)
    , MappedMemory(nullptr)
{
}

FD3D12UploadHeapAllocator::~FD3D12UploadHeapAllocator()
{
    BufferSize    = 0;
    CurrentOffset = 0;
    MappedMemory  = nullptr;
    Resource      = nullptr;
}

FD3D12UploadAllocation FD3D12UploadHeapAllocator::Allocate(uint64 Size, uint64 Alignment)
{
    FD3D12UploadAllocation Allocation;
    
    // Make sure the size is properly aligned
    Size = FMath::AlignUp<uint64>(Size, Alignment);

    // Maximum size for a upload buffer
    const uint64 MaxUploadSize = static_cast<uint64>(CVarMaxStagingAllocationSize.GetValue()) * 1024 * 1024;
    if (Size < MaxUploadSize)
    {
        // Lock the buffer and all variable within
        SCOPED_LOCK(CriticalSection);

        uint64 Offset    = FMath::AlignUp<uint64>(CurrentOffset, Alignment);
        uint64 NewOffset = Offset + Size;
        if (NewOffset >= BufferSize)
        {
            // Allocate a new 
            D3D12_HEAP_PROPERTIES HeapProperties;
            FMemory::Memzero(&HeapProperties);

            HeapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
            HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC Desc;
            FMemory::Memzero(&Desc);

            Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            Desc.Flags              = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            Desc.Format             = DXGI_FORMAT_UNKNOWN;
            Desc.Width              = MaxUploadSize;
            Desc.Height             = 1;
            Desc.DepthOrArraySize   = 1;
            Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            Desc.MipLevels          = 1;
            Desc.SampleDesc.Count   = 1;
            Desc.SampleDesc.Quality = 0;

            TComPtr<ID3D12Resource> NewResource;
            HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&NewResource));
            if (SUCCEEDED(Result))
            {
                Resource = NewResource;
                Resource->SetName(L"FD3D12UploadHeapAllocator Buffer");
                Resource->Map(0, nullptr, reinterpret_cast<void**>(&MappedMemory));
            }
            else
            {
                D3D12_ERROR("[FD3D12UploadHeapAllocator] Failed to create UploadBuffer");
                return Allocation;
            }

            CHECK(Size <= MaxUploadSize);

            BufferSize = MaxUploadSize;
            Offset     = 0;
            NewOffset  = Offset + Size;
        }

        Allocation.Resource       = Resource;
        Allocation.ResourceOffset = Offset;
        Allocation.Memory         = MappedMemory + Offset;
        CurrentOffset             = NewOffset;
    }
    else
    {
        // Allocate a new 
        D3D12_HEAP_PROPERTIES HeapProperties;
        FMemory::Memzero(&HeapProperties);

        HeapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
        HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC Desc;
        FMemory::Memzero(&Desc);

        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Width              = Size;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.MipLevels          = 1;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        TComPtr<ID3D12Resource> NewResource;
        HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&NewResource));
        if (FAILED(Result))
        {
            D3D12_ERROR("[FD3D12UploadHeapAllocator] Failed to create UploadBuffer");
            return Allocation;
        }
        
        NewResource->SetName(L"FD3D12UploadHeapAllocator Buffer");
        NewResource->Map(0, nullptr, reinterpret_cast<void**>(&Allocation.Memory));

        Allocation.Resource       = NewResource;
        Allocation.ResourceOffset = 0;
    }

    return Allocation;
}