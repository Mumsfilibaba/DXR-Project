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
// CShaderCompiler

void*                 CShaderCompiler::DXCLibrary            = nullptr;
DxcCreateInstanceProc CShaderCompiler::DxcCreateInstanceFunc = nullptr;

String                CShaderCompiler::AssetFolderPath;

bool CShaderCompiler::Initialize(const char* InAssetFolderPath)
{
    DXCLibrary = PlatformLibrary::LoadDynamicLib("dxcompiler");
    if (!DXCLibrary)
    {
        LOG_ERROR("Failed to load 'dxcompiler'");
        return false;
    }
        
    DxcCreateInstanceFunc = PlatformLibrary::LoadSymbolAddress<DxcCreateInstanceProc>("DxcCreateInstance", DXCLibrary);
    if (!DxcCreateInstanceFunc)
    {
        LOG_ERROR("Failed to load 'DxcCreateInstance'");
        return false;
    }
    
    AssetFolderPath = String(InAssetFolderPath);
    return true;
}

void CShaderCompiler::Release()
{
    if (DXCLibrary)
    {
        PlatformLibrary::FreeDynamicLib(DXCLibrary);
        DXCLibrary = nullptr;
    }
    
    DxcCreateInstanceFunc = nullptr;
}

bool CShaderCompiler::CompileFromFile(const String& Filename, const CShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    OutByteCode.Clear();

    // Use the assetfolder as base for the shaderfiles
    WString WideFilePath   = CharToWide(AssetFolderPath + '/' + Filename);
    WString WideEntrypoint = CharToWide(CompileInfo.EntryPoint);

    TComPtr<IDxcLibrary> Library;

    HRESULT Result = DxcCreateInstanceFunc(CLSID_DxcLibrary, IID_PPV_ARGS(&Library));
    if (FAILED(Result))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to create DxLibrary");
        return false;
    }

    TComPtr<IDxcBlobEncoding> SourceBlob;
    Result = Library->CreateBlobFromFile(WideFilePath.CStr(), nullptr, &SourceBlob);
    if (FAILED(Result))
    {
        LOG_ERROR("[CShaderCompiler]: FAILED to create Source Data");
        CDebug::DebugBreak();
        return false;
    }
    
    return true;
}

bool CShaderCompiler::CompileFromSource(const String& ShaderSource, const CShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode)
{
    return true;
}
