#pragma once
#include "Core/Containers/String.h"
#include "Core/Misc/IOutputDevice.h"
#include "Core/Platform/CriticalSection.h"

class CORE_API FTextFileOutputDevice final : public IOutputDevice
{
public:
    FTextFileOutputDevice(const FString& InFileName);
    ~FTextFileOutputDevice() override = default;

    virtual void Log(const FString& Message) override final;
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;
    virtual void Flush() override final;

    void FlushAsync();

    const FString& GetFilePath() const
    {
        return FilePath;
    }

private:
    static void WriteTextToFile(const FString& InFilePath, const FString& Text);
    void AppendLine(const FString& Message);

    FString          FilePath;
    FString          Buffer;
    FCriticalSection BufferCS;
};
