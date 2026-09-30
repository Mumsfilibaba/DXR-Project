#include "D3D11RHI/D3D11Shader.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11Loader.h"

static bool IsShaderResourceView(D3D_SHADER_INPUT_TYPE Type)
{
    return Type == D3D_SIT_TEXTURE || Type == D3D_SIT_TBUFFER || Type == D3D_SIT_BYTEADDRESS || Type == D3D_SIT_STRUCTURED;
}

static bool IsUnorderedAccessView(D3D_SHADER_INPUT_TYPE Type)
{
    return Type == D3D_SIT_UAV_RWTYPED || Type == D3D_SIT_UAV_RWBYTEADDRESS || Type == D3D_SIT_UAV_RWSTRUCTURED ||
        Type == D3D_SIT_UAV_APPEND_STRUCTURED || Type == D3D_SIT_UAV_CONSUME_STRUCTURED || Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER;
}

FD3D11Shader::FD3D11Shader(FD3D11Device* InDevice, EShaderVisibility::Type InShaderVisibility)
    : FD3D11DeviceChild(InDevice)
    , ByteCode()
    , ShaderVisibility(InShaderVisibility)
    , BindingInfo()
{
}

FD3D11Shader::~FD3D11Shader() = default;

bool FD3D11Shader::Initialize(const TArray<uint8>& InCode)
{
    ByteCode = InCode;

    TComPtr<ID3D11ShaderReflection> Reflection;
    if (FAILED(D3D11::D3DReflect(ByteCode.Data(), static_cast<SIZE_T>(ByteCode.Size()), IID_PPV_ARGS(&Reflection))))
    {
        D3D11_ERROR_CRITICAL("[FD3D11Shader]: FAILED to reflect the shader, D3D11RHI needs DXBC bytecode");
        return false;
    }

    D3D11_SHADER_DESC ShaderDesc = {};
    if (FAILED(Reflection->GetDesc(&ShaderDesc)))
    {
        D3D11_ERROR_CRITICAL("[FD3D11Shader]: FAILED to retrieve the shader description");
        return false;
    }

    return GetShaderResourceBindings(Reflection.Get(), ShaderDesc.BoundResources);
}

bool FD3D11Shader::GetShaderResourceBindings(ID3D11ShaderReflection* Reflection, uint32 NumBoundResources)
{
    FD3D11ShaderBindingInfo NewBindingInfo;

    for (uint32 Index = 0; Index < NumBoundResources; ++Index)
    {
        D3D11_SHADER_INPUT_BIND_DESC ShaderBindDesc = {};
        if (FAILED(Reflection->GetResourceBindingDesc(Index, &ShaderBindDesc)))
        {
            continue;
        }

        const uint16 Register = static_cast<uint16>(ShaderBindDesc.BindPoint);
        const uint16 Count    = static_cast<uint16>(Math::Max<UINT>(ShaderBindDesc.BindCount, 1));

        if (ShaderBindDesc.Type == D3D_SIT_CBUFFER)
        {
            if (CString::Strcmp(ShaderBindDesc.Name, D3D11_SHADER_CONSTANTS_CBUFFER_NAME) != 0)
            {
                NewBindingInfo.AddBinding(ED3D11BindingType::ConstantBuffer, Register, Count, ShaderBindDesc.Name);
                continue;
            }

            uint32 SizeInBytes = 0;
            if (ID3D11ShaderReflectionConstantBuffer* BufferVar = Reflection->GetConstantBufferByName(ShaderBindDesc.Name))
            {
                D3D11_SHADER_BUFFER_DESC BufferDesc = {};
                if (SUCCEEDED(BufferVar->GetDesc(&BufferDesc)))
                {
                    SizeInBytes = BufferDesc.Size;
                }
            }

            constexpr uint32 MaxSizeInBytes = D3D11_MAX_32BIT_SHADER_CONSTANTS_COUNT * sizeof(uint32);
            if (SizeInBytes > MaxSizeInBytes)
            {
                D3D11_ERROR_CRITICAL("[FD3D11Shader]: The shader constants are %u bytes, only %u bytes are supported", SizeInBytes, MaxSizeInBytes);
                return false;
            }

            NewBindingInfo.ShaderConstantsSlot = ShaderBindDesc.BindPoint;
            NewBindingInfo.NumShaderConstants  = SizeInBytes / sizeof(uint32);
        }
        else if (ShaderBindDesc.Type == D3D_SIT_SAMPLER)
        {
            NewBindingInfo.AddBinding(ED3D11BindingType::Sampler, Register, Count, ShaderBindDesc.Name);
        }
        else if (IsShaderResourceView(ShaderBindDesc.Type))
        {
            NewBindingInfo.AddBinding(ED3D11BindingType::SRV, Register, Count, ShaderBindDesc.Name);
        }
        else if (IsUnorderedAccessView(ShaderBindDesc.Type))
        {
            NewBindingInfo.AddBinding(ED3D11BindingType::UAV, Register, Count, ShaderBindDesc.Name);
        }
        else
        {
            D3D11_ERROR_CRITICAL("[FD3D11Shader]: Unhandled shader resource type '%u' for parameter '%s' at register %u", ShaderBindDesc.Type, ShaderBindDesc.Name, ShaderBindDesc.BindPoint);
            return false;
        }
    }

    BindingInfo = ::Move(NewBindingInfo);
    return true;
}

