#include "ShaderCodeTests.h"

#include <Core/Memory/Memory.h>
#include <Core/Templates/CString.h>
#include <ShaderCore/ShaderCode.h>
#include <ShaderCore/ShaderReflection.h>

#include "TestCommon/TestMacros.h"

static const uint8 GNativeCode[16] = { 0x44, 0x58, 0x42, 0x43, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

static TArrayView<const uint8> GetNativeCode()
{
    return TArrayView<const uint8>(GNativeCode, static_cast<int32>(ARRAY_COUNT(GNativeCode)));
}

static FShaderReflection CreateSixBindings()
{
    FShaderReflection Reflection;
    Reflection.Info.RequiredFeatures    = EShaderFeatureFlags::RequiresWaveOps;
    Reflection.Info.Flags               = EShaderReflectionFlags::HasEmbeddedRootSignature;
    Reflection.Info.ShaderConstantsSize = 48;

    Reflection.Bindings.Add({ EShaderResourceType::ConstantBuffer,        EShaderResourceDimension::Unknown,        EShaderBindingSpace::Global,          0, 1 });
    Reflection.Bindings.Add({ EShaderResourceType::Sampler,               EShaderResourceDimension::Unknown,        EShaderBindingSpace::Global,          3, 1 });
    Reflection.Bindings.Add({ EShaderResourceType::Texture,               EShaderResourceDimension::TextureCube,    EShaderBindingSpace::Global,          7, 4 });
    Reflection.Bindings.Add({ EShaderResourceType::StructuredBuffer,      EShaderResourceDimension::BufferEx,       EShaderBindingSpace::RayTracingLocal, 1, 1 });
    Reflection.Bindings.Add({ EShaderResourceType::RWTexture,             EShaderResourceDimension::Texture2DArray, EShaderBindingSpace::Global,          2, 1 });
    Reflection.Bindings.Add({ EShaderResourceType::AccelerationStructure, EShaderResourceDimension::Unknown,        EShaderBindingSpace::Global,          9, 1 });
    return Reflection;
}

static bool BindingsMatch(TArrayView<const FShaderResourceBinding> Lhs, const TArray<FShaderResourceBinding>& Rhs)
{
    if (Lhs.Size() != Rhs.Size())
    {
        return false;
    }

    for (int32 Index = 0; Index < Lhs.Size(); ++Index)
    {
        const FShaderResourceBinding& A = Lhs[Index];
        const FShaderResourceBinding& B = Rhs[Index];
        if (A.Type != B.Type || A.Dimension != B.Dimension || A.Space != B.Space || A.Register != B.Register || A.Count != B.Count)
        {
            return false;
        }
    }

    return true;
}

static bool NativeCodeMatches(TArrayView<const uint8> NativeCode)
{
    return NativeCode.Size() == GetNativeCode().Size() && Memory::Memcmp(NativeCode.Data(), GNativeCode, sizeof(GNativeCode)) == 0;
}

static bool IsAligned(const void* Pointer, uintptr_t Alignment)
{
    return (reinterpret_cast<uintptr_t>(Pointer) % Alignment) == 0;
}

/** @return The container is rejected by the full read, the header read result is not checked */
static bool IsRejected(const TArray<uint8>& ShaderCode)
{
    FShaderCodeView CodeView;
    return !FShaderCodeReader::Read(ShaderCode, CodeView);
}

bool ShaderCodeRoundTrip_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A release DXIL container with 6 bindings is 52 bytes plus the code");
    {
        const FShaderReflection Reflection = CreateSixBindings();

        TArray<uint8> ShaderCode;
        TEST_EXPECT(FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Pixel, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));
        TEST_EXPECT_EQ(ShaderCode.Size(), 52 + GetNativeCode().Size());

        FShaderCodeHeader Header;
        TEST_EXPECT(FShaderCodeReader::ReadHeader(ShaderCode, Header));
        TEST_EXPECT_EQ(Header.CodeOffset, 52u);

        FShaderCodeView CodeView;
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
        TEST_EXPECT(CodeView.GetStage() == EShaderStage::Pixel);
        TEST_EXPECT(CodeView.GetOutputLanguage() == EShaderOutputLanguage::DXIL);
        TEST_EXPECT(!CodeView.HasDebugInfo());
        TEST_EXPECT(CodeView.GetInfo().RequiredFeatures == EShaderFeatureFlags::RequiresWaveOps);
        TEST_EXPECT(CodeView.GetInfo().HasFlag(EShaderReflectionFlags::HasEmbeddedRootSignature));
        TEST_EXPECT_EQ(CodeView.GetInfo().ShaderConstantsSize, 48);
        TEST_EXPECT(BindingsMatch(CodeView.GetBindings(), Reflection.Bindings));
        TEST_EXPECT(CodeView.GetSpirvOffsets().IsEmpty());
        TEST_EXPECT(CodeView.GetMSLSlots().IsEmpty());
        TEST_EXPECT(CodeView.GetVertexInputs().IsEmpty());
        TEST_EXPECT(CodeView.GetEntryPoint()[0] == '\0');

        TEST_SECTION("The native code is 4-byte aligned and byte-identical to the input");
        TEST_EXPECT(IsAligned(CodeView.GetNativeCode().Data(), FShaderCodeWriter::CodeAlignment));
        TEST_EXPECT(NativeCodeMatches(CodeView.GetNativeCode()));

        TEST_SECTION("Release containers report empty binding names");
        for (int32 Index = 0; Index < CodeView.GetBindings().Size(); ++Index)
        {
            TEST_EXPECT(CodeView.GetBindingName(Index)[0] == '\0');
        }
    }

    TEST_SECTION("A SPIR-V container with 6 bindings and entry point 'main' is 80 bytes plus the code");
    {
        FShaderReflection Reflection = CreateSixBindings();
        Reflection.EntryPoint = "main";
        for (uint16 Index = 0; Index < 6; ++Index)
        {
            Reflection.SpirvOffsets.Add({ static_cast<uint16>(100 + Index * 8), static_cast<uint16>(104 + Index * 8) });
        }

        TArray<uint8> ShaderCode;
        TEST_EXPECT(FShaderCodeWriter::Write(EShaderOutputLanguage::SPIRV, EShaderStage::Compute, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));
        TEST_EXPECT_EQ(ShaderCode.Size(), 80 + GetNativeCode().Size());

        FShaderCodeView CodeView;
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
        TEST_EXPECT(CString::Strcmp(CodeView.GetEntryPoint(), "main") == 0);
        TEST_EXPECT(BindingsMatch(CodeView.GetBindings(), Reflection.Bindings));
        TEST_EXPECT(NativeCodeMatches(CodeView.GetNativeCode()));

        TEST_SECTION("The SPIR-V offsets are 2-byte aligned and survive the round trip");
        const TArrayView<const FSpirvBindingOffsets> SpirvOffsets = CodeView.GetSpirvOffsets();
        TEST_EXPECT_EQ(SpirvOffsets.Size(), 6);
        TEST_EXPECT(IsAligned(SpirvOffsets.Data(), alignof(FSpirvBindingOffsets)));
        for (int32 Index = 0; Index < SpirvOffsets.Size(); ++Index)
        {
            TEST_EXPECT_EQ(SpirvOffsets[Index].SetWordOffset, Reflection.SpirvOffsets[Index].SetWordOffset);
            TEST_EXPECT_EQ(SpirvOffsets[Index].BindingWordOffset, Reflection.SpirvOffsets[Index].BindingWordOffset);
        }
    }

    TEST_END();
}

