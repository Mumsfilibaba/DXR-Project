#include "Core/Containers/Map.h"
#include "Core/Misc/CRC.h"
#include "Core/Templates/CString.h"
#include "ShaderCompiler/Spirv/SpirvTransforms.h"

#define SPV_ENABLE_UTILITY_CODE
#include <spirv_cross_c.h>

struct SpirvOps
{
    static constexpr uint16 OpSourceContinued      = 2;
    static constexpr uint16 OpSource               = 3;
    static constexpr uint16 OpSourceExtension      = 4;
    static constexpr uint16 OpName                 = 5;
    static constexpr uint16 OpMemberName           = 6;
    static constexpr uint16 OpString               = 7;
    static constexpr uint16 OpLine                 = 8;
    static constexpr uint16 OpExtension            = 10;
    static constexpr uint16 OpCapability           = 17;
    static constexpr uint16 OpTypeVector           = 23;
    static constexpr uint16 OpTypeMatrix           = 24;
    static constexpr uint16 OpTypeImage            = 25;
    static constexpr uint16 OpTypeSampler          = 26;
    static constexpr uint16 OpTypeSampledImage     = 27;
    static constexpr uint16 OpTypeArray            = 28;
    static constexpr uint16 OpTypeRuntimeArray     = 29;
    static constexpr uint16 OpTypeStruct           = 30;
    static constexpr uint16 OpTypePointer          = 32;
    static constexpr uint16 OpTypeFunction         = 33;
    static constexpr uint16 OpDecorate             = 71;
    static constexpr uint16 OpMemberDecorate       = 72;
    static constexpr uint16 OpNoLine               = 317;
    static constexpr uint16 OpModuleProcessed      = 330;
    static constexpr uint16 OpDecorateString       = 5632;
    static constexpr uint16 OpMemberDecorateString = 5633;
    static constexpr uint16 OpDecorateId           = 332;

    static constexpr uint32 DecorationHlslCounterBufferGOOGLE = 5634;
    static constexpr uint32 DecorationHlslSemanticGOOGLE      = 5635;
    static constexpr uint32 DecorationUserTypeGOOGLE          = 5636;

    static constexpr uint32 CapabilityStorageImageReadWithoutFormat  = 55;
    static constexpr uint32 CapabilityStorageImageWriteWithoutFormat = 56;

    static constexpr uint32 ImageFormatUnknown = 0;

    // OpTypeImage operand layout: [1] Result, [2] SampledType, [3] Dim, [4] Depth, [5] Arrayed, [6] MS,
    // [7] Sampled, [8] ImageFormat. A Sampled operand of 2 means the image is used as a storage image.
    static constexpr uint16 OpTypeImageMinWords    = 9;
    static constexpr uint16 OpTypeImageSampledWord = 7;
    static constexpr uint16 OpTypeImageFormatWord  = 8;
    static constexpr uint32 ImageSampledStorage    = 2;
};

static bool SpvReadLiteralString(const uint32* Inst, uint16 InstWords, uint16 StartWord, CHAR* OutBuf, uint32 BufSize)
{
    if (StartWord >= InstWords || BufSize == 0)
    {
        return false;
    }

    const CHAR*  Src      = reinterpret_cast<const CHAR*>(&Inst[StartWord]);
    const uint32 MaxBytes = (InstWords - StartWord) * sizeof(uint32);
    
    uint32 i = 0;
    for (; i < MaxBytes && i < (BufSize - 1); ++i)
    {
        OutBuf[i] = Src[i];
        if (Src[i] == '\0')
        {
            return true;
        }
    }

    OutBuf[i < BufSize ? i : BufSize - 1] = '\0';
    return false;
}

static constexpr uint32 SpvMakeInstructionHeader(uint16 OpCode, uint16 InstWords)
{
    return (static_cast<uint32>(InstWords) << 16) | static_cast<uint32>(OpCode);
}

static bool SpvIsAnnotationOrDebugName(uint16 OpCode)
{
    switch (OpCode)
    {
        case SpirvOps::OpName:
        case SpirvOps::OpMemberName:
        case SpirvOps::OpDecorate:
        case SpirvOps::OpMemberDecorate:
        case SpirvOps::OpDecorateId:
        case SpirvOps::OpDecorateString:
        case SpirvOps::OpMemberDecorateString:
            return true;

        default:
            return false;
    }
}

