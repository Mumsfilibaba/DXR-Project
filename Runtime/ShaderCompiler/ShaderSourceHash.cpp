#include "Core/Misc/CRC.h"
#include "ShaderCompiler/ShaderSourceHash.h"

void ShaderSourceHash::NormalizeLineEndings(TArray<uint8>& InOutContents)
{
    int32 WriteIndex = 0;
    for (int32 ReadIndex = 0; ReadIndex < InOutContents.Size(); ++ReadIndex)
    {
        const uint8 Character = InOutContents[ReadIndex];
        if (Character != static_cast<uint8>('\r'))
        {
            InOutContents[WriteIndex++] = Character;
        }
    }

    InOutContents.Resize(WriteIndex);
}

uint32 ShaderSourceHash::Compute(const TArray<uint8>& Contents)
{
    if (Contents.IsEmpty())
    {
        return 0;
    }

    TArray<uint8> Normalized = Contents;
    NormalizeLineEndings(Normalized);

    if (Normalized.IsEmpty())
    {
        return 0;
    }

    return CRC32::Generate(Normalized.Data(), static_cast<uint64>(Normalized.Size()));
}
