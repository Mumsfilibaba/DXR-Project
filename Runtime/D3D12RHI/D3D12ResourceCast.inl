#pragma once
#include "D3D12RHITexture.h"
#include "D3D12RHIBuffer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

inline CD3D12Resource* D3D12ResourceCast( CRHIMemoryResource* Resource )
{
    if ( Resource )
    {
        if ( D3D12TextureCast( Resource->AsTexture() ) )
        {
            return static_cast<CD3D12BaseTexture*>(Resource)->GetResource();
        }
        else if ( D3D12BufferCast( Resource->AsBuffer() ) )
        {
            return static_cast<CD3D12BaseBuffer*>(Resource)->GetResource();
        }
    }

    return nullptr; 
}