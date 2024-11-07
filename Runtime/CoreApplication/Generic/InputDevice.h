#pragma once
#include "Core/Containers/SharedPtr.h"

struct FGenericApplicationMessageHandler;

class FInputDevice
{
public:
    FInputDevice()
        : MessageHandler(nullptr)
    {
    }

    virtual ~FInputDevice() = default;

    // Updates the device-state. This will update the button-, axis-, and trigger-states and send events to
    // the current message-handler if the state has changed between this and the previous call to UpdateDeviceState.
    virtual void UpdateDeviceState() = 0;
    
    // Checks if there currently are a device-connected and that is compatible with this FInputDevice-interface.
    virtual bool IsDeviceConnected() const = 0;

    void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
    {
        MessageHandler = InMessageHandler;
    }

    TSharedPtr<FGenericApplicationMessageHandler> GetMessageHandler() const
    {
        return MessageHandler;
    }

private:
    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;
};
