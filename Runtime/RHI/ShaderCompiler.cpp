#include "Core/Containers/ComPtr.h"
#include "Core/RefCountedBase.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Filesystem/File.h"
#include "Core/Memory/Malloc.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/CRC.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/ShaderStats.h"

#include <spirv_cross_c.h>

static TAutoConsoleVariable<bool> CVarShaderDebug(
    "RHI.ShaderCompiler.Debug",
    "Enable debug information in the Shaders",
    true);

static TAutoConsoleVariable<bool> CVarVerboseLogging(
    "RHI.ShaderCompiler.VerboseLogging",
    "Enable verbose logging in the ShaderCompiler",
    false);

static TAutoConsoleVariable<bool> CVarMapMin16FloatToFloat(
    "RHI.ShaderCompiler.MapMin16FloatToFloat",
    "Map the min16float type family to full-precision float on non-HLSL backends (works around DXC's SPIR-V "
    "RelaxedPrecision codegen bug). Disable to keep native min-precision types (also sets MIN16FLOAT_AVAILABLE).",
    true);

static FAutoConsoleCommand CCmdDumpShaderCompileStats(
    "RHI.DumpShaderCompileStats",
    "Logs how many shaders the compiler has built and how long they took",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        if (FShaderCompiler* Compiler = FShaderCompiler::TryGet())
        {
            Compiler->LogCompileStats();
        }
    }));

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

    if (CVarShaderDebug.GetValue())
    {
        OutArgs.Emplace(L"-Zi");
        OutArgs.Emplace(L"-Qembed_debug");
    }

    // Optimization level 3
    if (CompileInfo.bOptimize)
    {
        OutArgs.Emplace(L"-O3");                  // Highest optimization level
        OutArgs.Emplace(L"-all-resources-bound");
        OutArgs.Emplace(L"-Gfa");                 // Avoid flow-control. DXC rejects this on SM 5.1+ unless -all-resources-bound is also passed
    }
}

static void BuildSpirvCompileArguments(TArray<LPCWSTR>& OutArgs)
{
    OutArgs.Emplace(L"-spirv");
    OutArgs.Emplace(L"-fspv-target-env=vulkan1.2");
    OutArgs.Emplace(L"-fspv-reduce-load-size");
    OutArgs.Emplace(L"-fvk-use-dx-layout");

    // Set must match VULKAN_BINDLESS_HEAP_MARKER_SET in VulkanConstants.h.
    OutArgs.Emplace(L"-fvk-bind-resource-heap");
    OutArgs.Emplace(L"0");
    OutArgs.Emplace(L"31");

    OutArgs.Emplace(L"-fvk-bind-sampler-heap");
    OutArgs.Emplace(L"1");
    OutArgs.Emplace(L"31");

    // Binding must match VULKAN_BINDLESS_COUNTER_MARKER_BINDIN. The heap has no counter descriptors, so this only exists to be rejected.
    OutArgs.Emplace(L"-fvk-bind-counter-heap");
    OutArgs.Emplace(L"16");
    OutArgs.Emplace(L"31");
}

