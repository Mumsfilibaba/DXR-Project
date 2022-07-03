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
// FGenericApplication

class COREAPPLICATION_API FGenericApplication
{
    friend class FGenericApplicationMisc;
    
    friend struct TDefaultDelete<FGenericApplication>;

protected:

    FGenericApplication(const TSharedPtr<ICursor>& InCursor)
        : Cursor(InCursor)
        , MessageListener(nullptr)
    { }

    virtual ~FGenericApplication() = default;

public:

    virtual TSharedRef<FGenericWindow> CreateWindow() { return nullptr; }

    virtual void Tick(float Delta) { }

    virtual bool SupportsHighPrecisionMouse() const { return false; }

    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) { return true; }

    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) { }

    virtual TSharedRef<FGenericWindow> GetActiveWindow() const { return nullptr; }

    virtual void SetCapture(const TSharedRef<FGenericWindow>& Window) { }

    virtual TSharedRef<FGenericWindow> GetCapture() const { return nullptr; }

    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const { return nullptr; }

    virtual void SetMessageListener(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler) { MessageListener = InMessageHandler; }

    TSharedPtr<FGenericApplicationMessageHandler> GetMessageListener() const { return MessageListener; }

    TSharedPtr<ICursor> GetCursor() const { return Cursor; }

protected:
    TSharedPtr<ICursor>                           Cursor;
    TSharedPtr<FGenericApplicationMessageHandler> MessageListener;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
