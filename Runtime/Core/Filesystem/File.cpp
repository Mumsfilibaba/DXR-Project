#include "Core/Filesystem/File.h"
#include "Core/PlatformInterface/IPlatformFileSystem.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Memory/Memory.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Misc/OutputDeviceManager.h"

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
    FString Root;

    /** @brief The directory and file names after the root, with "." removed and ".." resolved where possible */
    TArray<FString> Segments;
};

static int32 GetRemainingBytes(IPlatformFile* InFile)
{
    const int64 NumBytes = InFile->Size() - InFile->Tell();
    CHECK((NumBytes >= 0) && (NumBytes < TNumericLimits<int32>::Max()));
    return static_cast<int32>(NumBytes);
}

static bool IsSeparator(CHAR_T Char)
{
    return (Char == '/') || (Char == '\\');
}

#if PLATFORM_WINDOWS
static bool IsDriveLetter(CHAR_T Char)
{
    return ((Char >= 'A') && (Char <= 'Z')) || ((Char >= 'a') && (Char <= 'z'));
}
#endif

static int32 FindLastSeparator(const FString& Path)
{
    for (int32 Index = Path.Length() - 1; Index >= 0; --Index)
    {
        if (IsSeparator(Path[Index]))
        {
            return Index;
        }
    }

    return FString::InvalidIndex;
}

static int32 ParseRoot(const FString& Path, FString& OutRoot)
{
    const int32 Length = Path.Length();

#if PLATFORM_WINDOWS
    if ((Length >= 2) && IsSeparator(Path[0]) && IsSeparator(Path[1]))
    {
        OutRoot = TEXT("//");
        return 2;
    }

    if ((Length >= 2) && (Path[1] == ':') && IsDriveLetter(Path[0]))
    {
        OutRoot = FString(*Path, 2);

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
        OutRoot = TEXT("/");
        return 1;
    }

    OutRoot.Clear();
    return 0;
}

static FPathParts SplitPath(const FString& Path)
{
    FPathParts Parts;

    const int32 Length = Path.Length();
    const int32 Start  = ParseRoot(Path, Parts.Root);

    // Nothing is above an absolute root, while a relative path is allowed to climb above where it starts
    const bool bIsAbsolute = Parts.Root.EndsWith(TEXT("/"));

    FString Segment;
    for (int32 Index = Start; Index <= Length; ++Index)
    {
        if ((Index < Length) && !IsSeparator(Path[Index]))
        {
            Segment.Append(Path[Index]);
            continue;
        }

        if (Segment.Equals(TEXT("..")))
        {
            if (!Parts.Segments.IsEmpty() && !Parts.Segments.Last().Equals(TEXT("..")))
            {
                Parts.Segments.Pop();
            }
            else if (!bIsAbsolute)
            {
                Parts.Segments.Add(Segment);
            }
        }
        else if (!Segment.IsEmpty() && !Segment.Equals(TEXT(".")))
        {
            Parts.Segments.Add(Segment);
        }

        Segment.Clear();
    }

    return Parts;
}

static bool CopyDirectoryContents(
    const FString&                                           Directory,
    const FString&                                           Destination,
    bool                                                     bReplaceExisting,
    TFunction<bool(const FString& Path, bool bIsDirectory)>& Filter)
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

        const FString Source = File::CombinePath(Directory, Entry.Name);
        if (Filter.IsValid() && !Filter(Source, Entry.bIsDirectory))
        {
            continue;
        }

        const FString Target = File::CombinePath(Destination, Entry.Name);
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

static FString JoinPath(const FPathParts& Parts)
{
    if (Parts.Segments.IsEmpty())
    {
        return Parts.Root.IsEmpty() ? FString(TEXT(".")) : Parts.Root;
    }

    FString Result = Parts.Root;
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

    const int32 NumBytes = GetRemainingBytes(InFile);
    if (NumBytes == 0)
    {
        OutData = FByteInputStream();
        return true;
    }

    uint8* Stream = reinterpret_cast<uint8*>(Memory::Malloc(static_cast<uint64>(NumBytes)));
    if (InFile->Read(Stream, static_cast<uint32>(NumBytes)) != NumBytes)
    {
        Memory::Free(Stream);
        return false;
    }

    OutData = FByteInputStream(Stream, NumBytes);
    return true;
}

bool File::ReadFile(IPlatformFile* InFile, TArray<uint8>& OutData)
{
    CHECK(InFile != nullptr);

    const int32 NumBytes = GetRemainingBytes(InFile);
    OutData.Resize(NumBytes);

    if ((NumBytes > 0) && (InFile->Read(OutData.Data(), static_cast<uint32>(NumBytes)) != NumBytes))
    {
        OutData.Clear(true);
        return false;
    }

    return true;
}

static bool ReadUtf8TextFile(IPlatformFile* InFile, TArray<ANSICHAR>& OutText)
{
    const bool  bIsAtStart = InFile->Tell() == 0;
    const int32 NumBytes   = GetRemainingBytes(InFile);
    OutText.Resize(NumBytes + 1);

    if ((NumBytes > 0) && (InFile->Read(reinterpret_cast<uint8*>(OutText.Data()), static_cast<uint32>(NumBytes)) != NumBytes))
    {
        OutText.Clear(true);
        return false;
    }

    OutText[NumBytes] = 0;

    const uint8* Bytes = reinterpret_cast<const uint8*>(OutText.Data());
    if (bIsAtStart && (NumBytes >= 3) && (Bytes[0] == 0xEF) && (Bytes[1] == 0xBB) && (Bytes[2] == 0xBF))
    {
        Memory::Memmove(OutText.Data(), OutText.Data() + 3, NumBytes - 3 + 1);
        OutText.Resize(NumBytes - 3 + 1);
    }

    return true;
}

