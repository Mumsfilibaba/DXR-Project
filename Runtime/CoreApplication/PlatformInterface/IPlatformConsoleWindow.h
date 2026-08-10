#pragma once
#include "Core/Misc/IOutputDevice.h"
#include "Core/Containers/String.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

enum class EConsoleTextColor : uint8
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

struct COREAPPLICATION_API IPlatformConsoleWindow : public IOutputDevice
{
    static IPlatformConsoleWindow* Create()
    {
        return nullptr;
    }

    virtual ~IPlatformConsoleWindow() = default;

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
    virtual void SetTitle(const String& Title) = 0;

    /**
     * @brief Sets the color for subsequent console output.
     *
     * @param Color An enumerator value representing the desired text color.
     */
    virtual void SetTextColor(EConsoleTextColor Color) = 0;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
