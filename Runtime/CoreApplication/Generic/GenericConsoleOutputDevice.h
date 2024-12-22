#pragma once
#include "Core/Misc/IOutputDevice.h"
#include "Core/Containers/String.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

/**
 * @enum EConsoleColor
 * @brief Defines a set of basic colors that can be used to style console output.
 */
enum class EConsoleColor : uint8
{
    /** @brief Red color, often used for errors or critical warnings. */
    Red = 0,

    /** @brief Green color, often used for success messages or other positive indicators. */
    Green = 1,

    /** @brief Yellow color, commonly used for warnings. */
    Yellow = 2,

    /** @brief White color, generally used for default or neutral text. */
    White = 3
};

/**
 * @struct FGenericConsoleOutputDevice
 * @brief An abstract base class for console output across different platforms.
 *
 * This interface extends IOutputDevice to provide additional console-specific
 * functionality such as showing/hiding the console, setting a title, and changing
 * text color. Each platform should implement a derived class (e.g., FWindowsConsoleOutputDevice)
 * that manages its own console window or equivalent environment.
 */

struct COREAPPLICATION_API FGenericConsoleOutputDevice : public IOutputDevice
{
public:
    /**
     * @brief Factory method that creates a default console output device for the current platform.
     *
     * This method should be overridden by platform-specific implementations to return
     * an appropriate concrete FGenericConsoleOutputDevice. By default, it returns null,
     * indicating no available console on this platform.
     *
     * @return A pointer to the created console output device, or nullptr if not supported.
     */
    static FGenericConsoleOutputDevice* Create()
    {
        return nullptr;
    }

public:

    /**
     * @brief Virtual destructor ensures proper cleanup in derived classes.
     */
    virtual ~FGenericConsoleOutputDevice() = default;

    /**
     * @brief Shows or hides the console window.
     *
     * @param bShow True to show the console, false to hide it.
     */
    virtual void Show(bool bShow) = 0;

    /**
     * @brief Checks if the console window is currently visible.
     *
     * @return True if the console is visible, false otherwise.
     */
    virtual bool IsVisible() const = 0;

    /**
     * @brief Sets the title of the console window, if supported by the platform.
     *
     * @param Title The new title for the console window.
     */
    virtual void SetTitle(const FString& Title) = 0;

    /**
     * @brief Sets the color for subsequent console output.
     *
     * @param Color An enumerator value representing the desired text color.
     */
    virtual void SetTextColor(EConsoleColor Color) = 0;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
