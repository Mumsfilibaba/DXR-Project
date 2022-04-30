#pragma once
#include "RHI/RHIResourceViews.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// SInterfaceImage

struct SInterfaceImage
{
    SInterfaceImage() = default;

    SInterfaceImage( const TSharedRef<CRHIShaderResourceView>& InImageView
                   , const TSharedRef<CRHITexture>& InImage
                   , EResourceAccess InBefore
                   , EResourceAccess InAfter)
        : ImageView(InImageView)
        , Image(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    { }

    TSharedRef<CRHIShaderResourceView> ImageView;
    TSharedRef<CRHITexture> Image;

    EResourceAccess BeforeState;
    EResourceAccess AfterState;

    bool bAllowBlending = false;
};