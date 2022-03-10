#pragma once
#include "PlatformApplicationMessageHandler.h"

#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

#include "CoreApplication/ICursor.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for application

class CPlatformApplication
{
public:

    /**
     * Create a new PlatformApplication
     * 
     * @return: Returns the newly created PlatformApplication 
     */
    static TSharedPtr<CPlatformApplication> CreateApplication() { return TSharedPtr<CPlatformApplication>(); }

    /**
     * Create a new PlatformWindow
     * 
     * @return: Returns the newly created PlatformWindow
     */
    virtual TSharedRef<CPlatformWindow> MakeWindow() { return TSharedRef<CPlatformWindow>(); }

    /**
     * Initialize the PlatformApplication
     * 
     * @return: Returns true if the initialization is successful
     */
    virtual bool Initialize() { return true; }

    /**
     * Tick and update the PlatformApplication 
     * 
     * @param Delta: Time in milliseconds since the last tick   
     */
    virtual void Tick(float Delta) { }

    /**
     * Check if the PlatformApplication support high-precision mouse-movement
     * 
     * @return: Returns true if the PlatformApplication supports high precision mouse-movement 
     */
    virtual bool SupportsHighPrecisionMouse() const { return false; }

    /**
     * Enable high-precision mouse input for the specified window
     * 
     * @param Window: PlatformWindow to enable high-precision for
     * @return: Returns true if high-precision mouse input was successfully enabled
     */
    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<CPlatformWindow>& Window) { return true; }

    /**
     * Set the Active PlatformWindow
     * 
     * @param Window: PlatformWindow that should become the active window
     */
    virtual void SetActiveWindow(const TSharedRef<CPlatformWindow>& Window) { }

    /**
     * Retrieve the Active PlatformWindow
     * 
     * @return: Returns the currently active PlatformWindow
     */
    virtual TSharedRef<CPlatformWindow> GetActiveWindow() const { return TSharedRef<CPlatformWindow>(); }

    /**
     * Sets the PlatformWindow that should recive keyboard focus
     * 
     * @param Window: PlatformWindow that should recive keyboard focus
     */
    virtual void SetCapture(const TSharedRef<CPlatformWindow>& Window) { }

    /**
     * Retrieve the current PlatformWindow that has keyboard focus
     * 
     * @return: Returns the currently active PlatformWindow
     */
    virtual TSharedRef<CPlatformWindow> GetCapture() const { return TSharedRef<CPlatformWindow>(); }

    /**
     * Retrieve the current PlatformWindow that is under the mouse-cursor
     * 
     * @return: Returns the current PlatformWindow that is under the mouse-cursor or nullptr if no application-window is under the cursor
     */
    virtual TSharedRef<CPlatformWindow> GetWindowUnderCursor() const { return TSharedRef<CPlatformWindow>(); }

    /**
     * Set message-listener interface
     * 
     * @param InMessageHandler: New message listener
     */
    virtual void SetMessageListener(const TSharedPtr<CPlatformApplicationMessageHandler>& InMessageHandler) { MessageListener = InMessageHandler; }

    /**
     * Retrieve the mouse-cursor interface 
     * 
     * @return: Returns the mouse-cursor interface 
     */
    FORCEINLINE TSharedPtr<ICursor> GetCursor() const
    {
        return Cursor;
    }

    /**
     * Retrieves the message handler
     * 
     * @return: Returns the current message handler
     */
    FORCEINLINE TSharedPtr<CPlatformApplicationMessageHandler> GetMessageListener() const
    {
        return MessageListener;
    }

protected:

    friend struct TDefaultDelete<CPlatformApplication>;
    
    CPlatformApplication(const TSharedPtr<ICursor>& InCursor)
        : Cursor(InCursor)
        , MessageListener(nullptr)
    {
    }
    
    virtual ~CPlatformApplication() = default;

    TSharedPtr<ICursor> Cursor;
    TSharedPtr<CPlatformApplicationMessageHandler> MessageListener;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
