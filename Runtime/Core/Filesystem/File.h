#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Stream.h"
#include "Core/Containers/Function.h"

struct IPlatformFile;
struct FDirectoryEntry;

/*
 * Engine paths. The path helpers in File (GetDirectoryOf through IsUnderDirectory) work on the engine's own path format
 * rather than the platform's, so a path means the same thing on every platform and in every project that is moved between them:
 *  - '/' is the separator. '\' is also read as a separator on every platform and written as '/', so paths authored
 *    on Windows (in an .mtl file, say) work everywhere. A file or directory name in an engine path never contains '\'.
 *  - Drive letters ("C:/") and UNC roots ("//Server/Share") are only recognized on Windows. Elsewhere "C:" is an
 *    ordinary name and a leading "//" is the same root as "/".
 *  - Paths compare without regard to case on Windows and with regard to it elsewhere.
 * FPlatformFile, by contrast, takes the platform's own paths and interprets nothing. The paths it returns, such as the
 * executable and working directory, always use '/', so every path the engine prints or stores uses the same separator.
 */

class CORE_API File
{
public:
    /**
     * @brief Read the rest of the file, from its current position, into a new buffer that OutData takes ownership of
     * @return Returns false if fewer bytes than the rest of the file could be read. An empty file gives an empty stream.
     */
    static bool ReadFile(IPlatformFile* InFile, FByteInputStream& OutData);

    /**
     * @brief Read the rest of the file, from its current position, into OutData
     * @return Returns false and empties OutData if fewer bytes than the rest of the file could be read. An empty file gives
     * an empty array.
     */
    static bool ReadFile(IPlatformFile* InFile, TArray<uint8>& OutData);

    /**
     * @brief Read the rest of the file, from its current position, into OutText followed by a null-terminator, so
     * OutText.Data() is a C string. The file holds UTF-8, converted to CHAR_T. A UTF-8 byte-order mark at the start of the
     * file is skipped.
     * @return Returns false and empties OutText if fewer bytes than the rest of the file could be read. An empty file gives
     * just the null-terminator.
     */
    static bool ReadTextFile(IPlatformFile* InFile, TArray<CHAR_T>& OutText);

    /**
     * @brief Write every element of Text at the file's current position as UTF-8, including a null-terminator if the array
     * holds one
     * @return Returns false if not every byte could be written. Writing an empty array succeeds.
     */
    static FORCEINLINE bool WriteTextFile(IPlatformFile* InFile, const TArray<CHAR_T>& Text)
    {
        return WriteTextFile(InFile, Text.Data(), Text.Size());
    }

    /**
     * @brief Write the characters of Text at the file's current position as UTF-8, without a null-terminator
     * @return Returns false if not every byte could be written. Writing an empty string succeeds.
     */
    static FORCEINLINE bool WriteTextFile(IPlatformFile* InFile, const FString& Text)
    {
        return WriteTextFile(InFile, Text.Data(), Text.Length());
    }

    /** @return Returns the directory containing the file, or an empty string when the path has no directory component */
    static FString GetDirectoryOf(const FString& Filepath);

    /** @return Returns the filename with its extension, without the rest of the path */
    static FString ExtractFilename(const FString& Filepath);

    /** @return Returns the filename without its extension or the rest of the path. A leading dot is part of the name. */
    static FString ExtractFilenameWithoutExtension(const FString& Filepath);

    /** @return Returns the last ".ext" including the dot, or empty if there is none */
    static FString ExtractExtension(const FString& Filepath);

    /** @brief Join Left and Right with a single '/', ignoring extra separators */
    static FString CombinePath(const FString& Left, const FString& Right);

    /**
     * @brief Clean up a path without touching the disk: '\' becomes '/', repeated separators and "." are removed, ".." is
     * resolved where possible and a trailing separator is dropped. "/A/../B/" becomes "/B", and "A/.." becomes ".".
     */
    static FString NormalizePath(const FString& Path);

    /** @return Returns Path normalized and, when relative, resolved against the current working directory */
    static FString MakeAbsolute(const FString& Path);

    /** @return Returns true if Path starts at a root, such as "/", "C:/" or a UNC "//", so it doesn't depend on any current directory */
    static bool IsAbsolutePath(const FString& Path);

    /**
     * @brief Express Path relative to BaseDirectory, e.g. "/Engine/Content/A.png" from "/Engine/Saved" is "../Content/A.png"
     * @return Returns the relative path, "." for the directory itself, or the absolute Path when the two have different
     * roots, such as two Windows drives
     */
    static FString MakeRelative(const FString& Path, const FString& BaseDirectory);

    /** @return Returns true if Path is Directory itself or anything inside it. Both are resolved before comparing. */
    static bool IsUnderDirectory(const FString& Path, const FString& Directory);

    /**
     * @brief Depth-first walk of Directory
     * @param Visitor Called for each file and subdirectory. Return false on a directory to skip its children; return false on
     * a file to abort the walk.
     * @return Returns false if Directory cannot be listed or a file visitor aborted
     */
    static bool IterateDirectoryTree(const FString& Directory, TFunction<bool(const FString& Path, bool bIsDirectory)> Visitor);

    /**
     * @brief Create every directory in Path that does not already exist
     * @return Returns true if the full directory tree exists afterwards
     */
    static bool CreateDirectoryTree(const FString& Path);

    /**
     * @brief Delete Directory and everything in it. Symbolic links are removed without touching what they point at.
     * @return Returns true if Directory is gone afterwards
     */
    static bool DeleteDirectoryTree(const FString& Directory);

    /**
     * @brief Copy Directory and everything in it to Destination, creating Destination if needed. Links to directories are
     * skipped rather than followed, and a link to a file is copied as the file it points at.
     * @param bReplaceExisting Overwrite files that already exist in Destination instead of failing
     * @param Filter Called with each source path before it is copied. Return false to skip a file, or a directory and
     * everything in it. May be empty.
     * @return Returns false if Destination is inside Directory, a directory cannot be listed or created, or a file cannot be
     * copied. Files copied before a failure are left in place.
     */
    static bool CopyDirectoryTree(
        const FString&                                          Directory,
        const FString&                                          Destination,
        bool                                                    bReplaceExisting = false,
        TFunction<bool(const FString& Path, bool bIsDirectory)> Filter           = nullptr);

private:
    static bool WriteTextFile(IPlatformFile* InFile, const CHAR_T* Text, int32 Length);
};
