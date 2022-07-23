#include "RHIShaderCompiler.h"

#include "Core/Containers/ComPtr.h"
#include "Core/Threading/AsyncTaskManager.h"
#include "Core/Threading/Platform/PlatformInterlocked.h"
#include "Core/Modules/Platform/PlatformLibrary.h"

#include <spirv_cross_c.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EDXCPart

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Convert the Shader-enums

static LPCWSTR GetShaderStageString(EShaderStage Stage)
{
    switch (Stage)
    {
        // Compute
        case EShaderStage::Compute:       return L"cs";

        // Graphics
        case EShaderStage::Vertex:        return L"vs";
        case EShaderStage::Hull:          return L"hs";
        case EShaderStage::Domain:        return L"ds";
        case EShaderStage::Geometry:      return L"gs";
        case EShaderStage::Pixel:         return L"ps";

        // New Graphics Pipeline
        case EShaderStage::Mesh:          return L"ms";
        case EShaderStage::Amplification: return L"as";

        // Ray tracing
        case EShaderStage::RayGen:
        case EShaderStage::RayAnyHit:
        case EShaderStage::RayClosestHit:
        case EShaderStage::RayMiss:       return L"lib";
    }

    return L"xxx";
}

static LPCWSTR GetShaderModelString(EShaderModel Model)
{
    switch (Model)
    {
        case EShaderModel::SM_5_0: return L"5_0";
        case EShaderModel::SM_5_1: return L"5_1";
        case EShaderModel::SM_6_0: return L"6_0";
        case EShaderModel::SM_6_1: return L"6_1";
        case EShaderModel::SM_6_2: return L"6_2";
        case EShaderModel::SM_6_3: return L"6_3";
        case EShaderModel::SM_6_4: return L"6_4";
        case EShaderModel::SM_6_5: return L"6_5";
        case EShaderModel::SM_6_6: return L"6_6";
    }

    return L"0_0";
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShaderBlob

class CShaderBlob : public IDxcBlob
{
public:

    CShaderBlob(LPCVOID InData, SIZE_T InSize)
        : Data(nullptr)
        , Size(InSize)
        , References(1)
    {
        Data = FMemory::Malloc(Size);
        FMemory::Memcpy(Data, InData, Size);
    }

    ~CShaderBlob()
    {
        FMemory::Free(Data);
    }

    virtual LPVOID GetBufferPointer() override
    {
        return Data;
    }

    virtual SIZE_T GetBufferSize() override
    {
        return Size;
    }

    virtual HRESULT QueryInterface(REFIID Riid, LPVOID* ppvObject)
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

    virtual ULONG AddRef()
    {
        FPlatformInterlocked::InterlockedIncrement(&References);
        return static_cast<ULONG>(References);
    }

    virtual ULONG Release()
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIShaderCompiler

TOptional<FRHIShaderCompiler> FRHIShaderCompiler::Instance;

FRHIShaderCompiler::FRHIShaderCompiler(const char* InAssetPath)
    : DXCLib(nullptr)
    , DxcCreateInstanceFunc(nullptr)
    , AssetPath(InAssetPath)
{ 
    DXCLib = FPlatformLibrary::LoadDynamicLib("dxcompiler");
    if (!DXCLib)
    {
        LOG_ERROR("Failed to load 'dxcompiler'");
        return;
    }

    DxcCreateInstanceFunc = FPlatformLibrary::LoadSymbolAddress<DxcCreateInstanceProc>("DxcCreateInstance", DXCLib);
    if (!DxcCreateInstanceFunc)
    {
        LOG_ERROR("Failed to load 'DxcCreateInstance'");
        return;
    }
}

FRHIShaderCompiler::~FRHIShaderCompiler()
{
    if (DXCLib)
    {
        FPlatformLibrary::FreeDynamicLib(DXCLib);
        DXCLib = nullptr;
    }

    DxcCreateInstanceFunc = nullptr;
}

bool FRHIShaderCompiler::Initialize(const char* InAssetFolderPath)
{
    Instance.Emplace(InAssetFolderPath);
    return (Instance->DXCLib != nullptr) && (Instance->DxcCreateInstanceFunc != nullptr);
}

void FRHIShaderCompiler::Release()
{
    Instance.Reset();
}

FRHIShaderCompiler& FRHIShaderCompiler::Get()
{
    Check(Instance.HasValue());
    return Instance.GetValue();
}

bool FRHIShaderCompiler::CompileFromFile(const FString& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    OutByteCode.Clear();

    // Use the asset-folder as base for the shader-files
    FWString WideFilePath   = CharToWide(AssetPath + '/' + Filename);

    TComPtr<IDxcCompiler> Compiler;
    HRESULT hResult = DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
    if (FAILED(hResult))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to create Compiler");
        return false;
    }

    TComPtr<IDxcLibrary> Library;
    hResult = DxcCreateInstanceFunc(CLSID_DxcLibrary, IID_PPV_ARGS(&Library));
    if (FAILED(hResult))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to create Library");
        return false;
    }

    TComPtr<IDxcIncludeHandler> IncludeHandler;
    hResult = Library->CreateIncludeHandler(&IncludeHandler);
    if (FAILED(hResult))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to create IncludeHandler");
        return false;
    }
    
    TComPtr<IDxcBlobEncoding> SourceBlob;
    hResult = Library->CreateBlobFromFile(WideFilePath.CStr(), nullptr, &SourceBlob);
    if (FAILED(hResult))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to create Source Data");
        DEBUG_BREAK();
        return false;
    }
    
    // Add compile arguments
    TArray<LPCWSTR> CompileArgs =
    {
        L"-HV 2021" // Use HLSL 2021
    };

    // Optimization level 3
    if (CompileInfo.bOptimize)
    {
        CompileArgs.Emplace(L"-O3");
    }
    
    if (CompileInfo.OutputLanguage != EShaderOutputLanguage::HLSL)
    {
        CompileArgs.Emplace(L"-spirv");
    }

    // Create a single string for printing all the shader arguments
    const FString ArgumentsString = CreateArgString(CompileArgs.CreateView());
    
    // Convert defines
    TArray<FWString>   StrBuff;
    TArray<DxcDefine> DxcDefines;
    
    TArrayView<FShaderDefine> Defines = CompileInfo.Defines;
    if (!Defines.IsEmpty())
    {
        StrBuff.Reserve(Defines.Size() * 2);
        DxcDefines.Reserve(Defines.Size());

        for (const FShaderDefine& Define : Defines)
        {
            const FWString& WideDefine = StrBuff.Emplace(CharToWide(Define.Define));
            const FWString& WideValue  = StrBuff.Emplace(CharToWide(Define.Value));
            DxcDefines.Push({ WideDefine.CStr(), WideValue.CStr() });
        }
    }
    
    // Retrieve the shader target
    LPCWSTR ShaderStageText = GetShaderStageString(CompileInfo.ShaderStage);
    LPCWSTR ShaderModelText = GetShaderModelString(CompileInfo.ShaderModel);

    constexpr uint32 BufferLength = sizeof("xxx_x_x");
    
    WCHAR TargetProfile[BufferLength];
    FWCString::FormatBuffer(TargetProfile, BufferLength, L"%ls_%ls", ShaderStageText, ShaderModelText);
    
    const FWString WideEntrypoint = CharToWide(CompileInfo.EntryPoint);

    TComPtr<IDxcOperationResult> Result;
    hResult = Compiler->Compile( SourceBlob.Get()
                               , WideFilePath.CStr()
                               , WideEntrypoint.CStr()
                               , TargetProfile
                               , CompileArgs.Data()
                               , CompileArgs.Size()
                               , DxcDefines.Data()
                               , DxcDefines.Size()
                               , IncludeHandler.Get()
                               , &Result);
    if (FAILED(hResult))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to Compile");
        DEBUG_BREAK();
        return false;
    }

    if (FAILED(Result->GetStatus(&hResult)))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to Retrieve result. Unknown Error.");
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
        if (PrintBlob8 && (PrintBlob8->GetBufferSize() > 0))
        {
            LOG_ERROR("[FRHIShaderCompiler]: FAILED to compile with error: %s", reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
        }
        else
        {
            LOG_ERROR("[FRHIShaderCompiler]: FAILED to compile with. Unknown ERROR.");
        }

        return false;
    }
    
    if (PrintBlob8 && (PrintBlob8->GetBufferSize() > 0))
    {
        const FString Output(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()), uint32(PrintBlob8->GetBufferSize()));
        LOG_INFO("[FRHIShaderCompiler]: Successfully compiled shader '%s', with arguments '%s' and with the following output: %s", Filename.CStr(), ArgumentsString.CStr(), Output.CStr());
    }
    else
    {
        LOG_INFO("[FRHIShaderCompiler]: Successfully compiled shader '%s', with arguments '%s'.", Filename.CStr(), ArgumentsString.CStr());
    }

    TComPtr<IDxcBlob> CompiledBlob;
    if (FAILED(Result->GetResult(&CompiledBlob)))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to retrieve result");
        return false;
    }

    const uint32 BlobSize = uint32(CompiledBlob->GetBufferSize());
    OutByteCode.Resize(BlobSize);

    LOG_INFO("[FRHIShaderCompiler]: Compiled Size: %u Bytes", BlobSize);

    FMemory::Memcpy(OutByteCode.Data(), CompiledBlob->GetBufferPointer(), BlobSize);
    
    // Convert SPIRV into MSL
    if (CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL)
    {
        if (!ConvertSpirvToMetalShader(CompileInfo.EntryPoint, OutByteCode))
        {
            return false;
        }

        return DumpContentToFile(OutByteCode, AssetPath + '/' + Filename + "_" + ToString(CompileInfo.ShaderStage) + ".metal");
    }

    return true;
}

