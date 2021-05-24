#include "D3D12ShaderCompiler.h"

#include "Utilities/StringUtilities.h"

#include "Windows/Windows.h"
#include "Windows/Windows.inl"

#include "Core/Application/Platform/PlatformMisc.h"

DxcCreateInstanceProc DxcCreateInstanceFunc = nullptr;

#ifndef MAKEFOURCC
    #define MAKEFOURCC(a, b, c, d) (unsigned int)((unsigned char)(a) | (unsigned char)(b) << 8 | (unsigned char)(c) << 16 | (unsigned char)(d) << 24)
#endif

enum DxilFourCC 
{
    DFCC_Container               = MAKEFOURCC('D', 'X', 'B', 'C'),
    DFCC_ResourceDef             = MAKEFOURCC('R', 'D', 'E', 'F'),
    DFCC_InputSignature          = MAKEFOURCC('I', 'S', 'G', '1'),
    DFCC_OutputSignature         = MAKEFOURCC('O', 'S', 'G', '1'),
    DFCC_PatchConstantSignature  = MAKEFOURCC('P', 'S', 'G', '1'),
    DFCC_ShaderStatistics        = MAKEFOURCC('S', 'T', 'A', 'T'),
    DFCC_ShaderDebugInfoDXIL     = MAKEFOURCC('I', 'L', 'D', 'B'),
    DFCC_ShaderDebugName         = MAKEFOURCC('I', 'L', 'D', 'N'),
    DFCC_FeatureInfo             = MAKEFOURCC('S', 'F', 'I', '0'),
    DFCC_PrivateData             = MAKEFOURCC('P', 'R', 'I', 'V'),
    DFCC_RootSignature           = MAKEFOURCC('R', 'T', 'S', '0'),
    DFCC_DXIL                    = MAKEFOURCC('D', 'X', 'I', 'L'),
    DFCC_PipelineStateValidation = MAKEFOURCC('P', 'S', 'V', '0'),
    DFCC_RuntimeData             = MAKEFOURCC('R', 'D', 'A', 'T'),
    DFCC_ShaderHash              = MAKEFOURCC('H', 'A', 'S', 'H'),
};

#undef MAKEFOURCC

