#pragma once
#include "D3D12RHITexture.h"
#include "D3D12RHIBuffer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

inline CD3D12Resource* D3D12ResourceCast( CRHIMemoryResource* Resource )
{
    if ( Resource )
    {
        if ( CRHITexture* Texture = Resource->AsTexture() )
        {
            CD3D12BaseTexture* DxTexture = D3D12TextureCast( Texture );
            return DxTexture ? DxTexture->GetResource() : nullptr;
        }
        else if ( CRHIBuffer* Buffer = Resource->AsBuffer()  )
        {
            CD3D12BaseBuffer* DxBuffer = D3D12BufferCast( Buffer );
            return DxBuffer ? DxBuffer->GetResource() : nullptr;
        }
    }

    return nullptr; 
}