#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/UniquePtr.h"
#include "Application/Elements/EditableText.h"

class FBorder;
class FConsoleCommandLine;
class FEditorFooterCandidateList;
class FHorizontalBox;
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
    struct FCandidateRow
    {
        TSharedPtr<FBorder>            Frame;
        TArray<TSharedPtr<FTextBlock>> NameRuns;
    };

    EKeyInterceptResult OnFieldKeyDown(const FKeyEvent& KeyEvent);
    void OnFieldTextChanged(const String& NewText);
    void SyncFieldFromCommandLine();
    void RebuildCandidateList();
    void AddCandidateRow(int32 CandidateIndex);
    void AddCandidateNameRuns(const TSharedPtr<FHorizontalBox>& Row, const String& Name, const FFloatColor& TextColor, TArray<TSharedPtr<FTextBlock>>& OutNameRuns);
    void ApplyCandidateSelection();
    void OnCandidateRowHovered(int32 RowIndex);
    void OnCandidateRowClicked(int32 RowIndex);
    void ClearHoveredCandidate();

    NODISCARD TSharedPtr<FVisualElement> MakeCandidateToolTip(int32 CandidateIndex) const;
    NODISCARD int32 GetCandidateRowHeight() const;
    NODISCARD String GetCandidateFilterText() const;

    TUniquePtr<FConsoleCommandLine>        CommandLine;
    TSharedPtr<FEditableText>              Field;
    TSharedPtr<FBorder>                    FieldFrame;
    TSharedPtr<FTextBlock>                 StatusLabel;
    TSharedPtr<FBorder>                    CandidateBackground;
    TSharedPtr<FEditorFooterCandidateList> CandidateList;
    TSharedPtr<FScrollBox>                 CandidateScrollBox;
    TSharedPtr<FVerticalBox>               CandidateRows;
    TArray<FCandidateRow>                  CandidateRowVisuals;
    TSharedPtr<FVisualElement>             HoveredCandidateRow;
    TSharedPtr<FVisualElement>             Element;
    bool                                   bIsSyncingField;
};
