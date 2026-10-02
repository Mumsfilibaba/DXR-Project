#include "MetalRHI/MetalShader.h"
#include "Core/Memory/Memory.h"

FMetalShader::FMetalShader(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , CompiledShader()
    , ThreadGroupSizeX(0)
    , ThreadGroupSizeY(0)
    , ThreadGroupSizeZ(0)
    , ShaderConstantsSize(0)
{
}

FMetalShader::~FMetalShader() = default;

bool FMetalShader::Initialize(const TArray<uint8>& InCode)
{
    TArrayView<const uint8> Source;

    if (!ParseMSLShaderByteCode(InCode, Bindings, Source))
    {
        LOG_ERROR("Shader bytecode is not a valid MSL blob");
        return false;
    }

    if (InCode.Size() >= static_cast<int32>(sizeof(FMSLShaderHeader)))
    {
        FMSLShaderHeader Header;
        Memory::Memcpy(&Header, InCode.Data(), sizeof(FMSLShaderHeader));

        if (Header.Magic == FMSLShaderHeader::ExpectedMagic && Header.Version == FMSLShaderHeader::ExpectedVersion)
        {
            ThreadGroupSizeX    = Header.ThreadGroupSizeX;
            ThreadGroupSizeY    = Header.ThreadGroupSizeY;
            ThreadGroupSizeZ    = Header.ThreadGroupSizeZ;
            ShaderConstantsSize = Header.ShaderConstantsSize;
        }
    }

    CompiledShader = GetDevice()->GetShaderLibraryCache().GetOrCompile(Source);
    return CompiledShader != nullptr;
}

FMetalRayTracingShader::FMetalRayTracingShader(FMetalDevice* InDevice)
    : FMetalShader(InDevice)
{
}

FMetalRayTracingShader::~FMetalRayTracingShader() = default;

bool FMetalRayTracingShader::Initialize(const TArray<uint8>& InCode)
{
    if (!FMetalShader::Initialize(InCode))
    {
        return false;
    }

    Identifier = String(CompiledShader->FunctionName);
    return true;
}
