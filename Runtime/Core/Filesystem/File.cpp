#include "Core/Filesystem/File.h"
#include "Core/PlatformInterface/IPlatformFileSystem.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceLogger.h"

#if PLATFORM_WINDOWS
    static constexpr EStringCaseType PATH_CASE_TYPE = EStringCaseType::NoCase;
#else
    static constexpr EStringCaseType PATH_CASE_TYPE = EStringCaseType::CaseSensitive;
#endif

struct FPathParts
{
    /**
     * @brief "/", or empty for a relative path. On Windows also "C:/", "//" for a UNC path, or "C:" for a path relative
     * to the current directory of a drive.
     */
    String Root;

    /** @brief The directory and file names after the root, with "." removed and ".." resolved where possible */
    TArray<String> Segments;
};

static bool IsSeparator(CHAR Char)
{
    return (Char == '/') || (Char == '\\');
}

#if PLATFORM_WINDOWS
static bool IsDriveLetter(CHAR Char)
{
    return ((Char >= 'A') && (Char <= 'Z')) || ((Char >= 'a') && (Char <= 'z'));
}
#endif

static int32 FindLastSeparator(const String& Path)
{
    for (int32 Index = Path.Length() - 1; Index >= 0; --Index)
    {
        if (IsSeparator(Path[Index]))
        {
            return Index;
        }
    }

    return String::InvalidIndex;
}

static int32 ParseRoot(const String& Path, String& OutRoot)
{
    const int32 Length = Path.Length();

#if PLATFORM_WINDOWS
    if ((Length >= 2) && IsSeparator(Path[0]) && IsSeparator(Path[1]))
    {
        OutRoot = "//";
        return 2;
    }

    if ((Length >= 2) && (Path[1] == ':') && IsDriveLetter(Path[0]))
    {
        OutRoot = String(*Path, 2);

        if ((Length >= 3) && IsSeparator(Path[2]))
        {
            OutRoot.Append('/');
            return 3;
        }

        return 2;
    }
#endif

    if ((Length >= 1) && IsSeparator(Path[0]))
    {
        OutRoot = "/";
        return 1;
    }

    OutRoot.Clear();
    return 0;
}

static FPathParts SplitPath(const String& Path)
{
    FPathParts Parts;

    const int32 Length = Path.Length();
    const int32 Start  = ParseRoot(Path, Parts.Root);

    // Nothing is above an absolute root, while a relative path is allowed to climb above where it starts
    const bool bIsAbsolute = Parts.Root.EndsWith("/");

    String Segment;
    for (int32 Index = Start; Index <= Length; ++Index)
    {
        if ((Index < Length) && !IsSeparator(Path[Index]))
        {
            Segment.Append(Path[Index]);
            continue;
        }

        if (Segment.Equals(".."))
        {
            if (!Parts.Segments.IsEmpty() && !Parts.Segments.Last().Equals(".."))
            {
                Parts.Segments.Pop();
            }
            else if (!bIsAbsolute)
            {
                Parts.Segments.Add(Segment);
            }
        }
        else if (!Segment.IsEmpty() && !Segment.Equals("."))
        {
            Parts.Segments.Add(Segment);
        }

        Segment.Clear();
    }

    return Parts;
}

static bool CopyDirectoryContents(
    const String&                                           Directory,
    const String&                                           Destination,
    bool                                                    bReplaceExisting,
    TFunction<bool(const String& Path, bool bIsDirectory)>& Filter)
{
    TArray<FDirectoryEntry> Entries;
    if (!FPlatformFile::IterateDirectory(*Directory, Entries) || !File::CreateDirectoryTree(Destination))
    {
        return false;
    }

    for (const FDirectoryEntry& Entry : Entries)
    {
        if (Entry.bIsDirectory && Entry.bIsSymbolicLink)
        {
            continue;
        }

        const String Source = File::CombinePath(Directory, Entry.Name);
        if (Filter.IsValid() && !Filter(Source, Entry.bIsDirectory))
        {
            continue;
        }

        const String Target = File::CombinePath(Destination, Entry.Name);
        if (Entry.bIsDirectory)
        {
            if (!CopyDirectoryContents(Source, Target, bReplaceExisting, Filter))
            {
                return false;
            }
        }
        else if (!FPlatformFile::CopyFile(*Source, *Target, bReplaceExisting))
        {
            return false;
        }
    }

    return true;
}

static String JoinPath(const FPathParts& Parts)
{
    if (Parts.Segments.IsEmpty())
    {
        return Parts.Root.IsEmpty() ? String(".") : Parts.Root;
    }

    String Result = Parts.Root;
    for (int32 Index = 0; Index < Parts.Segments.Size(); ++Index)
    {
        if (Index > 0)
        {
            Result.Append('/');
        }

        Result.Append(Parts.Segments[Index]);
    }

    return Result;
}