static bool SpvIsMergeableTypeDeclaration(uint16 OpCode)
{
    switch (OpCode)
    {
        case SpirvOps::OpTypeVector:
        case SpirvOps::OpTypeMatrix:
        case SpirvOps::OpTypeImage:
        case SpirvOps::OpTypeSampler:
        case SpirvOps::OpTypeSampledImage:
        case SpirvOps::OpTypePointer:
        case SpirvOps::OpTypeFunction:
            return true;

        default:
            return false;
    }
}

static bool SpvIsDebugInstruction(uint16 OpCode)
{
    switch (OpCode)
    {
        case SpirvOps::OpSourceContinued:
        case SpirvOps::OpSource:
        case SpirvOps::OpSourceExtension:
        case SpirvOps::OpName:
        case SpirvOps::OpMemberName:
        case SpirvOps::OpString:
        case SpirvOps::OpLine:
        case SpirvOps::OpNoLine:
        case SpirvOps::OpModuleProcessed:
            return true;

        default:
            return false;
    }
}

static void SpvRemapId(uint32& InOutId, const TMap<uint32, uint32>& IdRemap)
{
    if (const uint32* Remapped = IdRemap.Find(InOutId))
    {
        InOutId = *Remapped;
    }
}

static void SpvRemapTypeOperands(TArray<uint32>& Instruction, const TMap<uint32, uint32>& IdRemap)
{
    const uint16 OpCode   = static_cast<uint16>(Instruction[0] & 0xFFFFu);
    const int32  NumWords = Instruction.Size();

    bool bHasResult     = false;
    bool bHasResultType = false;
    SpvHasResultAndType(static_cast<SpvOp>(OpCode), &bHasResult, &bHasResultType);

    if (bHasResultType && NumWords >= 2)
    {
        SpvRemapId(Instruction[1], IdRemap);
    }

    switch (OpCode)
    {
        case SpirvOps::OpTypeVector:
        case SpirvOps::OpTypeMatrix:
        case SpirvOps::OpTypeSampledImage:
        case SpirvOps::OpTypeArray:
        case SpirvOps::OpTypeRuntimeArray:
        {
            if (NumWords >= 3)
            {
                SpvRemapId(Instruction[2], IdRemap);
            }
            
            break;
        }

        case SpirvOps::OpTypePointer:
        {
            if (NumWords >= 4)
            {
                SpvRemapId(Instruction[3], IdRemap);
            }

            break;
        }

        case SpirvOps::OpTypeStruct:
        case SpirvOps::OpTypeFunction:
        {
            for (int32 WordIndex = 2; WordIndex < NumWords; ++WordIndex)
            {
                SpvRemapId(Instruction[WordIndex], IdRemap);
            }

            break;
        }

        default:
            break;
    }
}

static uint64 SpvHashTypeDeclaration(const TArray<uint32>& Instruction)
{
    uint64 Hash = CRC32::Generate(Instruction.Data(), sizeof(uint32));
    if (Instruction.Size() > 2)
    {
        const uint64 OperandBytes = static_cast<uint64>(Instruction.Size() - 2) * sizeof(uint32);
        HashCombine(Hash, CRC32::Generate(Instruction.Data() + 2, OperandBytes));
    }

    return Hash;
}

static bool SpvTypeDeclarationsMatch(const uint32* Existing, const TArray<uint32>& Candidate)
{
    if (Existing[0] != Candidate[0])
    {
        return false;
    }

    for (int32 WordIndex = 2; WordIndex < Candidate.Size(); ++WordIndex)
    {
        if (Existing[WordIndex] != Candidate[WordIndex])
        {
            return false;
        }
    }

    return true;
}

static bool IsOneOfGoogleExtensions(const CHAR* Name)
{
    return (CString::Strcmp(Name, "SPV_GOOGLE_decorate_string") == 0) || (CString::Strcmp(Name, "SPV_GOOGLE_hlsl_functionality1") == 0) || 
        (CString::Strcmp(Name, "SPV_GOOGLE_user_type") == 0);
}

static bool IsGoogleDecorateStringDecoration(uint32 DecorationId)
{
    return DecorationId == SpirvOps::DecorationHlslSemanticGOOGLE || DecorationId == SpirvOps::DecorationUserTypeGOOGLE;
}

