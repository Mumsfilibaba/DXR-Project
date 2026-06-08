#include "Core/Filesystem/File.h"
#include "Core/Generic/GenericPlatformFile.h"
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

FString File::ExtractFilepath(const FString& Filepath)
{
    int32 LastSlash = Filepath.FindLastChar('/');
    if (LastSlash == FString::InvalidIndex)
    {
        LastSlash = Filepath.Length();
    }
    
    return FString(*Filepath, LastSlash);
}

FString File::ExtractFilename(const FString& Filepath)
{
    int32 LastSlash = Filepath.FindLastChar('/');
    if (LastSlash == FString::InvalidIndex)
    {
        LastSlash = 0;
    }
    else
    {
        LastSlash++;
    }
    
    int32 NewLength = Filepath.Length() - LastSlash;
    return FString(*Filepath + LastSlash, NewLength);
}
    
FString File::ExtractFilenameWithoutExtension(const FString& Filepath)
{
    int32 LastSlash = Filepath.FindLastChar('/');
    if (LastSlash == FString::InvalidIndex)
    {
        LastSlash = 0;
    }
    else
    {
        LastSlash++;
    }
    
    int32 ExtensionPos = Filepath.FindLastChar('.');
    if (ExtensionPos == FString::InvalidIndex)
    {
        ExtensionPos = FCString::Strlen(*Filepath + LastSlash);
    }
    
    int32 NewLength = ExtensionPos - LastSlash;
    return FString(*Filepath + LastSlash, NewLength);
}