// TODO: Maybe a compile time version?
static LPCWSTR GetTargetProfile(EShaderStage Stage, EShaderModel Model)
{
    switch (Stage)
    {
        case EShaderStage::Compute:
        {
            switch (Model)
            {
                case EShaderModel::SM_5_0: return L"cs_5_0";
                case EShaderModel::SM_5_1: return L"cs_5_1";
                case EShaderModel::SM_6_0: return L"cs_6_0";
                case EShaderModel::SM_6_1: return L"cs_6_1";
                case EShaderModel::SM_6_2: return L"cs_6_2";
                case EShaderModel::SM_6_3: return L"cs_6_3";
                case EShaderModel::SM_6_4: return L"cs_6_4";
                case EShaderModel::SM_6_5: return L"cs_6_5";
                default: break;
            }
        }

        case EShaderStage::Vertex:
        {
            switch (Model)
            {
                case EShaderModel::SM_5_0: return L"vs_5_0";
                case EShaderModel::SM_5_1: return L"vs_5_1";
                case EShaderModel::SM_6_0: return L"vs_6_0";
                case EShaderModel::SM_6_1: return L"vs_6_1";
                case EShaderModel::SM_6_2: return L"vs_6_2";
                case EShaderModel::SM_6_3: return L"vs_6_3";
                case EShaderModel::SM_6_4: return L"vs_6_4";
                case EShaderModel::SM_6_5: return L"vs_6_5";
                default: break;
            }
        }

        case EShaderStage::Hull:
        {
            switch (Model)
            {
                case EShaderModel::SM_5_0: return L"hs_5_0";
                case EShaderModel::SM_5_1: return L"hs_5_1";
                case EShaderModel::SM_6_0: return L"hs_6_0";
                case EShaderModel::SM_6_1: return L"hs_6_1";
                case EShaderModel::SM_6_2: return L"hs_6_2";
                case EShaderModel::SM_6_3: return L"hs_6_3";
                case EShaderModel::SM_6_4: return L"hs_6_4";
                case EShaderModel::SM_6_5: return L"hs_6_5";
                default: break;
            }
        }

        case EShaderStage::Domain:
        {
            switch (Model)
            {
                case EShaderModel::SM_5_0: return L"ds_5_0";
                case EShaderModel::SM_5_1: return L"ds_5_1";
                case EShaderModel::SM_6_0: return L"ds_6_0";
                case EShaderModel::SM_6_1: return L"ds_6_1";
                case EShaderModel::SM_6_2: return L"ds_6_2";
                case EShaderModel::SM_6_3: return L"ds_6_3";
                case EShaderModel::SM_6_4: return L"ds_6_4";
                case EShaderModel::SM_6_5: return L"ds_6_5";
                default: break;
            }
        }

        case EShaderStage::Geometry:
        {
            switch (Model)
            {
                case EShaderModel::SM_5_0: return L"gs_5_0";
                case EShaderModel::SM_5_1: return L"gs_5_1";
                case EShaderModel::SM_6_0: return L"gs_6_0";
                case EShaderModel::SM_6_1: return L"gs_6_1";
                case EShaderModel::SM_6_2: return L"gs_6_2";
                case EShaderModel::SM_6_3: return L"gs_6_3";
                case EShaderModel::SM_6_4: return L"gs_6_4";
                case EShaderModel::SM_6_5: return L"gs_6_5";
                default: break;
            }
        }

        case EShaderStage::Pixel:
        {
            switch (Model)
            {
                case EShaderModel::SM_5_0: return L"ps_5_0";
                case EShaderModel::SM_5_1: return L"ps_5_1";
                case EShaderModel::SM_6_0: return L"ps_6_0";
                case EShaderModel::SM_6_1: return L"ps_6_1";
                case EShaderModel::SM_6_2: return L"ps_6_2";
                case EShaderModel::SM_6_3: return L"ps_6_3";
                case EShaderModel::SM_6_4: return L"ps_6_4";
                case EShaderModel::SM_6_5: return L"ps_6_5";
                default: break;
            }
        }

        case EShaderStage::Mesh:
        {
            switch (Model)
            {
                case EShaderModel::SM_5_0: return L"ms_5_0";
                case EShaderModel::SM_5_1: return L"ms_5_1";
                case EShaderModel::SM_6_0: return L"ms_6_0";
                case EShaderModel::SM_6_1: return L"ms_6_1";
                case EShaderModel::SM_6_2: return L"ms_6_2";
                case EShaderModel::SM_6_3: return L"ms_6_3";
                case EShaderModel::SM_6_4: return L"ms_6_4";
                case EShaderModel::SM_6_5: return L"ms_6_5";
                default: break;
            }
        }

        case EShaderStage::Amplification:
        {
            switch (Model)
            {
                case EShaderModel::SM_5_0: return L"as_5_0";
                case EShaderModel::SM_5_1: return L"as_5_1";
                case EShaderModel::SM_6_0: return L"as_6_0";
                case EShaderModel::SM_6_1: return L"as_6_1";
                case EShaderModel::SM_6_2: return L"as_6_2";
                case EShaderModel::SM_6_3: return L"as_6_3";
                case EShaderModel::SM_6_4: return L"as_6_4";
                case EShaderModel::SM_6_5: return L"as_6_5";
                default: break;
            }
        }

        case EShaderStage::RayGen:
        case EShaderStage::RayAnyHit:
        case EShaderStage::RayClosestHit:
        case EShaderStage::RayMiss: 
        {
            switch (Model)
            {
                case EShaderModel::SM_6_3: return L"lib_6_3";
                case EShaderModel::SM_6_4: return L"lib_6_4";
                case EShaderModel::SM_6_5: return L"lib_6_5";
                default: break;
            }
        }
    }

    return L"Unknown";
}

// Custom blob for existing data
class ExistingBlob : public IDxcBlob
{
public:
    ExistingBlob(LPVOID InData, SIZE_T InSizeInBytes)
        : Data(nullptr)
        , SizeInBytes(InSizeInBytes)
        , References(0)
    {
        Data = Memory::Malloc(SizeInBytes);
        Memory::Memcpy(Data, InData, SizeInBytes);
    }

