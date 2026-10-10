#include "TestConsoleOutputDevice.h"

#include <Core/Platform/PlatformFile.h>
#include <Core/Platform/PlatformMisc.h>
#include <Core/Templates/CString.h>

void FTestConsoleOutputDevice::OpenLogFile(const CHAR_T* FilePath)
{
    if (!LogFile)
    {
        LogFile = FPlatformFile::OpenForWrite(FilePath, /*bTruncate=*/ false);
        if (LogFile)
        {
            LogFile->SeekFromEnd(0);
        }
    }
}

void FTestConsoleOutputDevice::CloseLogFile()
{
    if (LogFile)
    {
        LogFile->Close();
        LogFile = nullptr;
    }
}

void FTestConsoleOutputDevice::Write(const CHAR_T* Text)
{
    const uint32 Length = static_cast<uint32>(TCString<CHAR_T>::Strlen(Text));

    FPlatformMisc::WriteToStdOutput(Text, Length);

    if (LogFile && (Length > 0))
    {
        LogFile->Write(reinterpret_cast<const uint8*>(Text), Length);
    }
}

void FTestConsoleOutputDevice::Log(const String& Message)
{
    Write(Message.Data());
    Write("\n");
}

void FTestConsoleOutputDevice::Log(ELogSeverity Severity, const String& Message)
{
    const CHAR_T* Prefix = "";
    switch (Severity)
    {
        case ELogSeverity::Warning: Prefix = "[WARNING] "; break;
        case ELogSeverity::Error:   Prefix = "[ERROR] ";   break;
        default:                    Prefix = "";           break;
    }

    Write(Prefix);
    Write(Message.Data());
    Write("\n");
}

void FTestConsoleOutputDevice::Flush()
{
}
