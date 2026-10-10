#pragma once
#include "Core/Containers/Array.h"

struct SHADERCOMPILER_API ShaderSourceHash
{
    /** @brief Removes every '\r', so a CRLF checkout and an LF checkout of the same file have the same bytes */
    static void NormalizeLineEndings(TArray<uint8>& InOutContents);

    /** @return CRC32 of the contents after NormalizeLineEndings, zero for empty contents */
    NODISCARD static uint32 Compute(const TArray<uint8>& Contents);
};
