#pragma once
#include "Core/Containers/String.h"

/**
 * Generic system clipboard interface.
 */
struct FGenericPlatformSystemClipboard
{
    /**
     * @brief Returns true if the platform clipboard currently contains text.
     */
    static bool HasText()
    {
        return false;
    }

    /**
     * @brief Reads text from clipboard.
     * @param OutText Output string.
     * @return True if successful and clipboard contained text.
     */
    static bool GetText(FString& OutText)
    {
        OutText.Clear();
        return false;
    }

    /**
     * @brief Writes text to clipboard.
     * @param InText Text to write.
     * @return True if successful.
     */
    static bool SetText(const FString& InText)
    {
        (void)InText;
        return false;
    }

    /**
     * @brief Clears clipboard contents (text).
     */
    static void Clear()
    {
    }

    /**
     * @brief Convenience helper: returns clipboard text or empty string.
     */
    static FString GetTextOrEmpty()
    {
        FString Result;
        GetText(Result);
        return Result;
    }
};
