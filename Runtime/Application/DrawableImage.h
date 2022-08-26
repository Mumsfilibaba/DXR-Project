#pragma once
#include "RHI/RHIResourceViews.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FDrawableImage

struct FDrawableImage
{
    FDrawableImage() = default;

    FDrawableImage(
        const TSharedRef<FRHIShaderResourceView>& InImageView,
        const TSharedRef<FRHITexture>& InImage,
        EResourceAccess InBefore,
        EResourceAccess InAfter)
        : ImageView(InImageView)
        , Image(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    { }

    TSharedRef<FRHIShaderResourceView> ImageView;
    TSharedRef<FRHITexture>            Image;

    EResourceAccess BeforeState;
    EResourceAccess AfterState;

    bool bAllowBlending = false;
};