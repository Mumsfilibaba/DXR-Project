#include "Core/Math/Math.h"
#include "Core/Misc/CRC.h"
#include "Core/Templates/CString.h"
#include "ShaderCore/ShaderReflection.h"

uint32 HashShaderSemantic(StringView SemanticName)
{
    constexpr int32 MaxSemanticLength = 256;

    CHAR UpperName[MaxSemanticLength];

    const int32 Length = Math::Min<int32>(SemanticName.Length(), MaxSemanticLength);
    for (int32 Index = 0; Index < Length; ++Index)
    {
        UpperName[Index] = TCharTraits<CHAR>::ToUpper(SemanticName.Data()[Index]);
    }

    return CRC32::Generate(UpperName, static_cast<uint64>(Length));
}
