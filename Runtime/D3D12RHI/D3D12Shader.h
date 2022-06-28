#pragma once
#include "D3D12Device.h"
#include "D3D12DeviceChild.h"
#include "D3D12Constants.h"

#include "RHI/RHIResources.h"

#include <d3d12shader.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShaderVisibility

enum EShaderVisibility
{
    ShaderVisibility_All      = 0,
    ShaderVisibility_Vertex   = 1,
    ShaderVisibility_Hull     = 2,
    ShaderVisibility_Domain   = 3,
    ShaderVisibility_Geometry = 4,
    ShaderVisibility_Pixel    = 5,
    ShaderVisibility_Count    = ShaderVisibility_Pixel + 1
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EResourceType

enum EResourceType
{
    ResourceType_CBV     = 0,
    ResourceType_SRV     = 1,
    ResourceType_UAV     = 2,
    ResourceType_Sampler = 3,
    ResourceType_Count   = ResourceType_Sampler + 1,
    ResourceType_Unknown = 5,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SShaderResourceRange

struct SShaderResourceRange
{
    uint32 NumCBVs     = 0;
    uint32 NumSRVs     = 0;
    uint32 NumUAVs     = 0;
    uint32 NumSamplers = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SShaderResourceCount

struct SShaderResourceCount
{
    void Combine(const SShaderResourceCount& Other);
    bool IsCompatible(const SShaderResourceCount& Other) const;

    SShaderResourceRange Ranges;
    uint32               Num32BitConstants = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SD3D12ShaderParameter

struct SD3D12ShaderParameter
{
    SD3D12ShaderParameter() = default;

    FORCEINLINE SD3D12ShaderParameter(const String& InName, uint32 InRegister, uint32 InSpace, uint32 InNumDescriptors, uint32 InSizeInBytes)
        : Name(InName)
        , Register(InRegister)
        , Space(InSpace)
        , NumDescriptors(InNumDescriptors)
        , SizeInBytes(InSizeInBytes)
    { }

    String Name;
    uint32 Register       = 0;
    uint32 Space          = 0;
    uint32 NumDescriptors = 0;
    uint32 SizeInBytes    = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Shader

class CD3D12Shader : public CD3D12DeviceChild
{
public:
    CD3D12Shader(FD3D12Device* InDevice, const TArray<uint8>& InCode, EShaderVisibility ShaderVisibility);
    ~CD3D12Shader();

public:

    static bool GetShaderReflection(class CD3D12Shader* Shader);

    EShaderVisibility GetShaderVisibility() const { return Visibility; }

    const SShaderResourceCount& GetResourceCount() const { return ResourceCount; }

    const SShaderResourceCount& GetRTLocalResourceCount() const { return RTLocalResourceCount; }

    D3D12_SHADER_BYTECODE GetByteCode() const { return ByteCode; }

    FORCEINLINE bool HasRootSignature() const { return bContainsRootSignature; }
    
    FORCEINLINE const void* GetCode() const { return ByteCode.pShaderBytecode; }

    FORCEINLINE uint64 GetCodeSize() const { return static_cast<uint64>(ByteCode.BytecodeLength); }

    FORCEINLINE SD3D12ShaderParameter GetConstantBufferParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR_COND(ParameterIndex < static_cast<uint32>(ConstantBufferParameters.Size())
                        ,"Trying to access ParameterIndex=%u, but the shader only has %u slots", ParameterIndex, ConstantBufferParameters.Size());
        return ConstantBufferParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumConstantBufferParameters() { return ConstantBufferParameters.Size(); }

    FORCEINLINE SD3D12ShaderParameter GetShaderResourceParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR_COND(ParameterIndex < static_cast<uint32>(ShaderResourceParameters.Size())
                        ,"Trying to access ParameterIndex=%u, but the shader only has %u slots", ParameterIndex, ShaderResourceParameters.Size());
        return ShaderResourceParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumShaderResourceParameters() { return ShaderResourceParameters.Size(); }

    FORCEINLINE SD3D12ShaderParameter GetUnorderedAccessParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR_COND(ParameterIndex < static_cast<uint32>(UnorderedAccessParameters.Size())
                        ,"Trying to access ParameterIndex=%u, but the shader only has %u slots", ParameterIndex, UnorderedAccessParameters.Size());
        return UnorderedAccessParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumUnorderedAccessParameters() { return UnorderedAccessParameters.Size(); }

    FORCEINLINE SD3D12ShaderParameter GetSamplerStateParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR_COND( ParameterIndex < static_cast<uint32>(SamplerParameters.Size())
                         ,"Trying to access ParameterIndex=%u, but the shader only has %u slots", ParameterIndex, SamplerParameters.Size());
        return SamplerParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumSamplerStateParameters() { return SamplerParameters.Size(); }

protected:

    template<typename TD3D12ReflectionInterface>
    static bool GetShaderResourceBindings(TD3D12ReflectionInterface* Reflection, CD3D12Shader* Shader, uint32 NumBoundResources);