    ~ExistingBlob()
    {
        Memory::Free(Data);
    }

    virtual LPVOID GetBufferPointer(void) override
    {
        return Data;
    }

    virtual SIZE_T GetBufferSize(void) override
    {
        return SizeInBytes;
    }

    virtual HRESULT QueryInterface(REFIID riid, LPVOID* ppvObject)
    {
        if (!ppvObject)
        {
            return E_INVALIDARG;
        }

        *ppvObject = NULL;
        
        if (riid == IID_IUnknown || riid == IID_ID3DBlob)
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
    SIZE_T SizeInBytes;
    ULONG  References;
};

D3D12ShaderCompiler* GD3D12ShaderCompiler = nullptr;

D3D12ShaderCompiler::D3D12ShaderCompiler()
    : IShaderCompiler()
    , DxCompiler(nullptr)
    , DxLibrary(nullptr)
    , DxLinker(nullptr)
    , DxIncludeHandler(nullptr)
    , DxCompilerDLL()
{
    GD3D12ShaderCompiler = this;
}

D3D12ShaderCompiler::~D3D12ShaderCompiler()
{
    GD3D12ShaderCompiler = nullptr;

    DxCompiler.Reset();
    DxLibrary.Reset();
    DxLinker.Reset();
    DxIncludeHandler.Reset();
    DxReflection.Reset();

    ::FreeLibrary(DxCompilerDLL);
}

bool D3D12ShaderCompiler::CompileFromFile(
    const std::string& FilePath, 
    const std::string& EntryPoint, 
    const TArray<ShaderDefine>* Defines,
    EShaderStage ShaderStage, 
    EShaderModel ShaderModel, 
    TArray<uint8>& Code)
{
    Code.Clear();

    std::wstring WideFilePath   = ConvertToWide(FilePath);
    std::wstring WideEntrypoint = ConvertToWide(EntryPoint);

    TComPtr<IDxcBlobEncoding> SourceBlob;
    HRESULT Result = DxLibrary->CreateBlobFromFile(WideFilePath.c_str(), nullptr, &SourceBlob);
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create Source Data");

        Debug::DebugBreak();
        return false;
    }

    return InternalCompileFromSource(SourceBlob.Get(), WideFilePath.c_str(), WideEntrypoint.c_str(), ShaderStage, ShaderModel, Defines, Code);
}

bool D3D12ShaderCompiler::CompileShader(
    const std::string& ShaderSource, 
    const std::string& EntryPoint, 
    const TArray<ShaderDefine>* Defines,
    EShaderStage ShaderStage, 
    EShaderModel ShaderModel, 
    TArray<uint8>& Code)
{
    std::wstring WideEntrypoint = ConvertToWide(EntryPoint);

    TComPtr<IDxcBlobEncoding> SourceBlob;
    HRESULT Result = DxLibrary->CreateBlobWithEncodingOnHeapCopy(
        ShaderSource.c_str(), 
        sizeof(char) * static_cast<uint32>(ShaderSource.size()), 
        CP_UTF8, 
        &SourceBlob);
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create Source Data");

        Debug::DebugBreak();
        return false;
    }

    return InternalCompileFromSource(SourceBlob.Get(), nullptr, WideEntrypoint.c_str(), ShaderStage, ShaderModel, Defines, Code);
}

bool D3D12ShaderCompiler::GetReflection(D3D12BaseShader* Shader, ID3D12ShaderReflection** Reflection)
{
    TComPtr<IDxcBlob> ShaderBlob = DBG_NEW ExistingBlob((LPVOID)Shader->GetCode(), Shader->GetCodeSize());
    return InternalGetReflection(ShaderBlob, IID_PPV_ARGS(Reflection));
}

bool D3D12ShaderCompiler::GetLibraryReflection(D3D12BaseShader* Shader, ID3D12LibraryReflection** Reflection)
{
    TComPtr<IDxcBlob> ShaderBlob = DBG_NEW ExistingBlob((LPVOID)Shader->GetCode(), Shader->GetCodeSize());
    return InternalGetReflection(ShaderBlob, IID_PPV_ARGS(Reflection));
}

