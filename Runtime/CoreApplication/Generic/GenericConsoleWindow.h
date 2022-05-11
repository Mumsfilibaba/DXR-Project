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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EConsoleColor

enum class EConsoleColor : uint8
{
    Red    = 0,
    Green  = 1,
    Yellow = 2,
    White  = 3
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericConsoleWindow

class COREAPPLICATION_API CGenericConsoleWindow
{
protected:

    friend class CGenericApplicationMisc;

    CGenericConsoleWindow() = default;
    virtual ~CGenericConsoleWindow() = default;

public:

    virtual void Show(bool bShow) { }

    virtual void Print(const String& Message) { }

    virtual void PrintLine(const String& Message) { }

    virtual void Clear() { }

    virtual void SetTitle(const String& Title) { }

    virtual void SetColor(EConsoleColor Color) { }

    virtual void Release() { delete this; }

    virtual bool IsVisible() const { return bIsVisible; }

protected:
    bool bIsVisible = false;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif

