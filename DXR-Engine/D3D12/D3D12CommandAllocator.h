#pragma once
#include "D3D12Device.h"

class CD3D12CommandAllocator : public CD3D12DeviceChild
{
public:
    CD3D12CommandAllocator( CD3D12Device* InDevice )
        : CD3D12DeviceChild( InDevice )
        , Allocator( nullptr )
    {
    }

    FORCEINLINE bool Init( D3D12_COMMAND_LIST_TYPE Type )
    {
        HRESULT Result = GetDevice()->GetDevice()->CreateCommandAllocator( Type, IID_PPV_ARGS( &Allocator ) );
        if ( SUCCEEDED( Result ) )
        {
            LOG_INFO( "[D3D12Device]: Created CommandAllocator" );
            return true;
        }
        else
        {
            LOG_ERROR( "[D3D12Device]: FAILED to create CommandAllocator" );
            return false;
        }
    }

    FORCEINLINE bool Reset()
    {
        HRESULT Result = Allocator->Reset();
        if ( Result == DXGI_ERROR_DEVICE_REMOVED )
        {
            DeviceRemovedHandler( GetDevice() );
        }

        return SUCCEEDED( Result );
    }

    FORCEINLINE void SetName( const CString& Name )
    {
        WString WideName = CharToWide( Name );
        Allocator->SetName( WideName.CStr() );
    }

    FORCEINLINE ID3D12CommandAllocator* GetAllocator() const
    {
        return Allocator.Get();
    }

private:
    TComPtr<ID3D12CommandAllocator> Allocator;
};