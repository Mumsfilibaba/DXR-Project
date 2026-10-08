#include "ShaderCompilerTests.h"

#include <Core/Containers/Map.h>
#include <Core/Memory/Memory.h>
#include <Core/Templates/CString.h>
#include <RHI/RHI.h>
#include <ShaderCompiler/ShaderCompiler.h>
#include <ShaderCore/MSLShaderBindings.h>
#include <ShaderCore/ShaderCode.h>

#include "TestCommon/TestMacros.h"

static bool ReflectConstantBufferSlots(const TArray<uint8>& ShaderCode, TMap<String, uint32>& OutSlots)
{
    FShaderCodeView CodeView;
    if (!FShaderCodeReader::Read(ShaderCode, CodeView))
    {
        return false;
    }

    const TArrayView<const FShaderResourceBinding> Bindings = CodeView.GetBindings();
    for (int32 Index = 0; Index < Bindings.Size(); ++Index)
    {
        if (Bindings[Index].Type == EShaderResourceType::ConstantBuffer)
        {
            OutSlots.Add(String(CodeView.GetBindingName(Index)), Bindings[Index].Register);
        }
    }

    return true;
}

static bool CompileFile(const CHAR* Filename, const CHAR* EntryPoint, EShaderModel ShaderModel, EShaderStage Stage, EShaderOutputLanguage OutputLanguage, bool bDebugInfo, TArray<uint8>& OutShaderCode, const TArrayView<FShaderDefine>& Defines = TArrayView<FShaderDefine>())
{
    FShaderCompileInfo CompileInfo(EntryPoint, ShaderModel, Stage, OutputLanguage, Defines);
    CompileInfo.bDebugInfo = bDebugInfo;
    return FShaderCompiler::Get().CompileFromFile(Filename, CompileInfo, OutShaderCode);
}

static bool CompileSource(const String& Source, const CHAR* EntryPoint, EShaderModel ShaderModel, EShaderStage Stage, EShaderOutputLanguage OutputLanguage, bool bDebugInfo, TArray<uint8>& OutShaderCode)
{
    FShaderCompileInfo CompileInfo(EntryPoint, ShaderModel, Stage, OutputLanguage);
    CompileInfo.bDebugInfo = bDebugInfo;
    return FShaderCompiler::Get().CompileFromSource(Source, CompileInfo, OutShaderCode);
}

static constexpr uint32 MakeFourCC(CHAR A, CHAR B, CHAR C, CHAR D)
{
    return static_cast<uint32>(A) | (static_cast<uint32>(B) << 8) | (static_cast<uint32>(C) << 16) | (static_cast<uint32>(D) << 24);
}

static uint32 ReadUInt32(TArrayView<const uint8> ByteCode, uint32 Offset)
{
    uint32 Value = 0;
    if (Offset + sizeof(uint32) <= static_cast<uint64>(ByteCode.Size()))
    {
        Memory::Memcpy(&Value, ByteCode.Data() + Offset, sizeof(uint32));
    }

    return Value;
}

/** @return Returns true if ByteCode is a DXBC container that holds a part with the FourCC */
static bool ContainerHasPart(TArrayView<const uint8> ByteCode, uint32 PartFourCC)
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

static bool ReadPartUInt64(TArrayView<const uint8> ByteCode, uint32 PartFourCC, uint64& OutValue)
{
    constexpr uint32 PartCountOffset   = 28;
    constexpr uint32 PartOffsetsOffset = 32;

    const uint32 NumParts = ReadUInt32(ByteCode, PartCountOffset);
    for (uint32 Index = 0; Index < NumParts; ++Index)
    {
        const uint32 PartOffset = ReadUInt32(ByteCode, PartOffsetsOffset + Index * sizeof(uint32));
        if (ReadUInt32(ByteCode, PartOffset) == PartFourCC && ReadUInt32(ByteCode, PartOffset + 4) >= sizeof(uint64) && PartOffset + 8 + sizeof(uint64) <= static_cast<uint64>(ByteCode.Size()))
        {
            Memory::Memcpy(&OutValue, ByteCode.Data() + PartOffset + 8, sizeof(uint64));
            return true;
        }
    }

    return false;
}

static String GetMSLSource(TArrayView<const uint8> NativeCode)
{
    return String(reinterpret_cast<const CHAR*>(NativeCode.Data()), NativeCode.Size());
}

static bool HasExpectedSignature(const TArray<uint8>& ShaderCode, EShaderOutputLanguage OutputLanguage)
{
    FShaderCodeView CodeView;
    if (!FShaderCodeReader::Read(ShaderCode, CodeView) || CodeView.GetOutputLanguage() != OutputLanguage)
    {
        return false;
    }

    const TArrayView<const uint8> NativeCode = CodeView.GetNativeCode();
    switch (OutputLanguage)
    {
        case EShaderOutputLanguage::DXIL:
        {
            return ContainerHasPart(NativeCode, MakeFourCC('D', 'X', 'I', 'L'));
        }
        case EShaderOutputLanguage::DXBC:
        {
            return ContainerHasPart(NativeCode, MakeFourCC('S', 'H', 'E', 'X')) && !ContainerHasPart(NativeCode, MakeFourCC('D', 'X', 'I', 'L'));
        }
        case EShaderOutputLanguage::SPIRV:
        {
            return ReadUInt32(NativeCode, 0) == 0x07230203u;
        }
        case EShaderOutputLanguage::MSL:
        {
            return GetMSLSource(NativeCode).Contains("metal_stdlib");
        }
        default:
        {
            return false;
        }
    }
}

