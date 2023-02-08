#pragma once
#include "Core/Containers/SharedRef.h"
#include "Application/DrawableTexture.h"
#include "Application/Widget.h"
#include "RHI/RHIResources.h"

class FRenderTargetDebugWindow 
    : public FWidget
{
public:

     /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void OnDraw() override final;

     /** @brief - Add image for debug drawing */
    void AddTextureForDebugging(
        const FRHIShaderResourceViewRef& ImageView,
        const FRHITextureRef& Image,
        EResourceAccess BeforeState,
        EResourceAccess AfterState);

    void ClearImages()
    {
        DebugTextures.Clear();
    }

private:
    TArray<FDrawableTexture> DebugTextures;
    int32 SelectedTextureIndex = 0;
};
