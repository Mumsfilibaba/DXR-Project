#pragma once
#include "Core/Containers/String.h"

#include "CoreApplication/CoreApplication.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

#ifdef CreateWindow
    #undef CreateWindow
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EConsoleColor - Color of the console text

enum class EConsoleColor : uint8
{
    Red    = 0,
    Green  = 1,
    Yellow = 2,
    White  = 3
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CPlatformConsoleWindow - Platform interface for ConsoleWindow

class COREAPPLICATION_API CPlatformConsoleWindow
{
public:

    /**
     * Creates a new console-window
     * 
     * @return: Returns a newly created console-window, returns nullptr on failure
     */
    static CPlatformConsoleWindow* CreateWindow() { return dbg_new CPlatformConsoleWindow(); }

    /**
     * Show or hide the console-window 
     * 
     * @param bShow: True if the console-window should be visible, false otherwise
     */
    virtual void Show(bool bShow) { }

    /**
     * Prints text to the console-window, but does not start a new line 
     * 
     * @param Message: Message to print to the console-window
     */
    virtual void Print(const String& Message) { }

    /**
     * Prints text to the console-window and starts a new line
     *
     * @param Message: Message to print to the console
     */
    virtual void PrintLine(const String& Message) { }

    /**
     * Clear the console window 
     */
    virtual void Clear() { }

    /**
     *  Set the title of the console window
     * 
     * @param Title: New title for the console-window
     */
    virtual void SetTitle(const String& Title) { }

    /**
     * Set the text-color
     * 
     * @param Color: The new color for the text in the console-window
     */
    virtual void SetColor(EConsoleColor Color) { }

    /**
     * Releases the console-window and destroys the object 
     */
    virtual void Release() { delete this; }

    /**
     * Returns true if the console-window is currently being displayed 
     * 
     * @return: Return true if the console-window is visible
     */
    bool IsShowing() const { return bIsShowing; }

protected:

    CPlatformConsoleWindow() = default;
    virtual ~CPlatformConsoleWindow() = default;

    // True or false depending on if the console is visible or not
    bool bIsShowing = false;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