struct FSpirvScan
{
    bool           bValid               = false;
    bool           bHasDebugOpcodes     = false;
    bool           bHasDebugInfoImport  = false;
    bool           bHasGoogleExtensions = false;
    TArray<uint32> SetOperandOffsets;
    TArray<uint32> BindingOperandOffsets;
};

static FSpirvScan ScanSpirv(TArrayView<const uint8> NativeCode)
{
    constexpr uint16 OpExtension             = 10;
    constexpr uint16 OpExtInstImport         = 11;
    constexpr uint16 OpDecorate              = 71;
    constexpr uint16 OpDecorateId            = 332;
    constexpr uint16 OpDecorateString        = 5632;
    constexpr uint16 OpMemberDecorateString  = 5633;
    constexpr uint32 DecorationBinding       = 33;
    constexpr uint32 DecorationDescriptorSet = 34;

    FSpirvScan Scan;

    const uint32* Words    = reinterpret_cast<const uint32*>(NativeCode.Data());
    const uint32  NumWords = static_cast<uint32>(NativeCode.Size() / sizeof(uint32));
    if (NumWords < 5 || Words[0] != 0x07230203u)
    {
        return Scan;
    }

    const auto ReadString = [&](uint32 FirstWord, uint32 EndWord)
    {
        const CHAR*  Characters = reinterpret_cast<const CHAR*>(Words + FirstWord);
        const uint32 MaxLength  = (EndWord - FirstWord) * static_cast<uint32>(sizeof(uint32));

        uint32 Length = 0;
        while (Length < MaxLength && Characters[Length] != '\0')
        {
            ++Length;
        }

        return String(Characters, static_cast<int32>(Length));
    };

    for (uint32 Read = 5; Read < NumWords;)
    {
        const uint16 OpCode    = static_cast<uint16>(Words[Read] & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(Words[Read] >> 16);
        if (InstWords == 0 || Read + InstWords > NumWords)
        {
            return Scan;
        }

        if ((OpCode >= 2 && OpCode <= 8) || OpCode == 317 || OpCode == 330)
        {
            Scan.bHasDebugOpcodes = true;
        }

        if (OpCode == OpExtInstImport && InstWords > 2 && ReadString(Read + 2, Read + InstWords).Contains("NonSemantic.Shader.DebugInfo"))
        {
            Scan.bHasDebugInfoImport = true;
        }

        if (OpCode == OpExtension && InstWords > 1 && ReadString(Read + 1, Read + InstWords).Contains("SPV_GOOGLE"))
        {
            Scan.bHasGoogleExtensions = true;
        }

        if (OpCode == OpDecorateString || OpCode == OpMemberDecorateString)
        {
            Scan.bHasGoogleExtensions = true;
        }

        if ((OpCode == OpDecorate || OpCode == OpDecorateId) && InstWords >= 3 && Words[Read + 2] >= 5634 && Words[Read + 2] <= 5636)
        {
            Scan.bHasGoogleExtensions = true;
        }

        if (OpCode == OpDecorate && InstWords == 4)
        {
            if (Words[Read + 2] == DecorationDescriptorSet)
            {
                Scan.SetOperandOffsets.Add(Read + 3);
            }
            else if (Words[Read + 2] == DecorationBinding)
            {
                Scan.BindingOperandOffsets.Add(Read + 3);
            }
        }

        Read += InstWords;
    }

    Scan.bValid = true;
    return Scan;
}

struct FVertexInputExpectation
{
    const CHAR*          Semantic;
    uint8                SemanticIndex;
    EShaderComponentType ComponentType;
    uint8                NumComponents;
};

static const FShaderVertexInput* FindVertexInput(TArrayView<const FShaderVertexInput> VertexInputs, const CHAR* Semantic, uint8 SemanticIndex)
{
    const uint32 SemanticHash = HashShaderSemantic(Semantic);
    for (const FShaderVertexInput& VertexInput : VertexInputs)
    {
        if (VertexInput.SemanticHash == SemanticHash && VertexInput.SemanticIndex == SemanticIndex)
        {
            return &VertexInput;
        }
    }

    return nullptr;
}

bool ShaderCompilerOutputLanguages_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Every RHI maps to the output language it consumes");
    TEST_EXPECT_EQ(RHI::GetShaderOutputLanguage(ERHIType::D3D11), EShaderOutputLanguage::DXBC);
    TEST_EXPECT_EQ(RHI::GetShaderOutputLanguage(ERHIType::D3D12), EShaderOutputLanguage::DXIL);
    TEST_EXPECT_EQ(RHI::GetShaderOutputLanguage(ERHIType::Null), EShaderOutputLanguage::DXIL);
    TEST_EXPECT_EQ(RHI::GetShaderOutputLanguage(ERHIType::Vulkan), EShaderOutputLanguage::SPIRV);
    TEST_EXPECT_EQ(RHI::GetShaderOutputLanguage(ERHIType::Metal), EShaderOutputLanguage::MSL);

    TEST_SECTION("Without an RHI the compiler targets DXIL");
    TEST_EXPECT_EQ(RHI::GetShaderOutputLanguage(), EShaderOutputLanguage::DXIL);

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
    TEST_EXPECT(Languages.Contains(EShaderOutputLanguage::DXBC));
    TEST_EXPECT(Languages.Contains(EShaderOutputLanguage::SPIRV));
    TEST_EXPECT(!Languages.Contains(EShaderOutputLanguage::MSL));
#elif PLATFORM_MACOS
    TEST_EXPECT(Languages.Contains(EShaderOutputLanguage::MSL));
    TEST_EXPECT(!Languages.Contains(EShaderOutputLanguage::DXBC));
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

            FShaderCompileInfo CompileInfo(Case.EntryPoint, EShaderModel::SM_6_2, Case.Stage, OutputLanguage);

            TArray<uint8>  ShaderCode;
            TArray<String> Dependencies;
            const bool bCompiled = FShaderCompiler::Get().CompileFromFile(Case.Filename, CompileInfo, ShaderCode, &Dependencies);
            TEST_EXPECT(bCompiled);
            TEST_EXPECT(!ShaderCode.IsEmpty());
            TEST_EXPECT(HasExpectedSignature(ShaderCode, OutputLanguage));

            FShaderCodeHeader Header;
            TEST_EXPECT(FShaderCodeReader::ReadHeader(ShaderCode, Header));
            TEST_EXPECT(Header.Stage == Case.Stage);

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

        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute, OutputLanguage);

        TArray<uint8> ShaderCode;
        TEST_EXPECT(FShaderCompiler::Get().CompileFromSource(Source, CompileInfo, ShaderCode));
        TEST_EXPECT(HasExpectedSignature(ShaderCode, OutputLanguage));

        FShaderCodeView CodeView;
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
        TEST_EXPECT_EQ(CodeView.GetBindings().Size(), 1);
        TEST_EXPECT(CodeView.GetBindings().Size() == 1 && CodeView.GetBindings()[0].Type == EShaderResourceType::RWStructuredBuffer);
    }

    TEST_END();
}

