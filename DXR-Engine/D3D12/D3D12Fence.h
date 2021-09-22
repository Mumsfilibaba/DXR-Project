#pragma once
#include "D3D12DeviceChild.h"

class D3D12FenceHandle : public D3D12DeviceChild
{
public:
    D3D12FenceHandle( D3D12Device* InDevice )
        : D3D12DeviceChild( InDevice )
        , Fence( nullptr )
        , Event( 0 )
    {
    }

    ~D3D12FenceHandle()
    {
        if ( Event )
        {
            CloseHandle( Event );
        }
    }

    bool Init( uint64 InitalValue )
    {
        HRESULT Result = GetDevice()->GetDevice()->CreateFence( InitalValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &Fence ) );
        if ( FAILED( Result ) )
        {
            LOG_INFO( "[D3D12FenceHandle]: FAILED to create Fence" );
            return false;
        }

        Event = CreateEvent( nullptr, FALSE, FALSE, nullptr );
        if ( Event == NULL )
        {
            LOG_ERROR( "[D3D12FenceHandle]: FAILED to create Event for Fence" );
            return false;
        }

        return true;
    }

    bool WaitForValue( uint64 Value )
    {
        HRESULT Result = Fence->SetEventOnCompletion( Value, Event );
        if ( SUCCEEDED( Result ) )
        {
            WaitForSingleObject( Event, INFINITE );
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Signal( uint64 Value )
    {
        HRESULT Result = Fence->Signal( Value );
        return SUCCEEDED( Result );
    }

    void SetName( const std::string& Name )
    {
        std::wstring WideName = CharToWide( CString( Name.c_str(), Name.length() ) ).CStr();
        Fence->SetName( WideName.c_str() );
    }

    ID3D12Fence* GetFence() const
    {
        return Fence.Get();
    }

private:
    TComPtr<ID3D12Fence> Fence;
    HANDLE Event = 0;
};