bool ShaderCodeLanguageBlocks_Test()
{
    TEST_BEGIN();

    TEST_SECTION("The MSL info and slots survive the round trip");
    {
        FShaderReflection Reflection = CreateSixBindings();
        Reflection.EntryPoint                 = "Main0";
        Reflection.MSLInfo.ThreadGroupSize[0] = 8;
        Reflection.MSLInfo.ThreadGroupSize[1] = 4;
        Reflection.MSLInfo.ThreadGroupSize[2] = 2;
        for (uint8 Index = 0; Index < 6; ++Index)
        {
            Reflection.MSLSlots.Add({ static_cast<uint8>(Index * 2), static_cast<uint8>(Index) });
        }

        TArray<uint8> ShaderCode;
        TEST_EXPECT(FShaderCodeWriter::Write(EShaderOutputLanguage::MSL, EShaderStage::Compute, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));

        FShaderCodeView CodeView;
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
        TEST_EXPECT(CString::Strcmp(CodeView.GetEntryPoint(), "Main0") == 0);
        TEST_EXPECT_EQ(CodeView.GetMSLInfo().ThreadGroupSize[0], 8);
        TEST_EXPECT_EQ(CodeView.GetMSLInfo().ThreadGroupSize[1], 4);
        TEST_EXPECT_EQ(CodeView.GetMSLInfo().ThreadGroupSize[2], 2);
        TEST_EXPECT(CodeView.GetSpirvOffsets().IsEmpty());

        const TArrayView<const FMSLBindingSlot> Slots = CodeView.GetMSLSlots();
        TEST_EXPECT_EQ(Slots.Size(), 6);
        for (int32 Index = 0; Index < Slots.Size(); ++Index)
        {
            TEST_EXPECT_EQ(Slots[Index].Slot, Reflection.MSLSlots[Index].Slot);
            TEST_EXPECT_EQ(Slots[Index].NullTextureType, Reflection.MSLSlots[Index].NullTextureType);
        }
    }

    TEST_SECTION("A container without bindings holds only the header, the info and the entry point");
    {
        FShaderReflection Reflection;

        TArray<uint8> ShaderCode;
        TEST_EXPECT(FShaderCodeWriter::Write(EShaderOutputLanguage::DXBC, EShaderStage::Vertex, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));
        TEST_EXPECT_EQ(ShaderCode.Size(), 24 + GetNativeCode().Size());

        FShaderCodeView CodeView;
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
        TEST_EXPECT(CodeView.GetBindings().IsEmpty());
        TEST_EXPECT(NativeCodeMatches(CodeView.GetNativeCode()));
    }

    TEST_END();
}