bool ShaderCompilerCompileFailure_Test()
{
    TEST_BEGIN();

    const String BrokenSource = "float4 Main() : SV_Target { return MissingValue; }\n";
    const String ValidSource  = "float4 Main() : SV_Target { return float4(1.0, 0.0, 0.0, 1.0); }\n";

    for (EShaderOutputLanguage OutputLanguage : FShaderCompiler::Get().GetSupportedOutputLanguages())
    {
        TEST_SECTION(ToString(OutputLanguage));

        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel, OutputLanguage);

        TArray<uint8> ShaderCode;
        TEST_EXPECT(!FShaderCompiler::Get().CompileFromSource(BrokenSource, CompileInfo, ShaderCode));
        TEST_EXPECT(ShaderCode.IsEmpty());
    }

    if (FShaderCompiler::Get().IsOutputLanguageSupported(EShaderOutputLanguage::DXBC))
    {
        TEST_SECTION("DXBC is limited to what Shader Model 5.0 can express");

        TArray<uint8> ShaderCode;
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel, EShaderOutputLanguage::DXBC);
        TEST_EXPECT(FShaderCompiler::Get().CompileFromSource(ValidSource, CompileInfo, ShaderCode));

        CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_6, EShaderStage::Pixel, EShaderOutputLanguage::DXBC);
        TEST_EXPECT(!FShaderCompiler::Get().CompileFromSource(ValidSource, CompileInfo, ShaderCode));

        CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::RayGen, EShaderOutputLanguage::DXBC);
        TEST_EXPECT(!FShaderCompiler::Get().CompileFromSource(ValidSource, CompileInfo, ShaderCode));
    }

    TEST_END();
}

bool ShaderCompilerDXBCConstantsSlot_Test()
{
    TEST_BEGIN();

#if PLATFORM_WINDOWS
    if (FShaderCompiler::Get().IsOutputLanguageSupported(EShaderOutputLanguage::DXBC))
    {
        const auto CompileAndReflect = [](const String& Source, TMap<String, uint32>& OutSlots)
        {
            TArray<uint8> ShaderCode;
            return CompileSource(Source, "Main", EShaderModel::SM_6_0, EShaderStage::Pixel, EShaderOutputLanguage::DXBC, true, ShaderCode) && ReflectConstantBufferSlots(ShaderCode, OutSlots);
        };

        const String Declarations =
            "struct FFirst  { float4 A; };\n"
            "struct FSecond { float4 B; };\n"
            "struct FShaderBlockConstants { float4 C; };\n"
            "ConstantBuffer<FShaderBlockConstants> Constants : register(b0, space1);\n";

        TEST_SECTION("The shader constants take the slot after the constant buffers the shader declares");
        {
            const String Source = Declarations +
                "ConstantBuffer<FFirst>  First  : register(b0);\n"
                "ConstantBuffer<FSecond> Second : register(b1);\n"
                "float4 Main() : SV_Target { return First.A + Second.B + Constants.C; }\n";

            TMap<String, uint32> Slots;
            TEST_EXPECT(CompileAndReflect(Source, Slots));
            TEST_EXPECT(Slots.Find("First_CB") && *Slots.Find("First_CB") == 0);
            TEST_EXPECT(Slots.Find("Second_CB") && *Slots.Find("Second_CB") == 1);
            TEST_EXPECT(Slots.Find("Constants_CB") && *Slots.Find("Constants_CB") == 2);
        }

        TEST_SECTION("A gap in the declared slots is filled first");
        {
            const String Source = Declarations +
                "ConstantBuffer<FFirst>  First  : register(b0);\n"
                "ConstantBuffer<FSecond> Second : register(b2);\n"
                "float4 Main() : SV_Target { return First.A + Second.B + Constants.C; }\n";

            TMap<String, uint32> Slots;
            TEST_EXPECT(CompileAndReflect(Source, Slots));
            TEST_EXPECT(Slots.Find("Constants_CB") && *Slots.Find("Constants_CB") == 1);
        }

        TEST_SECTION("The shader constants are their own binding space, with their size");
        {
            const String Source = Declarations + "float4 Main() : SV_Target { return Constants.C; }\n";

            TArray<uint8> ShaderCode;
            TEST_EXPECT(CompileSource(Source, "Main", EShaderModel::SM_6_0, EShaderStage::Pixel, EShaderOutputLanguage::DXBC, false, ShaderCode));

            FShaderCodeView CodeView;
            TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
            TEST_EXPECT_EQ(CodeView.GetInfo().ShaderConstantsSize, 16);
            TEST_EXPECT(CodeView.GetBindings().Size() == 1 && CodeView.GetBindings()[0].Space == EShaderBindingSpace::ShaderConstants);
        }

        TEST_SECTION("An engine shader never shares a slot with its own constant buffers");
        {
            TArray<uint8> ShaderCode;
            TMap<String, uint32> Slots;
            TEST_EXPECT(CompileFile("Shaders/UserInterface.hlsl", "VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, EShaderOutputLanguage::DXBC, true, ShaderCode));
            TEST_EXPECT(ReflectConstantBufferSlots(ShaderCode, Slots));
            TEST_EXPECT(Slots.Find("Constants_CB") != nullptr);

            TArray<uint32> UsedSlots;
            Slots.Foreach([&](const String&, uint32 Slot)
            {
                TEST_EXPECT(!UsedSlots.Contains(Slot));
                UsedSlots.Add(Slot);
            });
        }
    }
#endif

    TEST_END();
}

bool ShaderCompilerCompileHash_Test()
{
    TEST_BEGIN();

    const String SourceFile = "Shaders/UserInterface.hlsl";

    TEST_SECTION("The same compile settings produce the same hash");
    FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, EShaderOutputLanguage::DXIL);
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
    CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, EShaderOutputLanguage::DXIL);
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo) != BaseHash);

    FShaderDefine Defines[] = { FShaderDefine("TEST_DEFINE", "1") };
    CompileInfo = FShaderCompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, EShaderOutputLanguage::DXIL, TArrayView<FShaderDefine>(Defines));
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo) != BaseHash);

    TEST_SECTION("Debug and release builds have their own hash");
    CompileInfo = FShaderCompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, EShaderOutputLanguage::DXIL);
    CompileInfo.bDebugInfo = !CompileInfo.bDebugInfo;
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo) != BaseHash);

    TEST_END();
}

