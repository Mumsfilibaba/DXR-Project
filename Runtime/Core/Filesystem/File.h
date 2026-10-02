#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Stream.h"
#include "Core/Containers/Function.h"

struct IPlatformFile;
struct FDirectoryEntry;

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

    static FORCEINLINE bool WriteTextFile(IPlatformFile* InFile, const String& Text)
    {
        return WriteTextFile(InFile, Text.Data(), Text.SizeInBytes());
    }

    // Returns the Path to the file (Excluding the filename)
    static String ExtractFilepath(const String& Filepath);

    // Returns the directory containing the file, or an empty string when the path has no directory component
    static String GetDirectoryOf(const String& Filepath);

    // Returns the Filename with the extension (Excluding the rest of the path)
    static String ExtractFilename(const String& Filepath);

    // Returns the Filename without the extension (Excluding the rest of the path)
    static String ExtractFilenameWithoutExtension(const String& Filepath);

    /** @return Returns the last ".ext" including the dot, or empty if there is none */
    static String ExtractExtension(const String& Filepath);

    /** @brief Join Left and Right with a single '/', ignoring extra separators */
    static String CombinePath(const String& Left, const String& Right);

    /**
     * @brief Depth-first walk of Directory
     * @param Visitor Called for each file and subdirectory. Return false on a directory to skip its children; return false on a file to abort the walk.
     * @return Returns false if Directory cannot be listed or a file visitor aborted
     */
    static bool IterateDirectoryTree(const String& Directory, TFunction<bool(const String& Path, bool bIsDirectory)> Visitor);

    // Recursively creates every directory in Path that does not already exist. Returns true if the full directory tree exists afterwards.
    static bool CreateDirectoryTree(const String& Path);

private:
    static bool WriteTextFile(IPlatformFile* InFile, const CHAR* Text, uint32 Size);
};
