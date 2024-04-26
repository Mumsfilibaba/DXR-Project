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

    virtual void UpdateDeviceState() = 0;
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