static void BuildCompileDefines(const FShaderCompileInfo& CompileInfo, TArray<WString>& OutStorage, TArray<DxcDefine>& OutDefines)
{
    // Add defines that identify the target shader backend
    OutDefines.Emplace(DxcDefine{ L"SHADER_BACKEND_D3D12" , L"(1)" });
    OutDefines.Emplace(DxcDefine{ L"SHADER_BACKEND_VULKAN", L"(2)" });
    OutDefines.Emplace(DxcDefine{ L"SHADER_BACKEND_METAL" , L"(3)" });

    if (CompileInfo.OutputLanguage == EShaderOutputLanguage::HLSL)
    {
        OutDefines.Add({ L"SHADER_BACKEND", L"SHADER_BACKEND_D3D12" });
    }
    else if (CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL)
    {
        OutDefines.Add({ L"SHADER_BACKEND", L"SHADER_BACKEND_METAL" });
    }
    else if (CompileInfo.OutputLanguage == EShaderOutputLanguage::SPIRV)
    {
        OutDefines.Add({ L"SHADER_BACKEND", L"SHADER_BACKEND_VULKAN" });
    }
    else
    {
        OutDefines.Add({ L"SHADER_BACKEND", L"(0)" });
    }

    const bool bMapMin16FloatToFloat = (CompileInfo.OutputLanguage != EShaderOutputLanguage::HLSL) && CVarMapMin16FloatToFloat.GetValue();
    if (bMapMin16FloatToFloat)
    {
        OutDefines.Add({ L"min16float",  L"float"  });
        OutDefines.Add({ L"min16float2", L"float2" });
        OutDefines.Add({ L"min16float3", L"float3" });
        OutDefines.Add({ L"min16float4", L"float4" });
        OutDefines.Add({ L"MIN16FLOAT_AVAILABLE", L"(0)" });
    }
    else
    {
        OutDefines.Add({ L"MIN16FLOAT_AVAILABLE", L"(1)" });
    }

    if (!CompileInfo.Defines.IsEmpty())
    {
        OutStorage.Reserve(CompileInfo.Defines.Size() * 2);

        for (const FShaderDefine& Define : CompileInfo.Defines)
        {
            const WString& WideDefine = OutStorage.Emplace(CharToWide(Define.Define));
            const WString& WideValue  = OutStorage.Emplace(CharToWide(Define.Value));

            OutDefines.Add({ *WideDefine, *WideValue });
        }
    }
}

class FScopedCompileTimer
{
public:
    FScopedCompileTimer(AtomicInt64& InNumCompiles, AtomicInt64& InTotalTimeNS)
        : NumCompiles(InNumCompiles)
        , TotalTimeNS(InTotalTimeNS)
        , StartTime(FPlatformTime::QueryPerformanceCounter())
    {
    }

    ~FScopedCompileTimer()
    {
        const uint64 Elapsed = FPlatformTime::QueryPerformanceCounter() - StartTime;
        const double Seconds = static_cast<double>(Elapsed) / static_cast<double>(FPlatformTime::QueryPerformanceFrequency());

        NumCompiles.Add(1);
        TotalTimeNS.Add(static_cast<int64>(Seconds * 1000.0 * 1000.0 * 1000.0));
    }

private:
    AtomicInt64& NumCompiles;
    AtomicInt64& TotalTimeNS;
    uint64       StartTime;
};

class FRecordingIncludeHandler final : public IDxcIncludeHandler
{
public:
    FRecordingIncludeHandler(IDxcIncludeHandler* InInnerHandler, TArray<String>& InRecordedIncludes)
        : InnerHandler(InInnerHandler)
        , RecordedIncludes(InRecordedIncludes)
    {
    }

    virtual HRESULT LoadSource(LPCWSTR Filename, IDxcBlob** ppIncludeSource) override final
    {
        const HRESULT Result = InnerHandler->LoadSource(Filename, ppIncludeSource);
        if (SUCCEEDED(Result) && Filename)
        {
            const String IncludePath = WideToChar(WString(Filename));
            if (!RecordedIncludes.Contains(IncludePath))
            {
                RecordedIncludes.Emplace(IncludePath);
            }
        }

        return Result;
    }

    virtual ULONG AddRef()  override final { return 1; }
    virtual ULONG Release() override final { return 1; }

    virtual HRESULT QueryInterface(REFIID Riid, LPVOID* ppvObject) override final
    {
        if (!ppvObject)
        {
            return E_INVALIDARG;
        }

        if (Riid == __uuidof(IUnknown) || Riid == __uuidof(IDxcIncludeHandler))
        {
            *ppvObject = reinterpret_cast<LPVOID>(this);
            AddRef();
            return S_OK;
        }

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

private:
    IDxcIncludeHandler* InnerHandler;
    TArray<String>&     RecordedIncludes;
};

class FShaderBlob final : public IDxcBlob, public FRefCountedBase
{
public:
    FShaderBlob(LPCVOID InData, SIZE_T InSize)
        : Data(nullptr)
        , Size(InSize)
    {
        Data = Memory::Malloc(Size);
        Memory::Memcpy(Data, InData, Size);
    }

