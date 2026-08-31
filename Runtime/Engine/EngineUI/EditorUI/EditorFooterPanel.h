#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/UniquePtr.h"
#include "Application/Elements/EditableText.h"

class FBorder;
class FConsoleCommandLine;
class FScrollBox;
class FTextBlock;
class FVerticalBox;
class FVisualElement;

class ENGINE_API FEditorFooterPanel
{
public:
    FEditorFooterPanel();
    ~FEditorFooterPanel();

    /**
     * @brief Builds the command line strip.
     *
     * @return True when the strip was built.
     */
    bool Initialize();

    /** @brief Re-reads the frame rate into the status text, and drops the candidate list once the field has lost the keyboard. */
    void Refresh();

    /** @return The element the shell places at the bottom of the window, null until Initialize has succeeded. */
    NODISCARD const TSharedPtr<FVisualElement>& GetElement() const
    {
        return Element;
    }

private:
    EKeyInterceptResult OnFieldKeyDown(const FKeyEvent& KeyEvent);

    void OnFieldTextChanged(const String& NewText);
    void SyncFieldFromCommandLine();
    void RebuildCandidateList();
    void AddCandidateRow(int32 CandidateIndex, int32 NameColumnWidth);

    NODISCARD int32 GetCandidateRowHeight() const;
    NODISCARD int32 GetCandidateNameColumnWidth() const;

    TUniquePtr<FConsoleCommandLine> CommandLine;
    TSharedPtr<FEditableText>       Field;
    TSharedPtr<FTextBlock>          StatusLabel;
    TSharedPtr<FBorder>             CandidateBackground;
    TSharedPtr<FScrollBox>          CandidateScrollBox;
    TSharedPtr<FVerticalBox>        CandidateRows;
    TSharedPtr<FVisualElement>      Element;
    bool                            bIsSyncingField;
};
