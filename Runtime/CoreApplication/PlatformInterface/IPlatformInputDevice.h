#pragma once
#include "Core/Containers/SharedPtr.h"

struct IPlatformApplicationMessageHandler;

struct IPlatformInputDevice
{
    virtual ~IPlatformInputDevice() = default;

    /** @brief Updates the device state (buttons, axes, triggers, etc.). */
    virtual void UpdateDeviceState() = 0;
    
    /**
     * @brief Checks if a compatible device is currently connected or available.
     * 
     * @return True if the device is present and ready to provide input data, otherwise false.
     */
    virtual bool IsDeviceConnected() const = 0;

    /**
     * @brief Assigns a new message handler for this input device.
     * 
     * @param InMessageHandler The message handler that will handle input events from this device.
     */
    virtual void SetMessageHandler(const TSharedPtr<IPlatformApplicationMessageHandler>& InMessageHandler) = 0;

    /**
     * @brief Retrieves the current message handler.
     * 
     * @return A shared pointer to the message handler that processes device events.
     */
    virtual TSharedPtr<IPlatformApplicationMessageHandler> GetMessageHandler() const = 0;
};
