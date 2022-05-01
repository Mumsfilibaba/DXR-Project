#pragma once
#include "RHIResourceBase.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/String.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShaderStage

enum class EShaderStage
{
    Vertex        = 1,
    Hull          = 2,
    Domain        = 3,
    Geometry      = 4,
    Mesh          = 5,
    Amplification = 6,
    Pixel         = 7,
    Compute       = 8,
    RayGen        = 9,
    RayAnyHit     = 10,
    RayClosestHit = 11,
    RayMiss       = 12,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SShaderParameterInfo

struct SShaderParameterInfo
{
    uint32 NumConstantBuffers      = 0;
    uint32 NumShaderResourceViews  = 0;
    uint32 NumUnorderedAccessViews = 0;
    uint32 NumSamplerStates        = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShader

class CRHIShader : public CRHIResource
{
protected:

    CRHIShader() = default;
    ~CRHIShader() = default;

public:

    /** @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type */
    virtual class CRHIVertexShader* AsVertexShader() { return nullptr; }
    
    /** @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type */
    virtual class CRHIHullShader* AsHullShader() { return nullptr; }

    /** @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type */
    virtual class CRHIDomainShader* AsDomainShader() { return nullptr; }

    /** @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type */
    virtual class CRHIGeometryShader* AsGeometryShader() { return nullptr; }

    /** @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type */
    virtual class CRHIPixelShader* AsPixelShader() { return nullptr; }

    /** @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type */
    virtual class CRHIComputeShader* AsComputeShader() { return nullptr; }

    /** @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type */
    virtual class CRHIRayGenShader* AsRayGenShader() { return nullptr; }

    /** @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type */
    virtual class CRHIRayAnyHitShader* AsRayAnyHitShader() { return nullptr; }

    /** @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type */
    virtual class CRHIRayClosestHitShader* AsRayClosestHitShader() { return nullptr; }

    /** @return: Returns a pointer to the interface if the type is correct or nullptr if the shader is another type */
    virtual class CRHIRayMissShader* AsRayMissShader() { return nullptr; }

    /** @return: Returns the native handle of the Shader */
    virtual void* GetRHIBaseResource() const { return nullptr; }

    /** @return: Returns the RHI-backend Shader interface */
    virtual void* GetRHIBaseShader() { return nullptr; }

    /**
     * @brief: Retrieve the number of ShaderParameters
     * 
     * @param OutShaderParameterInfo: A structure containing the number of different ShaderParameters
     */
    virtual void GetShaderParameterInfo(SShaderParameterInfo& OutShaderParameterInfo) const = 0;

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
protected:

    CRHIComputeShader() = default;
    ~CRHIComputeShader() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual CRHIComputeShader* AsComputeShader() { return this; }

public:

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
protected:

    CRHIVertexShader() = default;
    ~CRHIVertexShader() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual CRHIVertexShader* AsVertexShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIHullShader

class CRHIHullShader : public CRHIShader
{
protected:

    CRHIHullShader() = default;
    ~CRHIHullShader() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual CRHIHullShader* AsHullShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDomainShader

class CRHIDomainShader : public CRHIShader
{
protected:

    CRHIDomainShader() = default;
    ~CRHIDomainShader() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual CRHIDomainShader* AsDomainShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGeometryShader

class CRHIGeometryShader : public CRHIShader
{
protected:

    CRHIGeometryShader() = default;
    ~CRHIGeometryShader() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

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
protected:

    CRHIPixelShader() = default;
    ~CRHIPixelShader() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual CRHIPixelShader* AsPixelShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayGenShader

class CRHIRayGenShader : public CRHIShader
{
protected:

    CRHIRayGenShader() = default;
    ~CRHIRayGenShader() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual CRHIRayGenShader* AsRayGenShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayAnyHitShader

class CRHIRayAnyHitShader : public CRHIShader
{
protected:

    CRHIRayAnyHitShader() = default;
    ~CRHIRayAnyHitShader() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual CRHIRayAnyHitShader* AsRayAnyHitShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayClosestHitShader

class CRHIRayClosestHitShader : public CRHIShader
{
protected:

    CRHIRayClosestHitShader() = default;
    ~CRHIRayClosestHitShader() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual CRHIRayClosestHitShader* AsRayClosestHitShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayMissShader

class CRHIRayMissShader : public CRHIShader
{
protected:

    CRHIRayMissShader() = default;
    ~CRHIRayMissShader() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual CRHIRayMissShader* AsRayMissShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

inline bool ShaderStageIsGraphics(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::Vertex:
        case EShaderStage::Hull:
        case EShaderStage::Domain:
        case EShaderStage::Geometry:
        case EShaderStage::Pixel:
        case EShaderStage::Mesh:
        case EShaderStage::Amplification:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}

inline bool ShaderStageIsCompute(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::Compute:
        case EShaderStage::RayGen:
        case EShaderStage::RayClosestHit:
        case EShaderStage::RayAnyHit:
        case EShaderStage::RayMiss:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}

inline bool ShaderStageIsRayTracing(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::RayGen:
        case EShaderStage::RayClosestHit:
        case EShaderStage::RayAnyHit:
        case EShaderStage::RayMiss:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}