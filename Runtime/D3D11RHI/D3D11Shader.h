#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "D3D11RHI/D3D11DeviceChild.h"
#include <d3d11shader.h>

typedef TSharedRef<class FD3D11VertexShaderRHI>   FD3D11VertexShaderRHIRef;
typedef TSharedRef<class FD3D11HullShaderRHI>     FD3D11HullShaderRHIRef;
typedef TSharedRef<class FD3D11DomainShaderRHI>   FD3D11DomainShaderRHIRef;
typedef TSharedRef<class FD3D11GeometryShaderRHI> FD3D11GeometryShaderRHIRef;
typedef TSharedRef<class FD3D11PixelShaderRHI>    FD3D11PixelShaderRHIRef;
typedef TSharedRef<class FD3D11ComputeShaderRHI>  FD3D11ComputeShaderRHIRef;

struct EShaderVisibility
{
    enum Type : int32
    {
        Vertex = 0,
        Hull,
        Domain,
        Geometry,
        Pixel,
        Compute,
        Count = Compute + 1,
    };
};

NODISCARD constexpr EShaderVisibility::Type GetShaderVisibility(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::Vertex:   return EShaderVisibility::Vertex;
        case EShaderStage::Hull:     return EShaderVisibility::Hull;
        case EShaderStage::Domain:   return EShaderVisibility::Domain;
        case EShaderStage::Geometry: return EShaderVisibility::Geometry;
        case EShaderStage::Pixel:    return EShaderVisibility::Pixel;
        default:                     return EShaderVisibility::Compute;
    }
}

enum class ED3D11BindingType : uint8
{
    ConstantBuffer = 0,
    SRV,
    UAV,
    Sampler,
    Count,
};

struct FD3D11ShaderBindingInfo
{
    struct FResourceBinding
    {
        ED3D11BindingType BindingType;
        uint16            Register;
        uint16            Count;
        String            DebugName;
    };

    void AddBinding(ED3D11BindingType InType, uint16 InRegister, uint16 InCount, const String& InDebugName)
    {
        FResourceBinding& Binding = ResourceBindings.Emplace();
        Binding.BindingType = InType;
        Binding.Register    = InRegister;
        Binding.Count       = InCount;
        Binding.DebugName   = InDebugName;
    }

    TArray<FResourceBinding> ResourceBindings;

    /** The slot FXC gave the Constants_CB cbuffer, or -1 when the shader has no shader constants */
    int32 ShaderConstantsSlot = -1;

    /** Size of the shader constants in 32-bit values */
    uint32 NumShaderConstants = 0;
};

class FD3D11Shader : public FD3D11DeviceChild
{
public:
    FD3D11Shader(FD3D11Device* InDevice, EShaderVisibility::Type InShaderVisibility);
    virtual ~FD3D11Shader();

    virtual bool Initialize(const TArray<uint8>& InCode);

    FORCEINLINE const TArray<uint8>& GetByteCode() const
    {
        return ByteCode;
    }

    FORCEINLINE const FD3D11ShaderBindingInfo& GetBindingInfo() const
    {
        return BindingInfo;
    }

    FORCEINLINE EShaderVisibility::Type GetShaderVisibility() const
    {
        return ShaderVisibility;
    }

protected:
    bool GetShaderResourceBindings(ID3D11ShaderReflection* Reflection, uint32 NumBoundResources);

    TArray<uint8>           ByteCode;
    EShaderVisibility::Type ShaderVisibility;
    FD3D11ShaderBindingInfo BindingInfo;
};

class FD3D11VertexShaderRHI : public FRHIVertexShader, public FD3D11Shader
{
public:
    FD3D11VertexShaderRHI(FD3D11Device* InDevice);
    virtual ~FD3D11VertexShaderRHI();

    // FD3D11Shader Interface
    virtual bool Initialize(const TArray<uint8>& InCode) override final;

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return Shader.Get(); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D11Shader*>(this); }

    FORCEINLINE ID3D11VertexShader* GetD3D11Shader() const
    {
        return Shader.Get();
    }

private:
    TComPtr<ID3D11VertexShader> Shader;
};

class FD3D11HullShaderRHI : public FRHIHullShader, public FD3D11Shader
{
public:
    FD3D11HullShaderRHI(FD3D11Device* InDevice);
    virtual ~FD3D11HullShaderRHI();

    // FD3D11Shader Interface
    virtual bool Initialize(const TArray<uint8>& InCode) override final;

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return Shader.Get(); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D11Shader*>(this); }

    FORCEINLINE ID3D11HullShader* GetD3D11Shader() const
    {
        return Shader.Get();
    }

private:
    TComPtr<ID3D11HullShader> Shader;
};

class FD3D11DomainShaderRHI : public FRHIDomainShader, public FD3D11Shader
{
public:
    FD3D11DomainShaderRHI(FD3D11Device* InDevice);
    virtual ~FD3D11DomainShaderRHI();

    // FD3D11Shader Interface
    virtual bool Initialize(const TArray<uint8>& InCode) override final;

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return Shader.Get(); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D11Shader*>(this); }

    FORCEINLINE ID3D11DomainShader* GetD3D11Shader() const
    {
        return Shader.Get();
    }

private:
    TComPtr<ID3D11DomainShader> Shader;
};

class FD3D11GeometryShaderRHI : public FRHIGeometryShader, public FD3D11Shader
{
public:
    FD3D11GeometryShaderRHI(FD3D11Device* InDevice);
    virtual ~FD3D11GeometryShaderRHI();

    // FD3D11Shader Interface
    virtual bool Initialize(const TArray<uint8>& InCode) override final;

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return Shader.Get(); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D11Shader*>(this); }

    FORCEINLINE ID3D11GeometryShader* GetD3D11Shader() const
    {
        return Shader.Get();
    }

private:
    TComPtr<ID3D11GeometryShader> Shader;
};

class FD3D11PixelShaderRHI : public FRHIPixelShader, public FD3D11Shader
{
public:
    FD3D11PixelShaderRHI(FD3D11Device* InDevice);
    virtual ~FD3D11PixelShaderRHI();

    // FD3D11Shader Interface
    virtual bool Initialize(const TArray<uint8>& InCode) override final;

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return Shader.Get(); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D11Shader*>(this); }

    FORCEINLINE ID3D11PixelShader* GetD3D11Shader() const
    {
        return Shader.Get();
    }

private:
    TComPtr<ID3D11PixelShader> Shader;
};

class FD3D11ComputeShaderRHI : public FRHIComputeShader, public FD3D11Shader
{
public:
    FD3D11ComputeShaderRHI(FD3D11Device* InDevice);
    virtual ~FD3D11ComputeShaderRHI();

    // FD3D11Shader Interface
    virtual bool Initialize(const TArray<uint8>& InCode) override final;

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return Shader.Get(); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D11Shader*>(this); }

    FORCEINLINE ID3D11ComputeShader* GetD3D11Shader() const
    {
        return Shader.Get();
    }

private:
    TComPtr<ID3D11ComputeShader> Shader;
};

NODISCARD inline FD3D11Shader* GetD3D11Shader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FD3D11Shader*>(Shader->GetRHIBaseInterface()) : nullptr;
}
