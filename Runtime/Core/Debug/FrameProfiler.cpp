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

    if ( EnableProfiler )
    {
        const double Delta = Clock.GetDeltaTime().AsMilliSeconds();
        CPUFrameTime.AddSample( float( Delta ) );

        if ( GPUProfiler )
        {
            SRHITimestamp Query;
            GPUProfiler->GetTimestampFromIndex( Query, GPUFrameTime.TimeQueryIndex );

            const double Frequency = static_cast<double>(GPUProfiler->GetFrequency());
            const double DeltaTime = static_cast<double>(Query.End - Query.Begin);

            double Duration  = (DeltaTime / Frequency) * 1000.0;
            GPUFrameTime.AddSample( (float)Duration );
        }
    }
}

void CFrameProfiler::Enable()
{
    EnableProfiler = true;
}

void CFrameProfiler::Disable()
{
    EnableProfiler = false;
}

void CFrameProfiler::Reset()
{
    CPUFrameTime.Reset();
    GPUFrameTime.Reset();

    {
        TScopedLock Lock( CPUSamples );
        for ( auto& Sample : CPUSamples.Get() )
        {
            Sample.second.Reset();
        }
    }

    {
        TScopedLock Lock( GPUSamples );
        for ( auto& Sample : GPUSamples.Get() )
        {
            Sample.second.Reset();
        }
    }
}

void CFrameProfiler::BeginTraceScope( const char* Name )
{
    if ( EnableProfiler )
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
    if ( EnableProfiler )
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

void CFrameProfiler::BeginGPUFrame( CRHICommandList& CmdList )
{
    if ( GPUProfiler && EnableProfiler )
    {
        CmdList.BeginTimeStamp( GPUProfiler.Get(), GPUFrameTime.TimeQueryIndex );
    }
}

void CFrameProfiler::BeginGPUTrace( CRHICommandList& CmdList, const char* Name )
{
    if ( GPUProfiler && EnableProfiler )
    {
        const CString ScopeName = Name;

        int32 TimeQueryIndex = -1;

        {
            TScopedLock Lock( GPUSamples );

            auto Entry = GPUSamples.Get().find( ScopeName );
            if ( Entry == GPUSamples.Get().end() )
            {
                auto NewSample = GPUSamples.Get().insert( std::make_pair( ScopeName, SGPUProfileSample() ) );
                NewSample.first->second.TimeQueryIndex = ++CurrentTimeQueryIndex;
                TimeQueryIndex = NewSample.first->second.TimeQueryIndex;
            }
            else
            {
                TimeQueryIndex = Entry->second.TimeQueryIndex;
            }
        }

        if ( TimeQueryIndex >= 0 )
        {
            CmdList.BeginTimeStamp( GPUProfiler.Get(), TimeQueryIndex );
        }
    }
}

void CFrameProfiler::EndGPUTrace( CRHICommandList& CmdList, const char* Name )
{
    if ( GPUProfiler && EnableProfiler )
    {
        const CString ScopeName = Name;

        int32 TimeQueryIndex = -1;

        TScopedLock Lock( GPUSamples );

        auto Entry = GPUSamples.Get().find( ScopeName );
        if ( Entry != GPUSamples.Get().end() )
        {
            TimeQueryIndex = Entry->second.TimeQueryIndex;
            CmdList.EndTimeStamp( GPUProfiler.Get(), TimeQueryIndex );

            if ( TimeQueryIndex >= 0 )
            {
                SRHITimestamp Query;
                GPUProfiler->GetTimestampFromIndex( Query, TimeQueryIndex );

                const double Frequency = static_cast<double>(GPUProfiler->GetFrequency());

                double Duration  = NTime::ToSeconds(static_cast<double>((Query.End - Query.Begin) / Frequency));
                Entry->second.AddSample( (float)Duration );
            }
        }
    }
}

void CFrameProfiler::SetGPUProfiler( CRHITimestampQuery* Profiler )
{
    GPUProfiler = MakeSharedRef<CRHITimestampQuery>( Profiler );
}

void CFrameProfiler::GetCPUSamples( ProfileSamplesTable& OutCPUSamples )
{
    TScopedLock Lock( CPUSamples );
    OutCPUSamples = CPUSamples.Get();
}

void CFrameProfiler::GetGPUSamples( GPUProfileSamplesTable& OutGPUSamples )
{
    TScopedLock Lock( GPUSamples );
    OutGPUSamples = GPUSamples.Get();
}

void CFrameProfiler::EndGPUFrame( CRHICommandList& CmdList )
{
    if ( GPUProfiler && EnableProfiler )
    {
        CmdList.EndTimeStamp( GPUProfiler.Get(), GPUFrameTime.TimeQueryIndex );
    }
}
