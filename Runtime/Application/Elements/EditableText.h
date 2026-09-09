#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Math/Color.h"
#include "Application/Text/IFontFace.h"
#include "Application/Elements/VisualElement.h"

DECLARE_DELEGATE(FOnTextChangedDelegate, const String& /*NewText*/);
DECLARE_DELEGATE(FOnTextCommittedDelegate, const String& /*Text*/);

enum class EKeyInterceptResult : uint8
{
    /** The interceptor did not use the key, so the default editing behavior applies. */
    NotHandled,

    /** The interceptor consumed the key and the default editing behavior is skipped. */
    Handled,
};

DECLARE_RETURN_DELEGATE(FOnEditableTextKeyDownDelegate, EKeyInterceptResult, const FKeyEvent& /*KeyEvent*/);

class APPLICATION_API FEditableText final : public FVisualElement
{
public:
    struct FDesc
    {
        String                Text;
        String                HintText;
        TSharedPtr<IFontFace> Font = nullptr;
        FFloatColor           ForegroundColor = FFloatColor::White;
        FFloatColor           HintColor = FFloatColor(0.5f, 0.5f, 0.5f, 1.0f);
        FFloatColor           TextCursorColor = FFloatColor::White;
        FFloatColor           SelectionColor = FFloatColor(0.26f, 0.59f, 0.98f, 0.35f);
        FMargin               Padding = FMargin(4, 2, 4, 2);
        float                 TextCursorBlinkPeriod = 1.2f;
        EHorizontalAlignment  TextAlignment = EHorizontalAlignment::Left;
    };

public:
    static TSharedPtr<FEditableText> Create(const FDesc& Desc);

    /**
     * @brief Whether the character separates two words. The same set the console completes words with,
     * so a caret word and a Tab-completion word are the same thing and a name like
     * VulkanRHI.EnableBindless stays one of them.
     *
     * @param Character The character to test.
     * @return True when the character separates words.
     */
    NODISCARD static bool IsWordSeparator(CHAR Character);

public:
    FEditableText();
    virtual ~FEditableText();

    /**
     * @brief Initializes the editable text with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnKeyChar(const FKeyEvent& KeyEvent) override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnFocusGained() override;
    virtual FEventResponse OnFocusLost() override;
    virtual bool GetCursor(ECursor& OutCursor) const override;
    virtual bool SupportsKeyboardFocus() const override;
    virtual bool WantsTextInput() const override;

    /** @brief Replaces the text, clamps the text cursor and fires OnTextChanged. */
    void SetText(const String& InText);

    /** @brief Replaces the text and puts the text cursor at the end without firing OnTextChanged. */
    void SetTextSilently(const String& InText);

    /** @return The text currently being edited. */
    NODISCARD FORCEINLINE const String& GetText() const
    {
        return Text;
    }

    /** @brief Empties the text and fires OnTextChanged. */
    void ClearText();

    /**
     * @brief Inserts one character at the text cursor, which ends up just after it.
     *
     * @param Character The character to insert.
     */
    void InsertCharacter(CHAR Character);

    /**
     * @brief Inserts text at the text cursor, which ends up after the inserted text.
     *
     * @param InText The text to insert.
     */
    void InsertText(const StringView& InText);

    /**
     * @brief Replaces [Position, Position + Count) with InText and leaves the text cursor after it.
     *
     * @param Position The start of the range to replace.
     * @param Count    The length of the range to replace.
     * @param InText   The replacement text.
     */
    void ReplaceRange(int32 Position, int32 Count, const StringView& InText);

    /**
     * @brief Removes the character before the text cursor and drops any selection.
     *
     * @return True when a character was removed, false when the text cursor is already at the start.
     */
    bool DeleteBackward();

    /**
     * @brief Removes the character at the text cursor and drops any selection.
     *
     * @return True when a character was removed, false when the text cursor is already at the end.
     */
    bool DeleteForward();

    /**
     * @brief Moves the text cursor, clamped into the text.
     *
     * @param InTextCursorPosition The new text cursor index.
     */
    void SetTextCursorPosition(int32 InTextCursorPosition);

    /** @return Where the text cursor sits, as an index in [0, Length]. */
    NODISCARD FORCEINLINE int32 GetTextCursorPosition() const
    {
        return TextCursorPosition;
    }

    /**
     * @brief Moves the text cursor one character left, stopping at the start.
     *
     * @param bExtendSelection Keeps the selection anchor where it is, so the selection grows or shrinks.
     * Otherwise the selection collapses onto the text cursor.
     */
    void MoveTextCursorLeft(bool bExtendSelection = false);

    /**
     * @brief Moves the text cursor one character right, stopping at the end.
     *
     * @param bExtendSelection Keeps the selection anchor where it is, so the selection grows or shrinks.
     * Otherwise the selection collapses onto the text cursor.
     */
    void MoveTextCursorRight(bool bExtendSelection = false);

    /**
     * @brief Moves the text cursor in front of the first character.
     *
     * @param bExtendSelection Keeps the selection anchor where it is, so the selection grows or shrinks.
     * Otherwise the selection collapses onto the text cursor.
     */
    void MoveTextCursorToStart(bool bExtendSelection = false);

    /**
     * @brief Moves the text cursor past the last character.
     *
     * @param bExtendSelection Keeps the selection anchor where it is, so the selection grows or shrinks.
     * Otherwise the selection collapses onto the text cursor.
     */
    void MoveTextCursorToEnd(bool bExtendSelection = false);

    /**
     * @brief Moves the text cursor to the start of the word to its left, stopping at the start.
     *
     * @param bExtendSelection Keeps the selection anchor where it is, so the selection grows or shrinks.
     * Otherwise the selection collapses onto the text cursor.
     */
    void MoveTextCursorWordLeft(bool bExtendSelection = false);

