#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Stream.h"

struct IPlatformFile;

class CORE_API File
{
public:
    static bool ReadFile(IPlatformFile* InFile, FByteInputStream& OutData);
    static bool ReadFile(IPlatformFile* InFile, TArray<uint8>& OutData);
    static bool ReadTextFile(IPlatformFile* InFile, TArray<CHAR>& OutText);

    static FORCEINLINE bool WriteTextFile(IPlatformFile* InFile, const TArray<CHAR>& Text)
    {
        return WriteTextFile(InFile, Text.Data(), Text.SizeInBytes());
    }

    static FORCEINLINE bool WriteTextFile(IPlatformFile* InFile, const FString& Text)
    {
        return WriteTextFile(InFile, Text.Data(), Text.SizeInBytes());
    }

    // Returns the Path to the file (Excluding the filename)
    static FString ExtractFilepath(const FString& Filepath);

    // Returns the Filename with the extension (Excluding the rest of the path)
    static FString ExtractFilename(const FString& Filepath);

    // Returns the Filename without the extension (Excluding the rest of the path)
    static FString ExtractFilenameWithoutExtension(const FString& Filepath);

private:
    static bool WriteTextFile(IPlatformFile* InFile, const CHAR* Text, uint32 Size);
};