bool ShaderCompilerReleaseOutput_Test()
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
        { "Shaders/UserInterface.hlsl",       "VSMain", EShaderStage::Vertex  },
        { "Shaders/UserInterface.hlsl",       "PSMain", EShaderStage::Pixel   },
        { "Shaders/UserInterface.hlsl",       "VSText", EShaderStage::Vertex  },
        { "Shaders/ImGui.hlsl",               "VSMain", EShaderStage::Vertex  },
        { "Shaders/ImGui.hlsl",               "PSMain", EShaderStage::Pixel   },
        { "Shaders/GenerateMipsTex2D.hlsl",   "Main",   EShaderStage::Compute },
        { "Shaders/GenerateMipsTexCube.hlsl", "Main",   EShaderStage::Compute },
    };

    for (EShaderOutputLanguage OutputLanguage : FShaderCompiler::Get().GetSupportedOutputLanguages())
    {
        for (const FShaderFileCase& Case : Cases)
        {
            TEST_SECTION(ToString(OutputLanguage));

            TArray<uint8> ReleaseCode;
            TArray<uint8> DebugCode;
            const bool bCompiledRelease = CompileFile(Case.Filename, Case.EntryPoint, EShaderModel::SM_6_2, Case.Stage, OutputLanguage, false, ReleaseCode);
            const bool bCompiledDebug   = CompileFile(Case.Filename, Case.EntryPoint, EShaderModel::SM_6_2, Case.Stage, OutputLanguage, true, DebugCode);
            TEST_EXPECT(bCompiledRelease);
            TEST_EXPECT(bCompiledDebug);
            if (!bCompiledRelease || !bCompiledDebug)
            {
                continue;
            }

            FShaderCodeView Release;
            FShaderCodeView Debug;
            TEST_EXPECT(FShaderCodeReader::Read(ReleaseCode, Release));
            TEST_EXPECT(FShaderCodeReader::Read(DebugCode, Debug));

            TEST_SECTION("Release containers have no debug info and no names");
            TEST_EXPECT(!Release.HasDebugInfo());
            TEST_EXPECT(Debug.HasDebugInfo());
            for (int32 Index = 0; Index < Release.GetBindings().Size(); ++Index)
            {
                TEST_EXPECT(Release.GetBindingName(Index)[0] == '\0');
                TEST_EXPECT(Debug.GetBindingName(Index)[0] != '\0');
            }

            TEST_SECTION("Debug and release reflect the same bindings");
            TEST_EXPECT_EQ(Release.GetBindings().Size(), Debug.GetBindings().Size());
            TEST_EXPECT_EQ(Release.GetVertexInputs().Size(), Debug.GetVertexInputs().Size());
            TEST_EXPECT_EQ(Release.GetInfo().ShaderConstantsSize, Debug.GetInfo().ShaderConstantsSize);
            for (int32 Index = 0; Index < Release.GetBindings().Size() && Index < Debug.GetBindings().Size(); ++Index)
            {
                TEST_EXPECT(Release.GetBindings()[Index].Type == Debug.GetBindings()[Index].Type);
                TEST_EXPECT(Release.GetBindings()[Index].Register == Debug.GetBindings()[Index].Register);
                TEST_EXPECT(Release.GetBindings()[Index].Space == Debug.GetBindings()[Index].Space);
            }

            TEST_SECTION("The release build is smaller than the debug build");
            TEST_EXPECT(Release.GetNativeCode().Size() < Debug.GetNativeCode().Size());

            const TArrayView<const uint8> NativeCode = Release.GetNativeCode();
            if (OutputLanguage == EShaderOutputLanguage::DXIL)
            {
                TEST_SECTION("Release DXIL has no debug, statistics or private parts");
                TEST_EXPECT(ContainerHasPart(NativeCode, MakeFourCC('D', 'X', 'I', 'L')));
                TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('I', 'L', 'D', 'B')));
                TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('I', 'L', 'D', 'N')));
                TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('S', 'T', 'A', 'T')));
                TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('P', 'R', 'I', 'V')));
            }
            else if (OutputLanguage == EShaderOutputLanguage::DXBC)
            {
                TEST_SECTION("Release DXBC has no reflection or debug chunks, but keeps the input signature");
                TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('R', 'D', 'E', 'F')));
                TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('S', 'T', 'A', 'T')));
                TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('S', 'D', 'B', 'G')));
                TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('S', 'P', 'D', 'B')));
                TEST_EXPECT(ContainerHasPart(NativeCode, MakeFourCC('I', 'S', 'G', 'N')) || ContainerHasPart(NativeCode, MakeFourCC('I', 'S', 'G', '1')));
            }
            else if (OutputLanguage == EShaderOutputLanguage::SPIRV)
            {
                const FSpirvScan Scan = ScanSpirv(NativeCode);

                TEST_SECTION("Release SPIR-V has no debug instructions and no Google extensions");
                TEST_EXPECT(Scan.bValid);
                TEST_EXPECT(!Scan.bHasDebugOpcodes);
                TEST_EXPECT(!Scan.bHasDebugInfoImport);
                TEST_EXPECT(!Scan.bHasGoogleExtensions);

                TEST_SECTION("Every SPIR-V offset points at a DescriptorSet or Binding operand");
                const TArrayView<const FSpirvBindingOffsets> SpirvOffsets = Release.GetSpirvOffsets();
                TEST_EXPECT_EQ(SpirvOffsets.Size(), Release.GetBindings().Size());
                for (const FSpirvBindingOffsets& Offsets : SpirvOffsets)
                {
                    TEST_EXPECT(Scan.SetOperandOffsets.Contains(Offsets.SetWordOffset));
                    TEST_EXPECT(Scan.BindingOperandOffsets.Contains(Offsets.BindingWordOffset));
                }

                TEST_SECTION("SPIR-V always names its entry point");
                TEST_EXPECT(CString::Strcmp(Release.GetEntryPoint(), Case.EntryPoint) == 0);
            }
        }
    }

    TEST_END();
}

