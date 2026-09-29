#include "ShaderCompilerTests.h"

#include <Core/Memory/Memory.h>
#include <RHI/RHIDevice.h>
#include <ShaderCompiler/ShaderCompiler.h>

#include "TestCommon/TestMacros.h"

static constexpr uint32 MakeFourCC(CHAR A, CHAR B, CHAR C, CHAR D)
{
    return static_cast<uint32>(A) | (static_cast<uint32>(B) << 8) | (static_cast<uint32>(C) << 16) | (static_cast<uint32>(D) << 24);
}

static uint32 ReadUInt32(const TArray<uint8>& ByteCode, uint32 Offset)
{
    uint32 Value = 0;
    if (Offset + sizeof(uint32) <= static_cast<uint64>(ByteCode.Size()))
    {
        Memory::Memcpy(&Value, ByteCode.Data() + Offset, sizeof(uint32));
    }

    return Value;
}

/** @return Returns true if ByteCode is a DXBC container that holds a part with the FourCC */
static bool ContainerHasPart(const TArray<uint8>& ByteCode, uint32 PartFourCC)
{
    // Header: FourCC, 16 byte digest, 2x uint16 version, uint32 size, uint32 part count, followed by the part offsets
    constexpr uint32 PartCountOffset = 28;
    constexpr uint32 PartOffsetsOffset = 32;

    if (ReadUInt32(ByteCode, 0) != MakeFourCC('D', 'X', 'B', 'C') || ReadUInt32(ByteCode, 24) != static_cast<uint32>(ByteCode.Size()))
    {
        return false;
    }

    const uint32 NumParts = ReadUInt32(ByteCode, PartCountOffset);
    for (uint32 Index = 0; Index < NumParts; ++Index)
    {
        const uint32 PartOffset = ReadUInt32(ByteCode, PartOffsetsOffset + Index * sizeof(uint32));
        if (ReadUInt32(ByteCode, PartOffset) == PartFourCC)
        {
            return true;
        }
    }

    return false;
}

static bool HasExpectedSignature(const TArray<uint8>& ByteCode, EShaderOutputLanguage OutputLanguage)
{
    switch (OutputLanguage)
    {
        case EShaderOutputLanguage::DXIL:
        {
            return ContainerHasPart(ByteCode, MakeFourCC('D', 'X', 'I', 'L'));
        }
        case EShaderOutputLanguage::SPIRV:
        {
            return ReadUInt32(ByteCode, 0) == 0x07230203u;
        }
        case EShaderOutputLanguage::MSL:
        {
            const String Source(reinterpret_cast<const CHAR*>(ByteCode.Data()), ByteCode.Size());
            return Source.Contains("metal_stdlib");
        }
        default:
        {
            return false;
        }
    }
}

bool ShaderCompilerOutputLanguages_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Every RHI maps to the output language it consumes");
    TEST_EXPECT_EQ(FShaderCompiler::GetOutputLanguageForRHI(ERHIType::D3D12), EShaderOutputLanguage::DXIL);
    TEST_EXPECT_EQ(FShaderCompiler::GetOutputLanguageForRHI(ERHIType::Null), EShaderOutputLanguage::DXIL);
    TEST_EXPECT_EQ(FShaderCompiler::GetOutputLanguageForRHI(ERHIType::Vulkan), EShaderOutputLanguage::SPIRV);
    TEST_EXPECT_EQ(FShaderCompiler::GetOutputLanguageForRHI(ERHIType::Metal), EShaderOutputLanguage::MSL);

    TEST_SECTION("Without an RHI the compiler targets DXIL");
    TEST_EXPECT_EQ(FShaderCompiler::GetOutputLanguageBasedOnRHI(), EShaderOutputLanguage::DXIL);

    TEST_SECTION("The platform languages are supported and listed once");
    const TArray<EShaderOutputLanguage> Languages = FShaderCompiler::Get().GetSupportedOutputLanguages();
    TEST_EXPECT(!Languages.IsEmpty());

    for (int32 Index = 0; Index < Languages.Size(); ++Index)
    {
        TEST_EXPECT(FShaderCompiler::Get().IsOutputLanguageSupported(Languages[Index]));

        for (int32 Other = Index + 1; Other < Languages.Size(); ++Other)
        {
            TEST_EXPECT(Languages[Index] != Languages[Other]);
        }
    }

