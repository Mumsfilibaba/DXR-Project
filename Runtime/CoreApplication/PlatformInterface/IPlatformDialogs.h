#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FFileDialogFilter
{
    /** @brief Shown to the user, e.g. "Scenes" */
    String Description;

    /** @brief Extensions without dots separated by ';', e.g. "png;jpg;tga". "*" matches every file. */
    String Extensions;
};

struct FFileDialogDesc
{
    String Title;

    /** @brief The directory the dialog starts in, or empty to let the platform choose */
    String DefaultDirectory;

    /** @brief The name a save dialog starts with */
    String DefaultFilename;

    /** @brief The first filter is selected when the dialog opens, and its first extension is added to a save name without one */
    TArray<FFileDialogFilter> Filters;

    /** @brief Let an open dialog return more than one file */
    bool bAllowMultiple = false;

    /** @brief The native handle of the window the dialog is modal to (IPlatformWindow::GetPlatformHandle), or nullptr */
    void* ParentWindowHandle = nullptr;
};

struct IPlatformDialogs
{
    static FORCEINLINE bool OpenFile(const FFileDialogDesc& Desc, TArray<String>& OutFilenames)
    {
        return false;
    }

    static FORCEINLINE bool SaveFile(const FFileDialogDesc& Desc, String& OutFilename)
    {
        return false;
    }

    static FORCEINLINE bool PickFolder(const FFileDialogDesc& Desc, String& OutDirectory)
    {
        return false;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
