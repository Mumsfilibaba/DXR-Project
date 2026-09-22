#include "Application/Console/ConsoleLogBuffer.h"
#include "Core/Math/Math.h"
#include "Core/Misc/OutputDeviceManager.h"
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
    , Lines()
    , MaxLines(Math::Max(1, InMaxLines))
    , Revision(0)
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
    TrimToMaxLines();
    Revision++;
}

void FConsoleLogBuffer::RegisterWithLogger()
{
    if (!bIsRegisteredWithLogger)
    {
        FOutputDeviceManager::Get()->RegisterOutputDevice(this);
        bIsRegisteredWithLogger = true;
    }
}

void FConsoleLogBuffer::UnregisterFromLogger()
{
    if (bIsRegisteredWithLogger)
    {
        FOutputDeviceManager::Get()->UnregisterOutputDevice(this);
        bIsRegisteredWithLogger = false;
    }
}

void FConsoleLogBuffer::Clear()
{
    TScopedLock Lock(LinesCS);

    Lines.Clear();
    Revision++;
}

void FConsoleLogBuffer::GetSnapshot(TArray<FConsoleLogLine>& OutLines) const
{
    TScopedLock Lock(LinesCS);
    OutLines = Lines;
}

int32 FConsoleLogBuffer::GetNumLines() const
{
    TScopedLock Lock(LinesCS);
    return Lines.Size();
}

void FConsoleLogBuffer::SetMaxLines(int32 InMaxLines)
{
    TScopedLock Lock(LinesCS);

    MaxLines = Math::Max(1, InMaxLines);
    TrimToMaxLines();
}

uint64 FConsoleLogBuffer::GetRevision() const
{
    TScopedLock Lock(LinesCS);
    return Revision;
}

void FConsoleLogBuffer::TrimToMaxLines()
{
    // Called with the lock already held
    const int32 NumLinesToDrop = Lines.Size() - MaxLines;
    if (NumLinesToDrop > 0)
    {
        Lines.RemoveAt(0, NumLinesToDrop);
    }
}
