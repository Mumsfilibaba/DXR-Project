#pragma once
#include "RHI/RHIResourceViews.h"


/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// SInterfaceImage - Used when rendering images with ImGui

struct SInterfaceImage
{
    SInterfaceImage() = default;

    SInterfaceImage(const TSharedRef<CRHIShaderResourceView>& InImageView, const TSharedRef<CRHITexture>& InImage, ERHIResourceAccess InBefore, ERHIResourceAccess InAfter)
        : ImageView(InImageView)
        , Image(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    { }

    TSharedRef<CRHIShaderResourceView> ImageView;
    TSharedRef<CRHITexture> Image;

    ERHIResourceAccess BeforeState;
    ERHIResourceAccess AfterState;

    bool bAllowBlending = false;
};