#pragma once
#include "Core/Application/UI/IUIWindow.h"

class CInspectorWindow : public IUIWindow
{
public:

    CInspectorWindow() = default;
    ~CInspectorWindow() = default;

    /* Initializes the panel. The context handle should be set if the global context is not yet, this ensures that panels can be created from different DLLs*/
    virtual void InitContext( UIContextHandle ContextHandle ) override final;

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

private:

    /* Draws the scene info, should only be called from tick */
    void DrawSceneInfo();
};