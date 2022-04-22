#pragma once
#include "RHI/RHIResources.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// SInterfaceImage - Used when rendering images with ImGui

struct SInterfaceImage
{
    SInterfaceImage() = default;

    SInterfaceImage(const CRHIShaderResourceViewRef& InImageView, const CRHITextureRef& InImage, EResourceAccess InBefore, EResourceAccess InAfter)
        : ImageView(InImageView)
        , Image(InImage)
        , BeforeState(InBefore)
        , AfterState(InAfter)
    { }

    CRHIShaderResourceViewRef ImageView;
    CRHITextureRef            Image;

    EResourceAccess BeforeState;
    EResourceAccess AfterState;

    bool bAllowBlending = false;
};