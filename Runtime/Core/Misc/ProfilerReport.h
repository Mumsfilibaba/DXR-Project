#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

struct CORE_API FProfilerOptimizationTarget
{
    String Name;
    double SelfMillisecondsPerFrame = 0.0;
    double InclusiveMillisecondsPerFrame = 0.0;
    double BudgetPercent = 0.0;
    double CallsPerFrame = 0.0;
};

struct CORE_API FProfilerWorstFrame
{
    int32 FrameIndex = 0;
    float CpuMilliseconds = 0.0f;
};

struct CORE_API FProfilerScopeAggregate
{
    String Name;
    int32  Calls = 0;
    uint64 InclusiveNanoseconds = 0;
    uint64 ExclusiveNanoseconds = 0;
};

struct CORE_API FProfilerThreadAggregate
{
    void* ThreadHandle = nullptr;
    TArray<FProfilerScopeAggregate> Scopes;
};

struct CORE_API FProfilerReport
{
    /**
     * @brief Builds the CPU and boot sections of a profile-run dump.
     *
     * @return The report text, ready to write or to have a GPU section appended.
     */
    static String BuildCpuAndBootText();

    /**
     * @brief Ranks named CPU scopes by aggregate self time across stored frames.
     *
     * @param MaxTargets How many rows to keep.
     * @return Targets ordered by exclusive time, highest first.
     */
    static TArray<FProfilerOptimizationTarget> CollectOptimizationTargets(int32 MaxTargets = 20);

    /**
     * @brief Picks the slowest stored CPU frames.
     *
     * @param MaxFrames How many frames to keep.
     * @return Frames ordered by CpuMilliseconds, slowest first.
     */
    static TArray<FProfilerWorstFrame> CollectWorstFrames(int32 MaxFrames = 8);

    /**
     * @brief Sums named scopes per thread across the stored CPU ring.
     *
     * @param OutThreads One entry per thread handle seen.
     */
    static void CollectThreadAggregates(TArray<FProfilerThreadAggregate>& OutThreads);

    /**
     * @brief Builds a Chrome Trace JSON array of CPU and boot intervals.
     *
     * @return A document chrome://tracing and Perfetto can load.
     */
    static String BuildChromeCpuJson();

    /** @return The QPC stamp Chrome events are rebased from, or 0 when nothing was captured. */
    static uint64 GetChromeBaseTimeStamp();

    /**
     * @brief Places a frame-relative GPU timestamp on the same process timeline as the CPU trace.
     *
     * @param CpuFrameIndex           Correlated CPU frame index.
     * @param FrameRelativeNanoseconds Nanoseconds from the start of that GPU frame.
     * @param OutTimestampUs          Chrome trace timestamp in microseconds.
     * @return True when the correlated CPU frame is still stored.
     */
    static bool TryGetChromeFrameTimestampUs(
        int32 CpuFrameIndex, uint64 FrameRelativeNanoseconds, double& OutTimestampUs);

    /**
     * @brief Escapes a name for a JSON string value.
     *
     * @param Text The raw scope name.
     * @return Text with quotes and backslashes escaped.
     */
    static String EscapeChromeString(const CHAR* Text);

    /**
     * @brief Appends a complete (ph=X) Chrome trace event.
     */
    static void AppendChromeCompleteEvent(String& Out, const CHAR* Name, const CHAR* Category,
        double TimestampUs, double DurationUs, int32 Pid, int64 Tid);

    /**
     * @brief Appends an instant (ph=i) Chrome trace event.
     */
    static void AppendChromeInstantEvent(String& Out, const CHAR* Name, const CHAR* Category,
        double TimestampUs, int32 Pid, int64 Tid);

    /**
     * @brief Inserts extra events into a Chrome JSON array document before the closing bracket.
     *
     * @param CpuJson      A document produced by BuildChromeCpuJson.
     * @param ExtraEvents  Comma-separated event objects, or empty.
     * @return The merged document.
     */
    static String MergeChromeJson(const String& CpuJson, const String& ExtraEvents);

    /**
     * @brief Writes Text to Path, creating parent directories as needed.
     *
     * @param Path Destination file.
     * @param Text The full report.
     * @return True when the file was written.
     */
    static bool WriteFile(const String& Path, const String& Text);
};
