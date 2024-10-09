#include "RHI.h"
#include "ShaderCompiler.h"
#include "Core/Containers/ComPtr.h"
#include "Core/Platform/PlatformInterlocked.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Memory/Malloc.h"
#include <glslang/Public/resource_limits_c.h> // Required for use of glslang_default_resource
#include <spirv_cross_c.h>

static TAutoConsoleVariable<bool> CVarShaderDebug(
    "RHI.ShaderCompiler.Debug",
    "Enable debug information in the Shaders",
    true);

static TAutoConsoleVariable<bool> CVarVerboseShaderCompiler(
    "RHI.ShaderCompiler.Verbose",
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
        // Compute
        case EShaderStage::Compute:         return L"cs";

        // Graphics
        case EShaderStage::Vertex:          return L"vs";
        case EShaderStage::Hull:            return L"hs";
        case EShaderStage::Domain:          return L"ds";
        case EShaderStage::Geometry:        return L"gs";
        case EShaderStage::Pixel:           return L"ps";

        // New Graphics Pipeline
        case EShaderStage::Mesh:            return L"ms";
        case EShaderStage::Amplification:   return L"as";

        // Ray tracing
        case EShaderStage::RayGen:
        case EShaderStage::RayAnyHit:
        case EShaderStage::RayClosestHit:
        case EShaderStage::RayIntersection:
        case EShaderStage::RayCallable:
        case EShaderStage::RayMiss:         return L"lib";
            
        default:                            return L"xxx";
    }
}

static LPCWSTR GetShaderModelString(EShaderModel Model)
{
    switch (Model)
    {
        case EShaderModel::SM_6_0: return L"6_0";
        case EShaderModel::SM_6_1: return L"6_1";
        case EShaderModel::SM_6_2: return L"6_2";
        case EShaderModel::SM_6_3: return L"6_3";
        case EShaderModel::SM_6_4: return L"6_4";
        case EShaderModel::SM_6_5: return L"6_5";
        case EShaderModel::SM_6_6: return L"6_6";
        case EShaderModel::SM_6_7: return L"6_7";
        default:                   return L"0_0";
    }
}

static glslang_stage_t GetGlslangStage(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        // Graphics
        case EShaderStage::Vertex:   return GLSLANG_STAGE_VERTEX;
        case EShaderStage::Hull:     return GLSLANG_STAGE_TESSCONTROL;
        case EShaderStage::Domain:   return GLSLANG_STAGE_TESSEVALUATION;
        case EShaderStage::Geometry: return GLSLANG_STAGE_GEOMETRY;
        case EShaderStage::Pixel:    return GLSLANG_STAGE_FRAGMENT;
        // Compute
        case EShaderStage::Compute:  return GLSLANG_STAGE_COMPUTE;
        // Other
        default: return glslang_stage_t(-1);
    }
}


class FShaderBlob final : public IDxcBlob
{
public:
    FShaderBlob(LPCVOID InData, SIZE_T InSize)
        : Data(nullptr)
        , Size(InSize)
        , References(1)
    {
        Data = FMemory::Malloc(Size);
        FMemory::Memcpy(Data, InData, Size);
    }

    ~FShaderBlob()
    {
        FMemory::Free(Data);
    }

    virtual LPVOID GetBufferPointer() override final
    {
        return Data;
    }

    virtual SIZE_T GetBufferSize() override final
    {
        return Size;
    }

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

    virtual ULONG AddRef() override final
    {
        FPlatformInterlocked::InterlockedIncrement(&References);
        return static_cast<ULONG>(References);
    }

    virtual ULONG Release() override final
    {
        ULONG NumRefs = static_cast<ULONG>(FPlatformInterlocked::InterlockedDecrement(&References));
        if (NumRefs == 0)
        {
            delete this;
        }

        return NumRefs;
    }

private:
    LPVOID Data;
    SIZE_T Size;
    int32  References;
};


FShaderCompiler* FShaderCompiler::GInstance = nullptr;

FShaderCompiler::FShaderCompiler(FStringView InAssetPath)
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
    
    // Destroy GLslang
    glslang_finalize_process();
}

bool FShaderCompiler::Create(FStringView InAssetFolderPath)
{
    CHECK(GInstance == nullptr);

    GInstance = new FShaderCompiler(InAssetFolderPath);
    if (!GInstance->Initialize())
    {
        delete GInstance;
        GInstance = nullptr;
        return false;
    }

    return true;
}

