#pragma once
#include "RHI/RHIResourceViews.h"

// Used when rendering images with ImGui
struct SUIImage
{
    SUIImage() = default;

    SUIImage( const TSharedRef<CRHIShaderResourceView>& InImageView, const TSharedRef<CRHITexture>& InImage, EResourceState InBefore, EResourceState InAfter )
        : ImageView( InImageView )
        , Image( InImage )
        , BeforeState( InBefore )
        , AfterState( InAfter )
    {
    }

    TSharedRef<CRHIShaderResourceView> ImageView;
    TSharedRef<CRHITexture> Image;

    EResourceState BeforeState;
    EResourceState AfterState;

    bool AllowBlending = false;
};