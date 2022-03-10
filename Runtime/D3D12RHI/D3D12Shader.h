#pragma once
#include "RHI/RHIResources.h"

#include "D3D12Device.h"
#include "D3D12DeviceChild.h"
#include "D3D12Constants.h"

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
    {
    }

    String Name;
    uint32 Register = 0;
    uint32 Space = 0;
    uint32 NumDescriptors = 0;
    uint32 SizeInBytes = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseShader

class CD3D12BaseShader : public CD3D12DeviceObject
{
public:
    CD3D12BaseShader(CD3D12Device* InDevice, const TArray<uint8>& InCode, EShaderVisibility ShaderVisibility);
    ~CD3D12BaseShader();

    FORCEINLINE D3D12_SHADER_BYTECODE GetByteCode() const
    {
        return ByteCode;
    }

    FORCEINLINE const void* GetCode() const
    {
        return ByteCode.pShaderBytecode;
    }

    FORCEINLINE uint64 GetCodeSize()  const
    {
        return static_cast<uint64>(ByteCode.BytecodeLength);
    }

    FORCEINLINE SD3D12ShaderParameter GetConstantBufferParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR(ParameterIndex < static_cast<uint32>(ConstantBufferParameters.Size()), "Trying to access ParameterIndex=" + ToString(ParameterIndex) + ", but the shader only has " + ToString(ConstantBufferParameters.Size()) + " slots");
        return ConstantBufferParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumConstantBufferParameters()
    {
        return ConstantBufferParameters.Size();
    }

    FORCEINLINE SD3D12ShaderParameter GetShaderResourceParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR(ParameterIndex < static_cast<uint32>(ShaderResourceParameters.Size()), "Trying to access ParameterIndex=" + ToString(ParameterIndex) + ", but the shader only has " + ToString(ShaderResourceParameters.Size()) + " slots");
        return ShaderResourceParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumShaderResourceParameters()
    {
        return ShaderResourceParameters.Size();
    }

    FORCEINLINE SD3D12ShaderParameter GetUnorderedAccessParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR(ParameterIndex < static_cast<uint32>(UnorderedAccessParameters.Size()), "Trying to access ParameterIndex=" + ToString(ParameterIndex) + ", but the shader only has " + ToString(UnorderedAccessParameters.Size()) + " slots");
        return UnorderedAccessParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumUnorderedAccessParameters()
    {
        return UnorderedAccessParameters.Size();
    }

    FORCEINLINE SD3D12ShaderParameter GetSamplerStateParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR(ParameterIndex < static_cast<uint32>(SamplerParameters.Size()), "Trying to access ParameterIndex=" + ToString(ParameterIndex) + ", but the shader only has " + ToString(SamplerParameters.Size()) + " slots");
        return SamplerParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumSamplerStateParameters()
    {
        return SamplerParameters.Size();
    }

    FORCEINLINE bool HasRootSignature() const
    {
        return bContainsRootSignature;
    }

    FORCEINLINE EShaderVisibility GetShaderVisibility() const
    {
        return Visibility;
    }

    FORCEINLINE const SShaderResourceCount& GetResourceCount() const
    {
        return ResourceCount;
    }

    FORCEINLINE const SShaderResourceCount& GetRTLocalResourceCount() const
    {
        return RTLocalResourceCount;
    }

    static bool GetShaderReflection(class CD3D12BaseShader* Shader);

protected:

    template<typename TD3D12ReflectionInterface>
    static bool GetShaderResourceBindings(TD3D12ReflectionInterface* Reflection, CD3D12BaseShader* Shader, uint32 NumBoundResources);

    D3D12_SHADER_BYTECODE ByteCode;

    TArray<SD3D12ShaderParameter> ConstantBufferParameters;
    TArray<SD3D12ShaderParameter> ShaderResourceParameters;
    TArray<SD3D12ShaderParameter> UnorderedAccessParameters;
    TArray<SD3D12ShaderParameter> SamplerParameters;

    EShaderVisibility    Visibility;
    SShaderResourceCount ResourceCount;
    SShaderResourceCount RTLocalResourceCount;

    bool bContainsRootSignature = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseVertexShader

class CD3D12BaseVertexShader : public CRHIVertexShader, public CD3D12BaseShader
{
public:
    CD3D12BaseVertexShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIVertexShader()
        , CD3D12BaseShader(InDevice, InCode, ShaderVisibility_Vertex)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BasePixelShader

class CD3D12BasePixelShader : public CRHIPixelShader, public CD3D12BaseShader
{
public:
    CD3D12BasePixelShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIPixelShader()
        , CD3D12BaseShader(InDevice, InCode, ShaderVisibility_Pixel)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseRayTracingShader

class CD3D12BaseRayTracingShader : public CD3D12BaseShader
{
public:
    CD3D12BaseRayTracingShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CD3D12BaseShader(InDevice, InCode, ShaderVisibility_All)
    {
    }

    FORCEINLINE const String& GetIdentifier() const
    {
        return Identifier;
    }