void FShaderCompiler::Destroy()
{
    if (GInstance)
    {
        delete GInstance;
        GInstance = nullptr;
    }
}

EShaderOutputLanguage FShaderCompiler::GetOutputLanguageBasedOnRHI()
{
    if (FRHI* CurrentRHI = GetRHI())
    {
        const ERHIType RHIType = CurrentRHI->GetType();
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

    // Init GLslang
    glslang_initialize_process();
    return true;
}

FShaderCompiler& FShaderCompiler::Get()
{
    CHECK(GInstance != nullptr);
    return *GInstance;
}

bool FShaderCompiler::CompileFromFile(const FString& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    // Add asset-path to the filename
    const FString FilePath = AssetPath + '/' + Filename;
    
    // Store the ShaderFile in this array
    TArray<CHAR> Text;

    {
        // Open the file
        FFileHandleRef File = FPlatformFile::OpenForRead(FilePath);
        if (!File)
        {
            LOG_ERROR("[FShaderCompiler]: Failed to open file '%s'", Filename.GetCString());
            return false;
        }

        // Read the full file as a text-file
        if (!FFileHelpers::ReadTextFile(File.Get(), Text))
        {
            LOG_ERROR("[FShaderCompiler]: Failed to read file '%s'", Filename.GetCString());
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
    OutByteCode.Clear();

    TComPtr<IDxcCompiler> Compiler;
    HRESULT hResult = DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
    if (FAILED(hResult))
    {
        LOG_ERROR("[FShaderCompiler]: FAILED to create Compiler");
        DEBUG_BREAK();
        return false;
    }

    TComPtr<IDxcLibrary> Library;
    hResult = DxcCreateInstanceFunc(CLSID_DxcLibrary, IID_PPV_ARGS(&Library));
    if (FAILED(hResult))
    {
        LOG_ERROR("[FShaderCompiler]: FAILED to create Library");
        DEBUG_BREAK();
        return false;
    }

    TComPtr<IDxcIncludeHandler> IncludeHandler;
    hResult = Library->CreateIncludeHandler(&IncludeHandler);
    if (FAILED(hResult))
    {
        LOG_ERROR("[FShaderCompiler]: FAILED to create IncludeHandler");
        DEBUG_BREAK();
        return false;
    }

    // Add compile arguments
    TArray<LPCWSTR> CompileArgs =
    {
        L"-HV 2021" // Use HLSL 2021
    };

    CompileArgs.Emplace(L"-Gfa"); // Avoid flow-control
    CompileArgs.Emplace(L"-WX");  // Warnings as errors

    // Optimization level 3
    if (CompileInfo.bOptimize)
    {
        CompileArgs.Emplace(L"-O3"); // Highest optimization level
        CompileArgs.Emplace(L"-all-resources-bound");
    }

    // NOTE: Entrypoint needs to always be main when compiling SPIRV, this is due to the fact that the GLSL code cannot handle any
    // other name at the moment (GLSL being used when we change bindings for Vulkan).
    const FString EntryPoint = CompileInfo.OutputLanguage == EShaderOutputLanguage::SPIRV ? "main" : CompileInfo.EntryPoint;
    
    // Handle language selection
    FString Source(ShaderSource);
    if (CompileInfo.OutputLanguage != EShaderOutputLanguage::HLSL)
    {
        // When not using HLSL, we want to emit SPIR-V
        CompileArgs.Emplace(L"-spirv");
        CompileArgs.Emplace(L"-fspv-target-env=vulkan1.2");

        // NOTE: Change the entrypoint to be 'main', since this is always the entrypoint when we need to compile the
        // SPIRV into GLSL, and back to SPIRV. This happens when we change any bindings for resources.
        if (CompileInfo.OutputLanguage == EShaderOutputLanguage::SPIRV)
        {
            if (!PatchHLSLForSpirv(CompileInfo.EntryPoint, Source))
            {
                LOG_ERROR("[FShaderCompiler]: Failed to patch HLSL for the SPIR-V backend");
                DEBUG_BREAK();
                return false;
            }
        }
    }
    
    // NOTE: We are forced to embed debug information in order to get all the information we need for Vulkan
    CompileArgs.Emplace(L"-Qembed_debug");
    
    if (CVarShaderDebug.GetValue())
    {
        CompileArgs.Emplace(L"-Zi");
    }

    // Create a single string for printing all the shader arguments
    const FString ArgumentsString = CreateArgString(MakeArrayView(CompileArgs));

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
            DxcDefines.Add({ WideDefine.GetCString(), WideValue.GetCString() });
        }
    }
 
    const bool bVerboseLogging = CVarVerboseShaderCompiler.GetValue();
    if (bVerboseLogging)
    {
        if (!FilePath.IsEmpty())
        {
            LOG_INFO("[FShaderCompiler]: Compiling shader '%s', using the following defines:", FilePath.GetCString());
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
    
    // Retrieve the shader target
    const LPCWSTR ShaderStageText = GetShaderStageString(CompileInfo.ShaderStage);
    const LPCWSTR ShaderModelText = GetShaderModelString(CompileInfo.ShaderModel);

    constexpr uint32 BufferLength = sizeof("xxx_x_x");
    WCHAR TargetProfile[BufferLength];
    FCStringWide::Snprintf(TargetProfile, BufferLength, L"%ls_%ls", ShaderStageText, ShaderModelText);
    
    // Use the asset-folder as base for the shader-files
    const FStringWide WideFilePath   = CharToWide(FilePath);
    const FStringWide WideEntrypoint = CharToWide(EntryPoint);

    // Convert the source to a ShaderBlob
    TComPtr<IDxcBlob> SourceBlob = new FShaderBlob(Source.Data(), Source.SizeInBytes());
    
    // Actually compile the shader
    TComPtr<IDxcOperationResult> Result;
    hResult = Compiler->Compile(
        SourceBlob.Get(),
        WideFilePath.GetCString(),
        WideEntrypoint.GetCString(),
        TargetProfile,
        CompileArgs.Data(),
        CompileArgs.Size(),
        DxcDefines.Data(),
        DxcDefines.Size(),
        IncludeHandler.Get(),
        &Result);

    if (FAILED(hResult))
    {
        LOG_ERROR("[FShaderCompiler]: FAILED to Compile");
        DEBUG_BREAK();
        return false;
    }

    if (FAILED(Result->GetStatus(&hResult)))
    {
        LOG_ERROR("[FShaderCompiler]: FAILED to Retrieve result. Unknown Error.");
        DEBUG_BREAK();
        return false;
    }

    TComPtr<IDxcBlobEncoding> PrintBlob;
    TComPtr<IDxcBlobEncoding> PrintBlob8;
    if (SUCCEEDED(Result->GetErrorBuffer(&PrintBlob)))
    {
        Library->GetBlobAsUtf8(PrintBlob.Get(), &PrintBlob8);
    }

    if (FAILED(hResult))
    {
        if (PrintBlob8 && PrintBlob8->GetBufferSize() > 0)
        {
            LOG_ERROR("[FShaderCompiler]: FAILED to compile with error: %s", reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
        }
        else
        {
            LOG_ERROR("[FShaderCompiler]: FAILED to compile with. Unknown ERROR.");
        }

        DEBUG_BREAK();
        return false;
    }

    if (bVerboseLogging)
    {
        if (PrintBlob8 && PrintBlob8->GetBufferSize() > 0)
        {
            const FString Output(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()), uint32(PrintBlob8->GetBufferSize()));
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader with arguments '%s' and with the following output: %s", ArgumentsString.GetCString(), Output.GetCString());
        }
        else
        {
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader with arguments '%s'.", ArgumentsString.GetCString());
        }
    }

    TComPtr<IDxcBlob> CompiledBlob;
    if (FAILED(Result->GetResult(&CompiledBlob)))
    {
        LOG_ERROR("[FShaderCompiler]: FAILED to retrieve result");
        DEBUG_BREAK();
        return false;
    }

    const uint32 BlobSize = uint32(CompiledBlob->GetBufferSize());
    OutByteCode.Resize(BlobSize);
    FMemory::Memcpy(OutByteCode.Data(), CompiledBlob->GetBufferPointer(), BlobSize);

    if (CompileInfo.OutputLanguage != EShaderOutputLanguage::HLSL)
    {
        if (bVerboseLogging)
        {
            LOG_INFO("[FShaderCompiler]: Compiled Size (Before any transformations): %u Bytes", BlobSize);
        }

        // TODO: Investigate why we need to recompile our SPIR-V when compiled from HLSL directly
        if (CompileInfo.OutputLanguage == EShaderOutputLanguage::SPIRV)
        {
            if (!RecompileSpirv(FilePath, CompileInfo, OutByteCode))
            {
                DEBUG_BREAK();
                return false;
            }
        }
        else if (CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL)
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
        DEBUG_BREAK();
    }

    // If verbose logging is turned off, atleast log that we successfully compiled the shader
    if (!bVerboseLogging)
    {
        if (!FilePath.IsEmpty())
        {
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader '%s'", FilePath.GetCString());
        }
        else
        {
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader");
        }
    }
    
    return true;
}

bool FShaderCompiler::PatchHLSLForSpirv(const FString& Entrypoint, FString& OutSource)
{
    // Find the actual entrypoint, and change it to a specific spirv one. This is done since
    // the current version of DXC available on macOS does not support the compiler argument
    // that does this for us, therefor we now replace the entrypoint ourselves.
    int32 Position = FString::INVALID_INDEX;
    while (true)
    {
        Position = OutSource.Find(Entrypoint, Position);
        if (Position == FString::INVALID_INDEX)
        {
            return false;
        }
        
        const int32 BracketPosition = OutSource.FindChar('(', Position);
        if (BracketPosition == FString::INVALID_INDEX)
        {
            return false;
        }
        
        // Create a view of the entrypoint name to ensure that we found the whole thing
        // this is done in order to support entry-points with spaces etc. between bracket
        // and actual entrypoint name.
        const int32 EntrypointLength = BracketPosition - Position;
        FStringView CurrentEntrypoint(OutSource.Data(), EntrypointLength, Position);
        CurrentEntrypoint.TrimInline();
        
        // If we actually found the entrypoint, we can exit the loop and replace the entrypoint
        if (Entrypoint.Equals(CurrentEntrypoint))
        {
            OutSource.Remove(Position, Entrypoint.Size());
            OutSource.Insert("main", Position);
            break;
        }
        
        // Search for the next name
        Position++;
    }
    
    return true;
}

bool FShaderCompiler::RecompileSpirv(const FString& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    if (OutByteCode.IsEmpty())
    {
        LOG_ERROR("[FShaderCompiler]: No SPIR-V code supplied");
        return false;
    }
    
    if (OutByteCode.Size() % sizeof(uint32) != 0)
    {
        LOG_ERROR("[FShaderCompiler]: SPIR-V code needs to be aligned to 4 bytes, ensure that valid SPIR-V code is supplied");
        return false;
    }
    
    spvc_context Context = nullptr;
    spvc_result Result = spvc_context_create(&Context);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to create SpvcContext");
        return false;
    }

    spvc_context_set_error_callback(Context, [](void*, const CHAR* Error)
    {
        LOG_ERROR("[SPIRV-Cross Error] %s", Error);
    }, nullptr);

    // The code size needs to be aligned to the element-size
    spvc_parsed_ir ParsedCode = nullptr;
    const int32 SpirvCodeSize = OutByteCode.Size() / sizeof(uint32);
    Result = spvc_context_parse_spirv(Context, reinterpret_cast<const SpvId*>(OutByteCode.Data()), SpirvCodeSize, &ParsedCode);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to parse Spirv");
        return false;
    }

    spvc_compiler Compiler = nullptr;
    Result = spvc_context_create_compiler(Context, SPVC_BACKEND_GLSL, ParsedCode, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &Compiler);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to create SPIR-V compiler");
        return false;
    }

    // Compile the code to GLSL -> SPIR-V if we had to make any modifications
    // Modify options.
    spvc_compiler_options Options = nullptr;
    spvc_compiler_create_compiler_options(Compiler, &Options);
    spvc_compiler_options_set_uint(Options, SPVC_COMPILER_OPTION_GLSL_VERSION, 460);
    spvc_compiler_options_set_bool(Options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_FALSE);
    spvc_compiler_options_set_bool(Options, SPVC_COMPILER_OPTION_FORCE_TEMPORARY, SPVC_TRUE);
    spvc_compiler_options_set_bool(Options, SPVC_COMPILER_OPTION_GLSL_VULKAN_SEMANTICS, SPVC_TRUE);
    spvc_compiler_install_compiler_options(Compiler, Options);

    // Compile the GLSL code
    const CHAR* NewSource = nullptr;
    Result = spvc_compiler_compile(Compiler, &NewSource);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to compile changes into GLSL");
        return false;
    }

    // Create a new array (1 extra byte for a null-terminator)
    const uint32 SourceLength = FCString::Strlen(NewSource);
    TArray<uint8> NewShader(reinterpret_cast<const uint8*>(NewSource), (SourceLength + 1) * sizeof(uint8));
    NewShader[SourceLength] = 0;

    // Dump the metal file to disk
    if (!FilePath.IsEmpty())
    {
        if (!DumpContentToFile(NewShader, FilePath + "_" + ToString(CompileInfo.ShaderStage) + ".glsl"))
        {
            DEBUG_BREAK();
            return false;
        }
    }
    
    // Now we can destroy the context
    spvc_context_destroy(Context);

    // Convert the shader stage to glslang-enum
    const glslang_stage_t GlslangStage = GetGlslangStage(CompileInfo.ShaderStage);

    // Compile the GLSL to SPIRV
    glslang_input_t Input;
    Input.language                          = GLSLANG_SOURCE_GLSL;
    Input.stage                             = GlslangStage;
    Input.client                            = GLSLANG_CLIENT_VULKAN;
    Input.client_version                    = GLSLANG_TARGET_VULKAN_1_2;
    Input.target_language                   = GLSLANG_TARGET_SPV;
    Input.target_language_version           = GLSLANG_TARGET_SPV_1_5;
    Input.code                              = reinterpret_cast<CHAR*>(NewShader.Data());
    Input.default_version                   = 110;
    Input.default_profile                   = GLSLANG_NO_PROFILE;
    Input.force_default_version_and_profile = false;
    Input.forward_compatible                = false;
    Input.messages                          = GLSLANG_MSG_DEFAULT_BIT;
    Input.resource                          = glslang_default_resource();

    glslang_shader_t* Shader = glslang_shader_create(&Input);
    if (!glslang_shader_preprocess(Shader, &Input))
    {
        const CHAR* InfoLog      = glslang_shader_get_info_log(Shader);
        const CHAR* InfoDebugLog = glslang_shader_get_info_debug_log(Shader);
        
        LOG_ERROR("GLSL preprocessing failed");
        LOG_ERROR("    %s", InfoLog);
        LOG_ERROR("    %s", InfoDebugLog);
        
        glslang_shader_delete(Shader);
        
        DEBUG_BREAK();
        return false;
    }

    if (!glslang_shader_parse(Shader, &Input))
    {
        const CHAR* InfoLog      = glslang_shader_get_info_log(Shader);
        const CHAR* InfoDebugLog = glslang_shader_get_info_debug_log(Shader);
        
        LOG_ERROR("GLSL parsing failed");
        LOG_ERROR("    %s", InfoLog);
        LOG_ERROR("    %s", InfoDebugLog);
        LOG_ERROR("    %s", glslang_shader_get_preprocessed_code(Shader));

        glslang_shader_delete(Shader);
        
        DEBUG_BREAK();
        return false;
    }

    glslang_program_t* Program = glslang_program_create();
    glslang_program_add_shader(Program, Shader);

    if (!glslang_program_link(Program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
    {
        const CHAR* InfoLog      = glslang_program_get_info_log(Program);
        const CHAR* InfoDebugLog = glslang_program_get_info_debug_log(Program);
        
        LOG_ERROR("GLSL linking failed");
        LOG_ERROR("    %s", InfoLog);
        LOG_ERROR("    %s", InfoDebugLog);
        
        glslang_program_delete(Program);
        glslang_shader_delete(Shader);
        
        DEBUG_BREAK();
        return false;
    }

    // Retrieve the SPIRV shader code
    glslang_spv_options_t SpvOptions;
    FMemory::Memzero(&SpvOptions);

    SpvOptions.validate            = true;
    SpvOptions.generate_debug_info = true;

    if (CompileInfo.bOptimize)
    {
        SpvOptions.optimize_size = true;
    }
    else
    {
        SpvOptions.disable_optimizer = true;
    }
    
    glslang_program_SPIRV_generate_with_options(Program, GlslangStage, &SpvOptions);

    // Get the size and allocate enough room in the vector
    const uint64 ProgramSize = glslang_program_SPIRV_get_size(Program);
    
    // Transfer the SPIRV code into our format
    TArray<uint8> NewCode;
    NewCode.Resize(static_cast<int32>(ProgramSize) * sizeof(uint32));

    glslang_program_SPIRV_get(Program, reinterpret_cast<uint32*>(NewCode.Data()));
    OutByteCode = Move(NewCode);
    
    // Print any messages from linking
    if (const CHAR* SpirvMessages = glslang_program_SPIRV_get_messages(Program))
    {
        LOG_INFO("%s", SpirvMessages);
    }

    // Cleanup
    glslang_program_delete(Program);
    glslang_shader_delete(Shader);
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
    FFileHandleRef Output = FPlatformFile::OpenForWrite(Filename);
    if (!Output)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to open file '%s'", Filename.GetCString());
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
