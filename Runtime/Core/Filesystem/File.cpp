#include "Core/Filesystem/File.h"
#include "Core/Generic/GenericPlatformFile.h"
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