bool File::ReadFile(IPlatformFile* InFile, FByteInputStream& OutData)
{
    CHECK(InFile != nullptr);

    const int64 FileSize = InFile->Size();
    CHECK(FileSize < TNumericLimits<int32>::Max());

    uint8* Stream = reinterpret_cast<uint8*>(Memory::Malloc(static_cast<uint64>(FileSize)));
    const int32 ReadBytes = InFile->Read(Stream, static_cast<uint32>(FileSize));
    if (ReadBytes <= 0)
    {
        return false;
    }
    else
    {
        OutData = FByteInputStream(Stream, static_cast<int32>(FileSize));
        return true;
    }
}

bool File::ReadFile(IPlatformFile* InFile, TArray<uint8>& OutData)
{
    CHECK(InFile != nullptr);

    const int64 FileSize = InFile->Size();
    CHECK(FileSize < TNumericLimits<int32>::Max());
    OutData.Resize(static_cast<int32>(FileSize));

    const int32 ReadBytes = InFile->Read(reinterpret_cast<uint8*>(OutData.Data()), static_cast<uint32>(FileSize));
    if (ReadBytes <= 0)
    {
        OutData.Clear(true);
        return false;
    }
    else
    {
        return true;
    }
}

bool File::ReadTextFile(IPlatformFile* InFile, TArray<CHAR>& OutText)
{
    CHECK(InFile != nullptr);

    const int64 FileSize = InFile->Size();
    CHECK(FileSize < TNumericLimits<int32>::Max());

    // Get the filesize and add an extra character for the null-terminator
    OutText.Resize(static_cast<int32>(FileSize) + 1);

    const int32 ReadBytes = InFile->Read(reinterpret_cast<uint8*>(OutText.Data()), static_cast<uint32>(FileSize));
    if (ReadBytes <= 0)
    {
        OutText.Clear(true);
        return false;
    }
    else
    {
        OutText[ReadBytes] = 0;
        return true;
    }
}

