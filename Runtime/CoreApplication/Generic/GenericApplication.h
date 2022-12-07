#pragma once
#include "GenericApplicationMessageHandler.h"

#include "CoreApplication/ICursor.h"

#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FMonitorDesc
{
    float DisplayScaling = 1.0f;
};

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

    virtual FMonitorDesc GetMonitorDescFromWindow(const FGenericWindowRef& Window) const { return FMonitorDesc{}; }

    virtual void SetMessageListener(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
    { 
        MessageListener = InMessageHandler; 
    }

    TSharedPtr<FGenericApplicationMessageHandler> GetMessageListener() const { return MessageListener; }

    const TSharedPtr<ICursor> Cursor;

protected:
    TSharedPtr<FGenericApplicationMessageHandler> MessageListener;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
