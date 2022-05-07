#pragma once
#include "Core/Containers/SharedRef.h"

#include "RHI/RHIResourceViews.h"

#include "Canvas/CanvasImage.h"
#include "Canvas/CanvasWindow.h"

#include <imgui.h>

class CTextureDebugWindow : public CCanvasWindow
{
    INTERFACE_GENERATE_BODY();

public:

    static TSharedRef<CTextureDebugWindow> Make();

     /** @brief: Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

     /** @brief: Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

     /** @brief: Add image for debug drawing */
    void AddTextureForDebugging(const TSharedRef<CRHIShaderResourceView>& ImageView, const TSharedRef<CRHITexture>& Image, EResourceAccess BeforeState, EResourceAccess AfterState);

    void ClearImages()
    {
        DebugTextures.Clear();
    }

private:

    CTextureDebugWindow() = default;
    ~CTextureDebugWindow() = default;

     /** @brief: Debug images */
    TArray<SCanvasImage> DebugTextures;

     /** @brief: The selected image */
    int32 SelectedTextureIndex = -1;
};
