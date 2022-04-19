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
     * @brief: Creates a new console-window
     * 
     * @return: Returns a newly created console-window, returns nullptr on failure
     */
    static CPlatformConsoleWindow* CreateWindow() { return dbg_new CPlatformConsoleWindow(); }

    /**
     * @brief: Show or hide the console-window 
     * 
     * @param bShow: True if the console-window should be visible, false otherwise
     */
    virtual void Show(bool bShow) { }

    /**
     * @brief: Prints text to the console-window, but does not start a new line 
     * 
     * @param Message: Message to print to the console-window
     */
    virtual void Print(const String& Message) { }

    /**
     * @brief: Prints text to the console-window and starts a new line
     *
     * @param Message: Message to print to the console
     */
    virtual void PrintLine(const String& Message) { }

    /** @brief: Clear the console window  */
    virtual void Clear() { }

    /**
     * @brief:  Set the title of the console window
     * 
     * @param Title: New title for the console-window
     */
    virtual void SetTitle(const String& Title) { }

    /**
     * @brief: Set the text-color
     * 
     * @param Color: The new color for the text in the console-window
     */
    virtual void SetColor(EConsoleColor Color) { }

    /**
     * @brief: Releases the console-window and destroys the object 
     */
    virtual void Release() { delete this; }

    /** @return: Return true if the console-window is visible */
    bool IsVisible() const { return bIsVisible; }

protected:

    CPlatformConsoleWindow() = default;
    virtual ~CPlatformConsoleWindow() = default;

    // True or false depending on if the console is visible or not
    bool bIsVisible = false;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

