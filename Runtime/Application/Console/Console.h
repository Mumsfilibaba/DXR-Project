#pragma once
#include "Application/Console/ConsoleCommandLine.h"
#include "Application/Console/ConsoleLogBuffer.h"
#include "Application/Text/IFontFace.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Elements/EditableText.h"
#include "Application/Elements/ScrollBox.h"

struct FConsoleCandidateColumns
{
    FConsoleCandidateColumns()
        : NameWidth(0)
        , ValueWidth(0)
        , TypeWidth(0)
        , SetByWidth(0)
    {
    }

    int32 NameWidth;
    int32 ValueWidth;
    int32 TypeWidth;
    int32 SetByWidth;
};

class APPLICATION_API FConsole final : public FCompoundElement
{
public:
    struct FDesc
    {
        TSharedPtr<IFontFace> Font = nullptr;
        int32                 TextAreaHeight = 384;
        FFloatColor           BackgroundColor = FFloatColor(0.1f, 0.1f, 0.1f, 0.85f);
        FFloatColor           InputBackgroundColor = FFloatColor(0.04f, 0.04f, 0.04f, 0.85f);
        FFloatColor           SelectedCandidateColor = FFloatColor(0.3f, 0.3f, 0.3f, 1.0f);
        FFloatColor           CandidateDetailColor = FFloatColor(0.85f, 0.85f, 0.85f, 1.0f);
        int32                 MaxLogLines = FConsoleLogBuffer::DefaultMaxLines;
        bool                  bRegisterWithLogger : 1 = true;
    };

public:
    static TSharedPtr<FConsole> Create(const FDesc& Desc);

    /**
     * @brief Checks whether the key toggles the console, the grave accent or World1 as before.
     *
     * @param Key The key to test.
     * @return True when the key toggles the console.
     */
    NODISCARD static bool IsToggleKey(FKey Key);

public:
    FConsole();
    virtual ~FConsole();

    /**
     * @brief Initializes the console with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual bool CapturesAllInput() const override;
    virtual TSharedPtr<FVisualElement> GetFocusTarget() override;

    /** @brief Opens the console when it is closed, and closes it when it is open. */
    void Toggle();

    /**
     * @brief Opens or closes the console, resetting the command line either way.
     *
     * @param bInIsOpen True to open the console.
     */
    void SetIsOpen(bool bInIsOpen);

    /** @return True while the console is open, showing and taking the input. */
    NODISCARD FORCEINLINE bool IsOpen() const
    {
        return bIsOpen;
    }

    /** @return The model behind the input line, holding the text, the candidates and the history walk. */
    NODISCARD FORCEINLINE FConsoleCommandLine& GetCommandLine()
    {
        return CommandLine;
    }

    /** @return The log the console shows, which is also the device an executed command reports to. */
    NODISCARD FORCEINLINE FConsoleLogBuffer& GetLogBuffer()
    {
        return LogBuffer;
    }

    /** @return The input field the command line is edited through, which an open console also focuses. */
    NODISCARD FORCEINLINE const TSharedPtr<FEditableText>& GetInput() const
    {
        return Input;
    }

    /**
     * @brief Gets the scroll box above the input field.
     *
     * @return The scroll box, which owns the viewport the candidate rows or the log lines are scrolled
     * inside.
     */
    NODISCARD FORCEINLINE const TSharedPtr<FScrollBox>& GetScrollBox() const
    {
        return ScrollBox;
    }

    /**
     * @brief Gets the box the rows are placed in.
     *
     * @return The box, holding the candidate rows while a candidate list is showing and the log lines
     * otherwise.
     */
    NODISCARD FORCEINLINE const TSharedPtr<FVerticalBox>& GetScrollContent() const
    {
        return ScrollContent;
    }

private:
    void RebuildScrollContent();
    void SyncInputFromCommandLine();

    EKeyInterceptResult HandleInputKeyDown(const FKeyEvent& KeyEvent);
    void HandleTextChanged(const String& NewText);

    void AddLogLine(const FConsoleLogLine& Line);
    void AddCandidateRow(const TPair<IConsoleObject*, String>& Candidate, bool bIsSelected, const FConsoleCandidateColumns& Columns);

    NODISCARD int32 GetCandidateRowHeight() const;
    NODISCARD int32 GetInputFieldHeight() const;
    NODISCARD FConsoleCandidateColumns ComputeCandidateColumns() const;

    FConsoleLogBuffer         LogBuffer;
    FConsoleCommandLine       CommandLine;
    TSharedPtr<IFontFace>     Font;
    TSharedPtr<FBorder>       Background;
    TSharedPtr<FVerticalBox>  RootBox;
    TSharedPtr<FScrollBox>    ScrollBox;
    TSharedPtr<FVerticalBox>  ScrollContent;
    TSharedPtr<FBorder>       InputBackground;
    TSharedPtr<FEditableText> Input;
    FFloatColor               SelectedCandidateColor;
    FFloatColor               CandidateDetailColor;
    uint64                    LastLogRevision;
    int32                     TextAreaHeight;
    bool                      bIsOpen : 1;
    bool                      bIsScrollContentDirty : 1;
    bool                      bIsScrollToEndPending : 1;
    bool                      bIsSyncingInput : 1;
};
