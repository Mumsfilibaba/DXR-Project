#include "D3D12RHIShaderCompiler.h"

#include "Core/Utilities/StringUtilities.h"
#include "Core/Logging/Log.h"
#include "Core/Windows/Windows.h"
#include "Core/Modules/Platform/PlatformLibrary.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

DxcCreateInstanceProc DxcCreateInstanceFunc = nullptr;

#ifndef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d) (unsigned int)((unsigned char)(a) | (unsigned char)(b) << 8 | (unsigned char)(c) << 16 | (unsigned char)(d) << 24)
#endif

enum DxilFourCC
{
    DFCC_Container = MAKEFOURCC('D', 'X', 'B', 'C'),
    DFCC_ResourceDef = MAKEFOURCC('R', 'D', 'E', 'F'),
    DFCC_InputSignature = MAKEFOURCC('I', 'S', 'G', '1'),
    DFCC_OutputSignature = MAKEFOURCC('O', 'S', 'G', '1'),
    DFCC_PatchConstantSignature = MAKEFOURCC('P', 'S', 'G', '1'),
    DFCC_ShaderStatistics = MAKEFOURCC('S', 'T', 'A', 'T'),
    DFCC_ShaderDebugInfoDXIL = MAKEFOURCC('I', 'L', 'D', 'B'),
    DFCC_ShaderDebugName = MAKEFOURCC('I', 'L', 'D', 'N'),
    DFCC_FeatureInfo = MAKEFOURCC('S', 'F', 'I', '0'),
    DFCC_PrivateData = MAKEFOURCC('P', 'R', 'I', 'V'),
    DFCC_RootSignature = MAKEFOURCC('R', 'T', 'S', '0'),
    DFCC_DXIL = MAKEFOURCC('D', 'X', 'I', 'L'),
    DFCC_PipelineStateValidation = MAKEFOURCC('P', 'S', 'V', '0'),
    DFCC_RuntimeData = MAKEFOURCC('R', 'D', 'A', 'T'),
    DFCC_ShaderHash = MAKEFOURCC('H', 'A', 'S', 'H'),
};

#undef MAKEFOURCC

static LPCWSTR GetShaderStageText(ERHIShaderStage Stage)
{
    switch (Stage)
    {
        // Compute
    case ERHIShaderStage::Compute:       return L"cs";

        // Graphics
    case ERHIShaderStage::Vertex:        return L"vs";
    case ERHIShaderStage::Hull:          return L"hs";
    case ERHIShaderStage::Domain:        return L"ds";
    case ERHIShaderStage::Geometry:      return L"gs";
    case ERHIShaderStage::Pixel:         return L"ps";

        // New Graphics Pipeline
    case ERHIShaderStage::Mesh:          return L"ms";
    case ERHIShaderStage::Amplification: return L"as";

        // Ray tracing
    case ERHIShaderStage::RayGen:
    case ERHIShaderStage::RayAnyHit:
    case ERHIShaderStage::RayClosestHit:
    case ERHIShaderStage::RayMiss:       return L"lib";
    }

    return L"xxx";
}

static LPCWSTR GetShaderModelText(EShaderModel Model)
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
    default: break;
    }

    return L"0_0";
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ExistingBlob - Custom blob for existing data

class CExistingBlob : public IDxcBlob
{
public:
    CExistingBlob(LPVOID InData, SIZE_T InSizeInBytes)
        : Data(nullptr)
        , SizeInBytes(InSizeInBytes)
        , References(1)
    {
        Data = Memory::Malloc(SizeInBytes);
        Memory::Memcpy(Data, InData, SizeInBytes);
    }

    ~CExistingBlob()
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

