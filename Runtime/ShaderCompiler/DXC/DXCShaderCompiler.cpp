#include "Core/Platform/PlatformLibrary.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "ShaderCore/ShaderBindingConventions.h"
#include "ShaderCompiler/DXC/DXCShaderCompiler.h"
#include "ShaderCompiler/DXC/MSLShaderConverter.h"
#include "ShaderCompiler/Reflection/DXILShaderReflector.h"
#include "ShaderCompiler/Reflection/SpirvShaderReflector.h"
#include "ShaderCompiler/Spirv/SpirvTransforms.h"

static_assert(ShaderBindings::SpirvHeapMarkerSet == 31,      "Update the -fvk-bind-*-heap arguments");
static_assert(ShaderBindings::SpirvHeapResourceBinding == 0, "Update the -fvk-bind-resource-heap argument");
static_assert(ShaderBindings::SpirvHeapSamplerBinding == 1,  "Update the -fvk-bind-sampler-heap argument");
static_assert(ShaderBindings::SpirvHeapCounterBinding == 16, "Update the -fvk-bind-counter-heap argument");

enum class EDXCPart : uint32
{
    Container               = DXC_FOURCC('D', 'X', 'B', 'C'),
    ResourceDef             = DXC_FOURCC('R', 'D', 'E', 'F'),
    InputSignature          = DXC_FOURCC('I', 'S', 'G', '1'),
    OutputSignature         = DXC_FOURCC('O', 'S', 'G', '1'),
    PatchConstantSignature  = DXC_FOURCC('P', 'S', 'G', '1'),
    ShaderStatistics        = DXC_FOURCC('S', 'T', 'A', 'T'),
    ShaderDebugInfoDXIL     = DXC_FOURCC('I', 'L', 'D', 'B'),
    ShaderDebugName         = DXC_FOURCC('I', 'L', 'D', 'N'),
    FeatureInfo             = DXC_FOURCC('S', 'F', 'I', '0'),
    PrivateData             = DXC_FOURCC('P', 'R', 'I', 'V'),
    RootSignature           = DXC_FOURCC('R', 'T', 'S', '0'),
    DXIL                    = DXC_FOURCC('D', 'X', 'I', 'L'),
    PipelineStateValidation = DXC_FOURCC('P', 'S', 'V', '0'),
    RuntimeData             = DXC_FOURCC('R', 'D', 'A', 'T'),
    ShaderHash              = DXC_FOURCC('H', 'A', 'S', 'H'),
};

static LPCWSTR GetShaderStageString(EShaderStage Stage)
{
    switch (Stage)
    {
        // Compute Pipeline
        case EShaderStage::Compute:
            return L"cs";

        // Vertex Pipeline
        case EShaderStage::Vertex:
            return L"vs";
        case EShaderStage::Hull:
            return L"hs";
        case EShaderStage::Domain:
            return L"ds";
        case EShaderStage::Geometry:
            return L"gs";
        case EShaderStage::Pixel:
            return L"ps";

        // Mesh-shading Pipeline
        case EShaderStage::Mesh:
            return L"ms";
        case EShaderStage::Amplification:
            return L"as";

        // Ray-tracing Pipeline
        case EShaderStage::RayGen:
        case EShaderStage::RayAnyHit:
        case EShaderStage::RayClosestHit:
        case EShaderStage::RayIntersection:
        case EShaderStage::RayCallable:
        case EShaderStage::RayMiss:
            return L"lib";

        default:
            return L"xxx";
    }
}

static LPCWSTR GetShaderModelString(EShaderModel Model)
{
    switch (Model)
    {
        case EShaderModel::SM_6_0:
            return L"6_0";
        case EShaderModel::SM_6_1:
            return L"6_1";
        case EShaderModel::SM_6_2:
            return L"6_2";
        case EShaderModel::SM_6_3:
            return L"6_3";
        case EShaderModel::SM_6_4:
            return L"6_4";
        case EShaderModel::SM_6_5:
            return L"6_5";
        case EShaderModel::SM_6_6:
            return L"6_6";
        case EShaderModel::SM_6_7:
            return L"6_7";
        case EShaderModel::SM_6_8:
            return L"6_8";
        case EShaderModel::SM_6_9:
            return L"6_9";
        case EShaderModel::SM_6_10:
            return L"6_10";
        default:
            return L"0_0";
    }
}