bool ShaderCodeDebugInfo_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A DebugInfo container brings back every name");

    FShaderReflection Reflection = CreateSixBindings();
    Reflection.EntryPoint = "ClosestHit";

    const CHAR* const Names[] = { "PerFrame", "LinearSampler", "Skybox", "Materials", "Output", "Scene" };
    for (const CHAR* Name : Names)
    {
        Reflection.BindingNames.Add(Name);
    }

    TArray<uint8> ShaderCode;
    TEST_EXPECT(FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::RayClosestHit, EShaderCodeFlags::DebugInfo, Reflection, GetNativeCode(), ShaderCode));

    FShaderCodeView CodeView;
    TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
    TEST_EXPECT(CodeView.HasDebugInfo());
    TEST_EXPECT(CString::Strcmp(CodeView.GetEntryPoint(), "ClosestHit") == 0);
    TEST_EXPECT(NativeCodeMatches(CodeView.GetNativeCode()));
    TEST_EXPECT(IsAligned(CodeView.GetNativeCode().Data(), FShaderCodeWriter::CodeAlignment));

    for (int32 Index = 0; Index < CodeView.GetBindings().Size(); ++Index)
    {
        TEST_EXPECT(CString::Strcmp(CodeView.GetBindingName(Index), Names[Index]) == 0);
    }

    TEST_END();
}

bool ShaderCodeVertexInputs_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Vertex inputs survive the round trip");

    FShaderReflection Reflection;
    Reflection.EntryPoint = "VSMain";
    Reflection.VertexInputs.Add({ HashShaderSemantic("POSITION"), 0, 0, EShaderComponentType::Float32, 2 });
    Reflection.VertexInputs.Add({ HashShaderSemantic("TEXCOORD"), 0, 1, EShaderComponentType::Float32, 2 });
    Reflection.VertexInputs.Add({ HashShaderSemantic("COLOR"),    0, 2, EShaderComponentType::Float32, 4 });
    Reflection.Bindings.Add({ EShaderResourceType::Texture, EShaderResourceDimension::Texture2D, EShaderBindingSpace::Global, 0, 1 });

    TArray<uint8> ShaderCode;
    TEST_EXPECT(FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Vertex, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));

    FShaderCodeView CodeView;
    TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));
    TEST_EXPECT(BindingsMatch(CodeView.GetBindings(), Reflection.Bindings));

    const TArrayView<const FShaderVertexInput> VertexInputs = CodeView.GetVertexInputs();
    TEST_EXPECT_EQ(VertexInputs.Size(), 3);
    TEST_EXPECT(IsAligned(VertexInputs.Data(), alignof(FShaderVertexInput)));
    for (int32 Index = 0; Index < VertexInputs.Size(); ++Index)
    {
        TEST_EXPECT_EQ(VertexInputs[Index].SemanticHash, Reflection.VertexInputs[Index].SemanticHash);
        TEST_EXPECT_EQ(VertexInputs[Index].SemanticIndex, Reflection.VertexInputs[Index].SemanticIndex);
        TEST_EXPECT_EQ(VertexInputs[Index].Location, Reflection.VertexInputs[Index].Location);
        TEST_EXPECT(VertexInputs[Index].ComponentType == Reflection.VertexInputs[Index].ComponentType);
        TEST_EXPECT_EQ(VertexInputs[Index].NumComponents, Reflection.VertexInputs[Index].NumComponents);
    }

    TEST_SECTION("Only vertex shaders carry vertex inputs");
    TEST_EXPECT(!FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Pixel, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));

    TEST_END();
}

