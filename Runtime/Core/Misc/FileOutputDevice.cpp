#include "Core/Misc/FileOutputDevice.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Containers/StringConversion.h"

static const CHAR_T* SeverityToString(ELogSeverity Severity)
{
    switch (Severity)
    {
        case ELogSeverity::Error:   return TEXT("[Error]   ");
        case ELogSeverity::Warning: return TEXT("[Warning] ");
        case ELogSeverity::Info:    return TEXT("[Info]    ");
        default:                    return TEXT("");
    }
}

static constexpr int32 MaxPendingLines = 4096;

FFileOutputDevice::FFileOutputDevice(const FString& FilePath)
    : FileHandle(FPlatformFile::OpenForAsyncWrite(FilePath, true))
    , bIsInsideHandle(false)
{
}

FFileOutputDevice::~FFileOutputDevice()
{
    FlushBlocking();
}

void FFileOutputDevice::Log(const FString& Message)
{
    QueueWrite(Message + TEXT("\n"));
}

void FFileOutputDevice::Log(ELogSeverity Severity, const FString& Message)
{
    QueueWrite(FString(SeverityToString(Severity)) + Message + TEXT("\n"));

    if (Severity >= ELogSeverity::Warning)
    {
        FlushBlocking();
    }
}

void FFileOutputDevice::Flush()
{
    FlushBlocking();
}

void FFileOutputDevice::QueueWrite(const FString& Line)
{
    {
        TScopedLock Lock(PendingLinesCS);
        PendingLines.Emplace(Line);
        DropOverflowingLines();
    }

    FlushAsync();
}

void FFileOutputDevice::FlushAsync()
{
    TScopedLock Lock(FileCS);
    if (bIsInsideHandle)
    {
        return;
    }

    bIsInsideHandle = true;
    SubmitPendingLines();
    bIsInsideHandle = false;
}

void FFileOutputDevice::FlushBlocking()
{
    TScopedLock Lock(FileCS);
    if (bIsInsideHandle)
    {
        return;
    }

    bIsInsideHandle = true;
    SubmitPendingLines();

    if (FileHandle.IsValid())
    {
        FileHandle->WaitForPendingWrites();
    }

    bIsInsideHandle = false;
}

void FFileOutputDevice::SubmitPendingLines()
{
    TArray<FString> LinesToWrite;
    {
        TScopedLock Lock(PendingLinesCS);
        if (PendingLines.IsEmpty())
        {
            return;
        }

        LinesToWrite = Move(PendingLines);
        PendingLines.Clear();
    }

    const bool bWritten = WriteLines(LinesToWrite);

    if (!bWritten)
    {
        TScopedLock Lock(PendingLinesCS);
        LinesToWrite.Append(PendingLines);
        PendingLines = Move(LinesToWrite);
        DropOverflowingLines();
    }
}

bool FFileOutputDevice::WriteLines(const TArray<FString>& Lines)
{
    if (!FileHandle.IsValid() || FileHandle->HasWriteError())
    {
        return true;
    }

    // The log file is UTF-8 whatever CHAR_T is
    TArray<uint8> Buffer;
    for (const FString& Line : Lines)
    {
        const TStringConversion<ANSICHAR, CHAR_T> Utf8Line = StringCast<ANSICHAR>(Line);
        Buffer.Append(reinterpret_cast<const uint8*>(Utf8Line.Get()), Utf8Line.Length());
    }

    if (Buffer.IsEmpty())
    {
        return true;
    }

    return FileHandle->WriteAsync(Buffer.Data(), static_cast<uint32>(Buffer.Size()));
}

void FFileOutputDevice::DropOverflowingLines()
{
    if (PendingLines.Size() <= MaxPendingLines)
    {
        return;
    }

    PendingLines.RemoveAt(0, PendingLines.Size() - MaxPendingLines);
}
