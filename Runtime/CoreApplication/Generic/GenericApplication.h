#pragma once
#include "GenericApplicationMessageHandler.h"

#include "CoreApplication/ICursor.h"

#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

class COREAPPLICATION_API FGenericApplication
{
public:
    FGenericApplication(const TSharedPtr<ICursor>& InCursor)
        : Cursor(InCursor)
        , MessageListener(nullptr)
    { }

    virtual ~FGenericApplication() = default;

    virtual FGenericWindowRef CreateWindow() { return nullptr; }

    virtual void Tick(float Delta) { }

    virtual bool SupportsHighPrecisionMouse() const { return false; }

    virtual bool EnableHighPrecisionMouseForWindow(const FGenericWindowRef& Window) { return true; }

    virtual void SetActiveWindow(const FGenericWindowRef& Window) { }
    
    virtual void SetCapture(const FGenericWindowRef& Window) { }

    virtual FGenericWindowRef GetWindowUnderCursor() const { return nullptr; }
    
    virtual FGenericWindowRef GetCapture() const { return nullptr; }
    
    virtual FGenericWindowRef GetActiveWindow() const { return nullptr; }

    virtual void SetMessageListener(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
    { 
        MessageListener = InMessageHandler; 
    }

    TSharedPtr<FGenericApplicationMessageHandler> GetMessageListener() const { return MessageListener; }

    TSharedPtr<ICursor> GetCursor() const { return Cursor; }

protected:
    TSharedPtr<ICursor>                           Cursor;
    TSharedPtr<FGenericApplicationMessageHandler> MessageListener;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
