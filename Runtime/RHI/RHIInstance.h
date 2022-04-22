#pragma once
#include "RHITypes.h"
#include "RHIModule.h"
#include "RHIResources.h"
#include "RHIPipeline.h"
#include "RHICommandList.h"

#include "Core/Memory/Memory.h"

class CRHIRayTracingGeometry;
class CRHIRayTracingScene;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIInstance

class CRHIInstance
{
protected:

    CRHIInstance(ERHIType InType)
        : Type(InType)
    { }

    virtual ~CRHIInstance() = default;

public:

    /**
     * @brief: Initialize the RHI instance that the engine should be using
     * 
     * @param bEnableDebug: True if the debug-layer should be enabled
     * @return: Returns true if the initialization was successful
     */
    virtual bool Initialize(bool bEnableDebug) = 0;

    /**
     * @brief: Destroys the instance
     */
    virtual void Destroy() { delete this; }

    /**
     * @brief: Creates a Texture2D
     *
     * @param Initializer: Structure containing information for creating a Texture2D
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITexture2DRef CreateTexture2D(const CRHITexture2DInitializer& Initializer) = 0;

    /**
     * @brief: Creates a Texture2DArray
     *
     * @param Initializer: Structure containing information for creating a Texture2DArray
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITexture2DArrayRef CreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer) = 0;

    /**
     * @brief: Creates a TextureCube
     *
     * @param Initializer: Structure containing information for creating a TextureCube
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITextureCubeRef CreateTextureCube(const CRHITextureCubeInitializer& Initializer) = 0;

    /**
     * @brief: Creates a Texture3D
     *
     * @param Initializer: Structure containing information for creating a Texture3D
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITexture3DRef CreateTexture3D(const CRHITexture3DInitializer& Initializer) = 0;

    /**
     * @brief: Creates a VertexBuffer
     *
     * @param Initializer: State that contains information about a VertexBuffer
     * @param InitialData: Initial data supplied to the Buffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIVertexBufferRef CreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer) = 0;

    /**
     * @brief: Creates a IndexBuffer
     *
     * @param Initializer: State that contains information about a IndexBuffer
     * @param InitialData: Initial data supplied to the Buffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIIndexBufferRef CreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer) = 0;

    /**
     * @brief: Creates a GenericBuffer
     * 
     * @param Initializer: State that contains information about a GenericBuffer
     * @param InitialData: Initial data supplied to the Buffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIGenericBufferRef CreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer) = 0;

    /**
     * @brief: Creates a ConstantBuffer
     *
     * @param Initializer: State that contains information about a ConstantBuffer
     * @param InitialData: Initial data supplied to the Buffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIConstantBufferRef CreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer) = 0;

    /**
     * @brief: Create a SamplerState
     * 
     * @param Initializer: Structure with information about the SamplerState
     * @return: Returns the newly created SamplerState (Could be the same as a already created sampler state and a reference is added)
     */
    virtual CRHISamplerStateRef CreateSamplerState(const CRHISamplerStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new Ray tracing scene
     * 
     * @param Initializer: Structure containing information about the Scene
     * @return: Returns the newly created Ray tracing Scene
     */
    virtual CRHIRayTracingSceneRef CreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a new Ray tracing geometry
     *
     * @param Initializer: Structure containing information about the Geometry
     * @return: Returns the newly created Ray tracing Geometry
     */
    virtual CRHIRayTracingGeometryRef CreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer) = 0;

    /**
     * @brief: Create a new ShaderResourceView
     *
     * @param Initializer: Description for the new view to create
     * @return: Returns the newly created ShaderResourceView
     */
    virtual CRHIShaderResourceViewRef CreateShaderResourceView(const CRHITextureSRVInitializer& Initializer) = 0;

