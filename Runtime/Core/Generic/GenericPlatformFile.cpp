#include "GenericPlatformFile.h"

bool FFileHelpers::ReadFile(IFileHandle* File, TArray<uint8>& OutData)
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

    return true;
}

bool FFileHelpers::ReadTextFile(IFileHandle* File, TArray<CHAR>& OutText)
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

    OutText[ReadBytes] = 0;
    return true;
}

bool FFileHelpers::WriteTextFile(IFileHandle* File, const CHAR* Text, uint32 Size)
{
    CHECK(File != nullptr);

    const int32 WrittenBytes = File->Write(reinterpret_cast<const uint8*>(Text), Size);
    if (WrittenBytes <= 0)
    {
        return false;
    }

    return true;
}

FString FFileHelpers::ExtractFilepath(const FString& Filepath)
{
    int32 LastSlash = Filepath.FindLastChar('/');
    if (LastSlash == FString::INVALID_INDEX)
    {
        LastSlash = Filepath.Length();
    }
    
    return FString(Filepath.GetCString(), LastSlash);
}

FString FFileHelpers::ExtractFilename(const FString& Filepath)
{
    int32 LastSlash = Filepath.FindLastChar('/');
    if (LastSlash == FString::INVALID_INDEX)
    {
        LastSlash = 0;
    }
    else
    {
        LastSlash++;
    }
    
    int32 NewLength = Filepath.Length() - LastSlash;
    return FString(Filepath.GetCString() + LastSlash, NewLength);
}
    
FString FFileHelpers::ExtractFilenameWithoutExtension(const FString& Filepath)
{
    int32 LastSlash = Filepath.FindLastChar('/');
    if (LastSlash == FString::INVALID_INDEX)
    {
        LastSlash = 0;
    }
    else
    {
        LastSlash++;
    }
    
    int32 ExtensionPos = Filepath.FindLastChar('.');
    if (ExtensionPos == FString::INVALID_INDEX)
    {
        ExtensionPos = FCString::Strlen(Filepath.GetCString() + LastSlash);
    }
    
    int32 NewLength = ExtensionPos - LastSlash;
    return FString(Filepath.GetCString() + LastSlash, NewLength);
}
