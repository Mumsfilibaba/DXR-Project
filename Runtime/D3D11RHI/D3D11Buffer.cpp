#include "D3D11RHI/D3D11Buffer.h"
#include "D3D11RHI/D3D11Device.h"

FD3D11BufferRHI::FD3D11BufferRHI(FD3D11Device* InDevice, const FRHIBufferDesc& InBufferDesc)
    : FRHIBuffer(InBufferDesc)
    , FD3D11Resource()
    , FD3D11DeviceChild(InDevice)
{
}

FD3D11BufferRHI::~FD3D11BufferRHI() = default;

bool FD3D11BufferRHI::Initialize(ERHIResourceState InInitialState, const void* InInitialData)
{
    const FRHIBufferDesc& BufferDesc = GetDesc();
    if (BufferDesc.Size == 0 || BufferDesc.Size > TNumericLimits<UINT>::Max())
    {
        D3D11_ERROR("[FD3D11BufferRHI]: Buffers must be between 1 byte and 4 GB, got %llu bytes", BufferDesc.Size);
        return false;
    }

    const bool bIsConstantBuffer = BufferDesc.IsConstantBuffer();
    const bool bHasViews         = BufferDesc.IsShaderResourceBuffer() || BufferDesc.IsUnorderedAccessBuffer();
    const bool bIsIndirect       = BufferDesc.IsIndirectArguments();

    D3D11_BUFFER_DESC D3D11Desc = {};
    D3D11Desc.ByteWidth = static_cast<UINT>(bIsConstantBuffer ? Math::AlignUp<uint64>(BufferDesc.Size, D3D11_CONSTANT_BUFFER_ELEMENT_SIZE) : BufferDesc.Size);
    D3D11Desc.Usage     = ConvertBufferUsage(BufferDesc.Flags);
    D3D11Desc.BindFlags = ConvertBufferBindFlags(BufferDesc.Flags);

    if (D3D11Desc.Usage == D3D11_USAGE_STAGING)
    {
        D3D11Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    }
    else if (D3D11Desc.Usage == D3D11_USAGE_DYNAMIC)
    {
        D3D11Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }

    if (bIsConstantBuffer && D3D11Desc.BindFlags != D3D11_BIND_CONSTANT_BUFFER)
    {
        D3D11_ERROR("[FD3D11BufferRHI]: A constant buffer cannot be bound as anything else in D3D11");
        return false;
    }

    if (D3D11Desc.Usage == D3D11_USAGE_DYNAMIC && (D3D11Desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS))
    {
        D3D11_ERROR("[FD3D11BufferRHI]: Dynamic and transient buffers cannot have UnorderedAccessViews in D3D11");
        return false;
    }

    if (bHasViews)
    {
        const bool bStructured = BufferDesc.Stride > 0 && !BufferDesc.IsVertexBuffer() && !BufferDesc.IsIndexBuffer() && !bIsIndirect;
        if (bStructured)
        {
            D3D11Desc.MiscFlags          |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            D3D11Desc.StructureByteStride = BufferDesc.Stride;
        }
        else
        {
            D3D11Desc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        }
    }

    if (bIsIndirect)
    {
        D3D11Desc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    }

    TArray<uint8> PaddedData;
    const void* InitialData = InInitialData;
    if (InInitialData && D3D11Desc.ByteWidth != BufferDesc.Size)
    {
        PaddedData.Resize(static_cast<int32>(D3D11Desc.ByteWidth));
        Memory::Memzero(PaddedData.Data(), PaddedData.Size());
        Memory::Memcpy(PaddedData.Data(), InInitialData, BufferDesc.Size);
        InitialData = PaddedData.Data();
    }

    D3D11_SUBRESOURCE_DATA SubresourceData = {};
    SubresourceData.pSysMem = InitialData;

    TComPtr<ID3D11Buffer> NewBuffer;
    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateBuffer(&D3D11Desc, InitialData ? &SubresourceData : nullptr, &NewBuffer);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11BufferRHI]: FAILED to create buffer of %u bytes (0x%08X)", D3D11Desc.ByteWidth, static_cast<uint32>(Result));
        return false;
    }

    Resource     = NewBuffer;
    CurrentState = InInitialState;
    return true;
}

void* FD3D11BufferRHI::Map(uint64 Offset, uint64 /* Size */)
{
    D3D11_MAP MapType = D3D11_MAP_WRITE_DISCARD;
    if (GetDesc().IsReadBack())
    {
        MapType = D3D11_MAP_READ;
    }
    else if (!GetDesc().IsDynamic() && !GetDesc().IsTransient())
    {
        D3D11_ERROR("[FD3D11BufferRHI]: Only dynamic, transient and read-back buffers can be mapped");
        return nullptr;
    }

    D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
    const HRESULT Result = GetDevice()->GetD3D11Context()->Map(Resource.Get(), 0, MapType, 0, &MappedSubresource);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11BufferRHI]: FAILED to map buffer (0x%08X)", static_cast<uint32>(Result));
        return nullptr;
    }

    return static_cast<uint8*>(MappedSubresource.pData) + Offset;
}

void FD3D11BufferRHI::Unmap(uint64 /* Offset */, uint64 /* Size */)
{
    GetDevice()->GetD3D11Context()->Unmap(Resource.Get(), 0);
}

void FD3D11BufferRHI::SetDebugName(const String& InName)
{
    D3D11SetDebugName(Resource.Get(), InName);
}

void FD3D11BufferRHI::GetDebugName(String& OutDebugName) const
{
    D3D11GetDebugName(Resource.Get(), OutDebugName);
}
