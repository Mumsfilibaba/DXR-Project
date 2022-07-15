#pragma once
#include "Core/Containers/SharedRef.h"

#include "RHI/RHIResourceViews.h"

#include "Canvas/DrawableImage.h"
#include "Canvas/Window.h"

#include <imgui.h>

class FTextureDebugWindow : public FWindow
{
    INTERFACE_GENERATE_BODY();

public:

    static TSharedRef<FTextureDebugWindow> Make();

     /** @brief: Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

     /** @brief: Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

     /** @brief: Add image for debug drawing */
    void AddTextureForDebugging(const TSharedRef<FRHIShaderResourceView>& ImageView, const TSharedRef<FRHITexture>& Image, EResourceAccess BeforeState, EResourceAccess AfterState);

    void ClearImages()
    {
        DebugTextures.Clear();
    }

private:

    FTextureDebugWindow() = default;
    ~FTextureDebugWindow() = default;

     /** @brief: Debug images */
    TArray<FDrawableImage> DebugTextures;

     /** @brief: The selected image */
    int32 SelectedTextureIndex = -1;
};