bool ShaderCodeReaderRejects_Test()
{
    TEST_BEGIN();

    TArray<uint8> Valid;
    {
        const FShaderReflection Reflection = CreateSixBindings();
        TEST_EXPECT(FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Pixel, EShaderCodeFlags::None, Reflection, GetNativeCode(), Valid));
        TEST_EXPECT(!IsRejected(Valid));
    }

    constexpr int32 FirstBinding  = static_cast<int32>(sizeof(FShaderCodeHeader) + sizeof(FShaderReflectionInfo));
    constexpr int32 CodeOffsetPos = static_cast<int32>(offsetof(FShaderCodeHeader, CodeOffset));

    const auto WithCodeOffset = [&](uint32 CodeOffset)
    {
        TArray<uint8> ShaderCode = Valid;
        Memory::Memcpy(ShaderCode.Data() + CodeOffsetPos, &CodeOffset, sizeof(CodeOffset));
        return ShaderCode;
    };

    const auto WithByte = [&](int32 Position, uint8 Value)
    {
        TArray<uint8> ShaderCode = Valid;
        ShaderCode[Position] = Value;
        return ShaderCode;
    };

    TEST_SECTION("An empty array and a truncated header");
    {
        TArray<uint8> Empty;
        TEST_EXPECT(IsRejected(Empty));

        TArray<uint8> Truncated(Valid.Data(), 8);
        TEST_EXPECT(IsRejected(Truncated));
    }

    TEST_SECTION("A wrong magic and another version");
    TEST_EXPECT(IsRejected(WithByte(0, 'X')));
    TEST_EXPECT(IsRejected(WithByte(static_cast<int32>(offsetof(FShaderCodeHeader, Version)), FShaderCodeHeader::CurrentVersion + 1)));

    TEST_SECTION("An unknown stage or output language");
    TEST_EXPECT(IsRejected(WithByte(static_cast<int32>(offsetof(FShaderCodeHeader, Stage)), 0xFF)));
    TEST_EXPECT(IsRejected(WithByte(static_cast<int32>(offsetof(FShaderCodeHeader, OutputLanguage)), 0xFF)));

    TEST_SECTION("A CodeOffset that is unaligned or past the end");
    TEST_EXPECT(IsRejected(WithCodeOffset(50)));
    TEST_EXPECT(IsRejected(WithCodeOffset(static_cast<uint32>(Valid.Size()) + 4)));
    TEST_EXPECT(IsRejected(WithCodeOffset(8)));

    TEST_SECTION("An out-of-range Type, Dimension or Space, or Count == 0");
    TEST_EXPECT(IsRejected(WithByte(FirstBinding + static_cast<int32>(offsetof(FShaderResourceBinding, Type)), static_cast<uint8>(EShaderResourceType::Count))));
    TEST_EXPECT(IsRejected(WithByte(FirstBinding + static_cast<int32>(offsetof(FShaderResourceBinding, Dimension)), static_cast<uint8>(EShaderResourceDimension::Count))));
    TEST_EXPECT(IsRejected(WithByte(FirstBinding + static_cast<int32>(offsetof(FShaderResourceBinding, Space)), static_cast<uint8>(EShaderBindingSpace::Count))));
    TEST_EXPECT(IsRejected(WithByte(FirstBinding + static_cast<int32>(offsetof(FShaderResourceBinding, Count)), 0)));

    TEST_SECTION("More bindings than the reflection block holds");
    TEST_EXPECT(IsRejected(WithByte(static_cast<int32>(sizeof(FShaderCodeHeader) + offsetof(FShaderReflectionInfo, NumBindings)), 0xFF)));

    TEST_SECTION("A missing null terminator");
    {
        // The entry point ends exactly at the native code, so overwriting its terminator leaves no zero inside the block
        FShaderReflection Reflection;
        Reflection.EntryPoint = "abc";

        TArray<uint8> ShaderCode;
        TEST_EXPECT(FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Compute, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));
        TEST_EXPECT(!IsRejected(ShaderCode));

        FShaderCodeHeader Header;
        TEST_EXPECT(FShaderCodeReader::ReadHeader(ShaderCode, Header));
        TEST_EXPECT_EQ(Header.CodeOffset, 24u);

        ShaderCode[23] = 'd';
        TEST_EXPECT(IsRejected(ShaderCode));
    }

    TEST_SECTION("A name table with the wrong number of names");
    {
        FShaderReflection Reflection = CreateSixBindings();
        for (int32 Index = 0; Index < 6; ++Index)
        {
            Reflection.BindingNames.Add("N");
        }

        TArray<uint8> DebugCode;
        TEST_EXPECT(FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Pixel, EShaderCodeFlags::DebugInfo, Reflection, GetNativeCode(), DebugCode));
        TEST_EXPECT(!IsRejected(DebugCode));

        // Dropping the flag leaves six names nobody reads
        constexpr int32 FlagsPos = static_cast<int32>(offsetof(FShaderCodeHeader, Flags));

        TArray<uint8> WithoutFlag = DebugCode;
        WithoutFlag[FlagsPos] = static_cast<uint8>(EShaderCodeFlags::None);
        TEST_EXPECT(IsRejected(WithoutFlag));

        // Setting it on a release container asks for six names that are not there
        TArray<uint8> WithFlag = Valid;
        WithFlag[FlagsPos] = static_cast<uint8>(EShaderCodeFlags::DebugInfo);
        TEST_EXPECT(IsRejected(WithFlag));
    }

    TEST_SECTION("Non-zero padding before the native code");
    TEST_EXPECT(IsRejected(WithByte(51, 1)));

    TEST_END();
}

