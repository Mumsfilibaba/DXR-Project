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

class COREAPPLICATION_API CGenericApplication
{
    friend class CGenericApplicationMisc;
    
    friend struct TDefaultDelete<CGenericApplication>;

protected:

    CGenericApplication(const TSharedPtr<ICursor>& InCursor)
        : Cursor(InCursor)
        , MessageListener(nullptr)
    { }

    virtual ~CGenericApplication() = default;

public:

    virtual TSharedRef<CGenericWindow> MakeWindow() { return nullptr; }

    virtual bool Initialize() { return true; }

    virtual void Tick(float Delta) { }

    virtual bool SupportsHighPrecisionMouse() const { return false; }

    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<CGenericWindow>& Window) { return true; }

    virtual void SetActiveWindow(const TSharedRef<CGenericWindow>& Window) { }

    virtual TSharedRef<CGenericWindow> GetActiveWindow() const { return nullptr; }

    virtual void SetCapture(const TSharedRef<CGenericWindow>& Window) { }

    virtual TSharedRef<CGenericWindow> GetCapture() const { return nullptr; }

    virtual TSharedRef<CGenericWindow> GetWindowUnderCursor() const { return nullptr; }

    virtual void SetMessageListener(const TSharedPtr<CGenericApplicationMessageHandler>& InMessageHandler) { MessageListener = InMessageHandler; }

    TSharedPtr<CGenericApplicationMessageHandler> GetMessageListener() const { return MessageListener; }

    TSharedPtr<ICursor> GetCursor() const { return Cursor; }

protected:
    TSharedPtr<ICursor>                           Cursor;
    TSharedPtr<CGenericApplicationMessageHandler> MessageListener;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