    static bool GetRayTracingShaderReflection(class CD3D12BaseRayTracingShader* Shader);

protected:
    String Identifier;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseRayGenShader

class CD3D12BaseRayGenShader : public CRHIRayGenShader, public CD3D12BaseRayTracingShader
{
public:
    CD3D12BaseRayGenShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayGenShader()
        , CD3D12BaseRayTracingShader(InDevice, InCode)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseRayAnyhitShader

class CD3D12BaseRayAnyhitShader : public CRHIRayAnyHitShader, public CD3D12BaseRayTracingShader
{
public:
    CD3D12BaseRayAnyhitShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayAnyHitShader()
        , CD3D12BaseRayTracingShader(InDevice, InCode)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseRayClosestHitShader

class CD3D12BaseRayClosestHitShader : public CRHIRayClosestHitShader, public CD3D12BaseRayTracingShader
{
public:
    CD3D12BaseRayClosestHitShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayClosestHitShader()
        , CD3D12BaseRayTracingShader(InDevice, InCode)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseRayMissShader

class CD3D12BaseRayMissShader : public CRHIRayMissShader, public CD3D12BaseRayTracingShader
{
public:
    CD3D12BaseRayMissShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIRayMissShader()
        , CD3D12BaseRayTracingShader(InDevice, InCode)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseComputeShader

class CD3D12BaseComputeShader : public CRHIComputeShader, public CD3D12BaseShader
{
public:
    CD3D12BaseComputeShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : CRHIComputeShader()
        , CD3D12BaseShader(InDevice, InCode, ShaderVisibility_All)
        , ThreadGroupXYZ(0, 0, 0)
    {
    }

    bool Init();

    virtual CIntVector3 GetThreadGroupXYZ() const override
    {
        return ThreadGroupXYZ;
    }

protected:
    CIntVector3 ThreadGroupXYZ;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TD3D12RHIShader

template<typename BaseShaderType>
class TD3D12RHIShader : public BaseShaderType
{
public:
    TD3D12RHIShader(CD3D12Device* InDevice, const TArray<uint8>& InCode)
        : BaseShaderType(InDevice, InCode)
    {
    }

    virtual void GetShaderParameterInfo(SRHIShaderParameterInfo& OutShaderParameterInfo) const override
    {
        OutShaderParameterInfo.NumConstantBuffers = ConstantBufferParameters.Size();
        OutShaderParameterInfo.NumShaderResourceViews = ShaderResourceParameters.Size();
        OutShaderParameterInfo.NumUnorderedAccessViews = UnorderedAccessParameters.Size();
        OutShaderParameterInfo.NumSamplerStates = SamplerParameters.Size();
    }

    virtual bool GetShaderResourceViewIndexByName(const String& InName, uint32& OutIndex) const override
    {
        return FindParameterIndexByName(ShaderResourceParameters, InName, OutIndex);
    }

    virtual bool GetSamplerIndexByName(const String& InName, uint32& OutIndex) const override
    {
        return FindParameterIndexByName(SamplerParameters, InName, OutIndex);
    }

    virtual bool GetUnorderedAccessViewIndexByName(const String& InName, uint32& OutIndex) const override
    {
        return FindParameterIndexByName(UnorderedAccessParameters, InName, OutIndex);
    }

    virtual bool GetConstantBufferIndexByName(const String& InName, uint32& OutIndex) const override
    {
        return FindParameterIndexByName(ConstantBufferParameters, InName, OutIndex);
    }

    virtual bool IsValid() const override
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

using CD3D12VertexShader        = TD3D12RHIShader<CD3D12BaseVertexShader>;
using CD3D12PixelShader         = TD3D12RHIShader<CD3D12BasePixelShader>;

using CD3D12ComputeShader       = TD3D12RHIShader<CD3D12BaseComputeShader>;

using CD3D12RayGenShader        = TD3D12RHIShader<CD3D12BaseRayGenShader>;
using CD3D12RayAnyHitShader     = TD3D12RHIShader<CD3D12BaseRayAnyhitShader>;
using CD3D12RayClosestHitShader = TD3D12RHIShader<CD3D12BaseRayClosestHitShader>;
using CD3D12RayMissShader       = TD3D12RHIShader<CD3D12BaseRayMissShader>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12ShaderCast

inline CD3D12BaseShader* D3D12ShaderCast(CRHIShader* Shader)
{
    if (Shader->AsVertexShader())
    {
        return static_cast<CD3D12VertexShader*>(Shader);
    }
    else if (Shader->AsPixelShader())
    {
        return static_cast<CD3D12PixelShader*>(Shader);
    }
    else if (Shader->AsComputeShader())
    {
        return static_cast<CD3D12ComputeShader*>(Shader);
    }
    else if (Shader->AsRayGenShader())
    {
        return static_cast<CD3D12RayGenShader*>(Shader);
    }
    else if (Shader->AsRayAnyHitShader())
    {
        return static_cast<CD3D12RayAnyHitShader*>(Shader);
    }
    else if (Shader->AsRayClosestHitShader())
    {
        return static_cast<CD3D12RayClosestHitShader*>(Shader);
    }
    else if (Shader->AsRayMissShader())
    {
        return static_cast<CD3D12RayMissShader*>(Shader);
    }
    else
    {
        return nullptr;
    }
}
