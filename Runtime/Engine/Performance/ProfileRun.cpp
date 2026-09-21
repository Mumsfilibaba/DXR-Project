#include "Engine/Performance/ProfileRun.h"
#include "Core/CoreGlobals.h"
#include "Core/Filesystem/File.h"
#include "Core/Math/Math.h"
#include "Core/Misc/BootProfiler.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/Paths.h"
#include "Core/Misc/ProfilerReport.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Templates/CString.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Time/Time.h"
#include "RendererCore/Interfaces/IGPUProfiler.h"
#include "RendererCore/Interfaces/IRendererModule.h"

static TAutoConsoleVariable<float> CVarProfileDuration(
    "Profiler.Duration",
    "Seconds of frame capture after Init when -ProfileRun is set. Overridden by -ProfileDuration",
    60.0f,
    EConsoleVariableFlags::Default);

static bool   GProfileRun            = false;
static bool   GCaptureStarted        = false;
static bool   GReportWritten         = false;
static bool   GCaptureNativeStacks   = false;
static uint64 GCaptureStartTimeStamp = 0;

static String BuildFullProfileReport()
{
    String Text = FProfilerReport::BuildCpuAndBootText();
    Text.Append("\n== GPU ==\n");

    IRendererModule* RendererModule = IRendererModule::Get();
    if (!RendererModule)
    {
        Text.Append("GPU profiler unavailable\n");
        return Text;
    }

    IGPUProfiler& Gpu = RendererModule->GetGPUProfiler();

    float  MinMs = TNumericLimits<float>::Max();
    float  MaxMs = TNumericLimits<float>::Lowest();
    double Sum   = 0.0;
    int32  Count = 0;

    FProfilerGpuFrame Latest;
    FProfilerGpuFrame Stored;

    for (int32 Index = 0; Index < Gpu.GetStoredFrameCount(); ++Index)
    {
        if (!Gpu.GetStoredFrame(Index, Stored))
        {
            continue;
        }

        MinMs = Math::Min(MinMs, Stored.GpuMilliseconds);
        MaxMs = Math::Max(MaxMs, Stored.GpuMilliseconds);
        Sum  += static_cast<double>(Stored.GpuMilliseconds);
        ++Count;
        Latest = Stored;
    }

    Text.Append(String::Printf("GPU frame avg=%.3f ms  min=%.3f  max=%.3f\n",
        Count > 0 ? static_cast<float>(Sum / static_cast<double>(Count)) : 0.0f,
        MinMs == TNumericLimits<float>::Max() ? 0.0f : MinMs,
        MaxMs == TNumericLimits<float>::Lowest() ? 0.0f : MaxMs));

    if (!Latest.Intervals.IsEmpty())
    {
        Text.Append("GPU pass tree (latest collected frame):\n");
        for (const FGPUProfilerInterval& Interval : Latest.Intervals)
        {
            for (int32 Depth = 0; Depth < Interval.Depth; ++Depth)
            {
                Text.Append("  ");
            }

            const CHAR* Name = Interval.Name ? Interval.Name : "<unnamed>";
            Text.Append(String::Printf("%s  inclusive=%.3fms  exclusive=%.3fms\n", Name,
                Time::ToMilliseconds(static_cast<double>(Interval.InclusiveNanoseconds)),
                Time::ToMilliseconds(static_cast<double>(Interval.ExclusiveNanoseconds))));
        }
    }

    return Text;
}