bool File::ReadTextFile(IPlatformFile* InFile, TArray<CHAR_T>& OutText)
{
    CHECK(InFile != nullptr);

#if WIDE_CHAR_BUILD
    TArray<ANSICHAR> Utf8Text;
    if (!ReadUtf8TextFile(InFile, Utf8Text))
    {
        OutText.Clear(true);
        return false;
    }

    const TStringConversion<CHAR_T, ANSICHAR> Text = StringCast<CHAR_T>(Utf8Text.Data(), Utf8Text.Size() - 1);
    OutText.Reset(TArray<CHAR_T>(Text.Get(), Text.Length() + 1));
    return true;
#else
    return ReadUtf8TextFile(InFile, OutText);
#endif
}

bool File::WriteTextFile(IPlatformFile* InFile, const CHAR_T* Text, int32 Length)
{
    CHECK(InFile != nullptr);

    if (Length <= 0)
    {
        return true;
    }

#if WIDE_CHAR_BUILD
    const TStringConversion<ANSICHAR, CHAR_T> Utf8Text = StringCast<ANSICHAR>(Text, Length);
    const ANSICHAR* Bytes    = Utf8Text.Get();
    const int32     NumBytes = Utf8Text.Length();
#else
    const ANSICHAR* Bytes    = Text;
    const int32     NumBytes = Length;
#endif

    return InFile->Write(reinterpret_cast<const uint8*>(Bytes), static_cast<uint32>(NumBytes)) == NumBytes;
}

FString File::GetDirectoryOf(const FString& Filepath)
{
    const int32 LastSeparator = FindLastSeparator(Filepath);
    return (LastSeparator == FString::InvalidIndex) ? FString() : FString(*Filepath, LastSeparator);
}

FString File::ExtractFilename(const FString& Filepath)
{
    const int32 Start = FindLastSeparator(Filepath) + 1;
    return FString(*Filepath + Start, Filepath.Length() - Start);
}
    
FString File::ExtractFilenameWithoutExtension(const FString& Filepath)
{
    const FString Filename = ExtractFilename(Filepath);
    const int32   Dot      = Filename.FindLastChar('.');

    if ((Dot == FString::InvalidIndex) || (Dot == 0))
    {
        return Filename;
    }

    return FString(*Filename, Dot);
}

bool File::CreateDirectoryTree(const FString& Path)
{
    // Walk the path and create each intermediate directory in turn (like 'mkdir -p').
    FString Prefix;
    for (const CHAR_T* It = *Path; ; ++It)
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

bool File::DeleteDirectoryTree(const FString& Directory)
{
    TArray<FDirectoryEntry> Entries;
    if (!FPlatformFile::IterateDirectory(*Directory, Entries))
    {
        return !FPlatformFile::IsDirectory(*Directory);
    }

    for (const FDirectoryEntry& Entry : Entries)
    {
        const FString Child = CombinePath(Directory, Entry.Name);
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
    const FString&                                          Directory,
    const FString&                                          Destination,
    bool                                                    bReplaceExisting,
    TFunction<bool(const FString& Path, bool bIsDirectory)> Filter)
{
    if (IsUnderDirectory(Destination, Directory))
    {
        LOG_ERROR("[File]: Cannot copy '%s' into '%s', which is inside it", *Directory, *Destination);
        return false;
    }

    return CopyDirectoryContents(Directory, Destination, bReplaceExisting, Filter);
}

FString File::ExtractExtension(const FString& Filepath)
{
    const FString Filename = ExtractFilename(Filepath);
    const int32   Dot      = Filename.FindLastChar('.');
    if ((Dot == FString::InvalidIndex) || (Dot == 0))
    {
        return FString();
    }

    return FString(*Filename + Dot, Filename.Length() - Dot);
}

FString File::CombinePath(const FString& Left, const FString& Right)
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
        return Left + FString(*Right + 1);
    }
    if (!bLeftSlash && !bRightSlash)
    {
        return Left + TEXT("/") + Right;
    }

    return Left + Right;
}

bool File::IterateDirectoryTree(const FString& Directory, TFunction<bool(const FString& Path, bool bIsDirectory)> Visitor)
{
    TArray<FDirectoryEntry> Entries;
    if (!FPlatformFile::IterateDirectory(*Directory, Entries))
    {
        return false;
    }

    for (const FDirectoryEntry& Entry : Entries)
    {
        const FString Child = CombinePath(Directory, Entry.Name);
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

FString File::NormalizePath(const FString& Path)
{
    if (Path.IsEmpty())
    {
        return FString();
    }

    return JoinPath(SplitPath(Path));
}

FString File::MakeAbsolute(const FString& Path)
{
    const FPathParts Parts = SplitPath(Path);
    if (Parts.Root.EndsWith(TEXT("/")))
    {
        return JoinPath(Parts);
    }

    return NormalizePath(CombinePath(FPlatformFile::GetCurrentWorkingDirectory(), Path));
}

bool File::IsAbsolutePath(const FString& Path)
{
    return SplitPath(Path).Root.EndsWith(TEXT("/"));
}

FString File::MakeRelative(const FString& Path, const FString& BaseDirectory)
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
        Relative.Segments.Add(FString(TEXT("..")));
    }

    for (int32 Index = NumShared; Index < PathParts.Segments.Size(); ++Index)
    {
        Relative.Segments.Add(PathParts.Segments[Index]);
    }

    return JoinPath(Relative);
}

bool File::IsUnderDirectory(const FString& Path, const FString& Directory)
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
