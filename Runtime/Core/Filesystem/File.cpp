#include "Core/Filesystem/File.h"
#include "Core/PlatformInterface/IPlatformFileSystem.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Memory/Memory.h"

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

String File::ExtractFilepath(const String& Filepath)
{
    int32 LastSlash = Filepath.FindLastChar('/');
    if (LastSlash == String::InvalidIndex)
    {
        LastSlash = Filepath.Length();
    }
    
    return String(*Filepath, LastSlash);
}

String File::GetDirectoryOf(const String& Filepath)
{
    const int32 LastSlash = Filepath.FindLastChar('/');
    return (LastSlash == String::InvalidIndex) ? String() : String(*Filepath, LastSlash);
}

String File::ExtractFilename(const String& Filepath)
{
    int32 LastSlash = Filepath.FindLastChar('/');
    if (LastSlash == String::InvalidIndex)
    {
        LastSlash = 0;
    }
    else
    {
        LastSlash++;
    }
    
    int32 NewLength = Filepath.Length() - LastSlash;
    return String(*Filepath + LastSlash, NewLength);
}
    
String File::ExtractFilenameWithoutExtension(const String& Filepath)
{
    int32 LastSlash = Filepath.FindLastChar('/');
    if (LastSlash == String::InvalidIndex)
    {
        LastSlash = 0;
    }
    else
    {
        LastSlash++;
    }
    
    int32 ExtensionPos = Filepath.FindLastChar('.');
    if (ExtensionPos == String::InvalidIndex)
    {
        ExtensionPos = CString::Strlen(*Filepath + LastSlash);
    }
    
    int32 NewLength = ExtensionPos - LastSlash;
    return String(*Filepath + LastSlash, NewLength);
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