static String BuildChromeGpuEvents(IGPUProfiler& Gpu)
{
    String Events("{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":1,\"tid\":0,\"args\":{\"name\":\"GPU\"}}");

    FProfilerGpuFrame GpuFrame;
    for (int32 Index = 0; Index < Gpu.GetStoredFrameCount(); ++Index)
    {
        if (!Gpu.GetStoredFrame(Index, GpuFrame))
        {
            continue;
        }

        double FrameStartUs = 0.0;
        if (!FProfilerReport::TryGetChromeFrameTimestampUs(GpuFrame.CpuFrameIndex, 0, FrameStartUs))
        {
            continue;
        }

        for (const FGPUProfilerInterval& Interval : GpuFrame.Intervals)
        {
            const double TimestampUs =
                FrameStartUs + Time::ToMicroseconds(static_cast<double>(Interval.StartNanoseconds));
            const CHAR* Name = Interval.Name ? Interval.Name : "<unnamed>";

            if (Interval.bInstant || Interval.InclusiveNanoseconds == 0)
            {
                FProfilerReport::AppendChromeInstantEvent(Events, Name, "gpu", TimestampUs, 1, 0);
            }
            else
            {
                const double DurationUs =
                    Time::ToMicroseconds(static_cast<double>(Interval.InclusiveNanoseconds));
                FProfilerReport::AppendChromeCompleteEvent(Events, Name, "gpu", TimestampUs, DurationUs, 1, 0);
            }
        }
    }

    return Events;
}

static void WriteReportsAndExit()
{
    if (GReportWritten)
    {
        return;
    }

    GReportWritten = true;

    const uint64 Stamp = FPlatformTime::QueryPerformanceCounter();
    const String BasePath =
        File::CombinePath(Paths::GetProjectDir(), String::Printf("Profiling/ProfileRun_%llu", Stamp));

    FProfilerReport::WriteFile(BasePath + ".txt", BuildFullProfileReport());

    String GpuEvents;
    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        GpuEvents = BuildChromeGpuEvents(RendererModule->GetGPUProfiler());
    }

    FProfilerReport::WriteFile(
        BasePath + ".json",
        FProfilerReport::MergeChromeJson(FProfilerReport::BuildChromeCpuJson(), GpuEvents));

    RequestEngineExit("ProfileRun complete");
}

void FProfileRun::ConfigureFromCommandLine()
{
    GProfileRun          = CommandLine::FindOption("ProfileRun");
    GCaptureNativeStacks = CommandLine::FindOption("ProfileNativeStacks");

    StringView ProfileDurationText;
    if (CommandLine::FindOption("ProfileDuration", ProfileDurationText) && !ProfileDurationText.IsEmpty())
    {
        const String DurationString(ProfileDurationText);
        const float ParsedDuration = CString::Atof(*DurationString);
        if (ParsedDuration > 0.0f)
        {
            CVarProfileDuration.SetVariable(ParsedDuration, EConsoleVariableFlags::SetByCommandLine);
        }
    }
}

void FProfileRun::PrepareCapture()
{
    if (!GProfileRun)
    {
        return;
    }

    FBootProfiler::Get().SetCaptureNativeStacks(GCaptureNativeStacks);
    FFrameProfiler::Get().SetCaptureNativeStacks(GCaptureNativeStacks);
    FFrameProfiler::Get().SetRetainAllFrames(true);
}

void FProfileRun::BeginCapture()
{
    if (!GProfileRun)
    {
        return;
    }

    FFrameProfiler::Get().SetRetainAllFrames(true);
    FFrameProfiler::Get().SetCaptureNativeStacks(GCaptureNativeStacks);

    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        IGPUProfiler& Gpu = RendererModule->GetGPUProfiler();
        Gpu.Enable();
        Gpu.SetRetainAllFrames(true);
    }

    GCaptureStartTimeStamp = FPlatformTime::QueryPerformanceCounter();
    GCaptureStarted        = true;
}

void FProfileRun::Tick()
{
    if (!GProfileRun || !GCaptureStarted || GReportWritten)
    {
        return;
    }

    const uint64 Now       = FPlatformTime::QueryPerformanceCounter();
    const uint64 Frequency = FPlatformTime::QueryPerformanceFrequency();
    const double Elapsed   = Frequency > 0
        ? static_cast<double>(Now - GCaptureStartTimeStamp) / static_cast<double>(Frequency)
        : 0.0;

    if (Elapsed >= static_cast<double>(CVarProfileDuration.GetValue()))
    {
        WriteReportsAndExit();
    }
}
