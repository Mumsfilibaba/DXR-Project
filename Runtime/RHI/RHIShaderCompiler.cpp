#include "RHIShaderCompiler.h"

#include "Core/Threading/AsyncTaskManager.h"
#include "Core/Modules/Platform/PlatformLibrary.h"
#include "Core/Containers/ComPtr.h"

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
        Data = CMemory::Malloc(Size);
        CMemory::Memcpy(Data, InData, Size);
    }

    ~CShaderBlob()
    {
        CMemory::Free(Data);
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

        *ppvObject = NULL;

        // TODO: Could be ID3DBlob as well possibly, however, should not be needed for now
        if (Riid == __uuidof(IUnknown) || Riid == __uuidof(IDxcBlob))
        {
            *ppvObject = (LPVOID)this;
            AddRef();
            return NOERROR;
        }

        return E_NOINTERFACE;
    }

    virtual ULONG AddRef()
    {
        _InterlockedIncrement(&References);
        return References;
    }

    virtual ULONG Release()
    {
        ULONG NumRefs = _InterlockedDecrement(&References);
        if (NumRefs == 0)
        {
            delete this;
        }

        return NumRefs;
    }

private:
    LPVOID Data;
    SIZE_T Size;

    ULONG  References;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShaderCompiler

TOptional<CShaderCompiler> CShaderCompiler::Instance;

CShaderCompiler::CShaderCompiler(const char* InAssetPath)
    : DXCLibrary(nullptr)
    , DxcCreateInstanceFunc(nullptr)
    , AssetPath(InAssetPath)
{ 
    DXCLibrary = PlatformLibrary::LoadDynamicLib("dxcompiler");
    if (!DXCLibrary)
    {
        LOG_ERROR("Failed to load 'dxcompiler'");
        return;
    }

    DxcCreateInstanceFunc = PlatformLibrary::LoadSymbolAddress<DxcCreateInstanceProc>("DxcCreateInstance", DXCLibrary);
    if (!DxcCreateInstanceFunc)
    {
        LOG_ERROR("Failed to load 'DxcCreateInstance'");
        return;
    }
}

CShaderCompiler::~CShaderCompiler()
{
    if (DXCLibrary)
    {
        PlatformLibrary::FreeDynamicLib(DXCLibrary);
        DXCLibrary = nullptr;
    }

    DxcCreateInstanceFunc = nullptr;
}

bool CShaderCompiler::Initialize(const char* InAssetFolderPath)
{
    Instance.Emplace(InAssetFolderPath);
    return (Instance->DXCLibrary != nullptr) && (Instance->DxcCreateInstanceFunc != nullptr);
}

void CShaderCompiler::Release()
{
    Instance.Reset();
}

CShaderCompiler& CShaderCompiler::Get()
{
    Check(Instance.HasValue());
    return Instance.GetValue();
}

bool CShaderCompiler::CompileFromFile(const String& Filename, const CShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    OutByteCode.Clear();

    // Use the asset-folder as base for the shader-files
    WString WideFilePath   = CharToWide(AssetPath + '/' + Filename);

    TComPtr<IDxcCompiler> Compiler;
    HRESULT hResult = DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
    if (FAILED(hResult))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to create Compiler");
        return false;
    }

    TComPtr<IDxcLibrary> Library;
    hResult = DxcCreateInstanceFunc(CLSID_DxcLibrary, IID_PPV_ARGS(&Library));
    if (FAILED(hResult))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to create Library");
        return false;
    }

    TComPtr<IDxcIncludeHandler> IncludeHandler;
    hResult = Library->CreateIncludeHandler(&IncludeHandler);
    if (FAILED(hResult))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to create IncludeHandler");
        return false;
    }
    
    TComPtr<IDxcBlobEncoding> SourceBlob;
    hResult = Library->CreateBlobFromFile(WideFilePath.CStr(), nullptr, &SourceBlob);
    if (FAILED(hResult))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to create Source Data");
        CDebug::DebugBreak();
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
    WString WArgumentsString;
    for (LPCWSTR Arg : CompileArgs)
    {
        WArgumentsString += Arg + ' ';
    }

    const String ArgumentsString = WideToChar(WArgumentsString);
    
    // Convert defines
    TArray<WString>   StrBuff;
    TArray<DxcDefine> DxcDefines;
    
    TArrayView<SShaderDefine> Defines = CompileInfo.Defines;
    if (!Defines.IsEmpty())
    {
        StrBuff.Reserve(Defines.Size() * 2);
        DxcDefines.Reserve(Defines.Size());

        for (const SShaderDefine& Define : Defines)
        {
            const WString& WideDefine = StrBuff.Emplace(CharToWide(Define.Define));
            const WString& WideValue  = StrBuff.Emplace(CharToWide(Define.Value));
            DxcDefines.Push({ WideDefine.CStr(), WideValue.CStr() });
        }
    }
    
    // Retrieve the shader target
    const LPCWSTR ShaderStageText = GetShaderStageString(CompileInfo.ShaderStage);
    const LPCWSTR ShaderModelText = GetShaderModelString(CompileInfo.ShaderModel);

    constexpr uint32 BufferLength = sizeof("xxx_x_x");
    
    WCHAR TargetProfile[BufferLength];
    WStringUtils::FormatBuffer(TargetProfile, BufferLength, L"%ls_%ls", ShaderStageText, ShaderModelText);
    
    const WString WideEntrypoint = CharToWide(CompileInfo.EntryPoint);

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
        LOG_ERROR("[CShaderCompiler]: FAILED to Compile");
        CDebug::DebugBreak();
        return false;
    }

    if (FAILED(Result->GetStatus(&hResult)))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to Retrieve result. Unknown Error.");
        CDebug::DebugBreak();
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
            LOG_ERROR("[CShaderCompiler]: FAILED to compile with error: %s", reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
        }
        else
        {
            LOG_ERROR("[CShaderCompiler]: FAILED to compile with. Unknown ERROR.");
        }

        return false;
    }
    
    if (PrintBlob8 && (PrintBlob8->GetBufferSize() > 0))
    {
        const String Output(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()), uint32(PrintBlob8->GetBufferSize()));
        LOG_INFO("[CShaderCompiler]: Successfully compiled shader '%s', with arguments '%s' and with the following output: %s", Filename.CStr(), ArgumentsString.CStr(), Output.CStr());
    }
    else
    {
        LOG_INFO("[CShaderCompiler]: Successfully compiled shader '%s', with arguments '%s'.", Filename.CStr(), ArgumentsString.CStr());
    }

    TComPtr<IDxcBlob> CompiledBlob;
    if (FAILED(Result->GetResult(&CompiledBlob)))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to retrieve result");
        return false;
    }

    const uint32 BlobSize = uint32(CompiledBlob->GetBufferSize());
    OutByteCode.Resize(BlobSize);

    LOG_INFO("[CShaderCompiler]: Compiled Size: %u Bytes", BlobSize);

    CMemory::Memcpy(OutByteCode.Data(), CompiledBlob->GetBufferPointer(), BlobSize);
    
    return true;
}

