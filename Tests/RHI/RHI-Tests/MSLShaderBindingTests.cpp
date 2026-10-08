#include "MSLShaderBindingTests.h"

#include <Core/Misc/Paths.h>
#include <Core/Templates/CString.h>
#include <ShaderCompiler/ShaderCompiler.h>
#include <ShaderCore/MSLShaderBindings.h>
#include <ShaderCore/ShaderCode.h>

#include "TestCommon/TestMacros.h"

static const CHAR* const CollidingShaderFile = "Shaders/Shadows/CascadeMatrixGen.hlsl";

static constexpr uint8 InvalidSlot = UINT8_MAX;

static uint8 FindSlot(const TArray<FMSLShaderBinding>& Bindings, EMSLBindingType BindingType, uint8 RegisterIndex)
{
    for (const FMSLShaderBinding& Binding : Bindings)
    {
        if (Binding.BindingType == BindingType && Binding.RegisterIndex == RegisterIndex)
        {
            return Binding.SlotIndex;
        }
    }

    return InvalidSlot;
}

// Rebuilds the binding table MetalRHI derives from the container, so the checks below read the same slots the backend binds
static bool ReadMSLShaderCode(const TArray<uint8>& ShaderCode, FShaderCodeView& OutCodeView, TArray<FMSLShaderBinding>& OutBindings)
{
    OutBindings.Clear();

    if (!FShaderCodeReader::Read(ShaderCode, OutCodeView) || OutCodeView.GetOutputLanguage() != EShaderOutputLanguage::MSL)
    {
        return false;
    }

    const TArrayView<const FShaderResourceBinding> ShaderBindings = OutCodeView.GetBindings();
    const TArrayView<const FMSLBindingSlot>        MSLSlots       = OutCodeView.GetMSLSlots();
    if (MSLSlots.Size() != ShaderBindings.Size())
    {
        return false;
    }

    for (int32 Index = 0; Index < ShaderBindings.Size(); Index++)
    {
        FMSLShaderBinding& Binding = OutBindings.Emplace();
        Binding.BindingType     = GetMSLBindingType(ShaderBindings[Index]);
        Binding.RegisterIndex   = ShaderBindings[Index].Register;
        Binding.SlotIndex       = MSLSlots[Index].Slot;
        Binding.NullTextureType = MSLSlots[Index].NullTextureType;
    }

    return true;
}