    virtual HRESULT QueryInterface(REFIID Riid, LPVOID* ppvObject)
    {
        if (!ppvObject)
        {
            return E_INVALIDARG;
        }

        *ppvObject = NULL;

        if (Riid == IID_IUnknown || Riid == IID_ID3DBlob)
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

    ULONG References;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIShaderCompiler

CD3D12RHIShaderCompiler* GD3D12ShaderCompiler = nullptr;

CD3D12RHIShaderCompiler::CD3D12RHIShaderCompiler()
    : IRHIShaderCompiler()
    , DxCompiler(nullptr)
    , DxLibrary(nullptr)
    , DxLinker(nullptr)
    , DxIncludeHandler(nullptr)
    , DxCompilerDLL(0)
{
    GD3D12ShaderCompiler = this;
}

CD3D12RHIShaderCompiler::~CD3D12RHIShaderCompiler()
{
    GD3D12ShaderCompiler = nullptr;

    DxCompiler.Reset();
    DxLibrary.Reset();
    DxLinker.Reset();
    DxIncludeHandler.Reset();
    DxReflection.Reset();

    ::FreeLibrary(DxCompilerDLL);
}

bool CD3D12RHIShaderCompiler::CompileFromFile(
    const String& FilePath,
    const String& EntryPoint,
    const TArray<SShaderDefine>* Defines,
    ERHIShaderStage ShaderStage,
    EShaderModel ShaderModel,
    TArray<uint8>& Code)
{
    Code.Clear();

    WString WideFilePath = CharToWide(FilePath);
    WString WideEntrypoint = CharToWide(EntryPoint);

    TComPtr<IDxcBlobEncoding> SourceBlob;
    HRESULT Result = DxLibrary->CreateBlobFromFile(WideFilePath.CStr(), nullptr, &SourceBlob);
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to create Source Data");

        Debug::DebugBreak();
        return false;
    }

    return InternalCompileFromSource(SourceBlob.Get(), WideFilePath.CStr(), WideEntrypoint.CStr(), ShaderStage, ShaderModel, Defines, Code);
}

bool CD3D12RHIShaderCompiler::CompileShader(
    const String& ShaderSource,
    const String& EntryPoint,
    const TArray<SShaderDefine>* Defines,
    ERHIShaderStage ShaderStage,
    EShaderModel ShaderModel,
    TArray<uint8>& Code)
{
    WString WideEntrypoint = CharToWide(EntryPoint);

    TComPtr<IDxcBlobEncoding> SourceBlob;
    HRESULT Result = DxLibrary->CreateBlobWithEncodingOnHeapCopy(ShaderSource.CStr(), sizeof(char) * static_cast<uint32>(ShaderSource.Size()), CP_UTF8, &SourceBlob);
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to create Source Data");

        Debug::DebugBreak();
        return false;
    }

    return InternalCompileFromSource(SourceBlob.Get(), nullptr, WideEntrypoint.CStr(), ShaderStage, ShaderModel, Defines, Code);
}

bool CD3D12RHIShaderCompiler::GetReflection(CD3D12BaseShader* Shader, ID3D12ShaderReflection** Reflection)
{
    TComPtr<IDxcBlob> ShaderBlob = dbg_new CExistingBlob((LPVOID)Shader->GetCode(), Shader->GetCodeSize());
    return InternalGetReflection(ShaderBlob, IID_PPV_ARGS(Reflection));
}

bool CD3D12RHIShaderCompiler::GetLibraryReflection(CD3D12BaseShader* Shader, ID3D12LibraryReflection** Reflection)
{
    TComPtr<IDxcBlob> ShaderBlob = dbg_new CExistingBlob((LPVOID)Shader->GetCode(), Shader->GetCodeSize());
    return InternalGetReflection(ShaderBlob, IID_PPV_ARGS(Reflection));
}

bool CD3D12RHIShaderCompiler::HasRootSignature(CD3D12BaseShader* Shader)
{
    TComPtr<IDxcContainerReflection> Reflection;
    HRESULT Result = DxcCreateInstanceFunc(CLSID_DxcContainerReflection, IID_PPV_ARGS(&Reflection));
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to create IDxcContainerReflection");
        return false;
    }

    TComPtr<IDxcBlob> ShaderBlob = dbg_new CExistingBlob((LPVOID)Shader->GetCode(), Shader->GetCodeSize());
    Result = Reflection->Load(ShaderBlob.Get());
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: Reflection were not able to load shader");
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

bool CD3D12RHIShaderCompiler::Init()
{
    DxCompilerDLL = ::LoadLibrary("dxcompiler.dll");
    if (!DxCompilerDLL)
    {
        PlatformApplicationMisc::MessageBox("ERROR", "FAILED to load dxcompiler.dll");
        return false;
    }

    DxcCreateInstanceFunc = PlatformLibrary::LoadSymbolAddress<DxcCreateInstanceProc>("DxcCreateInstance", DxCompilerDLL);
    if (!DxcCreateInstanceFunc)
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to load DxcCreateInstance");
        return false;
    }

    HRESULT Result = DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&DxCompiler));
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to create DxCompiler");
        return false;
    }

    Result = DxcCreateInstanceFunc(CLSID_DxcLibrary, IID_PPV_ARGS(&DxLibrary));
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to create DxLibrary");
        return false;
    }

    Result = DxLibrary->CreateIncludeHandler(&DxIncludeHandler);
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to create DxIncludeHandler");
        return false;
    }

    Result = DxcCreateInstanceFunc(CLSID_DxcLinker, IID_PPV_ARGS(&DxLinker));
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to create DxLinker");
        return false;
    }

    Result = DxcCreateInstanceFunc(CLSID_DxcContainerReflection, IID_PPV_ARGS(&DxReflection));
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to create DxReflection");
        return false;
    }

    return true;
}