    D3D12_SHADER_BYTECODE         ByteCode;

    TArray<SD3D12ShaderParameter> ConstantBufferParameters;
    TArray<SD3D12ShaderParameter> ShaderResourceParameters;
    TArray<SD3D12ShaderParameter> UnorderedAccessParameters;
    TArray<SD3D12ShaderParameter> SamplerParameters;

    EShaderVisibility             Visibility;
    SShaderResourceCount          ResourceCount;
    SShaderResourceCount          RTLocalResourceCount;

    bool bContainsRootSignature = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12VertexShader

class CD3D12VertexShader : public CRHIVertexShader, public CD3D12Shader
{
public:

    CD3D12VertexShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIVertexShader()
        , CD3D12Shader(InDevice, InCode, ShaderVisibility_Vertex)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }

    virtual void* GetRHIBaseShader() override final
    {
        CD3D12Shader* D3D12Shader = static_cast<CD3D12Shader*>(this);
        return reinterpret_cast<void*>(D3D12Shader);
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12PixelShader

class CD3D12PixelShader : public CRHIPixelShader, public CD3D12Shader
{
public:

    CD3D12PixelShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIPixelShader()
        , CD3D12Shader(InDevice, InCode, ShaderVisibility_Pixel)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }

    virtual void* GetRHIBaseShader() override final
    {
        CD3D12Shader* D3D12Shader = static_cast<CD3D12Shader*>(this);
        return reinterpret_cast<void*>(D3D12Shader);
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayTracingShader

class CD3D12RayTracingShader : public CD3D12Shader
{
public:
    CD3D12RayTracingShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CD3D12Shader(InDevice, InCode, ShaderVisibility_All)
    { }

public:

    static bool GetRayTracingShaderReflection(class CD3D12RayTracingShader* Shader);
    
    FORCEINLINE const String& GetIdentifier() const { return Identifier; }

protected:
    String Identifier;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayGenShader

class CD3D12RayGenShader : public CRHIRayGenShader, public CD3D12RayTracingShader
{
public:

    CD3D12RayGenShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayGenShader()
        , CD3D12RayTracingShader(InDevice, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }

    virtual void* GetRHIBaseShader() override final
    {
        CD3D12RayTracingShader* D3D12Shader = static_cast<CD3D12RayTracingShader*>(this);
        return reinterpret_cast<void*>(D3D12Shader);
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayAnyHitShader

class CD3D12RayAnyHitShader : public CRHIRayAnyHitShader, public CD3D12RayTracingShader
{
public:
    
    CD3D12RayAnyHitShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayAnyHitShader()
        , CD3D12RayTracingShader(InDevice, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }

    virtual void* GetRHIBaseShader() override final
    {
        CD3D12RayTracingShader* D3D12Shader = static_cast<CD3D12RayTracingShader*>(this);
        return reinterpret_cast<void*>(D3D12Shader);
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayClosestHitShader

class CD3D12RayClosestHitShader : public CRHIRayClosestHitShader, public CD3D12RayTracingShader
{
public:
    
    CD3D12RayClosestHitShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayClosestHitShader()
        , CD3D12RayTracingShader(InDevice, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }

    virtual void* GetRHIBaseShader() override final
    {
        CD3D12RayTracingShader* D3D12Shader = static_cast<CD3D12RayTracingShader*>(this);
        return reinterpret_cast<void*>(D3D12Shader);
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayMissShader

class CD3D12RayMissShader : public CRHIRayMissShader, public CD3D12RayTracingShader
{
public:

    CD3D12RayMissShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayMissShader()
        , CD3D12RayTracingShader(InDevice, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }

    virtual void* GetRHIBaseShader() override final
    {
        CD3D12RayTracingShader* D3D12Shader = static_cast<CD3D12RayTracingShader*>(this);
        return reinterpret_cast<void*>(D3D12Shader);
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ComputeShader

class CD3D12ComputeShader : public CRHIComputeShader, public CD3D12Shader
{
public:

    CD3D12ComputeShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIComputeShader()
        , CD3D12Shader(InDevice, InCode, ShaderVisibility_All)
        , ThreadGroupXYZ(0, 0, 0)
    { }

    bool Init();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }

    virtual void* GetRHIBaseShader() override final
    {
        CD3D12Shader* D3D12Shader = static_cast<CD3D12Shader*>(this);
        return reinterpret_cast<void*>(D3D12Shader);
    }

    virtual CIntVector3 GetThreadGroupXYZ() const override final { return ThreadGroupXYZ; }

protected:
    CIntVector3 ThreadGroupXYZ;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GetD3D12Shader

inline CD3D12Shader* GetD3D12Shader(CRHIShader* Shader)
{
    return Shader ? reinterpret_cast<CD3D12Shader*>(Shader->GetRHIBaseShader()) : nullptr;
}

inline CD3D12RayTracingShader* D3D12RayTracingShaderCast(CRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<CD3D12RayTracingShader*>(Shader->GetRHIBaseShader()) : nullptr;
}
