#pragma once
#include "Core/Application/UI/IUIWindow.h"
#include "Core/Containers/SharedRef.h"

#include "RHI/RHIResourceViews.h"

// Used when rendering images with ImGui
struct SImGuiImage
{
    SImGuiImage() = default;

    SImGuiImage( const TSharedRef<CRHIShaderResourceView>& InImageView, const TSharedRef<CRHITexture>& InImage, EResourceState InBefore, EResourceState InAfter )
        : ImageView( InImageView )
        , Image( InImage )
        , BeforeState( InBefore )
        , AfterState( InAfter )
    {
    }

    TSharedRef<CRHIShaderResourceView> ImageView;
    TSharedRef<CRHITexture> Image;

    EResourceState BeforeState;
    EResourceState AfterState;
    
    bool AllowBlending = false;
};

class CTextureDebugWindow : public IUIWindow
{
public:

    static TSharedRef<CTextureDebugWindow> Make();

    /* Initializes the panel. The context handle should be set if the global context is not yet, this ensures that panels can be created from different DLLs*/
    virtual void InitContext( UIContextHandle ContextHandle ) override final;

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

    /* Add image for debug drawing */
    void AddTextureForDebugging();

private:

    CTextureDebugWindow() = default;
    ~CTextureDebugWindow() = default;

    /* Debug images */
    TArray<SImGuiImage*> Images;

    /* The selected image */
    int32 SelectedTextureIndex = -1;
};