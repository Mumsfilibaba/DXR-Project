#pragma once
#include "RenderLayer/Resources.h"

#include "D3D12Device.h"
#include "D3D12DeviceChild.h"
#include "D3D12Constants.h"

#include <d3d12shader.h>

enum EShaderVisibility
{
    ShaderVisibility_All = 0,
    ShaderVisibility_Vertex = 1,
    ShaderVisibility_Hull = 2,
    ShaderVisibility_Domain = 3,
    ShaderVisibility_Geometry = 4,
    ShaderVisibility_Pixel = 5,
    ShaderVisibility_Count = ShaderVisibility_Pixel + 1
};

enum EResourceType
{
    ResourceType_CBV = 0,
    ResourceType_SRV = 1,
    ResourceType_UAV = 2,
    ResourceType_Sampler = 3,
    ResourceType_Count = ResourceType_Sampler + 1,
    ResourceType_Unknown = 5,
};

struct ShaderResourceRange
{
    uint32 NumCBVs = 0;
    uint32 NumSRVs = 0;
    uint32 NumUAVs = 0;
    uint32 NumSamplers = 0;
};

struct ShaderResourceCount
{
    void Combine( const ShaderResourceCount& Other );
    bool IsCompatible( const ShaderResourceCount& Other ) const;

    ShaderResourceRange Ranges;
    uint32              Num32BitConstants = 0;
};

struct D3D12ShaderParameter
{
    D3D12ShaderParameter() = default;

    D3D12ShaderParameter( const std::string& InName, uint32 InRegister, uint32 InSpace, uint32 InNumDescriptors, uint32 InSizeInBytes )
        : Name( InName )
        , Register( InRegister )
        , Space( InSpace )
        , NumDescriptors( InNumDescriptors )
        , SizeInBytes( InSizeInBytes )
    {
    }

    std::string Name;
    uint32 Register = 0;
    uint32 Space = 0;
    uint32 NumDescriptors = 0;
    uint32 SizeInBytes = 0;
};

class D3D12BaseShader : public D3D12DeviceChild
{
public:
    D3D12BaseShader( D3D12Device* InDevice, const TArray<uint8>& InCode, EShaderVisibility ShaderVisibility );
    ~D3D12BaseShader();

    D3D12_SHADER_BYTECODE GetByteCode() const
    {
        return ByteCode;
    }

    const void* GetCode() const
    {
        return ByteCode.pShaderBytecode;
    }
    uint64 GetCodeSize()  const
    {
        return static_cast<uint64>(ByteCode.BytecodeLength);
    }

    D3D12ShaderParameter GetConstantBufferParameter( uint32 ParameterIndex )
    {
        return ConstantBufferParameters[ParameterIndex];
    }
    uint32 GetNumConstantBufferParameters()
    {
        return ConstantBufferParameters.Size();
    }

    D3D12ShaderParameter GetShaderResourceParameter( uint32 ParameterIndex )
    {
        return ShaderResourceParameters[ParameterIndex];
    }
    uint32 GetNumShaderResourceParameters()
    {
        return ShaderResourceParameters.Size();
    }

    D3D12ShaderParameter GetUnorderedAccessParameter( uint32 ParameterIndex )
    {
        return UnorderedAccessParameters[ParameterIndex];
    }
    uint32 GetNumUnorderedAccessParameters()
    {
        return UnorderedAccessParameters.Size();
    }

    D3D12ShaderParameter GetSamplerStateParameter( uint32 ParameterIndex )
    {
        return SamplerParameters[ParameterIndex];
    }
    uint32 GetNumSamplerStateParameters()
    {
        return SamplerParameters.Size();
    }

    bool HasRootSignature() const
    {
        return ContainsRootSignature;
    }

    EShaderVisibility GetShaderVisibility() const
    {
        return Visibility;
    };

    const ShaderResourceCount& GetResourceCount() const
    {
        return ResourceCount;
    }
    const ShaderResourceCount& GetRTLocalResourceCount() const
    {
        return RTLocalResourceCount;
    }