    ~FShaderBlob()
    {
        Memory::Free(Data);
    }

    virtual SIZE_T GetBufferSize()    override final { return Size; }
    virtual LPVOID GetBufferPointer() override final { return Data; }

    virtual ULONG AddRef()  override final { return static_cast<ULONG>(FRefCountedBase::AddRef()); }
    virtual ULONG Release() override final { return static_cast<ULONG>(FRefCountedBase::Release()); }

    virtual HRESULT QueryInterface(REFIID Riid, LPVOID* ppvObject) override final
    {
        if (!ppvObject)
        {
            return E_INVALIDARG;
        }

        *ppvObject = nullptr;

        // TODO: Could be ID3DBlob as well possibly, however, should not be needed for now
        if (Riid == __uuidof(IUnknown) || Riid == __uuidof(IDxcBlob))
        {
            *ppvObject = reinterpret_cast<LPVOID>(this);
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

private:
    LPVOID Data;
    SIZE_T Size;
};

FShaderCompiler* FShaderCompiler::ShaderCompiler = nullptr;

FShaderCompiler::FShaderCompiler(const String& InAssetPath)
    : DXCLib(nullptr)
    , DxcCreateInstanceFunc(nullptr)
    , AssetPath(InAssetPath)
    , DXCVersionMajor(0)
    , DXCVersionMinor(0)
    , NumCompiles(0)
    , TotalCompileTimeNS(0)
{
}

FShaderCompiler::~FShaderCompiler()
{
    // Destroy DXC
    if (DXCLib)
    {
        FPlatformLibrary::FreeDynamicLib(DXCLib);
        DXCLib = nullptr;
    }

    DxcCreateInstanceFunc = nullptr;
}

bool FShaderCompiler::Initialize(const String& InAssetPath)
{
    CHECK(ShaderCompiler == nullptr);

    ShaderCompiler = new FShaderCompiler(InAssetPath);
    if (!ShaderCompiler->InitializeDXC())
    {
        delete ShaderCompiler;
        ShaderCompiler = nullptr;
        return false;
    }

    return true;
}

void FShaderCompiler::Destroy()
{
    if (ShaderCompiler)
    {
        delete ShaderCompiler;
        ShaderCompiler = nullptr;
    }
}

EShaderOutputLanguage FShaderCompiler::GetOutputLanguageBasedOnRHI()
{
    if (RHI::IsInitialized())
    {
        const ERHIType RHIType = RHI::Device->GetRHIType();
        if (RHIType == ERHIType::Metal)
        {
            return EShaderOutputLanguage::MSL;
        }
        else if (RHIType == ERHIType::Vulkan)
        {
            return EShaderOutputLanguage::SPIRV;
        }
    }

    // Return HLSL for NullRHI and D3D12RHI
    return EShaderOutputLanguage::HLSL;
}

bool FShaderCompiler::InitializeDXC()
{
    // Init DXC
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
        if (SUCCEEDED(VersionInfo->GetVersion(&DXCVersionMajor, &DXCVersionMinor)))
        {
            uint32 Flags = 0;
            VersionInfo->GetFlags(&Flags);

            LOG_INFO("[FShaderCompiler]: Loaded 'dxcompiler' version %u.%u%s", DXCVersionMajor, DXCVersionMinor, (Flags & DxcVersionInfoFlags_Debug) ? " (Debug)" : "");
        }
    }
    else
    {
        LOG_WARNING("[FShaderCompiler]: Loaded 'dxcompiler' does not report version information");
    }

    return true;
}

bool FShaderCompiler::CompileFromFile(const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies)
{
    // Add asset-path to the filename
    const String FilePath = AssetPath + '/' + Filename;
    
    // Store the ShaderFile in this array
    TArray<CHAR> Text;

    {
        // Open the file
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(FilePath);
        if (!FileHandle)
        {
            LOG_ERROR("[FShaderCompiler]: Failed to open file '%s'", *Filename);
            return false;
        }

        // Read the full file as a text-file
        if (!File::ReadTextFile(FileHandle.Get(), Text))
        {
            LOG_ERROR("[FShaderCompiler]: Failed to read file '%s'", *Filename);
            return false;
        }
    }

    // The pre-processor only reports the files it includes, so the shader itself is added here
    if (OutDependencies)
    {
        OutDependencies->Emplace(FilePath);
    }

    // Compile the source
    const String Source(Text.Data(), Text.Size());
    return Compile(Source, FilePath, CompileInfo, OutByteCode, OutDependencies);
}

bool FShaderCompiler::CompileFromSource(const String& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies)
{
    return Compile(ShaderSource, "", CompileInfo, OutByteCode, OutDependencies);
}

static void HashWideString(uint64& OutHash, LPCWSTR Text)
{
    for (LPCWSTR Character = Text; Character && *Character; ++Character)
    {
        HashCombine(OutHash, static_cast<uint32>(*Character));
    }
}

uint64 FShaderCompiler::ComputeCompileHash(const String& SourceFile, const FShaderCompileInfo& CompileInfo) const
{
    uint64 Hash = THash<String>::GetHash(SourceFile);

    HashCombine(Hash, CompileInfo.EntryPoint);
    HashCombine(Hash, CompileInfo.ShaderModel);
    HashCombine(Hash, CompileInfo.ShaderStage);
    HashCombine(Hash, CompileInfo.OutputLanguage);
    HashCombine(Hash, DXCVersionMajor);
    HashCombine(Hash, DXCVersionMinor);

    const WString WideShaderIncludeDir = CharToWide(AssetPath + "/Shaders");

    TArray<LPCWSTR> CompileArgs;
    BuildFixedCompileArguments(CompileInfo, WideShaderIncludeDir, CompileArgs);

    if (CompileInfo.OutputLanguage != EShaderOutputLanguage::HLSL)
    {
        BuildSpirvCompileArguments(CompileArgs);
    }

    for (LPCWSTR Argument : CompileArgs)
    {
        HashWideString(Hash, Argument);
    }

    TArray<WString>   DefineStrings;
    TArray<DxcDefine> DxcDefines;
    BuildCompileDefines(CompileInfo, DefineStrings, DxcDefines);

    for (const DxcDefine& Define : DxcDefines)
    {
        HashWideString(Hash, Define.Name);
        HashWideString(Hash, Define.Value);
    }

    return Hash;
}

void FShaderCompiler::LogCompileStats() const
{
    const int64     Count = NumCompiles.Load();
    const FTimespan Total = FTimespan(static_cast<uint64>(TotalCompileTimeNS.Load()));

    if (Count <= 0)
    {
        LOG_INFO("[FShaderCompiler]: No shaders compiled");
        return;
    }

    LOG_INFO("[FShaderCompiler]: Compiled %lld shaders in %.2f seconds (%.1f ms average)",
        Count, Total.AsSeconds(), Total.AsMilliseconds() / static_cast<double>(Count));
}

bool FShaderCompiler::Compile(const String& ShaderSource, const String& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies)
{
    FScopedCompileTimer CompileTimer(NumCompiles, TotalCompileTimeNS);

    STAT_ADD(STAT_Shader_CompileCount, 1);
    OutByteCode.Clear();

    if (RHI::MaxShaderModel != EShaderModel::Unknown && CompileInfo.ShaderModel > RHI::MaxShaderModel)
    {
        LOG_ERROR("[FShaderCompiler]: '%s' requests Shader Model %s but the device supports at most %s",
            FilePath.IsEmpty() ? *CompileInfo.EntryPoint : *FilePath, ToString(CompileInfo.ShaderModel), ToString(RHI::MaxShaderModel));
    }

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

    TComPtr<IDxcIncludeHandler> DefaultIncludeHandler;
    hr = Utils->CreateDefaultIncludeHandler(&DefaultIncludeHandler);
    if (FAILED(hr))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to create IncludeHandler");
        return false;
    }

