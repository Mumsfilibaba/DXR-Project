#include "Core/Windows/WindowsPlatformLibrary.h"
#include "Core/Templates/CString.h"

static bool FindStringExport(const uint8* ImageBase, const ANSICHAR* SymbolName, FString& OutValue)
{
    const IMAGE_DOS_HEADER* DosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(ImageBase);
    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return false;
    }

    const IMAGE_NT_HEADERS* NtHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(ImageBase + DosHeader->e_lfanew);
    if (NtHeaders->Signature != IMAGE_NT_SIGNATURE || NtHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
    {
        return false;
    }

    const uint64 ImageSize = NtHeaders->OptionalHeader.SizeOfImage;
    const auto IsInImage = [ImageSize](uint64 Address, uint64 Size)
    {
        return Address + Size <= ImageSize;
    };

    const IMAGE_DATA_DIRECTORY& ExportDirectory = NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (ExportDirectory.VirtualAddress == 0 || !IsInImage(ExportDirectory.VirtualAddress, sizeof(IMAGE_EXPORT_DIRECTORY)))
    {
        return false;
    }

    const IMAGE_EXPORT_DIRECTORY* Exports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(ImageBase + ExportDirectory.VirtualAddress);
    if (!IsInImage(Exports->AddressOfNames, Exports->NumberOfNames * sizeof(uint32)) ||
        !IsInImage(Exports->AddressOfNameOrdinals, Exports->NumberOfNames * sizeof(uint16)) ||
        !IsInImage(Exports->AddressOfFunctions, Exports->NumberOfFunctions * sizeof(uint32)))
    {
        return false;
    }

    const uint32* NameAddresses  = reinterpret_cast<const uint32*>(ImageBase + Exports->AddressOfNames);
    const uint16* NameOrdinals   = reinterpret_cast<const uint16*>(ImageBase + Exports->AddressOfNameOrdinals);
    const uint32* ValueAddresses = reinterpret_cast<const uint32*>(ImageBase + Exports->AddressOfFunctions);
    const uint64  SymbolLength   = TCString<ANSICHAR>::Strlen(SymbolName);

    for (uint32 Index = 0; Index < Exports->NumberOfNames; ++Index)
    {
        if (!IsInImage(NameAddresses[Index], SymbolLength + 1) || TCString<ANSICHAR>::Strcmp(reinterpret_cast<const ANSICHAR*>(ImageBase + NameAddresses[Index]), SymbolName) != 0)
        {
            continue;
        }

        if (NameOrdinals[Index] >= Exports->NumberOfFunctions)
        {
            return false;
        }

        const uint64 ValueAddress = ValueAddresses[NameOrdinals[Index]];
        if (!IsInImage(ValueAddress, 1))
        {
            return false;
        }

        const ANSICHAR* Value     = reinterpret_cast<const ANSICHAR*>(ImageBase + ValueAddress);
        const uint64    MaxLength = ImageSize - ValueAddress;

        uint64 Length = 0;
        while (Length < MaxLength && Value[Length] != '\0')
        {
            ++Length;
        }

        if (Length == MaxLength)
        {
            return false;
        }

        OutValue = StringCast<CHAR_T>(Value, static_cast<int32>(Length)).ToString();
        return true;
    }

    return false;
}

bool FWindowsPlatformLibrary::ReadStringExport(const CHAR_T* Filepath, const CHAR_T* SymbolName, FString& OutValue)
{
    FString NativePath(Filepath);
    NativePath.ReplaceAll('/', '\\');

    HMODULE Image = ::LoadLibraryExW(StringCast<WIDECHAR>(NativePath).Get(), nullptr, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (!Image)
    {
        return false;
    }

    const uint8* ImageBase = reinterpret_cast<const uint8*>(reinterpret_cast<UINT_PTR>(Image) & ~static_cast<UINT_PTR>(3));
    const bool   bFound    = FindStringExport(ImageBase, StringCast<ANSICHAR>(SymbolName).Get(), OutValue);

    ::FreeLibrary(Image);
    return bFound;
}
