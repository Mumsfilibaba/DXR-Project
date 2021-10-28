#pragma once
#include "Core/Application/UI/IUIWindow.h"
#include "Core/Containers/SharedRef.h"

class CRendererInfoWindow : public IUIWindow
{
public:

    static TSharedRef<CRendererInfoWindow> Make()
    {
        return dbg_new CRendererInfoWindow();
    }

    /* Initializes the panel. The context handle should be set if the global context is not yet, this ensures that panels can be created from different DLLs*/
    virtual void InitContext( UIContextHandle ContextHandle ) override final;

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

private:

    CRendererInfoWindow() = default;
};