    TArray<String>           DiscardedIncludes;
    FRecordingIncludeHandler IncludeHandler(DefaultIncludeHandler.Get(), OutDependencies ? *OutDependencies : DiscardedIncludes);

    const WString WideShaderIncludeDir = CharToWide(AssetPath + "/Shaders");

    TArray<LPCWSTR> CompileArgs;
    BuildFixedCompileArguments(CompileInfo, WideShaderIncludeDir, CompileArgs);

    TArray<WString>   DefineStrings;
    TArray<DxcDefine> DxcDefines;
    BuildCompileDefines(CompileInfo, DefineStrings, DxcDefines);

    // Log all the defines that are used for this shader-compilation
    const bool bVerboseLogging = CVarVerboseLogging.GetValue();
    if (bVerboseLogging)
    {
        if (!FilePath.IsEmpty())
        {
            LOG_INFO("[FShaderCompiler]: Compiling shader '%s', using the following defines:", *FilePath);
        }
        else
        {
            LOG_INFO("[FShaderCompiler]: Compiling shader, using the following defines:");
        }

        for (const DxcDefine& Define : DxcDefines)
        {
            LOG_INFO("    %S = %S", Define.Name, Define.Value);
        }
    }

    // Helper for building arguments for compilation and preprocessing
    const auto BuildArguments = [&](const String& FilePath, const String& EntryPoint, const FShaderCompileInfo& CompileInfo)
    {
        // Retrieve the shader target
        const LPCWSTR ShaderStageText = GetShaderStageString(CompileInfo.ShaderStage);
        const LPCWSTR ShaderModelText = GetShaderModelString(CompileInfo.ShaderModel);

        constexpr uint32 BufferLength = sizeof("xxx_x_x");
        WCHAR TargetProfile[BufferLength];
        CStringWide::Snprintf(TargetProfile, BufferLength, L"%ls_%ls", ShaderStageText, ShaderModelText);

        // Use the asset-folder as base for the shader-files
        const WString WideFilePath   = CharToWide(FilePath);
        const WString WideEntrypoint = CharToWide(EntryPoint);

        // Build the arguments for the preprocessing step
        TComPtr<IDxcCompilerArgs> CompileArguments;
        HRESULT hr = Utils->BuildArguments(*WideFilePath, *WideEntrypoint, TargetProfile, CompileArgs.Data(), CompileArgs.Size(), DxcDefines.Data(), DxcDefines.Size(), &CompileArguments);
        if (FAILED(hr))
        {
            return TComPtr<IDxcCompilerArgs>(nullptr);
        }
        else
        {
            return CompileArguments;
        }
    };

