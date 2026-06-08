#include "Core/Misc/FileOutputDevice.h"
#include "Core/Platform/PlatformFile.h"

static const CHAR* SeverityToString(ELogSeverity Severity)
{
    switch (Severity)
    {
        case ELogSeverity::Error:   return "[Error]   ";
        case ELogSeverity::Warning: return "[Warning] ";
        case ELogSeverity::Info:    return "[Info]    ";
        default:                    return "";
    }
}

FFileOutputDevice::FFileOutputDevice(const FString& FilePath)
    : FileHandle(FPlatformFile::OpenForAsyncWrite(FilePath, true))
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
    }

    FlushAsync();
}

void FFileOutputDevice::FlushAsync()
{
    TArray<FString> LinesToFlush;
    {
        TScopedLock Lock(PendingLinesCS);
        if (PendingLines.IsEmpty())
        {
            return;
        }

        LinesToFlush = Move(PendingLines);
        PendingLines.Clear();
    }

    if (FileHandle.IsValid())
    {
        uint32 TotalSize = 0;
        for (const FString& Line : LinesToFlush)
        {
            TotalSize += Line.SizeInBytes();
        }

        TArray<uint8> Buffer;
        Buffer.Resize(TotalSize);

        uint32 Offset = 0;
        for (const FString& Line : LinesToFlush)
        {
            Memory::Memcpy(Buffer.Data() + Offset, *Line, Line.SizeInBytes());
            Offset += Line.SizeInBytes();
        }

        FileHandle->WriteAsync(Buffer.Data(), TotalSize);
    }
}

void FFileOutputDevice::FlushBlocking()
{
    FlushAsync();

    if (FileHandle.IsValid())
    {
        FileHandle->WaitForPendingWrites();
    }
}
