#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

struct FSpirvTransforms
{
    /** Hashed into ComputeCompileHash, bump when a pass changes its output */
    static constexpr uint32 Version = 1;

    /** @brief Strips the Google extensions, forces unknown storage-image formats and merges the duplicate types that leaves behind */
    static bool PrepareForVulkan(const TArray<uint32>& InWords, TArray<uint32>& OutWords, String* OutError = nullptr);

    /** @brief Removes OpSourceContinued, OpSource, OpSourceExtension, OpName, OpMemberName, OpString, OpLine, OpNoLine and OpModuleProcessed. Run it before reflection, it moves every later word. */
    static bool StripDebugInstructions(const TArray<uint32>& InWords, TArray<uint32>& OutWords);

    static bool StripGoogleSpirvRequirements(const TArray<uint32>& InWords, TArray<uint32>& OutWords);
    static bool ValidateNoGoogleSpirvRequirements(const TArray<uint32>& Words, String* OutErrorMessage = nullptr);
    static bool ForceUnknownStorageImageFormats(const TArray<uint32>& InWords, TArray<uint32>& OutWords, bool& bOutRewroteFormats);
    static bool MergeDuplicateTypeDeclarations(const TArray<uint32>& InWords, TArray<uint32>& OutWords);
};
