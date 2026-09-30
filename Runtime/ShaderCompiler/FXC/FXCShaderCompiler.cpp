#if PLATFORM_WINDOWS
#include "Core/Containers/ComPtr.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformLibrary.h"
#include "ShaderCompiler/FXC/FXCShaderCompiler.h"
#include "ShaderCompiler/FXC/FXCShaderTranslator.h"

static TAutoConsoleVariable<bool> CVarFXCWarningsAsErrors(
    "RHI.ShaderCompiler.FXC.WarningsAsErrors",
    "Treat FXC warnings as errors. FXC warns about implicit truncations that DXC accepts.",
    false);

static const CHAR* GetFXCProfile(EShaderStage Stage)
{
    switch (Stage)
    {
        case EShaderStage::Vertex:   return "vs_5_0";
        case EShaderStage::Hull:     return "hs_5_0";
        case EShaderStage::Domain:   return "ds_5_0";
        case EShaderStage::Geometry: return "gs_5_0";
        case EShaderStage::Pixel:    return "ps_5_0";
        case EShaderStage::Compute:  return "cs_5_0";
        default:                     return nullptr;
    }
}

FFXCShaderCompiler::FFXCShaderCompiler()
    : FShaderCompilerBackend()
    , D3DCompilerLib(nullptr)
    , D3DCompileFunc(nullptr)
{
}

FFXCShaderCompiler::~FFXCShaderCompiler()
{
    if (D3DCompilerLib)
    {
        FPlatformLibrary::FreeDynamicLib(D3DCompilerLib);
        D3DCompilerLib = nullptr;
    }

    D3DCompileFunc = nullptr;
}

bool FFXCShaderCompiler::Initialize()
{
    D3DCompilerLib = FPlatformLibrary::LoadDynamicLib("d3dcompiler_47");
    if (!D3DCompilerLib)
    {
        return false;
    }

    D3DCompileFunc = FPlatformLibrary::LoadSymbol<pD3DCompile>("D3DCompile", D3DCompilerLib);
    if (!D3DCompileFunc)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to load 'D3DCompile'");
        return false;
    }

    LOG_INFO("[FShaderCompiler]: Loaded 'd3dcompiler_47'");
    return true;
}

bool FFXCShaderCompiler::SupportsOutputLanguage(EShaderOutputLanguage OutputLanguage) const
{
    return OutputLanguage == EShaderOutputLanguage::DXBC;
}

UINT FFXCShaderCompiler::BuildCompileFlags(const FShaderCompileInfo& CompileInfo, bool bDebugInfo)
{
    UINT Flags = D3DCOMPILE_ENABLE_STRICTNESS;
    Flags |= CompileInfo.bOptimize ? D3DCOMPILE_OPTIMIZATION_LEVEL3 : D3DCOMPILE_SKIP_OPTIMIZATION;

    if (bDebugInfo)
    {
        Flags |= D3DCOMPILE_DEBUG;
    }

    if (CVarFXCWarningsAsErrors.GetValue())
    {
        Flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
    }

    return Flags;
}

void FFXCShaderCompiler::HashCompileSettings(const FShaderCompileInfo& CompileInfo, const String& IncludeDir, bool bDebugInfo, uint64& InOutHash) const
{
    HashCombine(InOutHash, THash<String>::GetHash(String("d3dcompiler_47")));
    HashCombine(InOutHash, THash<String>::GetHash(IncludeDir));
    HashCombine(InOutHash, static_cast<uint32>(BuildCompileFlags(CompileInfo, bDebugInfo)));

    if (const CHAR* Profile = GetFXCProfile(CompileInfo.ShaderStage))
    {
        HashCombine(InOutHash, THash<String>::GetHash(String(Profile)));
    }
}

bool FFXCShaderCompiler::TranslateSource(FShaderPreprocessorOutput& InOutSource) const
{
    return FFXCShaderTranslator::Translate(InOutSource);
}

bool FFXCShaderCompiler::Compile(const FShaderCompileRequest& Request, FShaderCompileResult& OutResult)
{
    const FShaderCompileInfo& CompileInfo = *Request.CompileInfo;

    const CHAR* Profile = GetFXCProfile(CompileInfo.ShaderStage);
    if (!Profile)
    {
        OutResult.Messages = String::Printf("Stage %s cannot be compiled to DXBC", ToString(CompileInfo.ShaderStage));
        return false;
    }

    // Requests up to SM 6.2 are lowered to 5.0, anything newer relies on features that D3D11 does not have
    if (CompileInfo.ShaderModel > EShaderModel::SM_6_2)
    {
        OutResult.Messages = String::Printf("Shader Model %s cannot be compiled to DXBC", ToString(CompileInfo.ShaderModel));
        return false;
    }

    // The source is already preprocessed, so there are no defines or includes to pass
    TComPtr<ID3DBlob> CodeBlob;
    TComPtr<ID3DBlob> ErrorBlob;

    const HRESULT Result = D3DCompileFunc(
        Request.Source.Data(),
        static_cast<SIZE_T>(Request.Source.Size()),
        Request.FilePath.IsEmpty() ? nullptr : *Request.FilePath,
        nullptr,
        nullptr,
        *CompileInfo.EntryPoint,
        Profile,
        BuildCompileFlags(CompileInfo, Request.bDebugInfo),
        0,
        &CodeBlob,
        &ErrorBlob);

    if (ErrorBlob && ErrorBlob->GetBufferSize() > 0)
    {
        const CHAR* Messages = reinterpret_cast<const CHAR*>(ErrorBlob->GetBufferPointer());

        int32 Length = static_cast<int32>(ErrorBlob->GetBufferSize());
        while (Length > 0 && Messages[Length - 1] == '\0')
        {
            --Length;
        }

        OutResult.Messages = String(Messages, Length);
    }

    if (FAILED(Result) || !CodeBlob)
    {
        return false;
    }

    const int32 BlobSize = static_cast<int32>(CodeBlob->GetBufferSize());
    OutResult.ByteCode.Resize(BlobSize);
    Memory::Memcpy(OutResult.ByteCode.Data(), CodeBlob->GetBufferPointer(), BlobSize);
    return true;
}

#endif