bool ShaderCompilerVertexInputs_Test()
{
    TEST_BEGIN();

    const FVertexInputExpectation Expected[] =
    {
        { "POSITION", 0, EShaderComponentType::Float32, 2 },
        { "TEXCOORD", 0, EShaderComponentType::Float32, 2 },
        { "COLOR",    0, EShaderComponentType::Float32, 4 },
    };

    const CHAR* const Filenames[] = { "Shaders/UserInterface.hlsl", "Shaders/ImGui.hlsl" };

    const EShaderOutputLanguage Languages[] = { EShaderOutputLanguage::DXIL, EShaderOutputLanguage::DXBC, EShaderOutputLanguage::SPIRV };

    for (const CHAR* Filename : Filenames)
    {
        TArray<uint8> FirstLocations;
        for (EShaderOutputLanguage OutputLanguage : Languages)
        {
            if (!FShaderCompiler::Get().IsOutputLanguageSupported(OutputLanguage))
            {
                continue;
            }

            TEST_SECTION(ToString(OutputLanguage));

            TArray<uint8> ShaderCode;
            TEST_EXPECT(CompileFile(Filename, "VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, OutputLanguage, false, ShaderCode));

            FShaderCodeView CodeView;
            TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));

            const TArrayView<const FShaderVertexInput> VertexInputs = CodeView.GetVertexInputs();
            TEST_EXPECT_EQ(VertexInputs.Size(), 3);

            TArray<uint8> Locations;
            for (const FVertexInputExpectation& Expectation : Expected)
            {
                const FShaderVertexInput* VertexInput = FindVertexInput(VertexInputs, Expectation.Semantic, Expectation.SemanticIndex);
                TEST_EXPECT(VertexInput != nullptr);
                if (VertexInput)
                {
                    TEST_EXPECT(VertexInput->ComponentType == Expectation.ComponentType);
                    TEST_EXPECT_EQ(VertexInput->NumComponents, Expectation.NumComponents);
                    Locations.Add(VertexInput->Location);
                }
            }

            TEST_SECTION("Every language reports the same locations");
            if (FirstLocations.IsEmpty())
            {
                FirstLocations = Locations;
            }
            else
            {
                TEST_EXPECT(FirstLocations == Locations);
            }

            TEST_SECTION("The inputs are sorted by location");
            for (int32 Index = 1; Index < VertexInputs.Size(); ++Index)
            {
                TEST_EXPECT(VertexInputs[Index - 1].Location < VertexInputs[Index].Location);
            }
        }
    }

    TEST_SECTION("System values are not vertex inputs");
    for (EShaderOutputLanguage OutputLanguage : Languages)
    {
        if (!FShaderCompiler::Get().IsOutputLanguageSupported(OutputLanguage))
        {
            continue;
        }

        TArray<uint8> ShaderCode;
        TEST_EXPECT(CompileFile("Shaders/UserInterface.hlsl", "VSText", EShaderModel::SM_6_2, EShaderStage::Vertex, OutputLanguage, false, ShaderCode));

        FShaderCodeView CodeView;
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
        TEST_EXPECT_EQ(CodeView.GetVertexInputs().Size(), 5);
        TEST_EXPECT(FindVertexInput(CodeView.GetVertexInputs(), "SV_VertexID", 0) == nullptr);
        TEST_EXPECT(FindVertexInput(CodeView.GetVertexInputs(), "TEXCOORD", 2) != nullptr);
    }

    TEST_SECTION("Semantics are case-insensitive");
    TEST_EXPECT_EQ(HashShaderSemantic("texcoord"), HashShaderSemantic("TEXCOORD"));

    TEST_END();
}

