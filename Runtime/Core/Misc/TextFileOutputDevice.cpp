#include "Core/Misc/TextFileOutputDevice.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Threading/AsyncTask.h"
#include "Core/Threading/ScopedLock.h"

FTextFileOutputDevice::FTextFileOutputDevice(const FString& InFileName)
    : FilePath()
    , Buffer()
    , BufferCS()
{
    const FString CurrentDir = FPlatformFile::GetCurrentWorkingDirectory();
    FilePath = FileUtils::NormalizeFilepath(FString::CreateFormatted("%s/%s", *CurrentDir, *InFileName));
}

void FTextFileOutputDevice::Log(const FString& Message)
{
    AppendLine(Message);
}

void FTextFileOutputDevice::Log(ELogSeverity Severity, const FString& Message)
{
    UNREFERENCED_VARIABLE(Severity);
    AppendLine(Message);
}

void FTextFileOutputDevice::Flush()
{
    FString Text;
    {
        TScopedLock Lock(BufferCS);
        Text = Buffer;
        Buffer.Clear();
    }

    WriteTextToFile(FilePath, Text);
}

void FTextFileOutputDevice::FlushAsync()
{
    FString Text;
    FString Path;
    {
        TScopedLock Lock(BufferCS);
        Text = Buffer;
        Buffer.Clear();
        Path = FilePath;
    }

    Async([Text, Path]()
    {
        FTextFileOutputDevice::WriteTextToFile(Path, Text);
    });
}

void FTextFileOutputDevice::AppendLine(const FString& Message)
{
    TScopedLock Lock(BufferCS);
    Buffer += Message;
    Buffer += "\n";
}

void FTextFileOutputDevice::WriteTextToFile(const FString& InFilePath, const FString& Text)
{
    FFileHandleRef File(FPlatformFile::OpenForWrite(InFilePath, true));
    if (!File.IsValid())
    {
        if (IOutputDevice* OutputDevice = FOutputDeviceLogger::Get())
        {
            OutputDevice->Log(ELogSeverity::Error, FString::CreateFormatted("Failed to write CVar dump: %s", *InFilePath));
        }
        return;
    }

    FileUtils::WriteTextFile(File.Get(), Text);

    if (IOutputDevice* OutputDevice = FOutputDeviceLogger::Get())
    {
        OutputDevice->Log(ELogSeverity::Info, FString::CreateFormatted("CVar dump written to: %s", *InFilePath));
    }
}
