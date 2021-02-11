#include "D3D12ShaderCompiler.h"

#include "Utilities/StringUtilities.h"

#include "Windows/Windows.h"
#include "Windows/Windows.inl"

#include "Application/Platform/PlatformDialogMisc.h"

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

D3D12ShaderCompiler::D3D12ShaderCompiler()
    : IShaderCompiler()
    , DxCompiler(nullptr)
    , DxLibrary(nullptr)
    , DxLinker(nullptr)
    , DxIncludeHandler(nullptr)
    , DxCompilerDLL()
{
}

D3D12ShaderCompiler::~D3D12ShaderCompiler()
{
    DxCompiler.Reset();
    DxLibrary.Reset();
    DxLinker.Reset();
    DxIncludeHandler.Reset();

    ::FreeLibrary(DxCompilerDLL);
}

Bool D3D12ShaderCompiler::CompileFromFile(
    const std::string& FilePath, 
    const std::string& EntryPoint, 
    const TArray<ShaderDefine>* Defines,
    EShaderStage ShaderStage, 
    EShaderModel ShaderModel, 
    TArray<UInt8>& Code) const
{
    std::wstring WideFilePath   = ConvertToWide(FilePath);
    std::wstring WideEntrypoint = ConvertToWide(EntryPoint);
    LPCWSTR TargetProfile       = GetTargetProfile(ShaderStage, ShaderModel);

    TComPtr<IDxcBlobEncoding> SourceBlob;
    HRESULT Result = DxLibrary->CreateBlobFromFile(WideFilePath.c_str(), nullptr, &SourceBlob);
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create Source Data");
        Debug::DebugBreak();

        return nullptr;
    }

    return InternalCompileFromSource(SourceBlob.Get(), WideFilePath.c_str(), WideEntrypoint.c_str(), TargetProfile, Defines, Code);
}

Bool D3D12ShaderCompiler::CompileShader(
    const std::string& ShaderSource, 
    const std::string& EntryPoint, 
    const TArray<ShaderDefine>* Defines,
    EShaderStage ShaderStage, 
    EShaderModel ShaderModel, 
    TArray<UInt8>& Code) const
{
    std::wstring WideEntrypoint = ConvertToWide(EntryPoint);
    LPCWSTR TargetProfile       = GetTargetProfile(ShaderStage, ShaderModel);

    TComPtr<IDxcBlobEncoding> SourceBlob;
    HRESULT Result = DxLibrary->CreateBlobWithEncodingOnHeapCopy(
        ShaderSource.c_str(), 
        sizeof(Char) * static_cast<UInt32>(ShaderSource.size()), 
        CP_UTF8, 
        &SourceBlob);
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create Source Data");
        Debug::DebugBreak();

        return nullptr;
    }

    return InternalCompileFromSource(SourceBlob.Get(), nullptr, WideEntrypoint.c_str(), TargetProfile, Defines, Code);
}

Bool D3D12ShaderCompiler::Init()
{
    DxCompilerDLL = ::LoadLibrary("dxcompiler.dll");
    if (!DxCompilerDLL)
    {
        PlatformDialogMisc::MessageBox("ERROR", "FAILED to load dxcompiler.dll");
        return false;
    }

    DxcCreateInstanceProc DxcCreateInstance_ = GetTypedProcAddress<DxcCreateInstanceProc>(DxCompilerDLL, "DxcCreateInstance");
    if (!DxcCreateInstance_)
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to load DxcCreateInstance");
        return false;
    }
    
    HRESULT Result = DxcCreateInstance_(CLSID_DxcCompiler, IID_PPV_ARGS(&DxCompiler));
    if (SUCCEEDED(Result))
    {
        Result = DxcCreateInstance_(CLSID_DxcLibrary, IID_PPV_ARGS(&DxLibrary));
        if (SUCCEEDED(Result))
        {
            Result = DxLibrary->CreateIncludeHandler(&DxIncludeHandler);
            if (SUCCEEDED(Result))
            {
                Result = DxcCreateInstance_(CLSID_DxcLinker, IID_PPV_ARGS(&DxLinker));
                if (SUCCEEDED(Result))
                {
                    return true;
                }
                else
                {
                    LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxLinker");
                    return false;
                }
            }
            else
            {
                LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxIncludeHandler");
                return false;
            }
        }
        else
        {
            LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxLibrary");
            return false;
        }
    }
    else
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxCompiler");
        return false;
    }
}

Bool D3D12ShaderCompiler::InternalCompileFromSource(
    IDxcBlob* SourceBlob, 
    LPCWSTR FilePath, 
    LPCWSTR Entrypoint, 
    LPCWSTR TargetProfile, 
    const TArray<ShaderDefine>* Defines,
    TArray<UInt8>& Code) const
{
    LPCWSTR Args[] =
    {
        L"-O3", // Optimization level 3
    };

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

    TComPtr<IDxcOperationResult> Result;
    HRESULT hResult = DxCompiler->Compile(
        SourceBlob, 
        FilePath, 
        Entrypoint, 
        TargetProfile, 
        Args, 1, 
        DxDefines.Data(),
        DxDefines.Size(),
        DxIncludeHandler.Get(), 
        &Result);
    if (FAILED(hResult))
    {
        LOG_ERROR("[D3D12ShaderCompiler]: FAILED to Compile");
        Debug::DebugBreak();

        return nullptr;
    }

    if (SUCCEEDED(Result->GetStatus(&hResult)))
    {
        TComPtr<IDxcBlobEncoding> PrintBlob;
        TComPtr<IDxcBlobEncoding> PrintBlob8;
        if (SUCCEEDED(Result->GetErrorBuffer(&PrintBlob)))
        {
            DxLibrary->GetBlobAsUtf8(PrintBlob.Get(), &PrintBlob8);
        }

        if (SUCCEEDED(hResult))
        {
            if (PrintBlob8->GetBufferSize() > 0)
            {
                LOG_INFO("[D3D12ShaderCompiler]: Compiled with the following output:");
                LOG_INFO(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
            }

            TComPtr<IDxcBlob> CompiledBlob;
            if (SUCCEEDED(Result->GetResult(&CompiledBlob)))
            {
                // Copy data to resulting bytecode
                const UInt32 BlobSize = UInt32(CompiledBlob->GetBufferSize());
                Code.Resize(BlobSize);
                Memory::Memcpy(Code.Data(), CompiledBlob->GetBufferPointer(), BlobSize);

                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            LOG_ERROR("[D3D12ShaderCompiler]: FAILED to compile with the following error:");
            LOG_ERROR(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));

            return false;
        }
    }
    else
    {
        return false;
    }
}