bool ShaderCompilerRayTracing_Test()
{
    TEST_BEGIN();

    struct FRayTracingCase
    {
        const CHAR*  Filename;
        const CHAR*  EntryPoint;
        EShaderStage Stage;
    };

    const FRayTracingCase Cases[] =
    {
        { "Shaders/RayGen.hlsl",     "RayGen",     EShaderStage::RayGen        },
        { "Shaders/Miss.hlsl",       "Miss",       EShaderStage::RayMiss       },
        { "Shaders/ClosestHit.hlsl", "ClosestHit", EShaderStage::RayClosestHit },
    };

    struct FPermutation
    {
        bool         bBindless;
        bool         bSER;
        EShaderModel ShaderModel;
    };

    const FPermutation Permutations[] =
    {
        { false, false, EShaderModel::SM_6_3 },
        { true,  false, EShaderModel::SM_6_6 },
        { true,  true,  EShaderModel::SM_6_9 },
    };

    const EShaderOutputLanguage Languages[] = { EShaderOutputLanguage::DXIL, EShaderOutputLanguage::SPIRV };

    for (EShaderOutputLanguage OutputLanguage : Languages)
    {
        if (!FShaderCompiler::Get().IsOutputLanguageSupported(OutputLanguage))
        {
            continue;
        }

        for (const FRayTracingCase& Case : Cases)
        {
            for (const FPermutation& Permutation : Permutations)
            {
                if (Permutation.bSER && OutputLanguage != EShaderOutputLanguage::DXIL)
                {
                    continue;
                }

                TEST_SECTION(ToString(OutputLanguage));

                FShaderDefine Defines[] =
                {
                    FShaderDefine("ENABLE_BINDLESS", Permutation.bBindless ? "1" : "0"),
                    FShaderDefine("RAY_TRACING_SHADER_EXECUTION_REORDERING", Permutation.bSER ? "1" : "0"),
                };

                TArray<uint8> ShaderCode;
                const bool bCompiled = CompileFile(Case.Filename, Case.EntryPoint, Permutation.ShaderModel, Case.Stage, OutputLanguage, false, ShaderCode, TArrayView<FShaderDefine>(Defines));
                TEST_EXPECT(bCompiled);
                if (!bCompiled)
                {
                    LOG_ERROR("    %s (%s), bindless=%d, ser=%d", Case.EntryPoint, ToString(OutputLanguage), Permutation.bBindless, Permutation.bSER);
                    continue;
                }

                FShaderCodeView CodeView;
                TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));

                TEST_SECTION("Ray-tracing shaders report their demangled export name");
                TEST_EXPECT(CString::Strcmp(CodeView.GetEntryPoint(), Case.EntryPoint) == 0);

                TEST_SECTION("Only the non-bindless closest-hit shader has local SRVs");
                uint32 NumLocalBindings = 0;
                for (const FShaderResourceBinding& Binding : CodeView.GetBindings())
                {
                    NumLocalBindings += Binding.Space == EShaderBindingSpace::RayTracingLocal ? 1 : 0;
                }

                if (Case.Stage == EShaderStage::RayClosestHit && !Permutation.bBindless)
                {
                    bool bHasAttributes = false;
                    bool bHasIndices    = false;
                    for (const FShaderResourceBinding& Binding : CodeView.GetBindings())
                    {
                        if (Binding.Space == EShaderBindingSpace::RayTracingLocal)
                        {
                            bHasAttributes |= Binding.Register == 0 && Binding.Type == EShaderResourceType::StructuredBuffer;
                            bHasIndices    |= Binding.Register == 1 && Binding.Type == EShaderResourceType::ByteAddressBuffer;
                        }
                    }

                    TEST_EXPECT(bHasAttributes);

                    // SPIR-V cannot tell a ByteAddressBuffer from a StructuredBuffer
                    TEST_EXPECT(bHasIndices || OutputLanguage == EShaderOutputLanguage::SPIRV);
                    TEST_EXPECT(NumLocalBindings >= 2);
                }
                else
                {
                    TEST_EXPECT_EQ(NumLocalBindings, 0u);
                }

                if (OutputLanguage == EShaderOutputLanguage::DXIL)
                {
                    const TArrayView<const uint8> NativeCode = CodeView.GetNativeCode();

                    TEST_SECTION("Release DXIL libraries keep RDAT and drop the debug, statistics and private parts");
                    TEST_EXPECT(ContainerHasPart(NativeCode, MakeFourCC('R', 'D', 'A', 'T')));
                    TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('S', 'T', 'A', 'T')));
                    TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('I', 'L', 'D', 'B')));
                    TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('I', 'L', 'D', 'N')));
                    TEST_EXPECT(!ContainerHasPart(NativeCode, MakeFourCC('P', 'R', 'I', 'V')));

                    // Libraries keep their per-function flags in RDAT, so the SFI0 part decides, as it did before the container
                    TEST_SECTION("The required features match the SFI0 part");
                    uint64 FeatureInfo = 0;
                    TEST_EXPECT(ReadPartUInt64(NativeCode, MakeFourCC('S', 'F', 'I', '0'), FeatureInfo));

                    constexpr uint64 HeapIndexingMask = 0x02000000;
                    constexpr uint64 WaveOpsMask      = 0x00004000;
                    TEST_EXPECT(((FeatureInfo & HeapIndexingMask) != 0) == IsEnumFlagSet(CodeView.GetInfo().RequiredFeatures, EShaderFeatureFlags::RequiresResourceDescriptorHeapIndexing));
                    TEST_EXPECT(((FeatureInfo & WaveOpsMask) != 0) == IsEnumFlagSet(CodeView.GetInfo().RequiredFeatures, EShaderFeatureFlags::RequiresWaveOps));
                }
            }
        }
    }

    TEST_SECTION("A library with two functions reflects the one named in the entry point");
    {
        const String Source =
            "struct FPayload { float4 Color; };\n"
            "Texture2D<float4> First  : register(t0);\n"
            "Texture2D<float4> Second : register(t1);\n"
            "[shader(\"miss\")]\n"
            "void MissA(inout FPayload Payload) { Payload.Color = First.Load(int3(0, 0, 0)); }\n"
            "[shader(\"miss\")]\n"
            "void MissB(inout FPayload Payload) { Payload.Color = Second.Load(int3(0, 0, 0)); }\n";

        for (EShaderOutputLanguage OutputLanguage : Languages)
        {
            if (!FShaderCompiler::Get().IsOutputLanguageSupported(OutputLanguage))
            {
                continue;
            }

            TArray<uint8> ShaderCode;
            TEST_EXPECT(CompileSource(Source, "MissB", EShaderModel::SM_6_3, EShaderStage::RayMiss, OutputLanguage, false, ShaderCode));

            FShaderCodeView CodeView;
            TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
            TEST_EXPECT(CString::Strcmp(CodeView.GetEntryPoint(), "MissB") == 0);

            // DXIL reflects each library function on its own
            if (OutputLanguage == EShaderOutputLanguage::DXIL)
            {
                TEST_EXPECT(CodeView.GetBindings().Size() == 1 && CodeView.GetBindings()[0].Register == 1);
            }
        }
    }

    TEST_END();
}