bool MSLShaderBinding_Test()
{
    TEST_BEGIN();

    TEST_SECTION("The shader compiler comes up");
    if (!FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        TEST_EXPECT(false);
        TEST_END();
    }

    TArray<uint8>             ByteCode;
    TArray<FMSLShaderBinding> Bindings;
    FShaderCodeView           CodeView;

    TEST_SECTION("A shader with colliding HLSL registers compiles to MSL");
    const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::MSL);
    const bool bCompiled = FShaderCompiler::Get().CompileFromFile(CollidingShaderFile, CompileInfo, ByteCode);
    TEST_EXPECT(bCompiled);

    if (bCompiled)
    {
        TEST_SECTION("The container splits into a binding table and MSL source");
        TEST_EXPECT(ReadMSLShaderCode(ByteCode, CodeView, Bindings));

        const TArrayView<const uint8> Source = CodeView.GetNativeCode();
        TEST_EXPECT(Source.Size() > 0);
        TEST_EXPECT(Bindings.Size() > 0);

        const String SourceText(reinterpret_cast<const CHAR*>(Source.Data()), Source.Size());
        TEST_EXPECT(SourceText.Contains("metal_stdlib"));

        TEST_SECTION("No aliased argument survives into the MSL");
        TEST_EXPECT(!SourceText.Contains("constant void*"));

        TEST_SECTION("Every register the shader declares reaches the table");
        TEST_EXPECT(FindSlot(Bindings, EMSLBindingType::ConstantBuffer, 0) != InvalidSlot);
        TEST_EXPECT(FindSlot(Bindings, EMSLBindingType::ConstantBuffer, 1) != InvalidSlot);
        TEST_EXPECT(FindSlot(Bindings, EMSLBindingType::UnorderedAccessBuffer, 0) != InvalidSlot);
        TEST_EXPECT(FindSlot(Bindings, EMSLBindingType::UnorderedAccessBuffer, 1) != InvalidSlot);
        TEST_EXPECT(FindSlot(Bindings, EMSLBindingType::ShaderResourceTexture, 0) != InvalidSlot);

        TEST_SECTION("Registers that collided in SPIR-V resolve to separate MSL slots");
        TEST_EXPECT(FindSlot(Bindings, EMSLBindingType::ConstantBuffer, 0) != FindSlot(Bindings, EMSLBindingType::UnorderedAccessBuffer, 0));
        TEST_EXPECT(FindSlot(Bindings, EMSLBindingType::ConstantBuffer, 1) != FindSlot(Bindings, EMSLBindingType::UnorderedAccessBuffer, 1));

        TEST_SECTION("No two bindings share a slot within one MSL table");
        for (int32 Index = 0; Index < Bindings.Size(); Index++)
        {
            for (int32 OtherIndex = Index + 1; OtherIndex < Bindings.Size(); OtherIndex++)
            {
                const FMSLShaderBinding& Binding = Bindings[Index];
                const FMSLShaderBinding& Other   = Bindings[OtherIndex];

                const bool bCollides = GetMSLBindingTable(Binding.BindingType) == GetMSLBindingTable(Other.BindingType) && Binding.SlotIndex == Other.SlotIndex;
                if (bCollides)
                {
                    LOG_ERROR("[FAIL] %s : %s register %u and %s register %u both resolved to slot %u",
                        _TestSection,
                        ToString(Binding.BindingType), Binding.RegisterIndex,
                        ToString(Other.BindingType), Other.RegisterIndex,
                        Binding.SlotIndex);
                }

                TEST_EXPECT(!bCollides);
            }
        }

        TEST_SECTION("Every resolved slot is below the per-table max");
        for (int32 Index = 0; Index < Bindings.Size(); Index++)
        {
            const FMSLShaderBinding& Binding = Bindings[Index];
            TEST_EXPECT(Binding.SlotIndex < GetMSLMaxSlotCount(Binding.BindingType));
        }

        TEST_SECTION("A compute shader container carries a non-zero threadgroup size");
        const FMSLShaderInfo& MSLInfo = CodeView.GetMSLInfo();
        TEST_EXPECT(MSLInfo.ThreadGroupSize[0] != 0);
        TEST_EXPECT_EQ(MSLInfo.ThreadGroupSize[1], static_cast<uint16>(1));
        TEST_EXPECT_EQ(MSLInfo.ThreadGroupSize[2], static_cast<uint16>(1));

        TEST_SECTION("A shader without descriptor heaps has no heap bindings");
        TEST_EXPECT(FindSlot(Bindings, EMSLBindingType::BindlessResourceHeap, 0) == InvalidSlot);
        TEST_EXPECT(FindSlot(Bindings, EMSLBindingType::BindlessSamplerHeap, 0) == InvalidSlot);

        TEST_SECTION("The container names the entry point");
        TEST_EXPECT(CString::Strlen(CodeView.GetEntryPoint()) > 0);
    }

    TEST_SECTION("A bindless compute shader pins heap tables at MSL buffers 29 and 30");
    {
        static const CHAR BindlessSource[] =
            "RWStructuredBuffer<uint> OutBuffer : register(u0);\n"
            "cbuffer Params : register(b0) { uint ResourceIndex; uint SamplerIndex; uint Pad0; uint Pad1; };\n"
            "[numthreads(1,1,1)]\n"
            "void Main()\n"
            "{\n"
            "    Texture2D<float4> Tex = ResourceDescriptorHeap[ResourceIndex];\n"
            "    SamplerState Samp = SamplerDescriptorHeap[SamplerIndex];\n"
            "    float4 Color = Tex.SampleLevel(Samp, float2(0.5, 0.5), 0);\n"
            "    OutBuffer[0] = (uint)(Color.x * 255.0f + 0.5f);\n"
            "}\n";

        TArray<uint8>             BindlessByteCode;
        TArray<FMSLShaderBinding> BindlessBindings;
        FShaderCodeView           BindlessCodeView;

        const FShaderCompileInfo BindlessInfo("Main", EShaderModel::SM_6_6, EShaderStage::Compute, EShaderOutputLanguage::MSL);
        const bool bBindlessCompiled = FShaderCompiler::Get().CompileFromSource(BindlessSource, BindlessInfo, BindlessByteCode);
        TEST_EXPECT(bBindlessCompiled);

        if (bBindlessCompiled)
        {
            TEST_EXPECT(ReadMSLShaderCode(BindlessByteCode, BindlessCodeView, BindlessBindings));

            const TArrayView<const uint8> BindlessSourceView = BindlessCodeView.GetNativeCode();

            TEST_EXPECT(FindSlot(BindlessBindings, EMSLBindingType::BindlessResourceHeap, 0) == MSL_BINDLESS_RESOURCE_HEAP_BUFFER_INDEX);
            TEST_EXPECT(FindSlot(BindlessBindings, EMSLBindingType::BindlessSamplerHeap, 0) == MSL_BINDLESS_SAMPLER_HEAP_BUFFER_INDEX);

            const String BindlessText(reinterpret_cast<const CHAR*>(BindlessSourceView.Data()), BindlessSourceView.Size());
            TEST_EXPECT(BindlessText.Contains("spvDescriptorSet31Binding0"));
            TEST_EXPECT(BindlessText.Contains("spvDescriptorSet31Binding1"));
            TEST_EXPECT(BindlessText.Contains("[[buffer(29)]]"));
            TEST_EXPECT(BindlessText.Contains("[[buffer(30)]]"));
        }
    }

    FShaderCompiler::Destroy();
    TEST_END();
}