static void BuildFixedCompileArguments(const FShaderCompileInfo& CompileInfo, const WString& IncludeDir, TArray<LPCWSTR>& OutArgs)
{
    OutArgs.Emplace(L"-HV 2021"); // Use HLSL 2021
    OutArgs.Emplace(L"-WX");      // Warnings as errors

    OutArgs.Emplace(L"-I");
    OutArgs.Emplace(*IncludeDir);

    if (CompileInfo.bDebugInfo)
    {
        OutArgs.Emplace(L"-Zi");
        OutArgs.Emplace(L"-Qembed_debug");
    }
    else if (CompileInfo.OutputLanguage == EShaderOutputLanguage::DXIL)
    {
        OutArgs.Emplace(L"-Qstrip_debug");
        OutArgs.Emplace(L"-Qstrip_reflect");
        OutArgs.Emplace(L"-Qstrip_priv");
    }

    // Optimization level 3
    if (CompileInfo.bOptimize)
    {
        OutArgs.Emplace(L"-O3");                  // Highest optimization level
        OutArgs.Emplace(L"-all-resources-bound");
        OutArgs.Emplace(L"-Gfa");                 // Avoid flow-control. DXC rejects this on SM 5.1+ unless -all-resources-bound is also passed
    }
}

static void BuildSpirvCompileArguments(const FShaderCompileInfo& CompileInfo, TArray<LPCWSTR>& OutArgs)
{
    OutArgs.Emplace(L"-spirv");
    OutArgs.Emplace(L"-fspv-target-env=vulkan1.2");
    OutArgs.Emplace(L"-fspv-reduce-load-size");
    OutArgs.Emplace(L"-fvk-use-dx-layout");

    // Emits HlslSemanticGOOGLE, which FSpirvShaderReflector::ReflectVertexInputs reads and FSpirvTransforms strips again
    if (CompileInfo.ShaderStage == EShaderStage::Vertex)
    {
        OutArgs.Emplace(L"-fspv-reflect");
    }

    // Set must match ShaderBindings::SpirvHeapMarkerSet, see the static_asserts above.
    OutArgs.Emplace(L"-fvk-bind-resource-heap");
    OutArgs.Emplace(L"0");
    OutArgs.Emplace(L"31");

    OutArgs.Emplace(L"-fvk-bind-sampler-heap");
    OutArgs.Emplace(L"1");
    OutArgs.Emplace(L"31");

    // Binding must match ShaderBindings::SpirvHeapCounterBinding. The heap has no counter descriptors, so this only exists to be rejected.
    OutArgs.Emplace(L"-fvk-bind-counter-heap");
    OutArgs.Emplace(L"16");
    OutArgs.Emplace(L"31");
}

static void BuildCompileArguments(const FShaderCompileInfo& CompileInfo, const WString& IncludeDir, TArray<LPCWSTR>& OutArgs)
{
    BuildFixedCompileArguments(CompileInfo, IncludeDir, OutArgs);

    if (CompileInfo.OutputLanguage != EShaderOutputLanguage::DXIL)
    {
        BuildSpirvCompileArguments(CompileInfo, OutArgs);
    }
}

static void HashWideString(uint64& OutHash, LPCWSTR Text)
{
    for (LPCWSTR Character = Text; Character && *Character; ++Character)
    {
        HashCombine(OutHash, static_cast<uint32>(*Character));
    }
}

FDXCShaderCompiler::FDXCShaderCompiler()
    : FShaderCompilerBackend()
    , DXCLib(nullptr)
    , DxcCreateInstanceFunc(nullptr)
    , VersionMajor(0)
    , VersionMinor(0)
{
}

FDXCShaderCompiler::~FDXCShaderCompiler()
{
    if (DXCLib)
    {
        FPlatformLibrary::FreeDynamicLib(DXCLib);
        DXCLib = nullptr;
    }

    DxcCreateInstanceFunc = nullptr;
}

