#pragma once
#include "GenericApplicationMessageHandler.h"

#include "CoreApplication/ICursor.h"

#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericApplication

class CGenericApplication
{
protected:

    friend struct TDefaultDelete<CGenericApplication>;

    CGenericApplication(const TSharedPtr<ICursor>& InCursor)
        : Cursor(InCursor)
        , MessageListener(nullptr)
    { }

    virtual ~CGenericApplication() = default;

public:

    /**
     * @brief: Create a new PlatformApplication
     * 
     * @return: Returns the newly created PlatformApplication 
     */
    static TSharedPtr<CGenericApplication> CreateApplication() { return nullptr; }

    /**
     * @brief: Create a new PlatformWindow
     * 
     * @return: Returns the newly created PlatformWindow
     */
    virtual TSharedRef<CGenericWindow> MakeWindow() { return nullptr; }

    /**
     * @brief: Initialize the PlatformApplication
     * 
     * @return: Returns true if the initialization is successful
     */
    virtual bool Initialize() { return true; }

    /**
     * @brief: Tick and update the PlatformApplication 
     * 
     * @param Delta: Time in milliseconds since the last tick   
     */
    virtual void Tick(float Delta) { }

    /**
     * @brief: Check if the PlatformApplication support high-precision mouse-movement
     * 
     * @return: Returns true if the PlatformApplication supports high precision mouse-movement 
     */
    virtual bool SupportsHighPrecisionMouse() const { return false; }

    /**
     * @brief: Enable high-precision mouse input for the specified window
     * 
     * @param Window: PlatformWindow to enable high-precision for
     * @return: Returns true if high-precision mouse input was successfully enabled
     */
    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<CGenericWindow>& Window) { return true; }

    /**
     * @brief: Set the Active PlatformWindow
     * 
     * @param Window: PlatformWindow that should become the active window
     */
    virtual void SetActiveWindow(const TSharedRef<CGenericWindow>& Window) { }

    /**
     * @brief: Retrieve the Active PlatformWindow
     * 
     * @return: Returns the currently active PlatformWindow
     */
    virtual TSharedRef<CGenericWindow> GetActiveWindow() const { return nullptr; }

    /**
     * @brief: Sets the PlatformWindow that should receive keyboard focus
     * 
     * @param Window: PlatformWindow that should receive keyboard focus
     */
    virtual void SetCapture(const TSharedRef<CGenericWindow>& Window) { }

    /**
     * @brief: Retrieve the current PlatformWindow that has keyboard focus
     * 
     * @return: Returns the currently active PlatformWindow
     */
    virtual TSharedRef<CGenericWindow> GetCapture() const { return nullptr; }

    /**
     * @brief: Retrieve the current PlatformWindow that is under the mouse-cursor
     * 
     * @return: Returns the current PlatformWindow that is under the mouse-cursor or nullptr if no application-window is under the cursor
     */
    virtual TSharedRef<CGenericWindow> GetWindowUnderCursor() const { return nullptr; }

    /**
     * @brief: Set message-listener interface
     * 
     * @param InMessageHandler: New message listener
     */
    virtual void SetMessageListener(const TSharedPtr<CGenericApplicationMessageHandler>& InMessageHandler) { MessageListener = InMessageHandler; }

    /**
     * @brief: Retrieve the mouse-cursor interface 
     * 
     * @return: Returns the mouse-cursor interface 
     */
    FORCEINLINE TSharedPtr<ICursor> GetCursor() const
    {
        return Cursor;
    }

    /**
     * @brief: Retrieves the message handler
     * 
     * @return: Returns the current message handler
     */
    FORCEINLINE TSharedPtr<CGenericApplicationMessageHandler> GetMessageListener() const
    {
        return MessageListener;
    }

protected:
    TSharedPtr<ICursor>                            Cursor;
    TSharedPtr<CGenericApplicationMessageHandler> MessageListener;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
