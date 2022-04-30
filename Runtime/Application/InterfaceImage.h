#pragma once
#include "RHI/RHIResourceViews.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// SInterfaceImage

struct SInterfaceImage
{
    SInterfaceImage() = default;

    SInterfaceImage( const TSharedRef<CRHIShaderResourceView>& InImageView
                   , const TSharedRef<CRHITexture>& InImage
                   , ERHIResourceState InBefore
                   , ERHIResourceState InAfter)
        : ImageView(InImageView)
        , Image(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    { }

    TSharedRef<CRHIShaderResourceView> ImageView;
    TSharedRef<CRHITexture> Image;

    ERHIResourceState BeforeState;
    ERHIResourceState AfterState;

    bool bAllowBlending = false;
};