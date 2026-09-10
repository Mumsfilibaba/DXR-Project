#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"

class FVisualElement;

/** @brief Called once the dialog has been dismissed. */
DECLARE_DELEGATE(FOnErrorDialogClosed);

struct ENGINE_API FEditorErrorDialog
{
    /** @brief The least wide the dialog is laid out, matching what the ImGui editor reserves. */
    static constexpr int32 MinWidth = 420;

    /** @brief How many detail rows the list shows before the rest of them have to be scrolled to. */
    static constexpr int32 MaxVisibleRows = 8;

    struct FDesc
    {
        /** @brief The caption above the message, left out when it is empty. */
        String Title = "Error";

        /** @brief The line above the list, naming what failed. */
        String Message;

        /** @brief One row per thing that failed, scrolled once there are more than MaxVisibleRows. */
        TArray<String> Details;

        /** @brief The label of the button that dismisses the dialog. */
        String DismissLabel = "OK";

        /** @brief Fired once the dialog has been dismissed. */
        FOnErrorDialogClosed OnClosed;
    };

    /**
     * @brief Opens the dialog centered on the window the anchor belongs to.
     *
     * @param AnchorElement An element of the window to center on, which the dialog is placed over.
     * @param Desc          The text and the callback the dialog carries.
     * @return True when the dialog opened, false when there was no window to put it over.
     */
    static bool Open(const TSharedPtr<FVisualElement>& AnchorElement, const FDesc& Desc);
};
