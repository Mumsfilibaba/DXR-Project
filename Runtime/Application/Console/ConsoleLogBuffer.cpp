#include "Application/Console/ConsoleLogBuffer.h"
#include "Core/Math/Math.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Threading/ScopedLock.h"

FFloatColor FConsoleLogBuffer::GetSeverityColor(ELogSeverity Severity)
{
    switch (Severity)
    {
        case ELogSeverity::Info:
        {
            return FFloatColor(1.0f, 1.0f, 1.0f, 1.0f);
        }
        case ELogSeverity::Warning:
        {
            return FFloatColor(1.0f, 1.0f, 0.0f, 1.0f);
        }
        case ELogSeverity::Error:
        {
            return FFloatColor(1.0f, 0.0f, 0.0f, 1.0f);
        }
        default:
        {
            return FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }
}

FConsoleLogBuffer::FConsoleLogBuffer(int32 InMaxLines)
    : IOutputDevice()
    , LinesCS()
    , Lines(Math::Max(1, InMaxLines))
    , Revision(0)
    , ClearRevision(0)
    , bIsRegisteredWithLogger(false)
{
}

FConsoleLogBuffer::~FConsoleLogBuffer()
{
    UnregisterFromLogger();
}

void FConsoleLogBuffer::Log(const String& Message)
{
    Log(ELogSeverity::Info, Message);
}

void FConsoleLogBuffer::Log(ELogSeverity Severity, const String& Message)
{
    TScopedLock Lock(LinesCS);

    Lines.Emplace(Message, Severity);
    Revision++;
}

void FConsoleLogBuffer::RegisterWithLogger()
{
    if (!bIsRegisteredWithLogger)
    {
        FOutputDeviceLogger::Get()->RegisterOutputDevice(this);
        bIsRegisteredWithLogger = true;
    }
}

void FConsoleLogBuffer::UnregisterFromLogger()
{
    if (bIsRegisteredWithLogger)
    {
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(this);
        bIsRegisteredWithLogger = false;
    }
}

void FConsoleLogBuffer::Clear()
{
    TScopedLock Lock(LinesCS);

    Lines.Clear();
    Revision++;
    ClearRevision = Revision;
}

uint64 FConsoleLogBuffer::GetSnapshot(TArray<FConsoleLogLine>& OutLines) const
{
    TScopedLock Lock(LinesCS);

    CopyNewestLines(Lines.Size(), OutLines);
    return Revision;
}

uint64 FConsoleLogBuffer::GetLinesSince(uint64 SinceRevision, TArray<FConsoleLogLine>& OutLines, bool& bOutContinuous) const
{
    TScopedLock Lock(LinesCS);

    const uint64 NumMissing = Revision - Math::Min(SinceRevision, Revision);
    bOutContinuous = SinceRevision >= ClearRevision && NumMissing <= static_cast<uint64>(Lines.Size());

    CopyNewestLines(bOutContinuous ? static_cast<int32>(NumMissing) : Lines.Size(), OutLines);
    return Revision;
}

int32 FConsoleLogBuffer::GetNumLines() const
{
    TScopedLock Lock(LinesCS);
    return Lines.Size();
}

void FConsoleLogBuffer::SetMaxLines(int32 InMaxLines)
{
    TScopedLock Lock(LinesCS);

    const int32 NewMaxLines = Math::Max(1, InMaxLines);
    if (NewMaxLines < Lines.Size())
    {
        Revision++;
        ClearRevision = Revision;
    }

    Lines.SetCapacity(NewMaxLines);
}

uint64 FConsoleLogBuffer::GetRevision() const
{
    TScopedLock Lock(LinesCS);
    return Revision;
}

void FConsoleLogBuffer::CopyNewestLines(int32 NumLines, TArray<FConsoleLogLine>& OutLines) const
{
    // Called with the lock already held
    OutLines.Clear();
    OutLines.Reserve(NumLines);

    for (int32 Index = Lines.Size() - NumLines; Index < Lines.Size(); ++Index)
    {
        OutLines.Add(Lines[Index]);
    }
}
