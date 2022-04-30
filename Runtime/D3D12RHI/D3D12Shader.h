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
// D3D12ShaderParameter

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
    CD3D12Shader(CD3D12Device* InDevice, const TArray<uint8>& InCode, EShaderVisibility ShaderVisibility);
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
        D3D12_ERROR( ParameterIndex < static_cast<uint32>(ConstantBufferParameters.Size())
                   , "Trying to access ParameterIndex=" + ToString(ParameterIndex) + ", but the shader only has " + ToString(ConstantBufferParameters.Size()) + " slots");
        return ConstantBufferParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumConstantBufferParameters() { return ConstantBufferParameters.Size(); }

    FORCEINLINE SD3D12ShaderParameter GetShaderResourceParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR( ParameterIndex < static_cast<uint32>(ShaderResourceParameters.Size())
                   , "Trying to access ParameterIndex=" + ToString(ParameterIndex) + ", but the shader only has " + ToString(ShaderResourceParameters.Size()) + " slots");
        return ShaderResourceParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumShaderResourceParameters() { return ShaderResourceParameters.Size(); }

    FORCEINLINE SD3D12ShaderParameter GetUnorderedAccessParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR( ParameterIndex < static_cast<uint32>(UnorderedAccessParameters.Size())
                   , "Trying to access ParameterIndex=" + ToString(ParameterIndex) + ", but the shader only has " + ToString(UnorderedAccessParameters.Size()) + " slots");
        return UnorderedAccessParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumUnorderedAccessParameters() { return UnorderedAccessParameters.Size(); }

    FORCEINLINE SD3D12ShaderParameter GetSamplerStateParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR( ParameterIndex < static_cast<uint32>(SamplerParameters.Size())
                   , "Trying to access ParameterIndex=" + ToString(ParameterIndex) + ", but the shader only has " + ToString(SamplerParameters.Size()) + " slots");
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
// D3D12RHIBaseVertexShader

class CD3D12RHIBaseVertexShader : public CRHIVertexShader, public CD3D12Shader
{
public:
    CD3D12RHIBaseVertexShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIVertexShader()
        , CD3D12Shader(InDevice, InCode, ShaderVisibility_Vertex)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBasePixelShader

class CD3D12RHIBasePixelShader : public CRHIPixelShader, public CD3D12Shader
{
public:
    CD3D12RHIBasePixelShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIPixelShader()
        , CD3D12Shader(InDevice, InCode, ShaderVisibility_Pixel)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBaseRayTracingShader

class CD3D12RHIBaseRayTracingShader : public CD3D12Shader
{
public:
    CD3D12RHIBaseRayTracingShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CD3D12Shader(InDevice, InCode, ShaderVisibility_All)
    { }

public:

    static bool GetRayTracingShaderReflection(class CD3D12RHIBaseRayTracingShader* Shader);
    
    FORCEINLINE const String& GetIdentifier() const
    {
        return Identifier;
    }

protected:
    String Identifier;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBaseRayGenShader

class CD3D12RHIBaseRayGenShader : public CRHIRayGenShader, public CD3D12RHIBaseRayTracingShader
{
public:
    CD3D12RHIBaseRayGenShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayGenShader()
        , CD3D12RHIBaseRayTracingShader(InDevice, InCode)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBaseRayAnyhitShader

class CD3D12RHIBaseRayAnyhitShader : public CRHIRayAnyHitShader, public CD3D12RHIBaseRayTracingShader
{
public:
    CD3D12RHIBaseRayAnyhitShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayAnyHitShader()
        , CD3D12RHIBaseRayTracingShader(InDevice, InCode)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBaseRayClosestHitShader

class CD3D12RHIBaseRayClosestHitShader : public CRHIRayClosestHitShader, public CD3D12RHIBaseRayTracingShader
{
public:
    CD3D12RHIBaseRayClosestHitShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayClosestHitShader()
        , CD3D12RHIBaseRayTracingShader(InDevice, InCode)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBaseRayMissShader

class CD3D12RHIBaseRayMissShader : public CRHIRayMissShader, public CD3D12RHIBaseRayTracingShader
{
public:
    CD3D12RHIBaseRayMissShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayMissShader()
        , CD3D12RHIBaseRayTracingShader(InDevice, InCode)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIBaseComputeShader

class CD3D12RHIBaseComputeShader : public CRHIComputeShader, public CD3D12Shader
{
public:

