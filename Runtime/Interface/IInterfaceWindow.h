#pragma once
#include "Core/RefCounted.h"

typedef void* InterfaceContext;

// Helper for init the current context
#define INIT_CONTEXT( ContextHandle )                                                       \
    {                                                                                       \
        ImGuiContext* NewImGuiContext     = reinterpret_cast<ImGuiContext*>(ContextHandle); \
        ImGuiContext* CurrentImGuiContext = ImGui::GetCurrentContext();                     \
        if ( NewImGuiContext != CurrentImGuiContext )                                       \
        {                                                                                   \
            ImGui::SetCurrentContext( NewImGuiContext );                                    \
        }                                                                                   \
    }

/* Helper for generating the default init-context function since it is unsure how DLLs handle the UIContext */
#define INTERFACE_GENERATE_BODY()                                             \
public:                                                                       \
                                                                              \
    virtual void InitContext( InterfaceContext ContextHandle ) override final \
    {                                                                         \
        INIT_CONTEXT( ContextHandle );                                        \
    }                                                                         \
                                                                              \
private:

class IInterfaceWindow : public CRefCounted
{
public:

    virtual ~IInterfaceWindow() = default;

    /* Initializes the panel. The context handle should be set if the global context is not yet, this ensures that panels can be created from different DLLs*/
    virtual void InitContext( InterfaceContext ContextHandle ) = 0;

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() = 0;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() = 0;
};