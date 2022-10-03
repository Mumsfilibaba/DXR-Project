#include "MetalTexture.h"
#include "MetalViewport.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalTexture

FMetalTexture::FMetalTexture(FMetalDeviceContext* InDeviceContext)
    : FMetalObject(InDeviceContext)
    , Texture(nil)
    , ShaderResourceView(nullptr)
{ }
    
~FMetalTexture()
{
    NSSafeRelease(Texture);
}

id<MTLTexture> FMetalTexture::GetMTLTexture() const
{
    // Need to get the texture from the viewport
    if (Viewport)
    {   
        return Viewport->GetDrawableTexture();
    }
    else
    {
        return Texture;
    }
}