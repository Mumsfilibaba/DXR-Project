#pragma once
#include "Core/Containers/String.h"

#include "CoreApplication/CoreApplicationModule.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

enum class EConsoleColor : uint8
{
    Red = 0,
    Green = 1,
    Yellow = 2,
    White = 3
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class COREAPPLICATION_API CPlatformConsoleWindow
{
public:

    /* Creates a new console */
    static CPlatformConsoleWindow* Make() { return dbg_new CPlatformConsoleWindow(); }

    /* Show or hide the console window */
    virtual void Show(bool bShow) { }

    /* Prints text to the console window, but does not start a new line */
    virtual void Print(const CString& Message) { }

    /* Prints a line to the console window */
    virtual void PrintLine(const CString& Message) { }

    /* Clear the console window */
    virtual void Clear() { }

    /* Set the title of the console window */
    virtual void SetTitle(const CString& Title) { }

    /* Set the text-color */
    virtual void SetColor(EConsoleColor Color) { }

    /* Releases the console window and destroys the object */
    virtual void Release() { delete this; }

    /* Returns true if the console window is currently being displayed */
    bool IsShowing() const { return bIsShowing; }

protected:

    CPlatformConsoleWindow() = default;
    virtual ~CPlatformConsoleWindow() = default;

    // True or false depending on if the console is visbile or not
    bool bIsShowing = false;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