    static bool GetShaderReflection( class D3D12BaseShader* Shader );

protected:
    template<typename TD3D12ReflectionInterface>
    static bool GetShaderResourceBindings( TD3D12ReflectionInterface* Reflection, D3D12BaseShader* Shader, uint32 NumBoundResources );

    D3D12_SHADER_BYTECODE        ByteCode;
    TArray<D3D12ShaderParameter> ConstantBufferParameters;
    TArray<D3D12ShaderParameter> ShaderResourceParameters;
    TArray<D3D12ShaderParameter> UnorderedAccessParameters;
    TArray<D3D12ShaderParameter> SamplerParameters;
    EShaderVisibility            Visibility;
    ShaderResourceCount          ResourceCount;
    ShaderResourceCount          RTLocalResourceCount;

    bool ContainsRootSignature = false;
};

class D3D12BaseVertexShader : public VertexShader, public D3D12BaseShader
{
public:
    D3D12BaseVertexShader( D3D12Device* InDevice, const TArray<uint8>& InCode )
        : VertexShader()
        , D3D12BaseShader( InDevice, InCode, ShaderVisibility_Vertex )
    {
    }
};

class D3D12BasePixelShader : public PixelShader, public D3D12BaseShader
{
public:
    D3D12BasePixelShader( D3D12Device* InDevice, const TArray<uint8>& InCode )
        : PixelShader()
        , D3D12BaseShader( InDevice, InCode, ShaderVisibility_Pixel )
    {
    }
};

class D3D12BaseRayTracingShader : public D3D12BaseShader
{
public:
    D3D12BaseRayTracingShader( D3D12Device* InDevice, const TArray<uint8>& InCode )
        : D3D12BaseShader( InDevice, InCode, ShaderVisibility_All )
    {
    }

    const std::string& GetIdentifier() const
    {
        return Identifier;
    }

    static bool GetRayTracingShaderReflection( class D3D12BaseRayTracingShader* Shader );

protected:
    std::string Identifier;
};

class D3D12BaseRayGenShader : public RayGenShader, public D3D12BaseRayTracingShader
{
public:
    D3D12BaseRayGenShader( D3D12Device* InDevice, const TArray<uint8>& InCode )
        : RayGenShader()
        , D3D12BaseRayTracingShader( InDevice, InCode )
    {
    }
};

class D3D12BaseRayAnyhitShader : public RayAnyHitShader, public D3D12BaseRayTracingShader
{
public:
    D3D12BaseRayAnyhitShader( D3D12Device* InDevice, const TArray<uint8>& InCode )
        : RayAnyHitShader()
        , D3D12BaseRayTracingShader( InDevice, InCode )
    {
    }
};

class D3D12BaseRayClosestHitShader : public RayClosestHitShader, public D3D12BaseRayTracingShader
{
public:
    D3D12BaseRayClosestHitShader( D3D12Device* InDevice, const TArray<uint8>& InCode )
        : RayClosestHitShader()
        , D3D12BaseRayTracingShader( InDevice, InCode )
    {
    }
};

class D3D12BaseRayMissShader : public RayMissShader, public D3D12BaseRayTracingShader
{
public:
    D3D12BaseRayMissShader( D3D12Device* InDevice, const TArray<uint8>& InCode )
        : RayMissShader()
        , D3D12BaseRayTracingShader( InDevice, InCode )
    {
    }
};

class D3D12BaseComputeShader : public ComputeShader, public D3D12BaseShader
{
public:
    D3D12BaseComputeShader( D3D12Device* InDevice, const TArray<uint8>& InCode )
        : ComputeShader()
        , D3D12BaseShader( InDevice, InCode, ShaderVisibility_All )
        , ThreadGroupXYZ( 0, 0, 0 )
    {
    }

    bool Init();

    virtual CIntPoint3 GetThreadGroupXYZ() const override
    {
        return ThreadGroupXYZ;
    }

protected:
    CIntPoint3 ThreadGroupXYZ;
};