bool FRHIShaderCompiler::CompileFromSource(const FString& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    OutByteCode.Clear();

    TComPtr<IDxcCompiler> Compiler;
    HRESULT hResult = DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
    if (FAILED(hResult))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to create Compiler");
        return false;
    }

    TComPtr<IDxcLibrary> Library;
    hResult = DxcCreateInstanceFunc(CLSID_DxcLibrary, IID_PPV_ARGS(&Library));
    if (FAILED(hResult))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to create Library");
        return false;
    }

    TComPtr<IDxcIncludeHandler> IncludeHandler;
    hResult = Library->CreateIncludeHandler(&IncludeHandler);
    if (FAILED(hResult))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to create IncludeHandler");
        return false;
    }

    // Add compile arguments
    TArray<LPCWSTR> CompileArgs =
    {
        L"-HV 2021" // Use HLSL 2021
    };

    // Optimization level 3
    if (CompileInfo.bOptimize)
    {
        CompileArgs.Emplace(L"-O3");
    }

    if (CompileInfo.OutputLanguage != EShaderOutputLanguage::HLSL)
    {
        CompileArgs.Emplace(L"-spirv");
    }

    // Create a single string for printing all the shader arguments
    const FString ArgumentsString = CreateArgString(CompileArgs.CreateView());

    // Convert defines
    TArray<FWString>   StrBuff;
    TArray<DxcDefine> DxcDefines;

    TArrayView<FShaderDefine> Defines = CompileInfo.Defines;
    if (!Defines.IsEmpty())
    {
        StrBuff.Reserve(Defines.Size() * 2);
        DxcDefines.Reserve(Defines.Size());

        for (const FShaderDefine& Define : Defines)
        {
            const FWString& WideDefine = StrBuff.Emplace(CharToWide(Define.Define));
            const FWString& WideValue  = StrBuff.Emplace(CharToWide(Define.Value));
            DxcDefines.Push({ WideDefine.CStr(), WideValue.CStr() });
        }
    }

    // Retrieve the shader target
    const LPCWSTR ShaderStageText = GetShaderStageString(CompileInfo.ShaderStage);
    const LPCWSTR ShaderModelText = GetShaderModelString(CompileInfo.ShaderModel);

    constexpr uint32 BufferLength = sizeof("xxx_x_x");

    WCHAR TargetProfile[BufferLength];
    FWCString::FormatBuffer(TargetProfile, BufferLength, L"%ls_%ls", ShaderStageText, ShaderModelText);
    
    // Use the asset-folder as base for the shader-files
    const FWString WideEntrypoint = CharToWide(CompileInfo.EntryPoint);

    TComPtr<IDxcBlob>            SourceBlob = dbg_new CShaderBlob(ShaderSource.Data(), ShaderSource.SizeInBytes());
    TComPtr<IDxcOperationResult> Result;
    hResult = Compiler->Compile( SourceBlob.Get()
                               , nullptr
                               , WideEntrypoint.CStr()
                               , TargetProfile
                               , CompileArgs.Data()
                               , CompileArgs.Size()
                               , DxcDefines.Data()
                               , DxcDefines.Size()
                               , IncludeHandler.Get()
                               , &Result);
    if (FAILED(hResult))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to Compile");
        DEBUG_BREAK();
        return false;
    }

    if (FAILED(Result->GetStatus(&hResult)))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to Retrieve result. Unknown Error.");
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
        if (PrintBlob8 && (PrintBlob8->GetBufferSize() > 0))
        {
            LOG_ERROR("[FRHIShaderCompiler]: FAILED to compile with error: %s", reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
        }
        else
        {
            LOG_ERROR("[FRHIShaderCompiler]: FAILED to compile with. Unknown ERROR.");
        }

        return false;
    }

    if (PrintBlob8 && (PrintBlob8->GetBufferSize() > 0))
    {
        const FString Output(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()), uint32(PrintBlob8->GetBufferSize()));
        LOG_INFO("[FRHIShaderCompiler]: Successfully compiled shader from source, with arguments '%s' and with the following output: %s", ArgumentsString.CStr(), Output.CStr());
    }
    else
    {
        LOG_INFO("[FRHIShaderCompiler]: Successfully compiled shader from source, with arguments '%s'.", ArgumentsString.CStr());
    }

    TComPtr<IDxcBlob> CompiledBlob;
    if (FAILED(Result->GetResult(&CompiledBlob)))
    {
        LOG_ERROR("[FRHIShaderCompiler]: FAILED to retrieve result");
        return false;
    }

    const uint32 BlobSize = uint32(CompiledBlob->GetBufferSize());
    OutByteCode.Resize(BlobSize);

    LOG_INFO("[FRHIShaderCompiler]: Compiled Size: %u Bytes", BlobSize);

    FMemory::Memcpy(OutByteCode.Data(), CompiledBlob->GetBufferPointer(), BlobSize);

    // Convert SPIRV into MSL
    if (CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL)
    {
        if (!ConvertSpirvToMetalShader(CompileInfo.EntryPoint, OutByteCode))
        {
            return false;
        }
    }

    return true;
}

