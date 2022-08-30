#pragma once
#include "RHI/RHIResourceViews.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FDrawableTexture

struct FDrawableTexture
{
    FDrawableTexture() = default;

    FDrawableTexture(
        const FRHITextureRef& InImage,
        EResourceAccess InBefore,
        EResourceAccess InAfter)
        : View(MakeSharedRef<FRHIShaderResourceView>(InImage ? InImage->GetShaderResourceView() : nullptr))
        , Texture(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    { }

    FDrawableTexture(
        const FRHIShaderResourceViewRef& InImageView,
        const FRHITextureRef& InImage,
        EResourceAccess InBefore,
        EResourceAccess InAfter)
        : View(InImageView)
        , Texture(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    { }

    FRHIShaderResourceViewRef View;
    FRHITextureRef            Texture;

    EResourceAccess BeforeState;
    EResourceAccess AfterState;

    bool bAllowBlending = false;
    bool bSamplerLinear = false;
};