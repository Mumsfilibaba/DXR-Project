#pragma once
#include "GenericApplicationMessageHandler.h"
#include "Core/Containers/SharedPtr.h"

class FInputDevice
{
public:
    FInputDevice()
        : MessageHandler(nullptr)
    {
    }

    virtual ~FInputDevice() = default;

    virtual void PollDeviceState() = 0;

    void SetMessageHandler(TSharedPtr<FGenericApplicationMessageHandler> InMessageHandler)
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