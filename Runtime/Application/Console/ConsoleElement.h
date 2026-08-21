#pragma once
#include "Application/Console/ConsoleCommandLine.h"
#include "Application/Console/ConsoleLogBuffer.h"
#include "Application/Text/IFontFace.h"
#include "Application/Elements/BorderElement.h"
#include "Application/Elements/BoxElements.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Elements/EditableTextElement.h"
#include "Application/Elements/ScrollBoxElement.h"

class APPLICATION_API FConsoleElement final : public FCompoundElement
{
public:
    struct FInitializer
    {
        FInitializer()
            : Font(nullptr)
            , TextAreaHeight(384)
            , BackgroundColor(0.06f, 0.06f, 0.06f, 0.8f)
            , InputBackgroundColor(0.1f, 0.1f, 0.1f, 0.8f)
            , SelectedCandidateColor(0.6f, 0.6f, 0.6f, 1.0f)
            , CandidateDetailColor(0.85f, 0.85f, 0.85f, 1.0f)
            , MaxLogLines(FConsoleLogBuffer::DefaultMaxLines)
            , bRegisterWithLogger(true)
        {
        }

        TSharedPtr<IFontFace> Font;
        int32                 TextAreaHeight;
        FFloatColor           BackgroundColor;
        FFloatColor           InputBackgroundColor;
        FFloatColor           SelectedCandidateColor;
        FFloatColor           CandidateDetailColor;
        int32                 MaxLogLines;
        bool                  bRegisterWithLogger;
    };

public:
    static TSharedPtr<FConsoleElement> Create(const FInitializer& Initializer);

    /**
     * @brief Checks whether the key toggles the console, the grave accent or World1 as before.
     *
     * @param Key The key to test.
     * @return True when the key toggles the console.
     */
    NODISCARD static bool IsToggleKey(FKey Key);

public:
    FConsoleElement();
    virtual ~FConsoleElement();

    /**
     * @brief Initializes the console with the specified parameters.
     *
     * @param Initializer Initialization parameters.
     */
    void Initialize(const FInitializer& Initializer);

    // FVisualElement Interface
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;

    /** @brief Opens the console when it is closed, and closes it when it is open. */
    void Toggle();

    /**
     * @brief Opens or closes the console, resetting the command line either way.
     *
     * @param bInIsOpen True to open the console.
     */
    void SetIsOpen(bool bInIsOpen);

    /** @brief True while the console is showing. */
    NODISCARD FORCEINLINE bool IsOpen() const
    {
        return bIsOpen;
    }

    /** @brief The headless model behind the input line. */
    NODISCARD FORCEINLINE FConsoleCommandLine& GetCommandLine()
    {
        return CommandLine;
    }

    /** @brief The log the console shows and commands report to. */
    NODISCARD FORCEINLINE FConsoleLogBuffer& GetLogBuffer()
    {
        return LogBuffer;
    }

    /** @brief The element that edits the command line. */
    NODISCARD FORCEINLINE const TSharedPtr<FEditableTextElement>& GetInputElement() const
    {
        return InputElement;
    }

    /** @brief The scroll box holding either the candidates or the log. */
    NODISCARD FORCEINLINE const TSharedPtr<FScrollBoxElement>& GetScrollBox() const
    {
        return ScrollBox;
    }

    /** @brief The box the candidate rows and log lines are placed in. */
    NODISCARD FORCEINLINE const TSharedPtr<FVerticalBoxElement>& GetScrollContent() const
    {
        return ScrollContent;
    }

private:
    void RebuildScrollContent();
    void SyncInputFromCommandLine();

    EKeyInterceptResult HandleInputKeyDown(const FKeyEvent& KeyEvent);
    void HandleTextChanged(const String& NewText);

    void AddLogLineElement(const FConsoleLogLine& Line);
    void AddCandidateRow(const TPair<IConsoleObject*, String>& Candidate, bool bIsSelected);

    FConsoleLogBuffer                LogBuffer;
    FConsoleCommandLine              CommandLine;
    TSharedPtr<IFontFace>            Font;
    TSharedPtr<FBorderElement>       Background;
    TSharedPtr<FVerticalBoxElement>  RootBox;
    TSharedPtr<FScrollBoxElement>    ScrollBox;
    TSharedPtr<FVerticalBoxElement>  ScrollContent;
    TSharedPtr<FBorderElement>       InputBackground;
    TSharedPtr<FEditableTextElement> InputElement;
    FFloatColor                      SelectedCandidateColor;
    FFloatColor                      CandidateDetailColor;
    uint64                           LastLogRevision;
    int32                            TextAreaHeight;
    bool                             bIsOpen : 1;
    bool                             bIsScrollContentDirty : 1;
    bool                             bIsScrollToEndPending : 1;
    bool                             bIsSyncingInput : 1;
};