    // Retrieve the pre-processing compiler arguments
    TComPtr<IDxcCompilerArgs> PreProcessorArguments = BuildArguments(FilePath, CompileInfo.EntryPoint, CompileInfo);
    if (!PreProcessorArguments)
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to create pre-process compiler arguments");
        return false;
    }
    else
    {
        LPCWSTR PreProcessArgs[] = 
        { 
            L"-P",
        };

        PreProcessorArguments->AddArguments(PreProcessArgs, 1);
    }

    // Preprocess shader source
    DxcBuffer SourceBuffer;
    SourceBuffer.Ptr      = ShaderSource.Data();
    SourceBuffer.Size     = ShaderSource.Size();
    SourceBuffer.Encoding = DXC_CP_ACP;

    TComPtr<IDxcResult> PreprocessResult;
    hr = Compiler->Compile(&SourceBuffer, PreProcessorArguments->GetArguments(), static_cast<uint32>(PreProcessorArguments->GetCount()), &IncludeHandler, IID_PPV_ARGS(&PreprocessResult));
    if (FAILED(hr))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to preprocess shader");
        DEBUG_BREAK();
        return false;
    }

    // Check the compilation result
    HRESULT PreProcessResult;
    if (FAILED(PreprocessResult->GetStatus(&PreProcessResult)))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to retrieve pre-process result. Unknown Error.");
        return false;
    }

    // If the error encountered an error
    if (FAILED(PreProcessResult))
    {
        // Retrieve errors
        TComPtr<IDxcBlobUtf8> PrintBlob;
        PreprocessResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&PrintBlob), nullptr);

        if (PrintBlob && PrintBlob->GetBufferSize() > 0)
        {
            LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to pre-process with error: %s", reinterpret_cast<LPCSTR>(PrintBlob->GetBufferPointer()));
        }
        else
        {
            LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to pre-process with. Unknown ERROR.");
        }

        return false;
    }

    TComPtr<IDxcBlob> PreprocessedBlob;
    hr = PreprocessResult->GetOutput(DXC_OUT_HLSL, IID_PPV_ARGS(&PreprocessedBlob), nullptr);
    if (FAILED(hr))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to retrieve pre-processed shader");
        return false;
    }

    // Handle language selection
    String Source(reinterpret_cast<const char*>(PreprocessedBlob->GetBufferPointer()), static_cast<int32>(PreprocessedBlob->GetBufferSize()));
    if (CompileInfo.OutputLanguage != EShaderOutputLanguage::HLSL)
    {
        BuildSpirvCompileArguments(CompileArgs);
    }

    // Build the arguments for the compiler
    TComPtr<IDxcCompilerArgs> CompileArguments = BuildArguments(FilePath, CompileInfo.EntryPoint, CompileInfo);
    if (!CompileArguments)
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to create compiler arguments");
        return false;
    }

    // Convert preprocessed source to DxcBuffer
    SourceBuffer.Ptr  = Source.Data();
    SourceBuffer.Size = Source.Size();

    // Compile shader
    TComPtr<IDxcResult> Result;
    hr = Compiler->Compile(&SourceBuffer, CompileArguments->GetArguments(), CompileArguments->GetCount(), &IncludeHandler, IID_PPV_ARGS(&Result));
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

    // If the error encountered an error
    if (FAILED(CompilationResult))
    {
        // Retrieve errors
        TComPtr<IDxcBlobUtf8> PrintBlob;
        Result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&PrintBlob), nullptr);

        if (PrintBlob && PrintBlob->GetBufferSize() > 0)
        {
            LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to compile with error: %s", reinterpret_cast<LPCSTR>(PrintBlob->GetBufferPointer()));
        }
        else
        {
            LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to compile with. Unknown ERROR.");
        }

        return false;
    }

    if (bVerboseLogging)
    {
        // Retrieve errors
        TComPtr<IDxcBlobUtf8> PrintBlob;
        Result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&PrintBlob), nullptr);

        if (PrintBlob && PrintBlob->GetBufferSize() > 0)
        {
            const String Output(reinterpret_cast<LPCSTR>(PrintBlob->GetBufferPointer()), uint32(PrintBlob->GetBufferSize()));
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader with the following output: %s", *Output);
        }
        else
        {
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader.");
        }
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
    OutByteCode.Resize(BlobSize);

    Memory::Memcpy(OutByteCode.Data(), CompiledBlob->GetBufferPointer(), BlobSize);

    if (CompileInfo.OutputLanguage != EShaderOutputLanguage::HLSL)
    {
        if (bVerboseLogging)
        {
            LOG_INFO("[FShaderCompiler]: Compiled Size (Before any transformations): %u Bytes", BlobSize);
        }

        if (CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL)
        {
            if (!ConvertSpirvToMetalShader(FilePath, CompileInfo, OutByteCode))
            {
                DEBUG_BREAK();
                return false;
            }
        }

        if (bVerboseLogging)
        {
            LOG_INFO("[FShaderCompiler]: Compiled Size (Final size): %u Bytes", OutByteCode.Size());
        }
    }
    else if (bVerboseLogging)
    {
        LOG_INFO("[FShaderCompiler]: Compiled Size: %u Bytes", BlobSize);
    }
    
    if (OutByteCode.IsEmpty())
    {
        LOG_WARNING("[FShaderCompiler]: Resulting bytecode is empty");
    }

    if (bVerboseLogging && OutDependencies)
    {
        LOG_INFO("[FShaderCompiler]: Compiled from the following files:");

        for (const String& Dependency : *OutDependencies)
        {
            LOG_INFO("    %s", *Dependency);
        }
    }

    // If verbose logging is turned off, atleast log that we successfully compiled the shader
    if (!bVerboseLogging)
    {
        if (!FilePath.IsEmpty())
        {
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader '%s'", *FilePath);
        }
        else
        {
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader");
        }
    }

    return true;
}

