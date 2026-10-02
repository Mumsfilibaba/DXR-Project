#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/RingBuffer.h"
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
     * @return The revision the copy is of, read under the same lock.
     */
    uint64 GetSnapshot(TArray<FConsoleLogLine>& OutLines) const;

    /**
     * @brief Copies out only the lines added since a revision, for a view that already shows the ones before.
     *
     * @param SinceRevision  The revision the view last caught up to.
     * @param OutLines       The array to fill with the new lines, oldest first, which is emptied first.
     * @param bOutContinuous False when the buffer was cleared since, or dropped lines the view never saw, in which
     *                       case OutLines holds every line and the view has to start over from it.
     * @return The revision the view has now caught up to.
     */
    uint64 GetLinesSince(uint64 SinceRevision, TArray<FConsoleLogLine>& OutLines, bool& bOutContinuous) const;

    /** @return How many lines are currently held, which is never more than the cap. */
    NODISCARD int32 GetNumLines() const;

    /** @return The line cap, which is how many lines the buffer keeps before dropping the oldest. */
    NODISCARD FORCEINLINE int32 GetMaxLines() const
    {
        return Lines.GetCapacity();
    }

    /**
     * @brief Sets the line cap, trimming immediately when it shrinks.
     *
     * @param InMaxLines The new cap, which is clamped to at least one.
     */
    void SetMaxLines(int32 InMaxLines);

    /**
     * @brief Gets the counter the view watches to tell that the lines changed, so it can skip a rebuild
     * when nothing did.
     *
     * @return The revision, bumped by every line added and every clear.
     */
    NODISCARD uint64 GetRevision() const;

private:
    void CopyNewestLines(int32 NumLines, TArray<FConsoleLogLine>& OutLines) const;

    mutable FCriticalSection      LinesCS;
    TRingBuffer<FConsoleLogLine>  Lines;
    uint64                        Revision;
    uint64                        ClearRevision;
    bool                          bIsRegisteredWithLogger : 1;
};
