#include "Core/Misc/FileOutputDevice.h"
#include "Core/Platform/PlatformFile.h"

static const CHAR_T* SeverityToString(ELogSeverity Severity)
{
    switch (Severity)
    {
        case ELogSeverity::Error:   return "[Error]   ";
        case ELogSeverity::Warning: return "[Warning] ";
        case ELogSeverity::Info:    return "[Info]    ";
        default:                    return "";
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
    QueueWrite(Message + "\n");
}

void FFileOutputDevice::Log(ELogSeverity Severity, const FString& Message)
{
    QueueWrite(FString(SeverityToString(Severity)) + Message + "\n");

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

    uint32 TotalSize = 0;
    for (const FString& Line : Lines)
    {
        TotalSize += Line.SizeInBytes();
    }

    if (TotalSize == 0)
    {
        return true;
    }

    TArray<uint8> Buffer;
    Buffer.Resize(TotalSize);

    uint32 Offset = 0;
    for (const FString& Line : Lines)
    {
        Memory::Memcpy(Buffer.Data() + Offset, *Line, Line.SizeInBytes());
        Offset += Line.SizeInBytes();
    }

    return FileHandle->WriteAsync(Buffer.Data(), TotalSize);
}

void FFileOutputDevice::DropOverflowingLines()
{
    if (PendingLines.Size() <= MaxPendingLines)
    {
        return;
    }

    PendingLines.RemoveAt(0, PendingLines.Size() - MaxPendingLines);
}