bool D3D12ShaderCompiler::HasRootSignature(D3D12BaseShader* Shader)
{
    TComPtr<IDxcContainerReflection> Reflection;
    HRESULT Result = DxcCreateInstanceFunc(CLSID_DxcContainerReflection, IID_PPV_ARGS(&Reflection));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create IDxcContainerReflection");
        return false;
    }

    TComPtr<IDxcBlob> ShaderBlob = DBG_NEW ExistingBlob((LPVOID)Shader->GetCode(), Shader->GetCodeSize());
    Result = Reflection->Load(ShaderBlob.Get());
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: Reflection were not able to load shader");
        return false;
    }

    uint32 PartIndex;
    Result = Reflection->FindFirstPartKind(DFCC_RootSignature, &PartIndex);
    if (FAILED(Result))
    {
        return false;
    }

    return true;
}

bool D3D12ShaderCompiler::Init()
{
    DxCompilerDLL = ::LoadLibrary("dxcompiler.dll");
    if (!DxCompilerDLL)
    {
        PlatformMisc::MessageBox("ERROR", "FAILED to load dxcompiler.dll");
        return false;
    }

    DxcCreateInstanceFunc = GetTypedProcAddress<DxcCreateInstanceProc>(DxCompilerDLL, "DxcCreateInstance");
    if (!DxcCreateInstanceFunc)
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to load DxcCreateInstance");
        return false;
    }
    
    HRESULT Result = DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&DxCompiler));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxCompiler");
        return false;
    }

    Result = DxcCreateInstanceFunc(CLSID_DxcLibrary, IID_PPV_ARGS(&DxLibrary));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxLibrary");
        return false;
    }

    Result = DxLibrary->CreateIncludeHandler(&DxIncludeHandler);
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxIncludeHandler");
        return false;
    }

    Result = DxcCreateInstanceFunc(CLSID_DxcLinker, IID_PPV_ARGS(&DxLinker));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxLinker");
        return false;
    }

    Result = DxcCreateInstanceFunc(CLSID_DxcContainerReflection, IID_PPV_ARGS(&DxReflection));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxReflection");
        return false;
    }

    return true;
}

bool D3D12ShaderCompiler::InternalCompileFromSource(
    IDxcBlob* SourceBlob, 
    LPCWSTR FilePath, 
    LPCWSTR Entrypoint, 
    EShaderStage ShaderStage,
    EShaderModel ShaderModel,
    const TArray<ShaderDefine>* Defines,
    TArray<uint8>& Code)
{
    TArray<LPCWSTR> Args =
    {
        L"-O3", // Optimization level 3
    };

    if (ShaderStageIsRayTracing(ShaderStage))
    {
        Args.EmplaceBack(L"-exports");
        Args.EmplaceBack(Entrypoint);
    }

    // Convert defines
    TArray<DxcDefine> DxDefines;
    TArray<std::wstring> StrBuff;
    if (Defines)
    {
        StrBuff.Reserve(Defines->Size() * 2);
        DxDefines.Reserve(Defines->Size());

        for (const ShaderDefine& Define : *Defines)
        {
            const std::wstring& WideDefine = StrBuff.EmplaceBack(ConvertToWide(Define.Define));
            const std::wstring& WideValue  = StrBuff.EmplaceBack(ConvertToWide(Define.Value));
            DxDefines.PushBack({ WideDefine.c_str(), WideValue.c_str() });
        }
    }

    LPCWSTR TargetProfile = GetTargetProfile(ShaderStage, ShaderModel);

    TComPtr<IDxcOperationResult> Result;
    HRESULT hResult = DxCompiler->Compile(
        SourceBlob, FilePath, Entrypoint, 
        TargetProfile, 
        Args.Data(), Args.Size(), 
        DxDefines.Data(), DxDefines.Size(),
        DxIncludeHandler.Get(), &Result);
    if (FAILED(hResult))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to Compile");

        Debug::DebugBreak();
        return false;
    }

    if (FAILED(Result->GetStatus(&hResult)))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to Retrive result. Unknown Error.");
        
        Debug::DebugBreak();
        return false;
    }

    TComPtr<IDxcBlobEncoding> PrintBlob;
    TComPtr<IDxcBlobEncoding> PrintBlob8;
    if (SUCCEEDED(Result->GetErrorBuffer(&PrintBlob)))
    {
        DxLibrary->GetBlobAsUtf8(PrintBlob.Get(), &PrintBlob8);
    }

    if (FAILED(hResult))
    {
        if (PrintBlob8 && PrintBlob8->GetBufferSize() > 0)
        {
            LOG_ERROR("[D3D12ShaderCompiler]: FAILED to compile with the following error:");
            LOG_ERROR(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
        }
        else
        {
            LOG_ERROR("[D3D12ShaderCompiler]: FAILED to compile with. Unknown ERROR.");
        }

        return false;
    }

    std::string AsciiFilePath = FilePath != nullptr ? ConvertToAscii(FilePath) : "";
    if (PrintBlob8 && PrintBlob8->GetBufferSize() > 0)
    {
        LOG_INFO("[D3D12ShaderCompiler]: Successfully compiled shader '" + AsciiFilePath + "' with the following output:");
        LOG_INFO(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
    }
    else
    {
        LOG_INFO("[D3D12ShaderCompiler]: Successfully compiled shader '" + AsciiFilePath + "'.");
    }

    TComPtr<IDxcBlob> CompiledBlob;
    if (FAILED(Result->GetResult(&CompiledBlob)))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to retrive result");
        return false;
    }

    const uint32 BlobSize = uint32(CompiledBlob->GetBufferSize());
    Code.Resize(BlobSize);

    LOG_INFO("[D3D12ShaderCompiler]: Compiled Size: " + std::to_string(BlobSize) + " Bytes");

    Memory::Memcpy(Code.Data(), CompiledBlob->GetBufferPointer(), BlobSize);

    if (ShaderStageIsRayTracing(ShaderStage))
    {
        return ValidateRayTracingShader(CompiledBlob, Entrypoint);
    }
    else
    {
        return true;
    }
}