    /**
     * @brief Moves the text cursor to the end of the word to its right, stopping at the end.
     *
     * @param bExtendSelection Keeps the selection anchor where it is, so the selection grows or shrinks.
     * Otherwise the selection collapses onto the text cursor.
     */
    void MoveTextCursorWordRight(bool bExtendSelection = false);

    /**
     * @brief The start of the word to the left of a position.
     *
     * @param From The position to search back from.
     * @return The index the word starts at, or zero.
     */
    NODISCARD int32 FindWordBoundaryLeft(int32 From) const;

    /**
     * @brief The end of the word to the right of a position.
     *
     * @param From The position to search forward from.
     * @return The index one past the end of the word, or the length of the text.
     */
    NODISCARD int32 FindWordBoundaryRight(int32 From) const;

    /** @return True when a range is selected, which is when the anchor and the text cursor sit apart. */
    NODISCARD bool HasSelection() const;

    /** @return The first selected index, which equals the text cursor when nothing is selected. */
    NODISCARD int32 GetSelectionStart() const;

    /** @return One past the last selected index, where the selection ends. */
    NODISCARD int32 GetSelectionEnd() const;

    /** @return The text inside the selection, or an empty string when nothing is selected. */
    NODISCARD String GetSelectedText() const;

    /** @brief Selects the whole text and leaves the text cursor at the end. */
    void SelectAll();

    /** @brief Drops the selection, leaving the text cursor where it is. */
    void ClearSelection();

    /**
     * @brief Removes the selected range, leaving the text cursor where the selection started.
     *
     * @return True when a selected range was removed, false when nothing was selected.
     */
    bool DeleteSelection();

    /** @brief Puts the selection on the system clipboard, or the whole text when nothing is selected. */
    void CopyToClipboard() const;

    /** @brief Copies as CopyToClipboard does, then removes the selection. */
    void CutToClipboard();

    /** @brief Replaces the selection with the system clipboard text, or inserts it at the text cursor. */
    void PasteFromClipboard();

    /**
     * @brief Sets the face the text is measured and drawn with.
     *
     * @param InFont The new face, which may be null.
     */
    void SetFont(const TSharedPtr<IFontFace>& InFont);

    /**
     * @brief Sets the color of the hint shown in place of the text while the field is empty, which an owner
     * driving a focused and an unfocused look has to change as focus moves.
     *
     * @param InHintColor The new hint color.
     */
    void SetHintColor(const FFloatColor& InHintColor);

    /** @return The delegate, which fires whenever the text changes for any reason other than a silent set. */
    NODISCARD FORCEINLINE FOnTextChangedDelegate& GetOnTextChanged()
    {
        return OnTextChanged;
    }

    /** @return The delegate, which fires when Enter is pressed and the interceptor did not claim it. */
    NODISCARD FORCEINLINE FOnTextCommittedDelegate& GetOnTextCommitted()
    {
        return OnTextCommitted;
    }

    /**
     * @brief Gets the delegate an owner claims keys with.
     *
     * @return The delegate, which runs before the default editing behavior on every key down.
     */
    NODISCARD FORCEINLINE FOnEditableTextKeyDownDelegate& GetOnKeyDownInterceptor()
    {
        return OnKeyDownInterceptor;
    }

    /**
     * @brief Gets whether this element holds keyboard focus, which is what shows the text cursor.
     *
     * @return True while it is focused.
     */
    NODISCARD FORCEINLINE bool HasKeyboardFocus() const
    {
        return bHasKeyboardFocus;
    }

    /**
     * @brief Whether the text cursor is on that far into a blink. Pure, so the phase can be checked
     * without a clock. The cursor is drawn for the first two thirds of every period, which is the on
     * and off time ImGui blinks a caret with.
     *
     * @param ElapsedSeconds The time since the phase was last reset.
     * @return True when the cursor is drawn.
     */
    NODISCARD bool IsTextCursorVisibleAt(double ElapsedSeconds) const;

    /** @return How long one text cursor blink lasts, in seconds, where zero leaves the cursor solid. */
    NODISCARD FORCEINLINE float GetTextCursorBlinkPeriod() const
    {
        return TextCursorBlinkPeriod;
    }

private:
    void NotifyTextChanged();

    NODISCARD FRectangle GetTextBounds(const FRectangle& Bounds) const;
    NODISCARD int32 GetTextBandHeight() const;
    NODISCARD int32 GetTextBandTop(const FRectangle& TextBounds) const;
    NODISCARD int32 FindTextCursorPositionAt(const IntVector2& ClientPosition) const;
    NODISCARD double GetSecondsSinceTextCursorBlinkReset() const;

    void MoveTextCursor(int32 NewTextCursorPosition, bool bExtendSelection);
    void ResetTextCursorBlink();

    String                         Text;
    String                         HintText;
    TSharedPtr<IFontFace>          Font;
    FFloatColor                    ForegroundColor;
    FFloatColor                    HintColor;
    FFloatColor                    TextCursorColor;
    FFloatColor                    SelectionColor;
    FMargin                        Padding;
    EHorizontalAlignment           TextAlignment;
    FOnTextChangedDelegate         OnTextChanged;
    FOnTextCommittedDelegate       OnTextCommitted;
    FOnEditableTextKeyDownDelegate OnKeyDownInterceptor;
    float                          TextCursorBlinkPeriod;
    uint64                         TextCursorBlinkResetCounter;
    int32                          TextCursorPosition;
    int32                          SelectionAnchor;
    bool                           bHasKeyboardFocus : 1;
    bool                           bIsSelectingWithMouse : 1;
};
