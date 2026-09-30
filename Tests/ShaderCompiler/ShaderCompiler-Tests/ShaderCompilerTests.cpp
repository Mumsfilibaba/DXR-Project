#include "ShaderCompilerTests.h"

#include <Core/Memory/Memory.h>
#include <RHI/RHIDevice.h>
#include <ShaderCompiler/ShaderCompiler.h>

#include "TestCommon/TestMacros.h"

#if PLATFORM_WINDOWS
    #include <Core/Containers/ComPtr.h>
    #include <Core/Containers/Map.h>
    #include <Core/Platform/PlatformLibrary.h>
    #include <d3d11shader.h>

typedef HRESULT(WINAPI* PFN_TEST_D3D_REFLECT)(LPCVOID SrcData, SIZE_T SrcDataSize, REFIID Interface, void** Reflector);

static bool ReflectConstantBufferSlots(const TArray<uint8>& ByteCode, TMap<String, uint32>& OutSlots)
{
    static void* D3DCompilerLib = FPlatformLibrary::LoadDynamicLib("d3dcompiler_47");
    static PFN_TEST_D3D_REFLECT D3DReflectFunc = D3DCompilerLib ? FPlatformLibrary::LoadSymbol<PFN_TEST_D3D_REFLECT>("D3DReflect", D3DCompilerLib) : nullptr;
    if (!D3DReflectFunc)
    {
        return false;
    }

    TComPtr<ID3D11ShaderReflection> Reflection;
    if (FAILED(D3DReflectFunc(ByteCode.Data(), static_cast<SIZE_T>(ByteCode.Size()), IID_PPV_ARGS(&Reflection))))
    {
        return false;
    }

    D3D11_SHADER_DESC ShaderDesc = {};
    Reflection->GetDesc(&ShaderDesc);

    for (UINT Index = 0; Index < ShaderDesc.BoundResources; ++Index)
    {
        D3D11_SHADER_INPUT_BIND_DESC BindDesc = {};
        Reflection->GetResourceBindingDesc(Index, &BindDesc);

        if (BindDesc.Type == D3D_SIT_CBUFFER)
        {
            OutSlots.Add(String(BindDesc.Name), BindDesc.BindPoint);
        }
    }

    return true;
}
#endif

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
        case EShaderOutputLanguage::DXBC:
        {
            return ContainerHasPart(ByteCode, MakeFourCC('S', 'H', 'E', 'X')) && !ContainerHasPart(ByteCode, MakeFourCC('D', 'X', 'I', 'L'));
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
    TEST_EXPECT_EQ(FShaderCompiler::GetOutputLanguageForRHI(ERHIType::D3D11), EShaderOutputLanguage::DXBC);
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

bool ShaderCompilerCompileFailure_Test()
{
    TEST_BEGIN();

    const String BrokenSource = "float4 Main() : SV_Target { return MissingValue; }\n";
    const String ValidSource  = "float4 Main() : SV_Target { return float4(1.0, 0.0, 0.0, 1.0); }\n";

    for (EShaderOutputLanguage OutputLanguage : FShaderCompiler::Get().GetSupportedOutputLanguages())
    {
        TEST_SECTION(ToString(OutputLanguage));

        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel, TArrayView<FShaderDefine>(), OutputLanguage);

        TArray<uint8> ByteCode;
        TEST_EXPECT(!FShaderCompiler::Get().CompileFromSource(BrokenSource, CompileInfo, ByteCode));
        TEST_EXPECT(ByteCode.IsEmpty());
    }

    if (FShaderCompiler::Get().IsOutputLanguageSupported(EShaderOutputLanguage::DXBC))
    {
        TEST_SECTION("DXBC is limited to what Shader Model 5.0 can express");

        TArray<uint8> ByteCode;
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel, TArrayView<FShaderDefine>(), EShaderOutputLanguage::DXBC);
        TEST_EXPECT(FShaderCompiler::Get().CompileFromSource(ValidSource, CompileInfo, ByteCode));

        CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_6, EShaderStage::Pixel, TArrayView<FShaderDefine>(), EShaderOutputLanguage::DXBC);
        TEST_EXPECT(!FShaderCompiler::Get().CompileFromSource(ValidSource, CompileInfo, ByteCode));

        CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::RayGen, TArrayView<FShaderDefine>(), EShaderOutputLanguage::DXBC);
        TEST_EXPECT(!FShaderCompiler::Get().CompileFromSource(ValidSource, CompileInfo, ByteCode));
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
            FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel, TArrayView<FShaderDefine>(), EShaderOutputLanguage::DXBC);

            TArray<uint8> ByteCode;
            return FShaderCompiler::Get().CompileFromSource(Source, CompileInfo, ByteCode) && ReflectConstantBufferSlots(ByteCode, OutSlots);
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

        TEST_SECTION("An engine shader never shares a slot with its own constant buffers");
        {
            FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, TArrayView<FShaderDefine>(), EShaderOutputLanguage::DXBC);

            TArray<uint8> ByteCode;
            TMap<String, uint32> Slots;
            TEST_EXPECT(FShaderCompiler::Get().CompileFromFile("Shaders/UserInterface.hlsl", CompileInfo, ByteCode));
            TEST_EXPECT(ReflectConstantBufferSlots(ByteCode, Slots));
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
