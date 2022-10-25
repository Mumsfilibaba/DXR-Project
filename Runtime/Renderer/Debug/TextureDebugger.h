#pragma once
#include "Core/Containers/SharedRef.h"

#include "RHI/RHIResources.h"

#include "Application/DrawableTexture.h"
#include "Application/Window.h"

#include <imgui.h>

class FRenderTargetDebugWindow 
    : public FWindow
{
    INTERFACE_GENERATE_BODY();

public:
    static TSharedRef<FRenderTargetDebugWindow> Create();

     /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

     /** @brief - Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

     /** @brief - Add image for debug drawing */
    void AddTextureForDebugging(const FRHIShaderResourceViewRef& ImageView, const FRHITextureRef& Image, EResourceAccess BeforeState, EResourceAccess AfterState);

    void ClearImages()
    {
        DebugTextures.Clear();
    }

private:
    FRenderTargetDebugWindow()  = default;
    ~FRenderTargetDebugWindow() = default;

     /** @brief - Debug images */
    TArray<FDrawableTexture> DebugTextures;

     /** @brief - The selected image */
    int32 SelectedTextureIndex = 0;
};