FD3D11VertexShaderRHI::FD3D11VertexShaderRHI(FD3D11Device* InDevice)
    : FRHIVertexShader()
    , FD3D11Shader(InDevice, EShaderVisibility::Vertex)
    , Shader(nullptr)
{
}

FD3D11VertexShaderRHI::~FD3D11VertexShaderRHI() = default;

bool FD3D11VertexShaderRHI::Initialize(const TArray<uint8>& InCode)
{
    if (!FD3D11Shader::Initialize(InCode))
    {
        return false;
    }

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateVertexShader(ByteCode.Data(), ByteCode.Size(), nullptr, &Shader);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11VertexShaderRHI]: FAILED to create VertexShader (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11HullShaderRHI::FD3D11HullShaderRHI(FD3D11Device* InDevice)
    : FRHIHullShader()
    , FD3D11Shader(InDevice, EShaderVisibility::Hull)
    , Shader(nullptr)
{
}

FD3D11HullShaderRHI::~FD3D11HullShaderRHI() = default;

bool FD3D11HullShaderRHI::Initialize(const TArray<uint8>& InCode)
{
    if (!FD3D11Shader::Initialize(InCode))
    {
        return false;
    }

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateHullShader(ByteCode.Data(), ByteCode.Size(), nullptr, &Shader);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11HullShaderRHI]: FAILED to create HullShader (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11DomainShaderRHI::FD3D11DomainShaderRHI(FD3D11Device* InDevice)
    : FRHIDomainShader()
    , FD3D11Shader(InDevice, EShaderVisibility::Domain)
    , Shader(nullptr)
{
}

FD3D11DomainShaderRHI::~FD3D11DomainShaderRHI() = default;

bool FD3D11DomainShaderRHI::Initialize(const TArray<uint8>& InCode)
{
    if (!FD3D11Shader::Initialize(InCode))
    {
        return false;
    }

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateDomainShader(ByteCode.Data(), ByteCode.Size(), nullptr, &Shader);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11DomainShaderRHI]: FAILED to create DomainShader (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11GeometryShaderRHI::FD3D11GeometryShaderRHI(FD3D11Device* InDevice)
    : FRHIGeometryShader()
    , FD3D11Shader(InDevice, EShaderVisibility::Geometry)
    , Shader(nullptr)
{
}

FD3D11GeometryShaderRHI::~FD3D11GeometryShaderRHI() = default;

bool FD3D11GeometryShaderRHI::Initialize(const TArray<uint8>& InCode)
{
    if (!FD3D11Shader::Initialize(InCode))
    {
        return false;
    }

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateGeometryShader(ByteCode.Data(), ByteCode.Size(), nullptr, &Shader);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11GeometryShaderRHI]: FAILED to create GeometryShader (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11PixelShaderRHI::FD3D11PixelShaderRHI(FD3D11Device* InDevice)
    : FRHIPixelShader()
    , FD3D11Shader(InDevice, EShaderVisibility::Pixel)
    , Shader(nullptr)
{
}

FD3D11PixelShaderRHI::~FD3D11PixelShaderRHI() = default;

bool FD3D11PixelShaderRHI::Initialize(const TArray<uint8>& InCode)
{
    if (!FD3D11Shader::Initialize(InCode))
    {
        return false;
    }

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreatePixelShader(ByteCode.Data(), ByteCode.Size(), nullptr, &Shader);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11PixelShaderRHI]: FAILED to create PixelShader (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11ComputeShaderRHI::FD3D11ComputeShaderRHI(FD3D11Device* InDevice)
    : FRHIComputeShader()
    , FD3D11Shader(InDevice, EShaderVisibility::Compute)
    , Shader(nullptr)
{
}

FD3D11ComputeShaderRHI::~FD3D11ComputeShaderRHI() = default;

bool FD3D11ComputeShaderRHI::Initialize(const TArray<uint8>& InCode)
{
    if (!FD3D11Shader::Initialize(InCode))
    {
        return false;
    }

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateComputeShader(ByteCode.Data(), ByteCode.Size(), nullptr, &Shader);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11ComputeShaderRHI]: FAILED to create ComputeShader (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}
