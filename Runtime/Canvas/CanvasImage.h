#pragma once
#include "RHI/RHIResourceViews.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// SCanvasImage

struct SCanvasImage
{
    SCanvasImage() = default;

    SCanvasImage( const TSharedRef<FRHIShaderResourceView>& InImageView
                , const TSharedRef<FRHITexture>& InImage
                , EResourceAccess InBefore
                , EResourceAccess InAfter)
        : ImageView(InImageView)
        , Image(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    { }

    TSharedRef<FRHIShaderResourceView> ImageView;
    TSharedRef<FRHITexture> Image;

    EResourceAccess BeforeState;
    EResourceAccess AfterState;

    bool bAllowBlending = false;
};