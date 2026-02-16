#include "Core/Generic/GenericPlatformFile.h"

FString FileUtils::NormalizeFilepath(const FString& Filepath)
{
    FString Result = Filepath;
    Result.ReplaceAll('\\', '/');
    return Result;
}

bool FileUtils::ReadFile(IFileHandle* File, FByteInputStream& OutData)
{
    CHECK(File != nullptr);

    const int64 FileSize = File->Size();
    CHECK(FileSize < TNumericLimits<int32>::Max());

    uint8* Stream = reinterpret_cast<uint8*>(FMemory::Malloc(static_cast<uint64>(FileSize)));
    const int32 ReadBytes = File->Read(Stream, static_cast<uint32>(FileSize));
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

bool FileUtils::ReadFile(IFileHandle* File, TArray<uint8>& OutData)
{
    CHECK(File != nullptr);

    const int64 FileSize = File->Size();
    CHECK(FileSize < TNumericLimits<int32>::Max());
    OutData.Resize(static_cast<int32>(FileSize));

    const int32 ReadBytes = File->Read(reinterpret_cast<uint8*>(OutData.Data()), static_cast<uint32>(FileSize));
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

bool FileUtils::ReadTextFile(IFileHandle* File, TArray<CHAR>& OutText)
{
    CHECK(File != nullptr);

    const int64 FileSize = File->Size();
    CHECK(FileSize < TNumericLimits<int32>::Max());

    // Get the filesize and add an extra character for the null-terminator
    OutText.Resize(static_cast<int32>(FileSize) + 1);

    const int32 ReadBytes = File->Read(reinterpret_cast<uint8*>(OutText.Data()), static_cast<uint32>(FileSize));
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

bool FileUtils::WriteTextFile(IFileHandle* File, const CHAR* Text, uint32 Size)
{
    CHECK(File != nullptr);

    const int32 WrittenBytes = File->Write(reinterpret_cast<const uint8*>(Text), Size);
    if (WrittenBytes <= 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

FString FileUtils::ExtractPath(const FString& Filepath)
{
    return ExtractFilepath(Filepath);
}

FString FileUtils::ExtractFilepath(const FString& Filepath)
{
    const FString Normalized = NormalizeFilepath(Filepath);
    int32 LastSlash = Normalized.FindLastChar('/');
    if (LastSlash == FString::InvalidIndex)
    {
        LastSlash = Normalized.Length();
    }
    
    return FString(*Normalized, LastSlash);
}

FString FileUtils::ExtractFilename(const FString& Filepath)
{
    const FString Normalized = NormalizeFilepath(Filepath);
    int32 LastSlash = Normalized.FindLastChar('/');
    if (LastSlash == FString::InvalidIndex)
    {
        LastSlash = 0;
    }
    else
    {
        LastSlash++;
    }
    
    int32 NewLength = Normalized.Length() - LastSlash;
    return FString(*Normalized + LastSlash, NewLength);
}
    
FString FileUtils::ExtractFilenameWithoutExtension(const FString& Filepath)
{
    const FString Normalized = NormalizeFilepath(Filepath);
    int32 LastSlash = Normalized.FindLastChar('/');
    if (LastSlash == FString::InvalidIndex)
    {
        LastSlash = 0;
    }
    else
    {
        LastSlash++;
    }
    
    int32 ExtensionPos = Normalized.FindLastChar('.');
    if (ExtensionPos == FString::InvalidIndex)
    {
        ExtensionPos = FCString::Strlen(*Normalized + LastSlash);
    }
    
    int32 NewLength = ExtensionPos - LastSlash;
    return FString(*Normalized + LastSlash, NewLength);
}