bool ShaderCodeWriterRejects_Test()
{
    TEST_BEGIN();

    TArray<uint8> ShaderCode;

    TEST_SECTION("More than 255 bindings");
    {
        FShaderReflection Reflection;
        for (int32 Index = 0; Index < 256; ++Index)
        {
            Reflection.Bindings.Add({ EShaderResourceType::Texture, EShaderResourceDimension::Texture2D, EShaderBindingSpace::Global, 0, 1 });
        }

        TEST_EXPECT(!FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Pixel, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));
    }

    TEST_SECTION("Mismatched SpirvOffsets");
    {
        FShaderReflection Reflection = CreateSixBindings();
        TEST_EXPECT(!FShaderCodeWriter::Write(EShaderOutputLanguage::SPIRV, EShaderStage::Pixel, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));

        Reflection.SpirvOffsets.Add({ 1, 2 });
        TEST_EXPECT(!FShaderCodeWriter::Write(EShaderOutputLanguage::SPIRV, EShaderStage::Pixel, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));
        TEST_EXPECT(!FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Pixel, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));
    }

    TEST_SECTION("Mismatched MSLSlots");
    {
        const FShaderReflection Reflection = CreateSixBindings();
        TEST_EXPECT(!FShaderCodeWriter::Write(EShaderOutputLanguage::MSL, EShaderStage::Pixel, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));
    }

    TEST_SECTION("Mismatched BindingNames, and names without DebugInfo");
    {
        FShaderReflection Reflection = CreateSixBindings();
        TEST_EXPECT(!FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Pixel, EShaderCodeFlags::DebugInfo, Reflection, GetNativeCode(), ShaderCode));

        Reflection.BindingNames.Add("Only");
        TEST_EXPECT(!FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Pixel, EShaderCodeFlags::DebugInfo, Reflection, GetNativeCode(), ShaderCode));

        for (int32 Index = 1; Index < 6; ++Index)
        {
            Reflection.BindingNames.Add("Name");
        }

        TEST_EXPECT(!FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Pixel, EShaderCodeFlags::None, Reflection, GetNativeCode(), ShaderCode));
        TEST_EXPECT(FShaderCodeWriter::Write(EShaderOutputLanguage::DXIL, EShaderStage::Pixel, EShaderCodeFlags::DebugInfo, Reflection, GetNativeCode(), ShaderCode));
    }

    TEST_END();
}

bool ShaderSemanticHash_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Semantics are hashed case-insensitively");
    TEST_EXPECT_EQ(HashShaderSemantic("texcoord"), HashShaderSemantic("TEXCOORD"));
    TEST_EXPECT_EQ(HashShaderSemantic("Position"), HashShaderSemantic("POSITION"));

    TEST_SECTION("Different semantics hash differently");
    TEST_EXPECT(HashShaderSemantic("POSITION") != HashShaderSemantic("NORMAL"));
    TEST_EXPECT(HashShaderSemantic("TEXCOORD") != HashShaderSemantic("COLOR"));

    TEST_END();
}
