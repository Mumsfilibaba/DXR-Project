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

FFileOutputDevice::FFileOutputDevice(const String& FilePath)
    : FileHandle(FPlatformFile::OpenForAsyncWrite(FilePath, true))
{
}

FFileOutputDevice::~FFileOutputDevice()
{
    FlushBlocking();
}

void FFileOutputDevice::Log(const String& Message)
{
    QueueWrite(Message + "\n");
}

void FFileOutputDevice::Log(ELogSeverity Severity, const String& Message)
{
    QueueWrite(String(SeverityToString(Severity)) + Message + "\n");
}

void FFileOutputDevice::Flush()
{
    FlushBlocking();
}

void FFileOutputDevice::QueueWrite(const String& Line)
{
    {
        TScopedLock Lock(PendingLinesCS);
        PendingLines.Emplace(Line);
    }

    FlushAsync();
}

void FFileOutputDevice::FlushAsync()
{
    TArray<String> LinesToFlush;
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
        for (const String& Line : LinesToFlush)
        {
            TotalSize += Line.SizeInBytes();
        }

        TArray<uint8> Buffer;
        Buffer.Resize(TotalSize);

        uint32 Offset = 0;
        for (const String& Line : LinesToFlush)
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
