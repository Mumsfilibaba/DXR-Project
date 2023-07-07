#include "GenericPlatformFile.h"

bool FFileHelpers::ReadFile(IFileHandle* File, TArray<uint8>& OutData)
{
    CHECK(File != nullptr);

    const int64 FileSize = File->Size();
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

    // Get the filesize and add an extra character for the null-terminator
    const int64 FileSize = File->Size();
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