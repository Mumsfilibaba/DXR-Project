#pragma once
#include <Core/CoreTypes.h>
#include <Core/Misc/IOutputDevice.h>

struct IPlatformFile;

class FTestConsoleOutputDevice final : public IOutputDevice
{
public:
    /** @brief Open an additional log file that mirrors all console output (append mode). */
    void OpenLogFile(const CHAR* FilePath);

    /** @brief Close the log file if one is open. */
    void CloseLogFile();

    virtual void Log(const String& Message) override;
    virtual void Log(ELogSeverity Severity, const String& Message) override;
    virtual void Flush() override;

private:
    void Write(const CHAR* Text);

    IPlatformFile* LogFile = nullptr;
};
