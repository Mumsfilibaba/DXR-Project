#include "RHI.h"
#include "ShaderCompiler.h"
#include "Core/Containers/ComPtr.h"
#include "Core/Platform/PlatformInterlocked.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/ConsoleManager.h"

// Headers for SPIR-V resource remapping and GLSL compilation
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h> // Required for use of glslang_default_resource
#include <spirv_cross_c.h>

static TAutoConsoleVariable<bool> CVarShaderDebug(
    "RHI.ShaderCompiler.Debug",
    "Enable debug information in the Shaders",
    true);

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

static glslang_stage_t GetGlslangStage(EShaderStage Stage)
{
    switch (Stage)
    {
        // Graphics
        case EShaderStage::Vertex:          return GLSLANG_STAGE_VERTEX;
        case EShaderStage::Hull:            return GLSLANG_STAGE_TESSCONTROL;
        case EShaderStage::Domain:          return GLSLANG_STAGE_TESSEVALUATION;
        case EShaderStage::Geometry:        return GLSLANG_STAGE_GEOMETRY;
        case EShaderStage::Pixel:           return GLSLANG_STAGE_FRAGMENT;
        // Mesh Pipeline
        case EShaderStage::Mesh:            return GLSLANG_STAGE_MESH;
        case EShaderStage::Amplification:   return GLSLANG_STAGE_TASK;
        // Compute
        case EShaderStage::Compute:         return GLSLANG_STAGE_COMPUTE;
        // Ray Tracing
        case EShaderStage::RayGen:          return GLSLANG_STAGE_RAYGEN;
        case EShaderStage::RayAnyHit:       return GLSLANG_STAGE_ANYHIT;
        case EShaderStage::RayClosestHit:   return GLSLANG_STAGE_CLOSESTHIT;
        case EShaderStage::RayIntersection: return GLSLANG_STAGE_INTERSECT;
        case EShaderStage::RayCallable:     return GLSLANG_STAGE_CALLABLE;
        case EShaderStage::RayMiss:         return GLSLANG_STAGE_MISS;
        // Other
        default:                            return glslang_stage_t(-1);
    }
}

