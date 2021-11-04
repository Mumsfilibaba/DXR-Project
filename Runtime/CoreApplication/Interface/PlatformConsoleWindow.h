#pragma once
#include "Core/CoreAPI.h"
#include "Core/Containers/String.h"

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

class CPlatformConsoleWindow
{
public:

    static CPlatformConsoleWindow* Make()
    {
        return dbg_new CPlatformConsoleWindow();
    }

    virtual void Print( const CString& Message ) {}
    virtual void PrintLine( const CString& Message ) {}

    virtual void Clear() {}

    virtual void SetTitle( const CString& Title ) {}
    virtual void SetColor( EConsoleColor Color ) {}

    virtual void Release()
    {
        delete this;
    }

protected:

    CPlatformConsoleWindow() = default;
    virtual ~CPlatformConsoleWindow() = default;
};

// TODO: Move out from COREAPPLICATION_API
extern CORE_API CPlatformConsoleWindow* GConsoleWindow;

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