    /**
     * @brief: Create a new ShaderResourceView
     *
     * @param Initializer: Description for the new view to create
     * @return: Returns the newly created ShaderResourceView
     */
    virtual CRHIShaderResourceViewRef CreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a new UnorderedAccessView for a texture
     *
     * @param Initializer: Description for the new view to create
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual CRHIUnorderedAccessViewRef CreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer) = 0;

    /**
     * @brief: Create a new UnorderedAccessView for a buffer
     *
     * @param Initializer: Description for the new view to create
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual CRHIUnorderedAccessViewRef CreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer) = 0;

    /**
     * @brief: Creates a new Compute Shader
     * 
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief: Creates a new Vertex Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Hull Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIHullShader* CreateHullShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Domain Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Geometry Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Mesh Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Amplification Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Pixel Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIPixelShader* CreatePixelShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief: Creates a new Ray-Generation Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Ray Any-Hit Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIRayAnyHitShader* CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Ray-Closest-Hit Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * @brief: Creates a new Ray-Miss Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIRayMissShader* CreateRayMissShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * @brief: Create a new DepthStencilState
     * 
     * @param Desc: Info about a DepthStencilState
     * @return: Returns the newly created DepthStencilState
     */
    virtual class CRHIDepthStencilState* CreateDepthStencilState(const CRHIDepthStencilStateInitializer& Desc) = 0;

    /**
     * @brief: Create a new RasterizerState
     *
     * @param Desc: Info about a RasterizerState
     * @return: Returns the newly created RasterizerState
     */
    virtual class CRHIRasterizerState* CreateRasterizerState(const CRHIRasterizerStateInitializer& Desc) = 0;

    /**
     * @brief: Create a new BlendState
     *
     * @param Desc: Info about a BlendState
     * @return: Returns the newly created BlendState
     */
    virtual class CRHIBlendState* CreateBlendState(const CRHIBlendStateInitializer& Desc) = 0;

    /**
     * @brief: Create a new InputLayoutState
     *
     * @param Desc: Info about a InputLayoutState
     * @return: Returns the newly created InputLayoutState
     */
    virtual class CRHIVertexInputLayout* CreateInputLayout(const CRHIVertexInputLayoutInitializer& Desc) = 0;

    /**
     * @brief: Create a Graphics PipelineState
     * 
     * @param Desc: Info about the Graphics PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState(const CRHIGraphicsPipelineStateInitializer& Desc) = 0;
    
    /**
     * @brief: Create a Compute PipelineState
     *
     * @param Desc: Info about the Compute PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIComputePipelineState* CreateComputePipelineState(const CRHIComputePipelineStateInitializer& Desc) = 0;
    
    /**
     * @brief: Create a Ray-Tracing PipelineState
     *
     * @param Desc: Info about the Ray-Tracing PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const CRHIRayTracingPipelineStateInitializer& Desc) = 0;

    /**
     * @brief: Create a new Timestamp Query
     * 
     * @return: Returns the newly created Timestamp Query
     */
    virtual CRHITimeQueryRef CreateTimeQuery() = 0;

    /**
     * @brief: Create a new Viewport
     * 
     * @param Initializer: Structure containing information about the viewport
     * @return: Returns the newly created viewport
     */
    virtual CRHIViewportRef CreateViewport(const CRHIViewportInitializer& Initializer) = 0;

    /**
     * @brief: Retrieve the default CommandContext
     * 
     * @return: Returns the default CommandContext
     */
    virtual class IRHICommandContext* GetDefaultCommandContext() = 0;

    /**
     * @brief: Retrieve the name of the Adapter
     * 
     * @return: Returns a string with the Adapter name
     */
    virtual String GetAdapterName() const { return String(); }

    /**
     * @brief: Check for Ray tracing support
     * 
     * @param OutSupport: Struct containing the Ray tracing support for the system and current RHI
     */
    virtual void CheckRayTracingSupport(SRayTracingSupport& OutSupport) const = 0;

    /**
     * @brief: Check for Shading-rate support
     *
     * @param OutSupport: Struct containing the Shading-rate support for the system and current RHI
     */
    virtual void CheckShadingRateSupport(SShadingRateSupport& OutSupport) const = 0;

    /**
     * @brief: Check if the current RHI supports UnorderedAccessViews for the specified format
     * 
     * @param Format: Format to check
     * @return: Returns true if the current RHI supports UnorderedAccessViews with the specified format
     */
    virtual bool UAVSupportsFormat(ERHIFormat Format) const { return false; }

    /**
     * @brief: retrieve the current API that is used
     * 
     * @return: Returns the current RHI's API
     */
    ERHIType GetType() const { return Type; }

protected:
    ERHIType Type;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper functions

FORCEINLINE CRHITexture2DRef RHICreateTexture2D(const CRHITexture2DInitializer& Initializer)
{
    return GRHIInstance->CreateTexture2D(Initializer);
}

FORCEINLINE CRHITexture2DArrayRef RHICreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer)
{
    return GRHIInstance->CreateTexture2DArray(Initializer);
}

FORCEINLINE CRHITextureCubeRef RHICreateTextureCube(const CRHITextureCubeInitializer& Initializer)
{
    return GRHIInstance->CreateTextureCube(Initializer);
}

FORCEINLINE CRHITexture3DRef RHICreateTexture3D(const CRHITexture3DInitializer& Initializer)
{
    return GRHIInstance->CreateTexture3D(Initializer);
}

FORCEINLINE CRHIVertexBufferRef RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
{
    return GRHIInstance->CreateVertexBuffer(Initializer);
}

FORCEINLINE CRHIIndexBufferRef RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
{
    return GRHIInstance->CreateIndexBuffer(Initializer);
}

FORCEINLINE CRHIGenericBufferRef RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
{
    return GRHIInstance->CreateGenericBuffer(Initializer);
}

