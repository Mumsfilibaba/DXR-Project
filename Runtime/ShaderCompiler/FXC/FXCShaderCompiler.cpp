#if PLATFORM_WINDOWS
#include "Core/Containers/ComPtr.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceManager.h"
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
    , D3DReflectFunc(nullptr)
    , D3DStripShaderFunc(nullptr)
{
}

FFXCShaderCompiler::~FFXCShaderCompiler()
{
    if (D3DCompilerLib)
    {
        FPlatformLibrary::FreeDynamicLib(D3DCompilerLib);
        D3DCompilerLib = nullptr;
    }

    D3DCompileFunc     = nullptr;
    D3DReflectFunc     = nullptr;
    D3DStripShaderFunc = nullptr;
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

    D3DReflectFunc = FPlatformLibrary::LoadSymbol<PFN_FXC_D3D_REFLECT>("D3DReflect", D3DCompilerLib);
    if (!D3DReflectFunc)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to load 'D3DReflect'");
        return false;
    }

    D3DStripShaderFunc = FPlatformLibrary::LoadSymbol<PFN_FXC_D3D_STRIP_SHADER>("D3DStripShader", D3DCompilerLib);
    if (!D3DStripShaderFunc)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to load 'D3DStripShader'");
        return false;
    }

    LOG_INFO("[FShaderCompiler]: Loaded 'd3dcompiler_47'");
    return true;
}

bool FFXCShaderCompiler::SupportsOutputLanguage(EShaderOutputLanguage OutputLanguage) const
{
    return OutputLanguage == EShaderOutputLanguage::DXBC;
}

UINT FFXCShaderCompiler::BuildCompileFlags(const FShaderCompileInfo& CompileInfo)
{
    UINT Flags = D3DCOMPILE_ENABLE_STRICTNESS;
    Flags |= CompileInfo.bOptimize ? D3DCOMPILE_OPTIMIZATION_LEVEL3 : D3DCOMPILE_SKIP_OPTIMIZATION;

    if (CompileInfo.bDebugInfo)
    {
        Flags |= D3DCOMPILE_DEBUG;
    }

    if (CVarFXCWarningsAsErrors.GetValue())
    {
        Flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
    }

    return Flags;
}

// Bump when BuildCompileFlags or GetFXCProfile changes.
static constexpr uint32 GFXCSettingsVersion = 1;

FShaderCompilerIdentity FFXCShaderCompiler::GetIdentity() const
{
    FShaderCompilerIdentity Identity;
    Identity.Name              = GetName();
    Identity.VersionMajor      = 47; // d3dcompiler_47 has no version query
    Identity.SettingsVersion   = GFXCSettingsVersion;
    Identity.TranslatorVersion = FFXCShaderTranslator::Version;
    return Identity;
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
        BuildCompileFlags(CompileInfo),
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

    if (!FDXBCShaderReflector::Reflect(D3DReflectFunc, CodeBlob->GetBufferPointer(), CodeBlob->GetBufferSize(), CompileInfo, OutResult.Reflection, OutResult.Messages))
    {
        return false;
    }

    TComPtr<ID3DBlob> OutputBlob = CodeBlob;
    if (!CompileInfo.bDebugInfo)
    {
        // The signatures stay, D3D11 creates input layouts from the vertex shader bytecode
        constexpr UINT StripFlags = D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS | D3DCOMPILER_STRIP_PRIVATE_DATA;

        TComPtr<ID3DBlob> StrippedBlob;
        if (FAILED(D3DStripShaderFunc(CodeBlob->GetBufferPointer(), CodeBlob->GetBufferSize(), StripFlags, &StrippedBlob)) || !StrippedBlob)
        {
            OutResult.Messages += "D3DStripShader failed\n";
            return false;
        }

        OutputBlob = StrippedBlob;
    }

    const int32 BlobSize = static_cast<int32>(OutputBlob->GetBufferSize());
    OutResult.ByteCode.Resize(BlobSize);
    Memory::Memcpy(OutResult.ByteCode.Data(), OutputBlob->GetBufferPointer(), BlobSize);
    return true;
}

#endif
