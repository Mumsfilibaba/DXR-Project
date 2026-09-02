#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"

class FVisualElement;

/** @brief Called once the dialog has closed, with true when the affirmative button was the one pressed. */
DECLARE_DELEGATE(FOnConfirmDialogClosed, bool /*bConfirmed*/);

struct ENGINE_API FEditorConfirmDialog
{
    /** @brief The least wide the dialog is laid out, matching what the ImGui editor reserves. */
    static constexpr int32 MinWidth = 360;

    struct FDesc
    {
        /** @brief The caption above the message, left out when it is empty. */
        String Title = "Confirm";

        /** @brief The question the buttons answer. */
        String Message = "Are you sure?";

        /** @brief The label of the button that answers yes. */
        String ConfirmLabel = "Yes";

        /** @brief The label of the button that answers no, which Escape and closing the dialog also pick. */
        String CancelLabel = "No";

        /** @brief Fired once the dialog has closed, with which button answered it. */
        FOnConfirmDialogClosed OnClosed;
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