FORCEINLINE CRHIConstantBufferRef RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
{
    return GRHIInstance->CreateConstantBuffer(Initializer);
}

FORCEINLINE CRHISamplerStateRef RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer)
{
    return GRHIInstance->CreateSamplerState(Initializer);
}

FORCEINLINE CRHIRayTracingSceneRef RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer)
{
    return GRHIInstance->CreateRayTracingScene(Initializer);
}

FORCEINLINE CRHIRayTracingGeometryRef RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer)
{
    return GRHIInstance->CreateRayTracingGeometry(Initializer);
}

FORCEINLINE CRHIShaderResourceViewRef RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer)
{
    return GRHIInstance->CreateShaderResourceView(Initializer);
}

FORCEINLINE CRHIShaderResourceViewRef RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer)
{
    return GRHIInstance->CreateShaderResourceView(Initializer);
}

FORCEINLINE CRHIUnorderedAccessViewRef RHICreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer)
{
    return GRHIInstance->CreateUnorderedAccessView(Initializer);
}

FORCEINLINE CRHIUnorderedAccessViewRef RHICreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer)
{
    return GRHIInstance->CreateUnorderedAccessView(Initializer);
}

FORCEINLINE CRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateComputeShader(ShaderCode);
}

FORCEINLINE CRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateVertexShader(ShaderCode);
}

FORCEINLINE CRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateHullShader(ShaderCode);
}

FORCEINLINE CRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateDomainShader(ShaderCode);
}

FORCEINLINE CRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateGeometryShader(ShaderCode);
}

FORCEINLINE CRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateMeshShader(ShaderCode);
}

FORCEINLINE CRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateAmplificationShader(ShaderCode);
}

FORCEINLINE CRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreatePixelShader(ShaderCode);
}

FORCEINLINE CRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateRayGenShader(ShaderCode);
}

FORCEINLINE CRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateRayAnyHitShader(ShaderCode);
}

FORCEINLINE CRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateRayClosestHitShader(ShaderCode);
}

FORCEINLINE CRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateRayMissShader(ShaderCode);
}

FORCEINLINE CRHIVertexInputLayout* RHICreateInputLayout(const CRHIVertexInputLayoutInitializer& Desc)
{
    return GRHIInstance->CreateInputLayout(Desc);
}

FORCEINLINE CRHIDepthStencilState* RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Desc)
{
    return GRHIInstance->CreateDepthStencilState(Desc);
}

FORCEINLINE CRHIRasterizerState* RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Desc)
{
    return GRHIInstance->CreateRasterizerState(Desc);
}

FORCEINLINE CRHIBlendState* RHICreateBlendState(const CRHIBlendStateInitializer& Desc)
{
    return GRHIInstance->CreateBlendState(Desc);
}

FORCEINLINE CRHIComputePipelineState* RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Desc)
{
    return GRHIInstance->CreateComputePipelineState(Desc);
}

FORCEINLINE CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const CRHIGraphicsPipelineStateInitializer& Desc)
{
    return GRHIInstance->CreateGraphicsPipelineState(Desc);
}

FORCEINLINE CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const CRHIRayTracingPipelineStateInitializer& Desc)
{
    return GRHIInstance->CreateRayTracingPipelineState(Desc);
}

FORCEINLINE CRHITimeQueryRef RHICreateTimeQuery()
{
    return GRHIInstance->CreateTimeQuery();
}

FORCEINLINE CRHIViewportRef RHICreateViewport(const CRHIViewportInitializer& Initializer)
{
    return GRHIInstance->CreateViewport(Initializer);
}

FORCEINLINE bool RHIUAVSupportsFormat(ERHIFormat Format)
{
    return GRHIInstance->UAVSupportsFormat(Format);
}

FORCEINLINE class IRHICommandContext* RHIGetDefaultCommandContext()
{
    return GRHIInstance->GetDefaultCommandContext();
}

FORCEINLINE String RHIGetAdapterName()
{
    return GRHIInstance->GetAdapterName();
}

FORCEINLINE void RHICheckShadingRateSupport(SShadingRateSupport& OutSupport)
{
    GRHIInstance->CheckShadingRateSupport(OutSupport);
}

FORCEINLINE void RHICheckRayTracingSupport(SRayTracingSupport& OutSupport)
{
    GRHIInstance->CheckRayTracingSupport(OutSupport);
}

FORCEINLINE bool RHISupportsRayTracing()
{
    SRayTracingSupport Support;
    RHICheckRayTracingSupport(Support);

    return (Support.Tier != ERayTracingTier::NotSupported);
}

FORCEINLINE bool RHISupportsVariableRateShading()
{
    SShadingRateSupport Support;
    RHICheckShadingRateSupport(Support);

    return (Support.Tier != EShadingRateTier::NotSupported);
}