static uint32 GetShaderStageDescriporSetOffset(EShaderStage Stage)
{
    switch (Stage)
    {
        // Graphics
        case EShaderStage::Vertex:        return 0;
        case EShaderStage::Hull:          return 1;
        case EShaderStage::Domain:        return 2;
        case EShaderStage::Geometry:      return 3;
        case EShaderStage::Pixel:         return 4;
        // Mesh Pipeline
        case EShaderStage::Mesh:          return 0;
        case EShaderStage::Amplification: return 1;
        // Compute
        case EShaderStage::Compute:
        // Ray Tracing
        case EShaderStage::RayGen:
        case EShaderStage::RayAnyHit:
        case EShaderStage::RayClosestHit:
        case EShaderStage::RayIntersection:
        case EShaderStage::RayCallable:
        case EShaderStage::RayMiss:
        // Other
        default:
            return 0;
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
    if (DXCLib)
    {
        FPlatformLibrary::FreeDynamicLib(DXCLib);
        DXCLib = nullptr;
    }

    DxcCreateInstanceFunc = nullptr;
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
    DXCLib = FPlatformLibrary::LoadDynamicLib("dxcompiler");
    if (!DXCLib)
    {
        LOG_ERROR("Failed to load 'dxcompiler'");
        return false;
    }

    DxcCreateInstanceFunc = FPlatformLibrary::LoadSymbol<DxcCreateInstanceProc>("DxcCreateInstance", DXCLib);
    if (!DxcCreateInstanceFunc)
    {
        LOG_ERROR("Failed to load 'DxcCreateInstance'");
        return false;
    }

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
    
    // Open the file
    FFileHandleRef File = FPlatformFile::OpenForRead(FilePath);
    if (!File)
    {
        LOG_ERROR("Failed to open file '%s'", Filename.GetCString());
        return false;
    }

    // Read the full file as a textfile
    TArray<CHAR> Text;
    if (!FFileHelpers::ReadTextFile(File.Get(), Text))
    {
        LOG_ERROR("Failed to read file '%s'", Filename.GetCString());
        return false;
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
        
        // NOTE: Change the entrypoint to be 'main', since this is always the entrypoint when we need to compile the
        // SPIRV into GLSL, and back to SPIRV. This happens when we change any bindings for resources.
        if (CompileInfo.OutputLanguage == EShaderOutputLanguage::SPIRV)
        {
            if (!PatchHLSLForSpirv(CompileInfo.EntryPoint, Source))
            {
                LOG_ERROR("Failed to patch HLSL for the SPIR-V backend");
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
 
    if (!FilePath.IsEmpty())
    {
        LOG_INFO("Compiling shader '%s', using the following defines:", FilePath.GetCString());
    }
    else
    {
        LOG_INFO("Compiling shader, using the following defines:");
    }
    
    for (const DxcDefine& Define : DxcDefines)
    {
        LOG_INFO("    %S = %S", Define.Name, Define.Value);
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

    if (PrintBlob8 && PrintBlob8->GetBufferSize() > 0)
    {
        const FString Output(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()), uint32(PrintBlob8->GetBufferSize()));
        LOG_INFO("[FShaderCompiler]: Successfully compiled shader with arguments '%s' and with the following output: %s", ArgumentsString.GetCString(), Output.GetCString());
    }
    else
    {
        LOG_INFO("[FShaderCompiler]: Successfully compiled shader with arguments '%s'.", ArgumentsString.GetCString());
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
        LOG_INFO("[FShaderCompiler]: Compiled Size (Before any transformations): %u Bytes", BlobSize);
    }
    else
    {
        LOG_INFO("[FShaderCompiler]: Compiled Size: %u Bytes", BlobSize);
    }

    // Convert SPIRV into MSL
    if (CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL)
    {
        if (!ConvertSpirvToMetalShader(FilePath, CompileInfo, OutByteCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }
    
    if (CompileInfo.OutputLanguage != EShaderOutputLanguage::HLSL)
    {
        LOG_INFO("[FShaderCompiler]: Compiled Size (Final size): %u Bytes", OutByteCode.Size());
    }
    
    if (OutByteCode.IsEmpty())
    {
        LOG_WARNING("Resulting bytecode is empty");
        DEBUG_BREAK();
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
        // this is done in order to support entrypoints with spaces etc. between bracket
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
        LOG_ERROR("Failed to create SpvcContext");
        DEBUG_BREAK();
        return false;
    }

    spvc_context_set_error_callback(Context, [](void*, const CHAR* Error)
    {
        LOG_ERROR("[SPIRV-Cross Error] %s", Error);
    }, nullptr);

    // The code size needs to be aligned to the elementsize
    constexpr uint32 ElementSize = sizeof(unsigned int) / sizeof(uint8);
    CHECK(OutByteCode.Size() % ElementSize == 0);
    const uint32 WordCount = OutByteCode.Size() / ElementSize;
    
    spvc_parsed_ir ParsedCode = nullptr;
    Result = spvc_context_parse_spirv(Context, reinterpret_cast<const SpvId*>(OutByteCode.Data()), WordCount, &ParsedCode);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("Failed to parse Spirv");
        DEBUG_BREAK();
        return false;
    }

    spvc_compiler CompilerMSL = nullptr;
    Result = spvc_context_create_compiler(Context, SPVC_BACKEND_MSL, ParsedCode, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &CompilerMSL);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("Failed to create MSL compiler");
        DEBUG_BREAK();
        return false;
    }

    const CHAR* MSLSource = nullptr;
    Result = spvc_compiler_compile(CompilerMSL, &MSLSource);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("Failed to create MSL");
        DEBUG_BREAK();
        return false;
    }

    // Create a new array
    const uint32 SourceLength = FCString::Strlen(MSLSource);
    TArray<uint8> NewShader(reinterpret_cast<const uint8*>(MSLSource), SourceLength * sizeof(const CHAR));
    
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
        LOG_ERROR("Failed to open file '%s'", Filename.GetCString());
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
