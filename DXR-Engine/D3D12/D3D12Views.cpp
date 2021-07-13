#include "D3D12Views.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

D3D12View::D3D12View( D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap )
    : D3D12DeviceChild( InDevice )
    , Resource( nullptr )
    , Heap( InHeap )
    , OfflineHandle( { 0 } )
{
    Assert( Heap != nullptr );
}

D3D12View::~D3D12View()
{
    // NOTE: Does not follow the rest of the engine's explicit alloc/dealloc, Ok for now
    Heap->Free( OfflineHandle, OfflineHeapIndex );
}

bool D3D12View::Init()
{
    OfflineHandle = Heap->Allocate( OfflineHeapIndex );
    return OfflineHandle != 0;
}

D3D12ConstantBufferView::D3D12ConstantBufferView( D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap )
    : D3D12View( InDevice, InHeap )
    , Desc()
{
}

bool D3D12ConstantBufferView::CreateView( D3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc )
{
    Assert( OfflineHandle != 0 );

    Resource = MakeSharedRef<D3D12Resource>( InResource );
    Desc = InDesc;
    GetDevice()->CreateConstantBufferView( &Desc, OfflineHandle );

    return true;
}

D3D12BaseShaderResourceView::D3D12BaseShaderResourceView( D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap )
    : D3D12View( InDevice, InHeap )
    , Desc()
{
}

bool D3D12BaseShaderResourceView::CreateView( D3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc )
{
    Assert( OfflineHandle != 0 );

    D3D12View::Resource = MakeSharedRef<D3D12Resource>( InResource );
    Desc = InDesc;

    ID3D12Resource* NativeResource = nullptr;
    if ( D3D12View::Resource )
    {
        NativeResource = D3D12View::Resource->GetResource();
    }

    GetDevice()->CreateShaderResourceView( NativeResource, &Desc, OfflineHandle );
    return true;
}

D3D12BaseUnorderedAccessView::D3D12BaseUnorderedAccessView( D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap )
    : D3D12View( InDevice, InHeap )
    , Desc()
    , CounterResource( nullptr )
{
}

bool D3D12BaseUnorderedAccessView::CreateView( D3D12Resource* InCounterResource, D3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc )
{
    Assert( OfflineHandle != 0 );

    Desc = InDesc;
    CounterResource = InCounterResource;
    D3D12View::Resource = MakeSharedRef<D3D12Resource>( InResource );

    ID3D12Resource* NativeCounterResource = nullptr;
    if ( CounterResource )
    {
        NativeCounterResource = CounterResource->GetResource();
    }

    ID3D12Resource* NativeResource = nullptr;
    if ( D3D12View::Resource )
    {
        NativeResource = D3D12View::Resource->GetResource();
    }

    GetDevice()->CreateUnorderedAccessView( NativeResource, NativeCounterResource, &Desc, OfflineHandle );
    return true;
}

D3D12BaseRenderTargetView::D3D12BaseRenderTargetView( D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap )
    : D3D12View( InDevice, InHeap )
    , Desc()
{
}

bool D3D12BaseRenderTargetView::CreateView( D3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc )
{
    Assert( InResource != nullptr );
    Assert( OfflineHandle != 0 );

    Desc = InDesc;
    D3D12View::Resource = MakeSharedRef<D3D12Resource>( InResource );
    GetDevice()->GetDevice()->CreateRenderTargetView( D3D12View::Resource->GetResource(), &Desc, OfflineHandle );

    return true;
}

D3D12BaseDepthStencilView::D3D12BaseDepthStencilView( D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InHeap )
    : D3D12View( InDevice, InHeap )
    , Desc()
{
}

bool D3D12BaseDepthStencilView::CreateView( D3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc )
{
    Assert( InResource != nullptr );
    Assert( OfflineHandle != 0 );

    Desc = InDesc;
    D3D12View::Resource = MakeSharedRef<D3D12Resource>( InResource );
    GetDevice()->GetDevice()->CreateDepthStencilView( D3D12View::Resource->GetResource(), &Desc, OfflineHandle );

    return true;
}
