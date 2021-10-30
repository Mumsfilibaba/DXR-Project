#pragma once
#include "Core/Application/UI/IUIWindow.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Application/UI/UIImage.h"

#include "RHI/RHIResourceViews.h"

class CTextureDebugWindow : public IUIWindow
{
public:

    static TSharedRef<CTextureDebugWindow> Make()
    {
        return dbg_new CTextureDebugWindow();
    }

    /* Initializes the panel. The context handle should be set if the global context is not yet, this ensures that panels can be created from different DLLs*/
    virtual void InitContext( UIContextHandle ContextHandle ) override final;

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

    /* Add image for debug drawing */
    void AddTextureForDebugging( const TSharedRef<CRHIShaderResourceView>& ImageView, const TSharedRef<CRHITexture>& Image, EResourceState BeforeState, EResourceState AfterState );

    void ClearImages()
    {
        DebugTextures.Clear();
    }

private:

    CTextureDebugWindow() = default;
    ~CTextureDebugWindow() = default;

    /* Debug images */
    TArray<SUIImage> DebugTextures;

    /* The selected image */
    int32 SelectedTextureIndex = -1;
};