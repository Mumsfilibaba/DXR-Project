#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Color.h"
#include "Core/Misc/IOutputDevice.h"
#include "Core/Platform/CriticalSection.h"

struct FConsoleLogLine
{
    FConsoleLogLine()
        : Message()
        , Severity(ELogSeverity::Info)
    {
    }

    FConsoleLogLine(const String& InMessage, ELogSeverity InSeverity)
        : Message(InMessage)
        , Severity(InSeverity)
    {
    }

    String       Message;
    ELogSeverity Severity;
};

class APPLICATION_API FConsoleLogBuffer final : public IOutputDevice
{
public:
    /** @brief Matches the limit the ImGui console used. */
    static constexpr int32 DefaultMaxLines = 100;

    /**
     * @brief The color the console draws a line of this severity in.
     *
     * @param Severity The severity of the line.
     * @return The color for that severity.
     */
    NODISCARD static FFloatColor GetSeverityColor(ELogSeverity Severity);

public:
    explicit FConsoleLogBuffer(int32 InMaxLines = DefaultMaxLines);

    /** @brief Unregisters from the logger, so a destroyed buffer is never left registered. */
    virtual ~FConsoleLogBuffer();

    // IOutputDevice Interface
    virtual void Log(const String& Message) override final;
    virtual void Log(ELogSeverity Severity, const String& Message) override final;

    /** @brief Starts receiving everything the engine logs. */
    void RegisterWithLogger();

    /** @brief Stops receiving what the engine logs. Safe to call when never registered. */
    void UnregisterFromLogger();

    /** @brief Drops every line and bumps the revision. */
    void Clear();

    /**
     * @brief Copies the current lines out under the lock, for the view to render from.
     *
     * @param OutLines The array to fill, which is emptied first.
     */
    void GetSnapshot(TArray<FConsoleLogLine>& OutLines) const;

    /** @brief The number of lines currently held. */
    NODISCARD int32 GetNumLines() const;

    /** @brief The number of lines the buffer keeps before dropping the oldest. */
    NODISCARD FORCEINLINE int32 GetMaxLines() const
    {
        return MaxLines;
    }

    /**
     * @brief Sets the line cap, trimming immediately when it shrinks.
     *
     * @param InMaxLines The new cap, which is clamped to at least one.
     */
    void SetMaxLines(int32 InMaxLines);

    /** @brief Bumped on every add and every clear, so the view can skip a rebuild when nothing changed. */
    NODISCARD uint64 GetRevision() const;

private:
    void TrimToMaxLines();

    mutable FCriticalSection LinesCS;
    TArray<FConsoleLogLine>  Lines;
    int32                    MaxLines;
    uint64                   Revision;
    bool                     bIsRegisteredWithLogger : 1;
};
