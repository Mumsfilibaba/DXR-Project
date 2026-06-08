#include "Core/Containers/ComPtr.h"
#include "Core/RefCountedBase.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Platform/PlatformFile.h"
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

enum class EDXCPart
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
        default:
            return L"0_0";
    }
}

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

FShaderCompiler* FShaderCompiler::GShaderCompiler = nullptr;

FShaderCompiler::FShaderCompiler(const FString& InAssetPath)
    : DXCLib(nullptr)
    , DxcCreateInstanceFunc(nullptr)
    , AssetPath(InAssetPath)
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

bool FShaderCompiler::Create(const FString& InAssetPath)
{
    CHECK(GShaderCompiler == nullptr);

    GShaderCompiler = new FShaderCompiler(InAssetPath);
    if (!GShaderCompiler->Initialize())
    {
        delete GShaderCompiler;
        GShaderCompiler = nullptr;
        return false;
    }

    return true;
}

void FShaderCompiler::Destroy()
{
    if (GShaderCompiler)
    {
        delete GShaderCompiler;
        GShaderCompiler = nullptr;
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

bool FShaderCompiler::Initialize()
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

    return true;
}

bool FShaderCompiler::CompileFromFile(const FString& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    // Add asset-path to the filename
    const FString FilePath = AssetPath + '/' + Filename;
    
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

    // Compile the source
    const FString Source(Text.Data(), Text.Size());
    return Compile(Source, FilePath, CompileInfo, OutByteCode);
}

bool FShaderCompiler::CompileFromSource(const FString& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    return Compile(ShaderSource, "", CompileInfo, OutByteCode);
}

bool FShaderCompiler::Compile(const FString& ShaderSource, const FString& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    STAT_ADD(STAT_Shader_CompileCount, 1);
    OutByteCode.Clear();

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

    TComPtr<IDxcIncludeHandler> IncludeHandler;
    hr = Utils->CreateDefaultIncludeHandler(&IncludeHandler);
    if (FAILED(hr))
    {
        LOG_ERROR_CRITICAL("[FShaderCompiler]: FAILED to create IncludeHandler");
        return false;
    }

    // Add compile arguments
    TArray<LPCWSTR> CompileArgs =
    {
        L"-HV 2021",     // Use HLSL 2021
        L"-Gfa",         // Avoid flow-control
        L"-WX",          // Warnings as errors
        L"-Qembed_debug" // We are forced to embed debug information in order to get all the information we need
    };

    if (CVarShaderDebug.GetValue())
    {
        CompileArgs.Emplace(L"-Zi");
    }

    // Optimization level 3
    if (CompileInfo.bOptimize)
    {
        CompileArgs.Emplace(L"-O3"); // Highest optimization level
        CompileArgs.Emplace(L"-all-resources-bound");
    }

    // Add defines that is based on language
    TArray<DxcDefine> DxcDefines =
    {
        { L"SHADER_LANG_HLSL" , L"(1)" },
        { L"SHADER_LANG_SPIRV", L"(2)" },
        { L"SHADER_LANG_MSL"  , L"(3)" },
    };

    if (CompileInfo.OutputLanguage == EShaderOutputLanguage::HLSL)
    {
        DxcDefines.Add({ L"SHADER_LANG", L"SHADER_LANG_HLSL" });
    }
    else if (CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL)
    {
        DxcDefines.Add({ L"SHADER_LANG", L"SHADER_LANG_MSL" });
    }
    else if (CompileInfo.OutputLanguage == EShaderOutputLanguage::SPIRV)
    {
        DxcDefines.Add({ L"SHADER_LANG", L"SHADER_LANG_SPIRV" });
    }
    else
    {
        DxcDefines.Add({ L"SHADER_LANG", L"(0)" });
    }

    // Convert defines
    TArray<FStringWide> DefineStrings;
    if (!CompileInfo.Defines.IsEmpty())
    {
        DefineStrings.Reserve(CompileInfo.Defines.Size() * 2);

        for (const FShaderDefine& Define : CompileInfo.Defines)
        {
            const FStringWide& WideDefine = DefineStrings.Emplace(CharToWide(Define.Define));
            const FStringWide& WideValue  = DefineStrings.Emplace(CharToWide(Define.Value));
            DxcDefines.Add({ *WideDefine, *WideValue });
        }
    }
 
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
    const auto BuildArguments = [&](const FString& FilePath, const FString& EntryPoint, const FShaderCompileInfo& CompileInfo)
    {
        // Retrieve the shader target
        const LPCWSTR ShaderStageText = GetShaderStageString(CompileInfo.ShaderStage);
        const LPCWSTR ShaderModelText = GetShaderModelString(CompileInfo.ShaderModel);

        constexpr uint32 BufferLength = sizeof("xxx_x_x");
        WCHAR TargetProfile[BufferLength];
        FCStringWide::Snprintf(TargetProfile, BufferLength, L"%ls_%ls", ShaderStageText, ShaderModelText);

        // Use the asset-folder as base for the shader-files
        const FStringWide WideFilePath   = CharToWide(FilePath);
        const FStringWide WideEntrypoint = CharToWide(EntryPoint);

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
            L"dummy.hlsl",
        };

        PreProcessorArguments->AddArguments(PreProcessArgs, 2);
    }

    // Preprocess shader source
    DxcBuffer SourceBuffer;
    SourceBuffer.Ptr      = ShaderSource.Data();
    SourceBuffer.Size     = ShaderSource.Size();
    SourceBuffer.Encoding = DXC_CP_ACP;

    TComPtr<IDxcResult> PreprocessResult;
    hr = Compiler->Compile(&SourceBuffer, PreProcessorArguments->GetArguments(), static_cast<uint32>(PreProcessorArguments->GetCount()), IncludeHandler.Get(), IID_PPV_ARGS(&PreprocessResult));
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
    FString Source(reinterpret_cast<const char*>(PreprocessedBlob->GetBufferPointer()), static_cast<int32>(PreprocessedBlob->GetBufferSize()));
    if (CompileInfo.OutputLanguage != EShaderOutputLanguage::HLSL)
    {
        CompileArgs.Emplace(L"-spirv");
        CompileArgs.Emplace(L"-fspv-target-env=vulkan1.2");
        CompileArgs.Emplace(L"-fspv-reduce-load-size");

        // Set must match VULKAN_BINDLESS_HEAP_MARKER_SET in VulkanConstants.h.
        CompileArgs.Emplace(L"-fvk-bind-resource-heap");
        CompileArgs.Emplace(L"0");
        CompileArgs.Emplace(L"31");

        CompileArgs.Emplace(L"-fvk-bind-sampler-heap");
        CompileArgs.Emplace(L"1");
        CompileArgs.Emplace(L"31");
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
    hr = Compiler->Compile(&SourceBuffer, CompileArguments->GetArguments(), CompileArguments->GetCount(), IncludeHandler.Get(), IID_PPV_ARGS(&Result));
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
            const FString Output(reinterpret_cast<LPCSTR>(PrintBlob->GetBufferPointer()), uint32(PrintBlob->GetBufferSize()));
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

bool FShaderCompiler::ConvertSpirvToMetalShader(const FString& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
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

    const CHAR* MSLSource = nullptr;
    Result = spvc_compiler_compile(CompilerMSL, &MSLSource);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to create MSL");
        DEBUG_BREAK();
        return false;
    }

    // Create a new array
    const uint32 SourceLength = FCString::Strlen(MSLSource);
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

bool FShaderCompiler::DumpContentToFile(const TArray<uint8>& ByteCode, const FString& Filename)
{
    TFileRef<IPlatformFile> Output = FPlatformFile::OpenForWrite(Filename);
    if (!Output)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to open file '%s'", *Filename);
        return false;
    }

    Output->Write(ByteCode.Data(), ByteCode.Size());
    return true;
}

FString FShaderCompiler::CreateArgString(const TArrayView<LPCWSTR> Args)
{
    FStringWide NewString;
    for (LPCWSTR CurrentArg : Args)
    {
        NewString += CurrentArg;
        NewString += L' ';
    }

    return WideToChar(NewString);
}