    CD3D12RHIBaseComputeShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIComputeShader()
        , CD3D12Shader(InDevice, InCode, ShaderVisibility_All)
        , ThreadGroupXYZ(0, 0, 0)
    { }

    bool Init();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual CIntVector3 GetThreadGroupXYZ() const override final { return ThreadGroupXYZ; }

protected:
    CIntVector3 ThreadGroupXYZ;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIShader

template<typename BaseShaderType>
class TD3D12RHIShader : public BaseShaderType
{
public:
    TD3D12RHIShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : BaseShaderType(InDevice, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseShader() override final
    {
        CD3D12Shader* D3D12Shader = static_cast<CD3D12Shader*>(this);
        return reinterpret_cast<void*>(D3D12Shader);
    }

    virtual void GetShaderParameterInfo(SShaderParameterInfo& OutShaderParameterInfo) const override final
    {
        OutShaderParameterInfo.NumConstantBuffers      = ConstantBufferParameters.Size();
        OutShaderParameterInfo.NumShaderResourceViews  = ShaderResourceParameters.Size();
        OutShaderParameterInfo.NumUnorderedAccessViews = UnorderedAccessParameters.Size();
        OutShaderParameterInfo.NumSamplerStates        = SamplerParameters.Size();
    }

    virtual bool GetShaderResourceViewIndexByName(const String& InName, uint32& OutIndex) const override final
    {
        return FindParameterIndexByName(ShaderResourceParameters, InName, OutIndex);
    }

    virtual bool GetSamplerIndexByName(const String& InName, uint32& OutIndex) const override final
    {
        return FindParameterIndexByName(SamplerParameters, InName, OutIndex);
    }

    virtual bool GetUnorderedAccessViewIndexByName(const String& InName, uint32& OutIndex) const override final
    {
        return FindParameterIndexByName(UnorderedAccessParameters, InName, OutIndex);
    }

    virtual bool GetConstantBufferIndexByName(const String& InName, uint32& OutIndex) const override final
    {
        return FindParameterIndexByName(ConstantBufferParameters, InName, OutIndex);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Deprecated

    virtual bool IsValid() const override final
    {
        return ByteCode.pShaderBytecode != nullptr && ByteCode.BytecodeLength > 0;
    }

private:
    bool FindParameterIndexByName(const TArray<SD3D12ShaderParameter>& Parameters, const String& InName, uint32& OutIndex) const
    {
        for (int32 i = 0; i < Parameters.Size(); i++)
        {
            if (Parameters[i].Name == InName)
            {
                OutIndex = i;
                return true;
            }
        }

        return false;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12 Shaders

using CD3D12RHIVertexShader     = TD3D12RHIShader<CD3D12RHIBaseVertexShader>;
using CD3D12RHIPixelShader      = TD3D12RHIShader<CD3D12RHIBasePixelShader>;

using CD3D12RHIComputeShader    = TD3D12RHIShader<CD3D12RHIBaseComputeShader>;

using CD3D12RHIRayGenShader     = TD3D12RHIShader<CD3D12RHIBaseRayGenShader>;
using CD3D12RHIRayAnyHitShader  = TD3D12RHIShader<CD3D12RHIBaseRayAnyhitShader>;
using CD3D12RayClosestHitShader = TD3D12RHIShader<CD3D12RHIBaseRayClosestHitShader>;
using CD3D12RHIRayMissShader    = TD3D12RHIShader<CD3D12RHIBaseRayMissShader>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12ShaderCast

inline CD3D12Shader* D3D12ShaderCast(CRHIShader* Shader)
{
    return Shader ? reinterpret_cast<CD3D12Shader*>(Shader->GetRHIBaseShader()) : nullptr;
}
