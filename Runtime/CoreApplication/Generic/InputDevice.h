#pragma once
#include "Core/Containers/SharedPtr.h"

struct FGenericApplicationMessageHandler;

/**
 * @brief Represents an input device interface for handling device states and interactions.
 */

class FInputDevice
{
public:

    /**
     * @brief Constructs an FInputDevice object.
     */
    FInputDevice()
        : MessageHandler(nullptr)
    {
    }

    /**
     * @brief Virtual destructor for FInputDevice.
     */
    virtual ~FInputDevice() = default;

    /**
     * @brief Updates the device state.
     * 
     * This method updates the states of buttons, axes, and triggers, and sends events to the current
     * message handler if the state has changed since the previous call to `UpdateDeviceState`.
     */
    virtual void UpdateDeviceState() = 0;
    
    /**
     * @brief Checks if a compatible device is currently connected.
     * 
     * @return True if a compatible device is connected; false otherwise.
     */
    virtual bool IsDeviceConnected() const = 0;

    /**
     * @brief Sets the message handler for this input device.
     * 
     * @param InMessageHandler The message handler to set.
     */
    void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
    {
        MessageHandler = InMessageHandler;
    }

    /**
     * @brief Gets the current message handler.
     * 
     * @return A shared pointer to the current message handler.
     */
    TSharedPtr<FGenericApplicationMessageHandler> GetMessageHandler() const
    {
        return MessageHandler;
    }

private:
    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler; /**< @brief The message handler used by the input device. */
};