bool File::WriteTextFile(IPlatformFile* InFile, const CHAR* Text, uint32 Size)
{
    CHECK(InFile != nullptr);

    const int32 WrittenBytes = InFile->Write(reinterpret_cast<const uint8*>(Text), Size);
    if (WrittenBytes <= 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

String File::GetDirectoryOf(const String& Filepath)
{
    const int32 LastSeparator = FindLastSeparator(Filepath);
    return (LastSeparator == String::InvalidIndex) ? String() : String(*Filepath, LastSeparator);
}

String File::ExtractFilename(const String& Filepath)
{
    const int32 Start = FindLastSeparator(Filepath) + 1;
    return String(*Filepath + Start, Filepath.Length() - Start);
}
    
String File::ExtractFilenameWithoutExtension(const String& Filepath)
{
    const String Filename = ExtractFilename(Filepath);
    const int32  Dot      = Filename.FindLastChar('.');

    if ((Dot == String::InvalidIndex) || (Dot == 0))
    {
        return Filename;
    }

    return String(*Filename, Dot);
}

bool File::CreateDirectoryTree(const String& Path)
{
    // Walk the path and create each intermediate directory in turn (like 'mkdir -p').
    String Prefix;
    for (const CHAR* It = *Path; ; ++It)
    {
        if (*It == '/' || *It == '\\' || *It == '\0')
        {
            // Skip empty segments and the Windows drive root ("C:")
            const bool bDriveRoot = (Prefix.Length() == 2 && Prefix[1] == ':');
            if (!Prefix.IsEmpty() && !bDriveRoot && !FPlatformFile::IsDirectory(*Prefix))
            {
                if (!FPlatformFile::CreateDirectory(*Prefix))
                {
                    return false;
                }
            }

            if (*It == '\0')
            {
                break;
            }
        }

        Prefix.Append(*It);
    }

    return true;
}

bool File::DeleteDirectoryTree(const String& Directory)
{
    TArray<FDirectoryEntry> Entries;
    if (!FPlatformFile::IterateDirectory(*Directory, Entries))
    {
        return !FPlatformFile::IsDirectory(*Directory);
    }

    for (const FDirectoryEntry& Entry : Entries)
    {
        const String Child = CombinePath(Directory, Entry.Name);
        if (Entry.bIsSymbolicLink)
        {
            if (!FPlatformFile::DeleteFile(*Child) && !FPlatformFile::RemoveDirectory(*Child))
            {
                return false;
            }
        }
        else if (Entry.bIsDirectory)
        {
            if (!DeleteDirectoryTree(Child))
            {
                return false;
            }
        }
        else if (!FPlatformFile::DeleteFile(*Child))
        {
            return false;
        }
    }

    return FPlatformFile::RemoveDirectory(*Directory);
}

bool File::CopyDirectoryTree(
    const String&                                          Directory,
    const String&                                          Destination,
    bool                                                   bReplaceExisting,
    TFunction<bool(const String& Path, bool bIsDirectory)> Filter)
{
    if (IsUnderDirectory(Destination, Directory))
    {
        LOG_ERROR("[File]: Cannot copy '%s' into '%s', which is inside it", *Directory, *Destination);
        return false;
    }

    return CopyDirectoryContents(Directory, Destination, bReplaceExisting, Filter);
}

String File::ExtractExtension(const String& Filepath)
{
    const String Filename = ExtractFilename(Filepath);
    const int32  Dot      = Filename.FindLastChar('.');
    if ((Dot == String::InvalidIndex) || (Dot == 0))
    {
        return String();
    }

    return String(*Filename + Dot, Filename.Length() - Dot);
}

String File::CombinePath(const String& Left, const String& Right)
{
    if (Left.IsEmpty())
    {
        return Right;
    }
    if (Right.IsEmpty())
    {
        return Left;
    }

    const bool bLeftSlash  = (Left[Left.Length() - 1] == '/') || (Left[Left.Length() - 1] == '\\');
    const bool bRightSlash = (Right[0] == '/') || (Right[0] == '\\');

    if (bLeftSlash && bRightSlash)
    {
        return Left + String(*Right + 1);
    }
    if (!bLeftSlash && !bRightSlash)
    {
        return Left + "/" + Right;
    }

    return Left + Right;
}

bool File::IterateDirectoryTree(const String& Directory, TFunction<bool(const String& Path, bool bIsDirectory)> Visitor)
{
    TArray<FDirectoryEntry> Entries;
    if (!FPlatformFile::IterateDirectory(*Directory, Entries))
    {
        return false;
    }

    for (const FDirectoryEntry& Entry : Entries)
    {
        const String Child = CombinePath(Directory, Entry.Name);
        if (!Visitor(Child, Entry.bIsDirectory))
        {
            if (Entry.bIsDirectory)
            {
                continue;
            }

            return false;
        }

        if (Entry.bIsDirectory && !IterateDirectoryTree(Child, Visitor))
        {
            return false;
        }
    }

    return true;
}

String File::NormalizePath(const String& Path)
{
    if (Path.IsEmpty())
    {
        return String();
    }

    return JoinPath(SplitPath(Path));
}

String File::MakeAbsolute(const String& Path)
{
    const FPathParts Parts = SplitPath(Path);
    if (Parts.Root.EndsWith("/"))
    {
        return JoinPath(Parts);
    }

    return NormalizePath(CombinePath(FPlatformFile::GetCurrentWorkingDirectory(), Path));
}

String File::MakeRelative(const String& Path, const String& BaseDirectory)
{
    const FPathParts PathParts = SplitPath(MakeAbsolute(Path));
    const FPathParts BaseParts = SplitPath(MakeAbsolute(BaseDirectory));

    if (!PathParts.Root.Equals(BaseParts.Root, PATH_CASE_TYPE))
    {
        return JoinPath(PathParts);
    }

    int32 NumShared = 0;
    while ((NumShared < PathParts.Segments.Size()) && (NumShared < BaseParts.Segments.Size()))
    {
        if (!PathParts.Segments[NumShared].Equals(BaseParts.Segments[NumShared], PATH_CASE_TYPE))
        {
            break;
        }

        ++NumShared;
    }

    FPathParts Relative;
    for (int32 Index = NumShared; Index < BaseParts.Segments.Size(); ++Index)
    {
        Relative.Segments.Add(String(".."));
    }

    for (int32 Index = NumShared; Index < PathParts.Segments.Size(); ++Index)
    {
        Relative.Segments.Add(PathParts.Segments[Index]);
    }

    return JoinPath(Relative);
}

bool File::IsUnderDirectory(const String& Path, const String& Directory)
{
    const FPathParts PathParts      = SplitPath(MakeAbsolute(Path));
    const FPathParts DirectoryParts = SplitPath(MakeAbsolute(Directory));

    if (!PathParts.Root.Equals(DirectoryParts.Root, PATH_CASE_TYPE) || (PathParts.Segments.Size() < DirectoryParts.Segments.Size()))
    {
        return false;
    }

    for (int32 Index = 0; Index < DirectoryParts.Segments.Size(); ++Index)
    {
        if (!PathParts.Segments[Index].Equals(DirectoryParts.Segments[Index], PATH_CASE_TYPE))
        {
            return false;
        }
    }

    return true;
}
