#pragma once
#include "RHIResourceBase.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/String.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIShaderStage

enum class ERHIShaderStage
{
    Vertex = 1,
    Hull = 2,
    Domain = 3,
    Geometry = 4,
    Mesh = 5,
    Amplification = 6,
    Pixel = 7,
    Compute = 8,
    RayGen = 9,
    RayAnyHit = 10,
    RayClosestHit = 11,
    RayMiss = 12,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIShaderParameterInfo

struct SRHIShaderParameterInfo
{
    uint32 NumConstantBuffers = 0;
    uint32 NumShaderResourceViews = 0;
    uint32 NumUnorderedAccessViews = 0;
    uint32 NumSamplerStates = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShader

class CRHIShader : public CRHIObject
{
public:

    /**
     * @brief: Cast shader to a VertexShader
     * 
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual class CRHIVertexShader* AsVertexShader() { return nullptr; }
    
    /**
     * @brief: Cast shader to a HullShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual class CRHIHullShader* AsHullShader() { return nullptr; }

    /**
     * @brief: Cast shader to a DomainShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual class CRHIDomainShader* AsDomainShader() { return nullptr; }

    /**
     * @brief: Cast shader to a GeometryShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual class CRHIGeometryShader* AsGeometryShader() { return nullptr; }

    /**
     * @brief: Cast shader to a PixelShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual class CRHIPixelShader* AsPixelShader() { return nullptr; }

    /**
     * @brief: Cast shader to a ComputeShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual class CRHIComputeShader* AsComputeShader() { return nullptr; }

    /**
     * @brief: Cast shader to a RayGenShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual class CRHIRayGenShader* AsRayGenShader() { return nullptr; }

    /**
     * @brief: Cast shader to a RayAnyHitShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual class CRHIRayAnyHitShader* AsRayAnyHitShader() { return nullptr; }

    /**
     * @brief: Cast shader to a RayClosestHitShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual class CRHIRayClosestHitShader* AsRayClosestHitShader() { return nullptr; }

    /**
     * @brief: Cast shader to a RayMissShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual class CRHIRayMissShader* AsRayMissShader() { return nullptr; }

    /**
     * @brief: Retrieve the number of ShaderParameters
     * 
     * @param OutShaderParameterInfo: A structure containing the number of different ShaderParameters
     */
    virtual void GetShaderParameterInfo(SRHIShaderParameterInfo& OutShaderParameterInfo) const = 0;

    /**
     * @brief: Retrieve a ConstantBuffer index by the name
     * 
     * @param InName: Name of the variable
     * @param OutIndex: Index of the variable
     * @return: Returns false if not variable with the specified name exists
     */
    virtual bool GetConstantBufferIndexByName(const String& InName, uint32& OutIndex) const = 0;

    /**
     * @brief: Retrieve a UnorderedAccessView index by the name
     *
     * @param InName: Name of the variable
     * @param OutIndex: Index of the variable
     * @return: Returns false if not variable with the specified name exists
     */
    virtual bool GetUnorderedAccessViewIndexByName(const String& InName, uint32& OutIndex) const = 0;

    /**
     * @brief: Retrieve a ShaderResourceView index by the name
     *
     * @param InName: Name of the variable
     * @param OutIndex: Index of the variable
     * @return: Returns false if not variable with the specified name exists
     */
    virtual bool GetShaderResourceViewIndexByName(const String& InName, uint32& OutIndex) const = 0;

    /**
     * @brief: Retrieve a Sampler index by the name
     *
     * @param InName: Name of the variable
     * @param OutIndex: Index of the variable
     * @return: Returns false if not variable with the specified name exists
     */
    virtual bool GetSamplerIndexByName(const String& InName, uint32& OutIndex) const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIComputeShader

class CRHIComputeShader : public CRHIShader
{
public:

    /**
     * @brief: Cast shader to a ComputeShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual CRHIComputeShader* AsComputeShader() { return this; }


    /**
     * @brief: Retrieve the threadgroup-count
     * 
     * @return: Returns a vector with the number of thread in each dimension
     */
    virtual CIntVector3 GetThreadGroupXYZ() const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexShader

class CRHIVertexShader : public CRHIShader
{
public:

    /**
     * @brief: Cast shader to a VertexShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual CRHIVertexShader* AsVertexShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIHullShader

class CRHIHullShader : public CRHIShader
{
public:

    /**
     * @brief: Cast shader to a HullShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual CRHIHullShader* AsHullShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDomainShader

class CRHIDomainShader : public CRHIShader
{
public:

    /**
     * @brief: Cast shader to a DomainShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual CRHIDomainShader* AsDomainShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGeometryShader

class CRHIGeometryShader : public CRHIShader
{
public:

    /**
     * @brief: Cast shader to a GeometryShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual CRHIGeometryShader* AsGeometryShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIMeshShader

class CRHIMeshShader : public CRHIShader
{
    // TODO
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIAmplificationShader

class CRHIAmplificationShader : public CRHIShader
{
    // TODO
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIPixelShader

class CRHIPixelShader : public CRHIShader
{
public:

    /**
     * @brief: Cast shader to a PixelShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual CRHIPixelShader* AsPixelShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayGenShader

class CRHIRayGenShader : public CRHIShader
{
public:

    /**
     * @brief: Cast shader to a RayGenShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual CRHIRayGenShader* AsRayGenShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayAnyHitShader

class CRHIRayAnyHitShader : public CRHIShader
{
public:

    /**
     * @brief: Cast shader to a RayAnyHitShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual CRHIRayAnyHitShader* AsRayAnyHitShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayClosestHitShader

class CRHIRayClosestHitShader : public CRHIShader
{
public:

    /**
     * @brief: Cast shader to a RayClosestHitShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual CRHIRayClosestHitShader* AsRayClosestHitShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayMissShader

class CRHIRayMissShader : public CRHIShader
{
public:

    /**
     * @brief: Cast shader to a RayMissShader
     *
     * @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type
     */
    virtual CRHIRayMissShader* AsRayMissShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

inline bool ShaderStageIsGraphics(ERHIShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
    case ERHIShaderStage::Vertex:
    case ERHIShaderStage::Hull:
    case ERHIShaderStage::Domain:
    case ERHIShaderStage::Geometry:
    case ERHIShaderStage::Pixel:
    case ERHIShaderStage::Mesh:
    case ERHIShaderStage::Amplification:
    {
        return true;
    }

    default:
    {
        return false;
    }
    }
}

inline bool ShaderStageIsCompute(ERHIShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
    case ERHIShaderStage::Compute:
    case ERHIShaderStage::RayGen:
    case ERHIShaderStage::RayClosestHit:
    case ERHIShaderStage::RayAnyHit:
    case ERHIShaderStage::RayMiss:
    {
        return true;
    }

    default:
    {
        return false;
    }
    }
}

inline bool ShaderStageIsRayTracing(ERHIShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
    case ERHIShaderStage::RayGen:
    case ERHIShaderStage::RayClosestHit:
    case ERHIShaderStage::RayAnyHit:
    case ERHIShaderStage::RayMiss:
    {
        return true;
    }

    default:
    {
        return false;
    }
    }
}