bool FDXCShaderCompiler::Initialize()
{
    DXCLib = FPlatformLibrary::LoadDynamicLib("dxcompiler");
    if (!DXCLib)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to load 'dxcompiler'");
        return false;
    }

    DxcCreateInstanceFunc = FPlatformLibrary::LoadSymbol<DxcCreateInstanceProc>("DxcCreateInstance", DXCLib);
    if (!DxcCreateInstanceFunc)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to load 'DxcCreateInstance'");
        return false;
    }

    TComPtr<IDxcVersionInfo> VersionInfo;
    if (SUCCEEDED(DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&VersionInfo))))
    {
        if (SUCCEEDED(VersionInfo->GetVersion(&VersionMajor, &VersionMinor)))
        {
            uint32 Flags = 0;
            VersionInfo->GetFlags(&Flags);

            LOG_INFO("[FShaderCompiler]: Loaded 'dxcompiler' version %u.%u%s", VersionMajor, VersionMinor, (Flags & DxcVersionInfoFlags_Debug) ? " (Debug)" : "");
        }
    }
    else
    {
        LOG_WARNING("[FShaderCompiler]: Loaded 'dxcompiler' does not report version information");
    }

    return true;
}

bool FDXCShaderCompiler::SupportsOutputLanguage(EShaderOutputLanguage OutputLanguage) const
{
    return OutputLanguage == EShaderOutputLanguage::DXIL || OutputLanguage == EShaderOutputLanguage::SPIRV || OutputLanguage == EShaderOutputLanguage::MSL;
}

void FDXCShaderCompiler::HashCompileSettings(const FShaderCompileInfo& CompileInfo, const String& IncludeDir, uint64& InOutHash) const
{
    HashCombine(InOutHash, VersionMajor);
    HashCombine(InOutHash, VersionMinor);

    const WString WideIncludeDir = CharToWide(IncludeDir);

    TArray<LPCWSTR> CompileArgs;
    BuildCompileArguments(CompileInfo, WideIncludeDir, CompileArgs);

    for (LPCWSTR Argument : CompileArgs)
    {
        HashWideString(InOutHash, Argument);
    }
}

