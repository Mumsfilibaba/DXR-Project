#pragma once
#include "RHI/RHIResourceViews.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FDrawableImage

struct FDrawableImage
{
    FDrawableImage() = default;

    FDrawableImage(
        const FRHIShaderResourceViewRef& InImageView,
        const FRHITextureRef& InImage,
        EResourceAccess InBefore,
        EResourceAccess InAfter)
        : ImageView(InImageView)
        , Image(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    { }

    FRHIShaderResourceViewRef ImageView;
    FRHITextureRef            Image;

    EResourceAccess BeforeState;
    EResourceAccess AfterState;

    bool bAllowBlending = false;
};