bool ShaderCompilerMSL_Test()
{
    TEST_BEGIN();

    // MSL is not a platform language on Windows, but DXC and SPIRV-Cross both run there
    if (!FShaderCompiler::Get().IsOutputLanguageSupported(EShaderOutputLanguage::MSL))
    {
        TEST_END();
    }

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
        { "Shaders/ImGui.hlsl",             "VSMain", EShaderStage::Vertex  },
        { "Shaders/ImGui.hlsl",             "PSMain", EShaderStage::Pixel   },
        { "Shaders/GenerateMipsTex2D.hlsl", "Main",   EShaderStage::Compute },
    };

    for (const FShaderFileCase& Case : Cases)
    {
        TEST_SECTION(Case.Filename);

        TArray<uint8> ReleaseCode;
        TArray<uint8> DebugCode;
        const bool bCompiledRelease = CompileFile(Case.Filename, Case.EntryPoint, EShaderModel::SM_6_2, Case.Stage, EShaderOutputLanguage::MSL, false, ReleaseCode);
        const bool bCompiledDebug   = CompileFile(Case.Filename, Case.EntryPoint, EShaderModel::SM_6_2, Case.Stage, EShaderOutputLanguage::MSL, true, DebugCode);
        TEST_EXPECT(bCompiledRelease);
        TEST_EXPECT(bCompiledDebug);
        if (!bCompiledRelease || !bCompiledDebug)
        {
            continue;
        }

        FShaderCodeView Release;
        FShaderCodeView Debug;
        TEST_EXPECT(FShaderCodeReader::Read(ReleaseCode, Release));
        TEST_EXPECT(FShaderCodeReader::Read(DebugCode, Debug));

        for (const FShaderCodeView* CodeView : { &Release, &Debug })
        {
            const String Source = GetMSLSource(CodeView->GetNativeCode());

            TEST_SECTION("The entry point is the MSL function name");
            TEST_EXPECT(CodeView->GetEntryPoint()[0] != '\0');
            TEST_EXPECT(Source.Contains(String(CodeView->GetEntryPoint()) + "("));

            TEST_SECTION("Every slot is inside its table and unique");
            const TArrayView<const FShaderResourceBinding> Bindings = CodeView->GetBindings();
            const TArrayView<const FMSLBindingSlot>        Slots    = CodeView->GetMSLSlots();
            TEST_EXPECT_EQ(Slots.Size(), Bindings.Size());

            for (int32 Index = 0; Index < Bindings.Size() && Index < Slots.Size(); ++Index)
            {
                const EMSLBindingType BindingType = GetMSLBindingType(Bindings[Index]);
                TEST_EXPECT(BindingType != EMSLBindingType::Unknown);
                TEST_EXPECT(Slots[Index].Slot < GetMSLMaxSlotCount(BindingType));

                // The vertex streams sit at the top of the buffer table, below the bindless heaps
                const bool bIsHeap = BindingType == EMSLBindingType::BindlessResourceHeap || BindingType == EMSLBindingType::BindlessSamplerHeap;
                if (GetMSLBindingTable(BindingType) == EMSLBindingTable::Buffer && !bIsHeap)
                {
                    TEST_EXPECT(Slots[Index].Slot <= MSL_VERTEX_STREAM_BUFFER_INDEX - MSL_MAX_VERTEX_STREAMS);
                }

                for (int32 Other = Index + 1; Other < Bindings.Size() && Other < Slots.Size(); ++Other)
                {
                    const EMSLBindingType OtherType = GetMSLBindingType(Bindings[Other]);
                    if (GetMSLBindingTable(OtherType) == GetMSLBindingTable(BindingType))
                    {
                        TEST_EXPECT(Slots[Other].Slot != Slots[Index].Slot);
                    }

                    if (OtherType == BindingType && BindingType != EMSLBindingType::ShaderConstants)
                    {
                        TEST_EXPECT(Bindings[Other].Register != Bindings[Index].Register);
                    }
                }
            }

            if (Case.Stage == EShaderStage::Compute)
            {
                TEST_SECTION("Compute shaders report their thread group size");
                TEST_EXPECT_EQ(CodeView->GetMSLInfo().ThreadGroupSize[0], 8);
                TEST_EXPECT_EQ(CodeView->GetMSLInfo().ThreadGroupSize[1], 8);
                TEST_EXPECT_EQ(CodeView->GetMSLInfo().ThreadGroupSize[2], 1);
            }
        }

        TEST_SECTION("Release MSL is generated from stripped SPIR-V");
        const String ReleaseSource = GetMSLSource(Release.GetNativeCode());
        for (int32 Index = 0; Index < Debug.GetBindings().Size(); ++Index)
        {
            const String Name = Debug.GetBindingName(Index);
            if (Name.Length() >= 6)
            {
                TEST_EXPECT(!ReleaseSource.Contains(Name));
            }
        }
    }

    TEST_SECTION("Every resource kind maps to an MSL binding type");
    {
        const String Source =
            "struct FData { float4 Value; };\n"
            "struct FConstants { uint Index; };\n"
            "[[vk::push_constant]] ConstantBuffer<FConstants> Constants : register(b0, space1);\n"
            "ConstantBuffer<FData>          Uniform    : register(b0);\n"
            "StructuredBuffer<FData>        ReadBuffer : register(t0);\n"
            "Texture2D<float4>              ReadImage  : register(t1);\n"
            "Buffer<float4>                 TexelRead  : register(t2);\n"
            "RWStructuredBuffer<FData>      OutBuffer  : register(u0);\n"
            "[[vk::image_format(\"rgba32f\")]] RWTexture2D<float4> OutImage : register(u1);\n"
            "SamplerState                   Sampler0   : register(s0);\n"
            "[numthreads(4, 2, 1)]\n"
            "void Main(uint3 ThreadID : SV_DispatchThreadID)\n"
            "{\n"
            "    float4 Value = Uniform.Value + ReadBuffer[Constants.Index].Value + ReadImage.SampleLevel(Sampler0, float2(0, 0), 0) + TexelRead[0];\n"
            "    OutBuffer[ThreadID.x].Value = Value;\n"
            "    OutImage[ThreadID.xy] = Value;\n"
            "}\n";

        TArray<uint8> ShaderCode;
        TEST_EXPECT(CompileSource(Source, "Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::MSL, true, ShaderCode));

        FShaderCodeView CodeView;
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
        TEST_EXPECT_EQ(CodeView.GetMSLInfo().ThreadGroupSize[0], 4);
        TEST_EXPECT_EQ(CodeView.GetMSLInfo().ThreadGroupSize[1], 2);
        TEST_EXPECT(CodeView.GetInfo().ShaderConstantsSize > 0);

        TArray<EMSLBindingType> Types;
        for (const FShaderResourceBinding& Binding : CodeView.GetBindings())
        {
            Types.Add(GetMSLBindingType(Binding));
        }

        TEST_EXPECT(Types.Contains(EMSLBindingType::ConstantBuffer));
        TEST_EXPECT(Types.Contains(EMSLBindingType::ShaderResourceBuffer));
        TEST_EXPECT(Types.Contains(EMSLBindingType::ShaderResourceTexture));
        TEST_EXPECT(Types.Contains(EMSLBindingType::UnorderedAccessBuffer));
        TEST_EXPECT(Types.Contains(EMSLBindingType::UnorderedAccessTexture));
        TEST_EXPECT(Types.Contains(EMSLBindingType::Sampler));
        TEST_EXPECT(Types.Contains(EMSLBindingType::ShaderConstants));
    }

    TEST_SECTION("The bindless heaps sit at buffers 29 and 30");
    {
        const String Source =
            "struct FConstants { uint TextureIndex; uint SamplerIndex; };\n"
            "[[vk::push_constant]] ConstantBuffer<FConstants> Constants : register(b0, space1);\n"
            "RWStructuredBuffer<float4> Output : register(u0);\n"
            "[numthreads(1, 1, 1)]\n"
            "void Main()\n"
            "{\n"
            "    Texture2D<float4> Texture = ResourceDescriptorHeap[Constants.TextureIndex];\n"
            "    SamplerState      Sampler = SamplerDescriptorHeap[Constants.SamplerIndex];\n"
            "    Output[0] = Texture.SampleLevel(Sampler, float2(0, 0), 0);\n"
            "}\n";

        TArray<uint8> ShaderCode;
        TEST_EXPECT(CompileSource(Source, "Main", EShaderModel::SM_6_6, EShaderStage::Compute, EShaderOutputLanguage::MSL, false, ShaderCode));

        FShaderCodeView CodeView;
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));

        bool bHasResourceHeap = false;
        bool bHasSamplerHeap  = false;
        for (int32 Index = 0; Index < CodeView.GetBindings().Size(); ++Index)
        {
            const EMSLBindingType BindingType = GetMSLBindingType(CodeView.GetBindings()[Index]);
            if (BindingType == EMSLBindingType::BindlessResourceHeap)
            {
                bHasResourceHeap = true;
                TEST_EXPECT_EQ(CodeView.GetMSLSlots()[Index].Slot, MSL_BINDLESS_RESOURCE_HEAP_BUFFER_INDEX);
            }
            else if (BindingType == EMSLBindingType::BindlessSamplerHeap)
            {
                bHasSamplerHeap = true;
                TEST_EXPECT_EQ(CodeView.GetMSLSlots()[Index].Slot, MSL_BINDLESS_SAMPLER_HEAP_BUFFER_INDEX);
            }
        }

        TEST_EXPECT(bHasResourceHeap);
        TEST_EXPECT(bHasSamplerHeap);
    }

    TEST_END();
}
