#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"

#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Buffer

class FD3D12Buffer : public FD3D12DeviceChild
{
public:

    FD3D12Buffer(FD3D12Device* InDevice)
        : FD3D12DeviceChild(InDevice)
        , Resource(nullptr)
    { }

    virtual void SetResource(FD3D12Resource* InResource) { Resource = InResource; }

    uint64 GetSizeInBytes() const { return Resource ? static_cast<uint64>(Resource->GetDesc().Width) : 0u; }

    FD3D12Resource* GetD3D12Resource() const { return Resource.Get(); }

protected:
    TSharedRef<FD3D12Resource> Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12VertexBuffer

class FD3D12VertexBuffer : public FRHIVertexBuffer, public FD3D12Buffer
{
public:

    FD3D12VertexBuffer(FD3D12Device* InDevice, const FRHIVertexBufferInitializer& Initializer)
        : FRHIVertexBuffer(Initializer)
        , FD3D12Buffer(InDevice)
        , View()
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIVertexBuffer Interface

    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<FD3D12Buffer*>(this)); }

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void SetName(const String& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FD3D12Buffer Interface

    virtual void SetResource(FD3D12Resource* InResource) override final
    {
        FD3D12Buffer::SetResource(InResource);

        FMemory::Memzero(&View);
        View.StrideInBytes  = GetStride();
        View.SizeInBytes    = GetNumVertices() * View.StrideInBytes;
        View.BufferLocation = FD3D12Buffer::Resource->GetGPUVirtualAddress();
    }

public:

    FORCEINLINE const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return View; }

private:
    D3D12_VERTEX_BUFFER_VIEW View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12IndexBuffer

class FD3D12IndexBuffer : public FRHIIndexBuffer, public FD3D12Buffer
{
public:

    FD3D12IndexBuffer(FD3D12Device* InDevice, const FRHIIndexBufferInitializer& Initializer)
        : FRHIIndexBuffer(Initializer)
        , FD3D12Buffer(InDevice)
        , View()
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIIndexBuffer Interface

    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<FD3D12Buffer*>(this)); }

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void SetName(const String& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FD3D12Buffer Interface

    virtual void SetResource(FD3D12Resource* InResource) override final
    {
        FD3D12Buffer::SetResource(InResource);

        FMemory::Memzero(&View);

        EIndexFormat IndexFormat = GetFormat();
        if (IndexFormat != EIndexFormat::Unknown)
        {
            View.Format         = (IndexFormat == EIndexFormat::uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            View.SizeInBytes    = GetNumIndicies() * GetStrideFromIndexFormat(IndexFormat);
            View.BufferLocation = FD3D12Buffer::Resource->GetGPUVirtualAddress();
        }
    }

public:

    FORCEINLINE const D3D12_INDEX_BUFFER_VIEW& GetView() const { return View; }

private:
    D3D12_INDEX_BUFFER_VIEW View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ConstantBuffer

class FD3D12ConstantBuffer : public FRHIConstantBuffer, public FD3D12Buffer
{
public:

    FD3D12ConstantBuffer(FD3D12Device* InDevice, FD3D12OfflineDescriptorHeap* InOfflineHeap, const FRHIConstantBufferInitializer& Initializer)
        : FRHIConstantBuffer(Initializer)
        , FD3D12Buffer(InDevice)
        , View(InDevice, InOfflineHeap)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIConstantBuffer Interface

    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<FD3D12Buffer*>(this)); }

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const String& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FD3D12Buffer Interface

    virtual void SetResource(FD3D12Resource* InResource) override final
    {
        FD3D12Buffer::SetResource(InResource);

        D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
        FMemory::Memzero(&ViewDesc);

        ViewDesc.BufferLocation = FD3D12Buffer::Resource->GetGPUVirtualAddress();
        ViewDesc.SizeInBytes    = (uint32)FD3D12Buffer::GetSizeInBytes();

        if (View.GetOfflineHandle() == 0)
        {
            if (!View.AllocateHandle())
            {
                return;
            }
        }

        View.CreateView(FD3D12Buffer::Resource.Get(), ViewDesc);
    }

public:

    FD3D12ConstantBufferView& GetView() { return View; }

    const FD3D12ConstantBufferView& GetView() const { return View; }

private:
    FD3D12ConstantBufferView View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12GenericBuffer

class FD3D12GenericBuffer : public FRHIGenericBuffer, public FD3D12Buffer
{
public:
    
    FD3D12GenericBuffer(FD3D12Device* InDevice, const FRHIGenericBufferInitializer& Initializer)
        : FRHIGenericBuffer(Initializer)
        , FD3D12Buffer(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIGenericBuffer Interface

    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<FD3D12Buffer*>(this)); }

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void SetName(const String& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GetD3D12Buffer

inline FD3D12Buffer* GetD3D12Buffer(FRHIBuffer* Buffer)
{
    return Buffer ? reinterpret_cast<FD3D12Buffer*>(Buffer->GetRHIBaseBuffer()) : nullptr;
}

inline FD3D12Resource* GetD3D12Resource(FRHIBuffer* Buffer)
{
    return Buffer ? reinterpret_cast<FD3D12Resource*>(Buffer->GetRHIBaseResource()) : nullptr;
}