bool FDXCShaderCompiler::Compile(const FShaderCompileRequest& Request, FShaderCompileResult& OutResult)
{
    const FShaderCompileInfo& CompileInfo = *Request.CompileInfo;

    TComPtr<IDxcUtils> Utils;
    HRESULT hr = DxcCreateInstanceFunc(CLSID_DxcUtils, IID_PPV_ARGS(&Utils));
    if (FAILED(hr))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to create Utils");
        return false;
    }

    TComPtr<IDxcCompiler3> Compiler;
    hr = DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
    if (FAILED(hr))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to create Compiler");
        return false;
    }

    const WString WideShaderIncludeDir = CharToWide(Request.IncludeDir);

    TArray<LPCWSTR> CompileArgs;
    BuildCompileArguments(CompileInfo, WideShaderIncludeDir, CompileArgs);

    // Retrieve the shader target
    const LPCWSTR ShaderStageText = GetShaderStageString(CompileInfo.ShaderStage);
    const LPCWSTR ShaderModelText = GetShaderModelString(CompileInfo.ShaderModel);

    constexpr uint32 BufferLength = sizeof("xxx_x_x");
    WCHAR TargetProfile[BufferLength];
    CStringWide::Snprintf(TargetProfile, BufferLength, L"%ls_%ls", ShaderStageText, ShaderModelText);

    const WString WideFilePath   = CharToWide(Request.FilePath);
    const WString WideEntrypoint = CharToWide(CompileInfo.EntryPoint);

    // The source is already preprocessed, so there are no defines to pass
    TComPtr<IDxcCompilerArgs> CompileArguments;
    hr = Utils->BuildArguments(*WideFilePath, *WideEntrypoint, TargetProfile, CompileArgs.Data(), CompileArgs.Size(), nullptr, 0, &CompileArguments);
    if (FAILED(hr))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to create compiler arguments");
        return false;
    }

    DxcBuffer SourceBuffer;
    SourceBuffer.Ptr      = Request.Source.Data();
    SourceBuffer.Size     = Request.Source.Size();
    SourceBuffer.Encoding = DXC_CP_ACP;

    // Compile shader
    TComPtr<IDxcResult> Result;
    hr = Compiler->Compile(&SourceBuffer, CompileArguments->GetArguments(), CompileArguments->GetCount(), nullptr, IID_PPV_ARGS(&Result));
    if (FAILED(hr))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to Compile");
        return false;
    }

    // Check the compilation result
    HRESULT CompilationResult;
    if (FAILED(Result->GetStatus(&CompilationResult)))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to Retrieve result. Unknown Error.");
        return false;
    }

    // Retrieve errors and warnings
    TComPtr<IDxcBlobUtf8> PrintBlob;
    Result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&PrintBlob), nullptr);

    if (PrintBlob && PrintBlob->GetBufferSize() > 0)
    {
        OutResult.Messages = String(reinterpret_cast<LPCSTR>(PrintBlob->GetBufferPointer()), static_cast<int32>(PrintBlob->GetBufferSize()));
    }

    // If the error encountered an error
    if (FAILED(CompilationResult))
    {
        return false;
    }

    // Retrieve the compiled blob
    TComPtr<IDxcBlob> CompiledBlob;
    hr = Result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&CompiledBlob), nullptr);
    if (FAILED(hr))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to retrieve compiled shader");
        return false;
    }

    const uint32 BlobSize = static_cast<uint32>(CompiledBlob->GetBufferSize());

    if (CompileInfo.OutputLanguage == EShaderOutputLanguage::DXIL)
    {
    #if PLATFORM_WINDOWS
        TComPtr<IDxcBlob> ReflectionBlob;
        if (FAILED(Result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&ReflectionBlob), nullptr)) || !ReflectionBlob)
        {
            ReflectionBlob = CompiledBlob;
        }

        if (!FDXILShaderReflector::Reflect(Utils.Get(), CompiledBlob.Get(), ReflectionBlob.Get(), CompileInfo, OutResult.Reflection, OutResult.Messages))
        {
            return false;
        }

        OutResult.ByteCode.Resize(static_cast<int32>(BlobSize));
        Memory::Memcpy(OutResult.ByteCode.Data(), CompiledBlob->GetBufferPointer(), BlobSize);
        return true;
    #else
        OutResult.Messages += "DXIL can only be reflected on Windows\n";
        return false;
    #endif
    }

    if (Request.bVerboseLogging && CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL)
    {
        LOG_INFO("[FShaderCompiler]: Compiled Size (Before any transformations): %u Bytes", BlobSize);
    }

    TArray<uint32> Spirv(reinterpret_cast<const uint32*>(CompiledBlob->GetBufferPointer()), static_cast<int32>(BlobSize / sizeof(uint32)));

    // The semantics only exist as HlslSemanticGOOGLE decorations, which PrepareForVulkan strips
    if (CompileInfo.ShaderStage == EShaderStage::Vertex && !FSpirvShaderReflector::ReflectVertexInputs(Spirv, OutResult.Reflection, OutResult.Messages))
    {
        return false;
    }

    if (CompileInfo.OutputLanguage == EShaderOutputLanguage::SPIRV)
    {
        TArray<uint32> PreparedSpirv;
        String         PrepareError;
        if (!FSpirvTransforms::PrepareForVulkan(Spirv, PreparedSpirv, &PrepareError))
        {
            OutResult.Messages += PrepareError;
            return false;
        }

        Spirv = ::Move(PreparedSpirv);
    }

    // Before reflection, so the word offsets match the words that ship
    if (!CompileInfo.bDebugInfo)
    {
        TArray<uint32> StrippedSpirv;
        if (!FSpirvTransforms::StripDebugInstructions(Spirv, StrippedSpirv))
        {
            OutResult.Messages += "Failed to strip the SPIR-V debug instructions\n";
            return false;
        }

        Spirv = ::Move(StrippedSpirv);
    }

    // MSL slots only exist once SPIRV-Cross has emitted the source, so MSL reflects inside the conversion instead
    if (CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL)
    {
        return FMSLShaderConverter::Convert(Spirv, CompileInfo, OutResult.Reflection, OutResult.ByteCode, OutResult.Messages);
    }

    if (!FSpirvShaderReflector::Reflect(Spirv, CompileInfo, OutResult.Reflection, OutResult.Messages))
    {
        return false;
    }

    OutResult.ByteCode = TArray<uint8>(reinterpret_cast<const uint8*>(Spirv.Data()), Spirv.SizeInBytes());
    return true;
}