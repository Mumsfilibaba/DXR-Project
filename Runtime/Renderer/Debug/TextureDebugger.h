#pragma once
#include "Core/Containers/SharedRef.h"

#include "RHI/RHIResourceViews.h"

#include "Application/InterfaceImage.h"
#include "Application/IWindow.h"

#include <imgui.h>

class CTextureDebugWindow : public IWindow
{
    INTERFACE_GENERATE_BODY();

public:

    static TSharedRef<CTextureDebugWindow> Make();

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

    /* Add image for debug drawing */
    void AddTextureForDebugging(const TSharedRef<CRHIShaderResourceView>& ImageView, const TSharedRef<CRHITexture>& Image, ERHIResourceAccess BeforeState, ERHIResourceAccess AfterState);

    void ClearImages()
    {
        DebugTextures.Clear();
    }

private:

    CTextureDebugWindow() = default;
    ~CTextureDebugWindow() = default;

    /* Debug images */
    TArray<SInterfaceImage> DebugTextures;

    /* The selected image */
    int32 SelectedTextureIndex = -1;
};