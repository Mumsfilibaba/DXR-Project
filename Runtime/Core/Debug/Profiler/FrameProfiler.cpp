#include "FrameProfiler.h"

#include "Core/Threading/ScopedLock.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FrameProfiler

CFrameProfiler& CFrameProfiler::Get()
{
    static CFrameProfiler Instance;
    return Instance;
}

void CFrameProfiler::Tick()
{
    Clock.Tick();

    CurrentFps++;
    if (Clock.GetTotalTime().AsSeconds() > 1.0f)
    {
        Fps = CurrentFps;
        CurrentFps = 0;

        Clock.Reset();
    }

    if (bEnabled)
    {
        const double Delta = Clock.GetDeltaTime().AsMilliSeconds();
        CPUFrameTime.AddSample(float(Delta));
    }
}

void CFrameProfiler::Enable()
{
	CFrameProfiler& Instance = CFrameProfiler::Get();
    Instance.bEnabled = true;
}

void CFrameProfiler::Disable()
{
	CFrameProfiler& Instance = CFrameProfiler::Get();
    Instance.bEnabled = false;
}

void CFrameProfiler::Reset()
{
    CPUFrameTime.Reset();

    {
        TScopedLock Lock(CPUSamples);
        for (auto& Sample : CPUSamples.Get())
        {
            Sample.second.Reset();
        }
    }
}

void CFrameProfiler::BeginTraceScope(const char* Name)
{
    if (bEnabled)
    {
        TScopedLock Lock(CPUSamples);

        const CString ScopeName = Name;

        auto Entry = CPUSamples.Get().find(ScopeName);
        if (Entry == CPUSamples.Get().end())
        {
            auto NewSample = CPUSamples.Get().insert(std::make_pair(ScopeName, SProfileSample()));
            NewSample.first->second.Begin();
        }
        else
        {
            Entry->second.Begin();
        }
    }
}

void CFrameProfiler::EndTraceScope(const char* Name)
{
    if (bEnabled)
    {
        const CString ScopeName = Name;

        TScopedLock Lock(CPUSamples);

        auto Entry = CPUSamples.Get().find(ScopeName);
        if (Entry != CPUSamples.Get().end())
        {
            Entry->second.End();
        }
        else
        {
            Assert(false);
        }
    }
}

void CFrameProfiler::GetCPUSamples(ProfileSamplesTable& OutCPUSamples)
{
    TScopedLock Lock(CPUSamples);
    OutCPUSamples = CPUSamples.Get();
}