static bool IsGoogleDecorateIdDecoration(uint32 DecorationId)
{
    return DecorationId == SpirvOps::DecorationHlslCounterBufferGOOGLE;
}

bool FSpirvTransforms::PrepareForVulkan(const TArray<uint32>& InWords, TArray<uint32>& OutWords, String* OutError)
{
    TArray<uint32> StrippedWords;
    if (!StripGoogleSpirvRequirements(InWords, StrippedWords))
    {
        if (OutError)
        {
            *OutError = "Failed to strip the Google SPIR-V requirements";
        }

        return false;
    }

    String GoogleValidationError;
    if (!ValidateNoGoogleSpirvRequirements(StrippedWords, &GoogleValidationError))
    {
        if (OutError)
        {
            *OutError = String::Printf("Google SPIR-V requirements remain after stripping: %s", *GoogleValidationError);
        }

        return false;
    }

    TArray<uint32> RewrittenWords;
    bool bRewroteFormats = false;
    if (!ForceUnknownStorageImageFormats(StrippedWords, RewrittenWords, bRewroteFormats))
    {
        if (OutError)
        {
            *OutError = "Failed to rewrite the storage-image formats";
        }

        return false;
    }

    if (!bRewroteFormats)
    {
        OutWords = ::Move(RewrittenWords);
        return true;
    }

    if (!MergeDuplicateTypeDeclarations(RewrittenWords, OutWords))
    {
        if (OutError)
        {
            *OutError = "Failed to merge the duplicate type declarations";
        }

        return false;
    }

    return true;
}

