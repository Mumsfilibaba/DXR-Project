#pragma once
#include "Core/Containers/SharedPtr.h"

struct FGenericApplicationMessageHandler;

/**
 * @class FInputDevice
 * @brief Represents a base interface for any input device (keyboard, mouse, gamepad, etc.).
 *
 * An FInputDevice handles polling or event-driven updates from a physical device
 * and communicates changes (key presses, button states, axis movements, etc.)
 * to a message handler (FGenericApplicationMessageHandler) for further processing
 * by the engine.
 */
class FInputDevice
{
public:

    /**
     * @brief Default constructor that initializes the message handler pointer to null.
     */
    FInputDevice()
        : MessageHandler(nullptr)
    {
    }

    /**
     * @brief Virtual destructor for FInputDevice, allowing derived classes to clean up resources.
     */
    virtual ~FInputDevice() = default;

    /**
     * @brief Updates the device state (buttons, axes, triggers, etc.).
     * 
     * Implementations of this pure virtual function should gather the latest input data 
     * from the physical device. If the state has changed (e.g., a button was pressed or 
     * an axis moved), the device should notify the current message handler of these changes.
     */
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
     * The message handler is responsible for processing input events (e.g., key presses, button clicks).
     * 
     * @param InMessageHandler The message handler that will handle input events from this device.
     */
    void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
    {
        MessageHandler = InMessageHandler;
    }

    /**
     * @brief Retrieves the current message handler.
     * 
     * @return A shared pointer to the message handler that processes device events.
     */
    TSharedPtr<FGenericApplicationMessageHandler> GetMessageHandler() const
    {
        return MessageHandler;
    }

private:
    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;
};