template<typename TBaseShader>
class TD3D12Shader : public TBaseShader
{
public:
    TD3D12Shader( D3D12Device* InDevice, const TArray<uint8>& InCode )
        : TBaseShader( InDevice, InCode )
    {
    }

    virtual void GetShaderParameterInfo( ShaderParameterInfo& OutShaderParameterInfo ) const override
    {
        OutShaderParameterInfo.NumConstantBuffers = ConstantBufferParameters.Size();
        OutShaderParameterInfo.NumShaderResourceViews = ShaderResourceParameters.Size();
        OutShaderParameterInfo.NumUnorderedAccessViews = UnorderedAccessParameters.Size();
        OutShaderParameterInfo.NumSamplerStates = SamplerParameters.Size();
    }

    virtual bool GetShaderResourceViewIndexByName( const std::string& InName, uint32& OutIndex ) const override
    {
        return InternalFindParameterIndexByName( ShaderResourceParameters, InName, OutIndex );
    }

    virtual bool GetSamplerIndexByName( const std::string& InName, uint32& OutIndex ) const override
    {
        return InternalFindParameterIndexByName( SamplerParameters, InName, OutIndex );
    }

    virtual bool GetUnorderedAccessViewIndexByName( const std::string& InName, uint32& OutIndex ) const override
    {
        return InternalFindParameterIndexByName( UnorderedAccessParameters, InName, OutIndex );
    }

    virtual bool GetConstantBufferIndexByName( const std::string& InName, uint32& OutIndex ) const override
    {
        return InternalFindParameterIndexByName( ConstantBufferParameters, InName, OutIndex );
    }

    virtual bool IsValid() const override
    {
        return ByteCode.pShaderBytecode != nullptr && ByteCode.BytecodeLength > 0;
    }

private:
    bool InternalFindParameterIndexByName( const TArray<D3D12ShaderParameter>& Parameters, const std::string& InName, uint32& OutIndex ) const
    {
        for ( uint32 i = 0; i < Parameters.Size(); i++ )
        {
            if ( Parameters[i].Name == InName )
            {
                OutIndex = i;
                return true;
            }
        }

        return false;
    }
};

using D3D12VertexShader = TD3D12Shader<D3D12BaseVertexShader>;
using D3D12PixelShader = TD3D12Shader<D3D12BasePixelShader>;

using D3D12ComputeShader = TD3D12Shader<D3D12BaseComputeShader>;

using D3D12RayGenShader = TD3D12Shader<D3D12BaseRayGenShader>;
using D3D12RayAnyHitShader = TD3D12Shader<D3D12BaseRayAnyhitShader>;
using D3D12RayClosestHitShader = TD3D12Shader<D3D12BaseRayClosestHitShader>;
using D3D12RayMissShader = TD3D12Shader<D3D12BaseRayMissShader>;

inline D3D12BaseShader* D3D12ShaderCast( Shader* Shader )
{
    if ( Shader->AsVertexShader() )
    {
        return static_cast<D3D12VertexShader*>(Shader);
    }
    else if ( Shader->AsPixelShader() )
    {
        return static_cast<D3D12PixelShader*>(Shader);
    }
    else if ( Shader->AsComputeShader() )
    {
        return static_cast<D3D12ComputeShader*>(Shader);
    }
    else if ( Shader->AsRayGenShader() )
    {
        return static_cast<D3D12RayGenShader*>(Shader);
    }
    else if ( Shader->AsRayAnyHitShader() )
    {
        return static_cast<D3D12RayAnyHitShader*>(Shader);
    }
    else if ( Shader->AsRayClosestHitShader() )
    {
        return static_cast<D3D12RayClosestHitShader*>(Shader);
    }
    else if ( Shader->AsRayMissShader() )
    {
        return static_cast<D3D12RayMissShader*>(Shader);
    }
    else
    {
        return nullptr;
    }
}
