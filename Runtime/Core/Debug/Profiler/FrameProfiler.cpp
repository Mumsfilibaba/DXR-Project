#include "FrameProfiler.h"

#include "Core/Threading/ScopedLock.h"

CFrameProfiler CFrameProfiler::Instance;

void CFrameProfiler::Tick()
{
    Clock.Tick();

    CurrentFps++;
    if ( Clock.GetTotalTime().AsSeconds() > 1.0f )
    {
        Fps = CurrentFps;
        CurrentFps = 0;

        Clock.Reset();
    }

    if ( Enabled )
    {
        const double Delta = Clock.GetDeltaTime().AsMilliSeconds();
        CPUFrameTime.AddSample( float( Delta ) );
    }
}

void CFrameProfiler::Enable()
{
    Instance.Enabled = true;
}

void CFrameProfiler::Disable()
{
    Instance.Enabled = false;
}

void CFrameProfiler::Reset()
{
    CPUFrameTime.Reset();

    {
        TScopedLock Lock( CPUSamples );
        for ( auto& Sample : CPUSamples.Get() )
        {
            Sample.second.Reset();
        }
    }
}

void CFrameProfiler::BeginTraceScope( const char* Name )
{
    if ( Enabled )
    {
        TScopedLock Lock( CPUSamples );

        const CString ScopeName = Name;

        auto Entry = CPUSamples.Get().find( ScopeName );
        if ( Entry == CPUSamples.Get().end() )
        {
            auto NewSample = CPUSamples.Get().insert( std::make_pair( ScopeName, SProfileSample() ) );
            NewSample.first->second.Begin();
        }
        else
        {
            Entry->second.Begin();
        }
    }
}

void CFrameProfiler::EndTraceScope( const char* Name )
{
    if ( Enabled )
    {
        const CString ScopeName = Name;

        TScopedLock Lock( CPUSamples );

        auto Entry = CPUSamples.Get().find( ScopeName );
        if ( Entry != CPUSamples.Get().end() )
        {
            Entry->second.End();
        }
        else
        {
            Assert( false );
        }
    }
}

void CFrameProfiler::GetCPUSamples( ProfileSamplesTable& OutCPUSamples )
{
    TScopedLock Lock( CPUSamples );
    OutCPUSamples = CPUSamples.Get();
}