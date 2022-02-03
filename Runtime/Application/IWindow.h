#pragma once
#include "Core/RefCounted.h"

typedef void* InterfaceContext;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper macros

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

// Helper for generating the default init-context function since it is unsure how DLLs handle the UIContext
#define INTERFACE_GENERATE_BODY()                                           \
public:                                                                     \
                                                                            \
    virtual void InitContext(InterfaceContext ContextHandle) override final \
    {                                                                       \
        INIT_CONTEXT(ContextHandle);                                        \
    }                                                                       \
                                                                            \
private:

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IWindow

class IWindow : public CRefCounted
{
public:

    virtual ~IWindow() = default;

    /**
     * Initializes the window's context
     * 
     * @param ContextHandle: Context for the interface 
     */
    virtual void InitContext(InterfaceContext ContextHandle) = 0;

    /** Update the window */
    virtual void Tick() = 0;

    /**
     * Check if the window should be updated this frame
     * 
     * @return: Returns true if the window should be updated
     */
    virtual bool IsTickable() = 0;
};