bool D3D12ShaderCompiler::InternalGetReflection(const TComPtr<IDxcBlob>& ShaderBlob, REFIID iid, void** ppvObject)
{
    HRESULT Result = DxReflection->Load(ShaderBlob.Get());
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: Were not able to validate ray tracing shader");
        return false;
    }

    uint32 PartIndex;
    Result = DxReflection->FindFirstPartKind(DFCC_DXIL, &PartIndex);
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: Were not able to validate ray tracing shader");
        return false;
    }

    Result = DxReflection->GetPartReflection(PartIndex, iid, ppvObject);
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: Were not able to validate ray tracing shader");
        return false;
    }

    return true;
}

bool D3D12ShaderCompiler::ValidateRayTracingShader(const TComPtr<IDxcBlob>& ShaderBlob, LPCWSTR Entrypoint)
{
    TComPtr<ID3D12LibraryReflection> LibaryReflection;
    if (!InternalGetReflection(ShaderBlob, IID_PPV_ARGS(&LibaryReflection)))
    {
        return false;
    }

    D3D12_LIBRARY_DESC LibDesc;
    Memory::Memzero(&LibDesc);

    HRESULT Result = LibaryReflection->GetDesc(&LibDesc);
    if (FAILED(Result) )
    {
        LOG_ERROR("[D3D12ShaderCompiler]: Were not able to validate ray tracing shader");
        return false;
    }

    Assert(LibDesc.FunctionCount > 0);

    // Make sure that the first shader is the one we wanted
    ID3D12FunctionReflection* Function = LibaryReflection->GetFunctionByIndex(0);
    
    D3D12_FUNCTION_DESC FuncDesc;
    Memory::Memzero(&FuncDesc);

    Result = Function->GetDesc(&FuncDesc);
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: Were not able to validate ray tracing shader");
        return false;
    }

    char Buffer[256];
    Memory::Memzero(Buffer, sizeof(Buffer));

    size_t ConvertedChars;
    wcstombs_s(&ConvertedChars, Buffer, 256, Entrypoint, _TRUNCATE);

    std::string FuncName(FuncDesc.Name);
    auto result = FuncName.find(Buffer);
    if (result == std::string::npos)
    {
        LOG_ERROR("[D3D12ShaderCompiler]: First exported function does not have correct entrypoint '" + std::string(Buffer) + "'. Name=" + FuncName);
        return false;
    }

    return true;
}
