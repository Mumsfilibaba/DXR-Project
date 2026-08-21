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

class APPLICATION_API FEditableTextElement final : public FVisualElement
{
public:
    struct FInitializer
    {
        FInitializer()
            : Text()
            , HintText()
            , Font(nullptr)
            , ForegroundColor(FFloatColor::White)
            , HintColor(0.5f, 0.5f, 0.5f, 1.0f)
            , TextCursorColor(FFloatColor::White)
            , Padding(4, 2, 4, 2)
        {
        }

        String                Text;
        String                HintText;
        TSharedPtr<IFontFace> Font;
        FFloatColor           ForegroundColor;
        FFloatColor           HintColor;
        FFloatColor           TextCursorColor;
        FMargin               Padding;
    };

public:
    static TSharedPtr<FEditableTextElement> Create(const FInitializer& Initializer);

public:
    FEditableTextElement();
    virtual ~FEditableTextElement();

    /**
     * @brief Initializes the editable text with the specified parameters.
     *
     * @param Initializer Initialization parameters.
     */
    void Initialize(const FInitializer& Initializer);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnKeyChar(const FKeyEvent& KeyEvent) override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnFocusGained() override;
    virtual FEventResponse OnFocusLost() override;

    /** @brief Replaces the text, clamps the text cursor and fires OnTextChanged. */
    void SetText(const String& InText);

    /** @brief Replaces the text and puts the text cursor at the end without firing OnTextChanged. */
    void SetTextSilently(const String& InText);

    /** @brief The text currently being edited. */
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

    /** @return True when a character was removed. */
    bool DeleteBackward();

    /** @return True when a character was removed. */
    bool DeleteForward();

    /**
     * @brief Moves the text cursor, clamped into the text.
     *
     * @param InTextCursorPosition The new text cursor index.
     */
    void SetTextCursorPosition(int32 InTextCursorPosition);

    /** @brief The text cursor index, in [0, Length]. */
    NODISCARD FORCEINLINE int32 GetTextCursorPosition() const
    {
        return TextCursorPosition;
    }

    /** @brief Moves the text cursor one character left, stopping at the start. */
    void MoveTextCursorLeft();

    /** @brief Moves the text cursor one character right, stopping at the end. */
    void MoveTextCursorRight();

    /** @brief Moves the text cursor in front of the first character. */
    void MoveTextCursorToStart();

    /** @brief Moves the text cursor past the last character. */
    void MoveTextCursorToEnd();

    /** @brief Puts the whole text on the system clipboard. */
    void CopyToClipboard() const;

    /** @brief Inserts the system clipboard text at the text cursor. */
    void PasteFromClipboard();

    /**
     * @brief Sets the face the text is measured and drawn with.
     *
     * @param InFont The new face, which may be null.
     */
    void SetFont(const TSharedPtr<IFontFace>& InFont);

    /** @brief Fires whenever the text changes for any reason other than a silent set. */
    NODISCARD FORCEINLINE FOnTextChangedDelegate& GetOnTextChanged()
    {
        return OnTextChanged;
    }

    /** @brief Fires when Enter is pressed and the interceptor did not claim it. */
    NODISCARD FORCEINLINE FOnTextCommittedDelegate& GetOnTextCommitted()
    {
        return OnTextCommitted;
    }

    /** @brief Runs before the default editing behavior on every key down, so an owner can claim keys. */
    NODISCARD FORCEINLINE FOnEditableTextKeyDownDelegate& GetOnKeyDownInterceptor()
    {
        return OnKeyDownInterceptor;
    }

    /** @brief True while this element holds keyboard focus, which is what shows the text cursor. */
    NODISCARD FORCEINLINE bool HasKeyboardFocus() const
    {
        return bHasKeyboardFocus;
    }

private:
    void NotifyTextChanged();

    String                         Text;
    String                         HintText;
    TSharedPtr<IFontFace>          Font;
    FFloatColor                    ForegroundColor;
    FFloatColor                    HintColor;
    FFloatColor                    TextCursorColor;
    FMargin                        Padding;
    FOnTextChangedDelegate         OnTextChanged;
    FOnTextCommittedDelegate       OnTextCommitted;
    FOnEditableTextKeyDownDelegate OnKeyDownInterceptor;
    int32                          TextCursorPosition;
    bool                           bHasKeyboardFocus : 1;
};
