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
 * FPlatformFile, by contrast, takes and returns the platform's own paths and interprets nothing.
 */

class CORE_API File
{
public:
    /**
     * @brief Read the whole file into a new buffer that OutData takes ownership of. The file is expected to be at its start.
     * @return Returns false if nothing could be read, which includes an empty file
     */
    static bool ReadFile(IPlatformFile* InFile, FByteInputStream& OutData);

    /**
     * @brief Read the whole file into OutData, which is resized to the file size. The file is expected to be at its start.
     * @return Returns false and empties OutData if nothing could be read, which includes an empty file
     */
    static bool ReadFile(IPlatformFile* InFile, TArray<uint8>& OutData);

    /**
     * @brief Read the whole file into OutText followed by a null-terminator, so OutText.Data() is a C string. The file is
     * expected to be at its start.
     * @return Returns false and empties OutText if nothing could be read, which includes an empty file
     */
    static bool ReadTextFile(IPlatformFile* InFile, TArray<CHAR>& OutText);

    /**
     * @brief Write every element of Text at the file's current position, including a null-terminator if the array holds one
     * @return Returns false if nothing could be written
     */
    static FORCEINLINE bool WriteTextFile(IPlatformFile* InFile, const TArray<CHAR>& Text)
    {
        return WriteTextFile(InFile, Text.Data(), Text.SizeInBytes());
    }

    /**
     * @brief Write the characters of Text at the file's current position, without a null-terminator
     * @return Returns false if nothing could be written
     */
    static FORCEINLINE bool WriteTextFile(IPlatformFile* InFile, const String& Text)
    {
        return WriteTextFile(InFile, Text.Data(), Text.SizeInBytes());
    }

    /** @return Returns the directory containing the file, or an empty string when the path has no directory component */
    static String GetDirectoryOf(const String& Filepath);

    /** @return Returns the filename with its extension, without the rest of the path */
    static String ExtractFilename(const String& Filepath);

    /** @return Returns the filename without its extension or the rest of the path. A leading dot is part of the name. */
    static String ExtractFilenameWithoutExtension(const String& Filepath);

    /** @return Returns the last ".ext" including the dot, or empty if there is none */
    static String ExtractExtension(const String& Filepath);

    /** @brief Join Left and Right with a single '/', ignoring extra separators */
    static String CombinePath(const String& Left, const String& Right);

    /**
     * @brief Clean up a path without touching the disk: '\' becomes '/', repeated separators and "." are removed, ".." is
     * resolved where possible and a trailing separator is dropped. "/A/../B/" becomes "/B", and "A/.." becomes ".".
     */
    static String NormalizePath(const String& Path);

    /** @return Returns Path normalized and, when relative, resolved against the current working directory */
    static String MakeAbsolute(const String& Path);

    /**
     * @brief Express Path relative to BaseDirectory, e.g. "/Engine/Content/A.png" from "/Engine/Saved" is "../Content/A.png"
     * @return Returns the relative path, "." for the directory itself, or the absolute Path when the two have different
     * roots, such as two Windows drives
     */
    static String MakeRelative(const String& Path, const String& BaseDirectory);

    /** @return Returns true if Path is Directory itself or anything inside it. Both are resolved before comparing. */
    static bool IsUnderDirectory(const String& Path, const String& Directory);

    /**
     * @brief Depth-first walk of Directory
     * @param Visitor Called for each file and subdirectory. Return false on a directory to skip its children; return false on
     * a file to abort the walk.
     * @return Returns false if Directory cannot be listed or a file visitor aborted
     */
    static bool IterateDirectoryTree(const String& Directory, TFunction<bool(const String& Path, bool bIsDirectory)> Visitor);

    /**
     * @brief Create every directory in Path that does not already exist
     * @return Returns true if the full directory tree exists afterwards
     */
    static bool CreateDirectoryTree(const String& Path);

    /**
     * @brief Delete Directory and everything in it. Symbolic links are removed without touching what they point at.
     * @return Returns true if Directory is gone afterwards
     */
    static bool DeleteDirectoryTree(const String& Directory);

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
        const String&                                          Directory,
        const String&                                          Destination,
        bool                                                   bReplaceExisting = false,
        TFunction<bool(const String& Path, bool bIsDirectory)> Filter           = nullptr);

private:
    static bool WriteTextFile(IPlatformFile* InFile, const CHAR* Text, uint32 Size);
};