bool FSpirvTransforms::StripDebugInstructions(const TArray<uint32>& InWords, TArray<uint32>& OutWords)
{
    OutWords.Clear();

    if (InWords.Size() < 5)
    {
        return false;
    }

    OutWords.Reserve(InWords.Size());
    OutWords.Append(InWords.Data(), 5);

    const uint32* Words     = InWords.Data();
    const uint32  WordCount = static_cast<uint32>(InWords.Size());

    uint32 Read = 5;
    while (Read < WordCount)
    {
        const uint16 OpCode    = static_cast<uint16>(Words[Read] & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(Words[Read] >> 16);
        if (InstWords == 0 || (Read + InstWords) > WordCount)
        {
            return false;
        }

        if (!SpvIsDebugInstruction(OpCode))
        {
            OutWords.Append(Words + Read, InstWords);
        }

        Read += InstWords;
    }

    return true;
}

bool FSpirvTransforms::StripGoogleSpirvRequirements(const TArray<uint32>& InWords, TArray<uint32>& OutWords)
{
    OutWords.Clear();

    if (InWords.Size() < 5)
    {
        return false;
    }

    OutWords.Reserve(InWords.Size());

    for (uint32 i = 0; i < 5; ++i)
    {
        OutWords.Add(InWords[i]);
    }

    const uint32* Words     = InWords.Data();
    const uint32  WordCount = static_cast<uint32>(InWords.Size());

    uint32 Read = 5;
    while (Read < WordCount)
    {
        const uint32 FirstWord = Words[Read];
        const uint16 OpCode    = static_cast<uint16>(FirstWord & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(FirstWord >> 16);

        if (InstWords == 0 || (Read + InstWords) > WordCount)
        {
            return false;
        }

        const uint32* Inst = &Words[Read];

        bool bSkip = false;
        if (OpCode == SpirvOps::OpExtension)
        {
            CHAR ExtName[256] = {};
            if (SpvReadLiteralString(Inst, InstWords, 1, ExtName, sizeof(ExtName)))
            {
                if (IsOneOfGoogleExtensions(ExtName))
                {
                    bSkip = true;
                }
            }
        }

        if (!bSkip && (OpCode == SpirvOps::OpDecorateString || OpCode == SpirvOps::OpMemberDecorateString))
        {
            if (OpCode == SpirvOps::OpDecorateString && InstWords >= 3 && IsGoogleDecorateStringDecoration(Inst[2]))
            {
                bSkip = true;
            }
            else if (OpCode == SpirvOps::OpMemberDecorateString && InstWords >= 4 && IsGoogleDecorateStringDecoration(Inst[3]))
            {
                bSkip = true;
            }
        }

        if (!bSkip && (OpCode == SpirvOps::OpDecorate || OpCode == SpirvOps::OpDecorateId))
        {
            if (InstWords >= 3 && IsGoogleDecorateIdDecoration(Inst[2]))
            {
                bSkip = true;
            }
        }

        if (!bSkip && OpCode == SpirvOps::OpMemberDecorate)
        {
            if (InstWords >= 4 && IsGoogleDecorateIdDecoration(Inst[3]))
            {
                bSkip = true;
            }
        }

        if (!bSkip)
        {
            for (uint16 w = 0; w < InstWords; ++w)
            {
                OutWords.Add(Inst[w]);
            }
        }

        Read += InstWords;
    }

    return true;
}

bool FSpirvTransforms::ValidateNoGoogleSpirvRequirements(const TArray<uint32>& Words, String* OutErrorMessage)
{
    if (Words.Size() < 5)
    {
        return true;
    }

    const uint32* Data      = Words.Data();
    const uint32  WordCount = static_cast<uint32>(Words.Size());

    uint32 Read = 5;
    while (Read < WordCount)
    {
        const uint32 FirstWord = Data[Read];
        const uint16 OpCode    = static_cast<uint16>(FirstWord & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(FirstWord >> 16);

        if (InstWords == 0 || (Read + InstWords) > WordCount)
        {
            break;
        }

        const uint32* Inst = &Data[Read];
        if (OpCode == SpirvOps::OpExtension)
        {
            CHAR ExtName[256] = {};
            if (SpvReadLiteralString(Inst, InstWords, 1, ExtName, sizeof(ExtName)))
            {
                if (IsOneOfGoogleExtensions(ExtName))
                {
                    if (OutErrorMessage)
                    {
                        *OutErrorMessage = String::Printf("Found Google extension: %s", ExtName);
                    }

                    return false;
                }
            }
        }

        Read += InstWords;
    }

    return true;
}

bool FSpirvTransforms::ForceUnknownStorageImageFormats(const TArray<uint32>& InWords, TArray<uint32>& OutWords, bool& bOutRewroteFormats)
{
    OutWords.Clear();
    bOutRewroteFormats = false;

    if (InWords.Size() < 5)
    {
        return false;
    }

    const uint32* Words     = InWords.Data();
    const uint32  WordCount = static_cast<uint32>(InWords.Size());

    bool bNeedsRewrite          = false;
    bool bHasWriteWithoutFormat = false;
    bool bHasReadWithoutFormat  = false;

    uint32 Read = 5;
    while (Read < WordCount)
    {
        const uint16 OpCode    = static_cast<uint16>(Words[Read] & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(Words[Read] >> 16);

        if (InstWords == 0 || (Read + InstWords) > WordCount)
        {
            return false;
        }

        if (OpCode == SpirvOps::OpCapability && InstWords >= 2)
        {
            if (Words[Read + 1] == SpirvOps::CapabilityStorageImageWriteWithoutFormat)
            {
                bHasWriteWithoutFormat = true;
            }
            else if (Words[Read + 1] == SpirvOps::CapabilityStorageImageReadWithoutFormat)
            {
                bHasReadWithoutFormat = true;
            }
        }
        else if (OpCode == SpirvOps::OpTypeImage && InstWords >= SpirvOps::OpTypeImageMinWords)
        {
            if (Words[Read + SpirvOps::OpTypeImageSampledWord] == SpirvOps::ImageSampledStorage &&
                Words[Read + SpirvOps::OpTypeImageFormatWord]  != SpirvOps::ImageFormatUnknown)
            {
                bNeedsRewrite = true;
            }
        }

        Read += InstWords;
    }

    bOutRewroteFormats = bNeedsRewrite;

    if (!bNeedsRewrite)
    {
        OutWords = InWords;
        return true;
    }

    const bool bAddWriteWithoutFormat = !bHasWriteWithoutFormat;
    const bool bAddReadWithoutFormat  = !bHasReadWithoutFormat;

    OutWords.Reserve(InWords.Size() + 4);
    for (uint32 Index = 0; Index < 5; ++Index)
    {
        OutWords.Add(Words[Index]);
    }

    bool bWroteCapabilities = false;

    Read = 5;
    while (Read < WordCount)
    {
        const uint16 OpCode    = static_cast<uint16>(Words[Read] & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(Words[Read] >> 16);

        if (!bWroteCapabilities && OpCode != SpirvOps::OpCapability)
        {
            if (bAddWriteWithoutFormat)
            {
                OutWords.Add(SpvMakeInstructionHeader(SpirvOps::OpCapability, 2));
                OutWords.Add(SpirvOps::CapabilityStorageImageWriteWithoutFormat);
            }

            if (bAddReadWithoutFormat)
            {
                OutWords.Add(SpvMakeInstructionHeader(SpirvOps::OpCapability, 2));
                OutWords.Add(SpirvOps::CapabilityStorageImageReadWithoutFormat);
            }

            bWroteCapabilities = true;
        }

        const int32 InstStart = OutWords.Size();
        for (uint16 WordIndex = 0; WordIndex < InstWords; ++WordIndex)
        {
            OutWords.Add(Words[Read + WordIndex]);
        }

        if (OpCode == SpirvOps::OpTypeImage && InstWords >= SpirvOps::OpTypeImageMinWords &&
            OutWords[InstStart + SpirvOps::OpTypeImageSampledWord] == SpirvOps::ImageSampledStorage)
        {
            OutWords[InstStart + SpirvOps::OpTypeImageFormatWord] = SpirvOps::ImageFormatUnknown;
        }

        Read += InstWords;
    }

    return true;
}

bool FSpirvTransforms::MergeDuplicateTypeDeclarations(const TArray<uint32>& InWords, TArray<uint32>& OutWords)
{
    OutWords.Clear();

    if (InWords.Size() < 5)
    {
        return false;
    }

    const uint32* Words     = InWords.Data();
    const uint32  WordCount = static_cast<uint32>(InWords.Size());

    TMap<uint32, uint32> IdRemap;        // Merged-away result id -> surviving result id
    TMap<uint64, int32>  TypeSignatures; // Type signature -> offset of the surviving declaration in CanonicalWords

    TArray<uint32> CanonicalWords;
    TArray<uint32> Instruction;

    uint32 Read = 5;
    while (Read < WordCount)
    {
        const uint16 OpCode    = static_cast<uint16>(Words[Read] & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(Words[Read] >> 16);

        if (InstWords == 0 || (Read + InstWords) > WordCount)
        {
            return false;
        }

        if (SpvIsMergeableTypeDeclaration(OpCode) && InstWords >= 2)
        {
            Instruction.Clear();
            for (uint16 WordIndex = 0; WordIndex < InstWords; ++WordIndex)
            {
                Instruction.Add(Words[Read + WordIndex]);
            }

            SpvRemapTypeOperands(Instruction, IdRemap);

            const uint64 Signature = SpvHashTypeDeclaration(Instruction);

            // On a hash collision the exact comparison fails and the declaration is left alone
            const int32* ExistingOffset = TypeSignatures.Find(Signature);
            if (ExistingOffset && SpvTypeDeclarationsMatch(CanonicalWords.Data() + *ExistingOffset, Instruction))
            {
                IdRemap.Add(Instruction[1], CanonicalWords[*ExistingOffset + 1]);
            }
            else if (!ExistingOffset)
            {
                TypeSignatures.Add(Signature, CanonicalWords.Size());
                CanonicalWords.Append(Instruction);
            }
        }

        Read += InstWords;
    }

    if (IdRemap.IsEmpty())
    {
        OutWords = InWords;
        return true;
    }

    OutWords.Reserve(InWords.Size());
    for (uint32 Index = 0; Index < 5; ++Index)
    {
        OutWords.Add(Words[Index]);
    }

    Read = 5;
    while (Read < WordCount)
    {
        const uint16 OpCode    = static_cast<uint16>(Words[Read] & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(Words[Read] >> 16);

        const bool bTargetsMergedId = (InstWords >= 2) && IdRemap.Contains(Words[Read + 1]);
        if (bTargetsMergedId && (SpvIsAnnotationOrDebugName(OpCode) || SpvIsMergeableTypeDeclaration(OpCode)))
        {
            Read += InstWords;
            continue;
        }

        Instruction.Clear();
        for (uint16 WordIndex = 0; WordIndex < InstWords; ++WordIndex)
        {
            Instruction.Add(Words[Read + WordIndex]);
        }

        SpvRemapTypeOperands(Instruction, IdRemap);
        OutWords.Append(Instruction);

        Read += InstWords;
    }

    return true;
}