bool FShaderCompiler::ConvertSpirvToMetalShader(const String& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    if (OutByteCode.IsEmpty() || CompileInfo.EntryPoint.IsEmpty())
    {
        return false;
    }

    spvc_context Context = nullptr;
    spvc_result Result = spvc_context_create(&Context);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to create SpvcContext");
        DEBUG_BREAK();
        return false;
    }

    spvc_context_set_error_callback(Context, [](void*, const CHAR* Error)
    {
        LOG_ERROR("[SPIRV-Cross Error] %s", Error);
    }, nullptr);

    // The code size needs to be aligned to the element-size
    constexpr uint32 ElementSize = sizeof(unsigned int) / sizeof(uint8);
    CHECK(OutByteCode.Size() % ElementSize == 0);
    const uint32 WordCount = OutByteCode.Size() / ElementSize;
    
    spvc_parsed_ir ParsedCode = nullptr;
    Result = spvc_context_parse_spirv(Context, reinterpret_cast<const SpvId*>(OutByteCode.Data()), WordCount, &ParsedCode);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to parse Spirv");
        DEBUG_BREAK();
        return false;
    }

    spvc_compiler CompilerMSL = nullptr;
    Result = spvc_context_create_compiler(Context, SPVC_BACKEND_MSL, ParsedCode, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &CompilerMSL);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to create MSL compiler");
        DEBUG_BREAK();
        return false;
    }

    spvc_compiler_options CompilerOptions = nullptr;
    Result = spvc_compiler_create_compiler_options(CompilerMSL, &CompilerOptions);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to create MSL compiler options");
        DEBUG_BREAK();
        return false;
    }

    Result = spvc_compiler_options_set_uint(CompilerOptions, SPVC_COMPILER_OPTION_MSL_VERSION, SPVC_MAKE_MSL_VERSION(2, 3, 0));
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to set the MSL version");
        DEBUG_BREAK();
        return false;
    }

    Result = spvc_compiler_install_compiler_options(CompilerMSL, CompilerOptions);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to install MSL compiler options");
        DEBUG_BREAK();
        return false;
    }

    const CHAR* MSLSource = nullptr;
    Result = spvc_compiler_compile(CompilerMSL, &MSLSource);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to create MSL");
        DEBUG_BREAK();
        return false;
    }

    // Create a new array
    const uint32 SourceLength = CString::Strlen(MSLSource);
    TArray<uint8> NewShader(reinterpret_cast<const uint8*>(MSLSource), (SourceLength + 1) * sizeof(uint8));
    NewShader[SourceLength] = 0;

    // Now we can destroy the context
    spvc_context_destroy(Context);

    // Dump the metal file to disk
    if (!FilePath.IsEmpty())
    {
        if (!DumpContentToFile(NewShader, FilePath + "_" + ToString(CompileInfo.ShaderStage) + ".metal"))
        {
            DEBUG_BREAK();
            return false;
        }
    }
    
    // Output the code
    OutByteCode = ::Move(NewShader);
    return true;
}

bool FShaderCompiler::DumpContentToFile(const TArray<uint8>& ByteCode, const String& Filename)
{
    // Permutations of one shader all dump to the same path, so concurrent compiles would otherwise interleave in the file.
    TScopedLock Lock(DumpCS);

    TFileRef<IPlatformFile> Output = FPlatformFile::OpenForWrite(Filename);
    if (!Output)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to open file '%s'", *Filename);
        return false;
    }

    Output->Write(ByteCode.Data(), ByteCode.Size());
    return true;
}

String FShaderCompiler::CreateArgString(const TArrayView<LPCWSTR> Args)
{
    WString NewString;
    for (LPCWSTR CurrentArg : Args)
    {
        NewString += CurrentArg;
        NewString += L' ';
    }

    return WideToChar(NewString);
}