void FRHIShaderCompiler::ErrorCallback(void* Userdata, const char* Error)
{
    UNREFERENCED_VARIABLE(Userdata);

    LOG_ERROR("[SPIRV-Cross Error] %s", Error);
}

bool FRHIShaderCompiler::ConvertSpirvToMetalShader(const FString& Entrypoint, TArray<uint8>& OutByteCode)
{
    if (OutByteCode.IsEmpty() || Entrypoint.IsEmpty())
    {
        return false;
    }

    spvc_context Context = nullptr;
    spvc_result Result = spvc_context_create(&Context);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("Failed to create SpvcContext");
        return false;
    }

    spvc_context_set_error_callback(Context, FRHIShaderCompiler::ErrorCallback, reinterpret_cast<void*>(this));

    constexpr uint32 ElementSize = sizeof(unsigned int) / sizeof(uint8);

    spvc_compiler CompilerMSL = nullptr;

    {
        spvc_parsed_ir ParsedRepresentation = nullptr;

        const uint32 WordCount = OutByteCode.Size() / ElementSize;
        Result = spvc_context_parse_spirv(Context, reinterpret_cast<const SpvId*>(OutByteCode.Data()), WordCount, &ParsedRepresentation);
        if (Result != SPVC_SUCCESS)
        {
            LOG_ERROR("Failed to parse Spirv");
            return false;
        }

        Result = spvc_context_create_compiler(Context, SPVC_BACKEND_MSL, ParsedRepresentation, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &CompilerMSL);
        if (Result != SPVC_SUCCESS)
        {
            LOG_ERROR("Failed to create MSL compiler");
            return false;
        }
    }

    const char* MSLSource = nullptr;
    Result =  spvc_compiler_compile(CompilerMSL, &MSLSource);
    if (Result != SPVC_SUCCESS)
    {
        LOG_ERROR("Failed to create MSL");
        return false;
    }

    // Start by adding the entrypoint to the shader, which is needed when we create native shader objects
    const FString Comment = "// " + Entrypoint + "\n\n";
    TArray<uint8> NewShader(reinterpret_cast<const uint8*>(Comment.Data()), Comment.Length() * sizeof(const char));

    const uint32 SourceLength = FCString::Length(MSLSource);
    NewShader.Append(reinterpret_cast<const uint8*>(MSLSource), SourceLength * sizeof(const char));

    spvc_context_destroy(Context);

    OutByteCode.Swap(NewShader);
    return true;
}

bool FRHIShaderCompiler::DumpContentToFile(const TArray<uint8>& ByteCode, const FString& Filename)
{
    FILE* Output = fopen(Filename.CStr(), "w");
    if (!Output)
    {
        LOG_ERROR("Failed to open file '%s'", Filename.CStr());
        return false;
    }

    fwrite(ByteCode.Data(), ByteCode.Size(), sizeof(uint8), Output);

    fclose(Output);
    return true;
}

FString FRHIShaderCompiler::CreateArgString(const TArrayView<LPCWSTR> Args)
{
    FWString WArgumentsString;
    for (LPCWSTR Arg : Args)
    {
        WArgumentsString += Arg + ' ';
    }

    return WideToChar(WArgumentsString);
}
