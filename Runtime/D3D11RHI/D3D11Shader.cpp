#include "D3D11RHI/D3D11Shader.h"
#include "D3D11RHI/D3D11Device.h"

static D3D_SRV_DIMENSION GetD3D11SRVDimension(EShaderResourceDimension Dimension)
{
    switch (Dimension)
    {
        case EShaderResourceDimension::Buffer:           return D3D_SRV_DIMENSION_BUFFER;
        case EShaderResourceDimension::BufferEx:         return D3D_SRV_DIMENSION_BUFFEREX;
        case EShaderResourceDimension::Texture1D:        return D3D_SRV_DIMENSION_TEXTURE1D;
        case EShaderResourceDimension::Texture1DArray:   return D3D_SRV_DIMENSION_TEXTURE1DARRAY;
        case EShaderResourceDimension::Texture2D:        return D3D_SRV_DIMENSION_TEXTURE2D;
        case EShaderResourceDimension::Texture2DArray:   return D3D_SRV_DIMENSION_TEXTURE2DARRAY;
        case EShaderResourceDimension::Texture2DMS:      return D3D_SRV_DIMENSION_TEXTURE2DMS;
        case EShaderResourceDimension::Texture2DMSArray: return D3D_SRV_DIMENSION_TEXTURE2DMSARRAY;
        case EShaderResourceDimension::Texture3D:        return D3D_SRV_DIMENSION_TEXTURE3D;
        case EShaderResourceDimension::TextureCube:      return D3D_SRV_DIMENSION_TEXTURECUBE;
        case EShaderResourceDimension::TextureCubeArray: return D3D_SRV_DIMENSION_TEXTURECUBEARRAY;
        default:                                         return D3D_SRV_DIMENSION_UNKNOWN;
    }
}

FD3D11Shader::FD3D11Shader(FD3D11Device* InDevice, EShaderVisibility::Type InShaderVisibility)
    : FD3D11DeviceChild(InDevice)
    , ByteCode()
    , ShaderVisibility(InShaderVisibility)
    , BindingInfo()
{
}

FD3D11Shader::~FD3D11Shader() = default;

bool FD3D11Shader::Initialize(const FShaderCodeView& InCode)
{
    const TArrayView<const uint8> NativeCode = InCode.GetNativeCode();
    ByteCode = TArray<uint8>(NativeCode.Data(), NativeCode.Size());

    FD3D11ShaderBindingInfo NewBindingInfo;

    const TArrayView<const FShaderResourceBinding> Bindings = InCode.GetBindings();
    for (int32 Index = 0; Index < Bindings.Size(); ++Index)
    {
        const FShaderResourceBinding& Binding   = Bindings[Index];
        const uint32                  FirstSlot = Binding.Register;
        const uint32                  EndSlot   = FirstSlot + Binding.Count;

        if (Binding.Space == EShaderBindingSpace::ShaderConstants)
        {
            NewBindingInfo.ShaderConstantsSlot = Binding.Register;
            continue;
        }

        switch (GetShaderResourceClass(Binding.Type))
        {
            case EShaderResourceClass::ConstantBuffer:
            {
                NewBindingInfo.AddBinding(ED3D11BindingType::ConstantBuffer, Binding.Register, Binding.Count, InCode.GetBindingName(Index));
                break;
            }

            case EShaderResourceClass::Sampler:
            {
                NewBindingInfo.AddBinding(ED3D11BindingType::Sampler, Binding.Register, Binding.Count, InCode.GetBindingName(Index));
                break;
            }

            case EShaderResourceClass::SRV:
            {
                NewBindingInfo.AddBinding(ED3D11BindingType::SRV, Binding.Register, Binding.Count, InCode.GetBindingName(Index));

                for (uint32 Slot = FirstSlot; Slot < Math::Min<uint32>(EndSlot, D3D11_MAX_SHADER_RESOURCE_VIEWS); ++Slot)
                {
                    NewBindingInfo.ShaderResourceViewDimensions[Slot] = static_cast<uint8>(GetD3D11SRVDimension(Binding.Dimension));
                }

                break;
            }

            case EShaderResourceClass::UAV:
            {
                NewBindingInfo.AddBinding(ED3D11BindingType::UAV, Binding.Register, Binding.Count, InCode.GetBindingName(Index));

                for (uint32 Slot = FirstSlot; Slot < Math::Min<uint32>(EndSlot, D3D11_MAX_UNORDERED_ACCESS_VIEWS); ++Slot)
                {
                    NewBindingInfo.UnorderedAccessViewMask |= uint64(1) << Slot;
                }

                break;
            }
        }
    }

    NewBindingInfo.NumShaderConstants = InCode.GetInfo().ShaderConstantsSize / sizeof(uint32);

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

bool FD3D11VertexShaderRHI::Initialize(const FShaderCodeView& InCode)
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

bool FD3D11HullShaderRHI::Initialize(const FShaderCodeView& InCode)
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

bool FD3D11DomainShaderRHI::Initialize(const FShaderCodeView& InCode)
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

bool FD3D11GeometryShaderRHI::Initialize(const FShaderCodeView& InCode)
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

bool FD3D11PixelShaderRHI::Initialize(const FShaderCodeView& InCode)
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

bool FD3D11ComputeShaderRHI::Initialize(const FShaderCodeView& InCode)
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