#if PLATFORM_WINDOWS
    TEST_EXPECT(Languages.Contains(EShaderOutputLanguage::DXIL));
    TEST_EXPECT(Languages.Contains(EShaderOutputLanguage::SPIRV));
    TEST_EXPECT(!Languages.Contains(EShaderOutputLanguage::MSL));
#elif PLATFORM_MACOS
    TEST_EXPECT(Languages.Contains(EShaderOutputLanguage::MSL));
#endif

    TEST_END();
}

bool ShaderCompilerCompileFromFile_Test()
{
    TEST_BEGIN();

    struct FShaderFileCase
    {
        const CHAR*  Filename;
        const CHAR*  EntryPoint;
        EShaderStage Stage;
    };

    const FShaderFileCase Cases[] =
    {
        { "Shaders/UserInterface.hlsl",     "VSMain", EShaderStage::Vertex  },
        { "Shaders/UserInterface.hlsl",     "PSMain", EShaderStage::Pixel   },
        { "Shaders/GenerateMipsTex2D.hlsl", "Main",   EShaderStage::Compute },
    };

    for (EShaderOutputLanguage OutputLanguage : FShaderCompiler::Get().GetSupportedOutputLanguages())
    {
        for (const FShaderFileCase& Case : Cases)
        {
            TEST_SECTION(ToString(OutputLanguage));

            FShaderCompileInfo CompileInfo(Case.EntryPoint, EShaderModel::SM_6_2, Case.Stage, TArrayView<FShaderDefine>(), OutputLanguage);

            TArray<uint8>  ByteCode;
            TArray<String> Dependencies;
            const bool bCompiled = FShaderCompiler::Get().CompileFromFile(Case.Filename, CompileInfo, ByteCode, &Dependencies);
            TEST_EXPECT(bCompiled);
            TEST_EXPECT(!ByteCode.IsEmpty());
            TEST_EXPECT(HasExpectedSignature(ByteCode, OutputLanguage));

            // The shader itself is always the first dependency
            TEST_EXPECT(!Dependencies.IsEmpty());
            TEST_EXPECT(!Dependencies.IsEmpty() && Dependencies[0].EndsWith(Case.Filename));
        }
    }

    TEST_END();
}

bool ShaderCompilerCompileFromSource_Test()
{
    TEST_BEGIN();

    const String Source =
        "RWStructuredBuffer<uint> Output : register(u0);\n"
        "[numthreads(64, 1, 1)]\n"
        "void Main(uint3 ThreadID : SV_DispatchThreadID)\n"
        "{\n"
        "    Output[ThreadID.x] = ThreadID.x * 2;\n"
        "}\n";

    for (EShaderOutputLanguage OutputLanguage : FShaderCompiler::Get().GetSupportedOutputLanguages())
    {
        TEST_SECTION(ToString(OutputLanguage));

        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute, TArrayView<FShaderDefine>(), OutputLanguage);

        TArray<uint8> ByteCode;
        TEST_EXPECT(FShaderCompiler::Get().CompileFromSource(Source, CompileInfo, ByteCode));
        TEST_EXPECT(HasExpectedSignature(ByteCode, OutputLanguage));
    }

    TEST_END();
}

bool ShaderCompilerCompileHash_Test()
{
    TEST_BEGIN();

    const String SourceFile = "Shaders/UserInterface.hlsl";

    TEST_SECTION("The same compile settings produce the same hash");
    FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, TArrayView<FShaderDefine>(), EShaderOutputLanguage::DXIL);
    const uint64 BaseHash = FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo);
    TEST_EXPECT_EQ(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo), BaseHash);

    TEST_SECTION("Each output language produces its own hash");
    const TArray<EShaderOutputLanguage> Languages = FShaderCompiler::Get().GetSupportedOutputLanguages();

    TArray<uint64> Hashes;
    for (EShaderOutputLanguage OutputLanguage : Languages)
    {
        CompileInfo.OutputLanguage = OutputLanguage;

        const uint64 Hash = FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo);
        TEST_EXPECT(!Hashes.Contains(Hash));
        Hashes.Add(Hash);
    }

    TEST_SECTION("Entry point, stage and defines change the hash");
    CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, TArrayView<FShaderDefine>(), EShaderOutputLanguage::DXIL);
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo) != BaseHash);

    FShaderDefine Defines[] = { FShaderDefine("TEST_DEFINE", "1") };
    CompileInfo = FShaderCompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, TArrayView<FShaderDefine>(Defines), EShaderOutputLanguage::DXIL);
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo) != BaseHash);

    TEST_END();
}