bool CShaderCompiler::CompileFromSource(const String& ShaderSource, const CShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    OutByteCode.Clear();

    TComPtr<IDxcCompiler> Compiler;
    HRESULT hResult = DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
    if (FAILED(hResult))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to create Compiler");
        return false;
    }

    TComPtr<IDxcLibrary> Library;
    hResult = DxcCreateInstanceFunc(CLSID_DxcLibrary, IID_PPV_ARGS(&Library));
    if (FAILED(hResult))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to create Library");
        return false;
    }

    TComPtr<IDxcIncludeHandler> IncludeHandler;
    hResult = Library->CreateIncludeHandler(&IncludeHandler);
    if (FAILED(hResult))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to create IncludeHandler");
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
    WString WArgumentsString;
    for (LPCWSTR Arg : CompileArgs)
    {
        WArgumentsString += Arg + ' ';
    }

    const String ArgumentsString = WideToChar(WArgumentsString);

    // Convert defines
    TArray<WString>   StrBuff;
    TArray<DxcDefine> DxcDefines;

    TArrayView<SShaderDefine> Defines = CompileInfo.Defines;
    if (!Defines.IsEmpty())
    {
        StrBuff.Reserve(Defines.Size() * 2);
        DxcDefines.Reserve(Defines.Size());

        for (const SShaderDefine& Define : Defines)
        {
            const WString& WideDefine = StrBuff.Emplace(CharToWide(Define.Define));
            const WString& WideValue  = StrBuff.Emplace(CharToWide(Define.Value));
            DxcDefines.Push({ WideDefine.CStr(), WideValue.CStr() });
        }
    }

    // Retrieve the shader target
    const LPCWSTR ShaderStageText = GetShaderStageString(CompileInfo.ShaderStage);
    const LPCWSTR ShaderModelText = GetShaderModelString(CompileInfo.ShaderModel);

    constexpr uint32 BufferLength = sizeof("xxx_x_x");

    WCHAR TargetProfile[BufferLength];
    WStringUtils::FormatBuffer(TargetProfile, BufferLength, L"%ls_%ls", ShaderStageText, ShaderModelText);
	
    // Use the asset-folder as base for the shader-files
	const WString WideEntrypoint = CharToWide(CompileInfo.EntryPoint);

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
        LOG_ERROR("[CShaderCompiler]: FAILED to Compile");
        CDebug::DebugBreak();
        return false;
    }

    if (FAILED(Result->GetStatus(&hResult)))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to Retrieve result. Unknown Error.");
        CDebug::DebugBreak();
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
            LOG_ERROR("[CShaderCompiler]: FAILED to compile with error: %s", reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
        }
        else
        {
            LOG_ERROR("[CShaderCompiler]: FAILED to compile with. Unknown ERROR.");
        }

        return false;
    }

    if (PrintBlob8 && (PrintBlob8->GetBufferSize() > 0))
    {
        const String Output(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()), uint32(PrintBlob8->GetBufferSize()));
        LOG_INFO("[CShaderCompiler]: Successfully compiled shader from source, with arguments '%s' and with the following output: %s", ArgumentsString.CStr(), Output.CStr());
    }
    else
    {
        LOG_INFO("[CShaderCompiler]: Successfully compiled shader from source, with arguments '%s'.", ArgumentsString.CStr());
    }

    TComPtr<IDxcBlob> CompiledBlob;
    if (FAILED(Result->GetResult(&CompiledBlob)))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to retrieve result");
        return false;
    }

    const uint32 BlobSize = uint32(CompiledBlob->GetBufferSize());
    OutByteCode.Resize(BlobSize);

    LOG_INFO("[CShaderCompiler]: Compiled Size: %u Bytes", BlobSize);

    CMemory::Memcpy(OutByteCode.Data(), CompiledBlob->GetBufferPointer(), BlobSize);

    return true;
}