bool CD3D12RHIShaderCompiler::InternalCompileFromSource(
    IDxcBlob* SourceBlob,
    LPCWSTR FilePath,
    LPCWSTR Entrypoint,
    ERHIShaderStage ShaderStage,
    EShaderModel ShaderModel,
    const TArray<SShaderDefine>* Defines,
    TArray<uint8>& Code)
{
    TArray<LPCWSTR> Args =
    {
        L"-O3", // Optimization level 3
    };

    if (ShaderStageIsRayTracing(ShaderStage))
    {
        Args.Emplace(L"-exports");
        Args.Emplace(Entrypoint);
    }

    // Convert defines
    TArray<DxcDefine> DxDefines;
    TArray<WString> StrBuff;
    if (Defines)
    {
        StrBuff.Reserve(Defines->Size() * 2);
        DxDefines.Reserve(Defines->Size());

        for (const SShaderDefine& Define : *Defines)
        {
            const WString& WideDefine = StrBuff.Emplace(CharToWide(Define.Define));
            const WString& WideValue = StrBuff.Emplace(CharToWide(Define.Value));
            DxDefines.Push({ WideDefine.CStr(), WideValue.CStr() });
        }
    }

    // Retrieve the shader target
    LPCWSTR ShaderStageText = GetShaderStageText(ShaderStage);
    LPCWSTR ShaderModelText = GetShaderModelText(ShaderModel);

    constexpr uint32 BufferLength = sizeof("xxx_x_x");
    wchar_t TargetProfile[BufferLength];
    WStringUtils::FormatBuffer(TargetProfile, BufferLength, L"%ls_%ls", ShaderStageText, ShaderModelText);

    TComPtr<IDxcOperationResult> Result;
    HRESULT hResult = DxCompiler->Compile(
        SourceBlob, FilePath, Entrypoint,
        TargetProfile,
        Args.Data(), Args.Size(),
        DxDefines.Data(), DxDefines.Size(),
        DxIncludeHandler.Get(), &Result);
    if (FAILED(hResult))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to Compile");

        Debug::DebugBreak();
        return false;
    }

    if (FAILED(Result->GetStatus(&hResult)))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to Retrieve result. Unknown Error.");

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
            LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to compile with the following error:");
            LOG_ERROR(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
        }
        else
        {
            LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to compile with. Unknown ERROR.");
        }

        return false;
    }

    String AsciiFilePath = (FilePath != nullptr) ? WideToChar(WString(FilePath)) : "";
    if (PrintBlob8 && PrintBlob8->GetBufferSize() > 0)
    {
        LOG_INFO("[CD3D12RHIShaderCompiler]: Successfully compiled shader '" + AsciiFilePath + "' with the following output:");
        LOG_INFO(String(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()), uint32(PrintBlob8->GetBufferSize())));
    }
    else
    {
        LOG_INFO("[CD3D12RHIShaderCompiler]: Successfully compiled shader '" + AsciiFilePath + "'.");
    }

    TComPtr<IDxcBlob> CompiledBlob;
    if (FAILED(Result->GetResult(&CompiledBlob)))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: FAILED to retrive result");
        return false;
    }

    const uint32 BlobSize = uint32(CompiledBlob->GetBufferSize());
    Code.Resize(BlobSize);

    LOG_INFO("[CD3D12RHIShaderCompiler]: Compiled Size: " + ToString(BlobSize) + " Bytes");

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

bool CD3D12RHIShaderCompiler::InternalGetReflection(const TComPtr<IDxcBlob>& ShaderBlob, REFIID iid, void** ppvObject)
{
    HRESULT Result = DxReflection->Load(ShaderBlob.Get());
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: Were not able to get reflect of shader");
        return false;
    }

    uint32 PartIndex;
    Result = DxReflection->FindFirstPartKind(DFCC_DXIL, &PartIndex);
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: Were not able to get reflect of shader");
        return false;
    }

    Result = DxReflection->GetPartReflection(PartIndex, iid, ppvObject);
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: Were not able to get reflect of shader");
        return false;
    }

    return true;
}

bool CD3D12RHIShaderCompiler::ValidateRayTracingShader(const TComPtr<IDxcBlob>& ShaderBlob, LPCWSTR Entrypoint)
{
    TComPtr<ID3D12LibraryReflection> LibaryReflection;
    if (!InternalGetReflection(ShaderBlob, IID_PPV_ARGS(&LibaryReflection)))
    {
        return false;
    }

    D3D12_LIBRARY_DESC LibDesc;
    Memory::Memzero(&LibDesc);

    HRESULT Result = LibaryReflection->GetDesc(&LibDesc);
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: Were not able to validate ray tracing shader");
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
        LOG_ERROR("[CD3D12RHIShaderCompiler]: Were not able to validate ray tracing shader");
        return false;
    }

    char Buffer[256];
    Memory::Memzero(Buffer, sizeof(Buffer));

    size_t ConvertedChars;
    wcstombs_s(&ConvertedChars, Buffer, 256, Entrypoint, _TRUNCATE);

    String FuncName(FuncDesc.Name);
    auto result = FuncName.Find(Buffer);
    if (result == String::NPos)
    {
        LOG_ERROR("[CD3D12RHIShaderCompiler]: First exported function does not have correct entrypoint '" + String(Buffer) + "'. Name=" + FuncName);
        return false;
    }

    return true;
}
