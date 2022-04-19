#pragma once
#include "RHITypes.h"
#include "RHIModule.h"
#include "RHIResources.h"
#include "RHIPipeline.h"
#include "RHICommandList.h"

#include "CoreApplication/Interface/PlatformWindow.h"

struct SResourceInitializer;
struct SRHIClearValue;
class CRHIRayTracingGeometry;
class CRHIRayTracingScene;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureInitializer

class CRHITextureInitializer
{
public:

    CRHITextureInitializer()
        : InitialAccess(EResourceAccess::Common)
    { }

    CTextureClearValue ClearValue;

    CInt16Vector3      Extent;

    ERHIFormat         Format;
    ETextureUsageFlags UsageFlags;
    
    uint8              NumMips;
    uint8              NumSamples;

    EResourceAccess    InitialAccess;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIInstance

class CRHIInstance
{
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
     * @param Format: Texture Format
     * @param UsageFlags: Texture usage flags
     * @param Width: Width of the Texture
     * @param Height: Height of the Texture
     * @param NumMips: Number of Texture MipLevels
     * @param NumSamples: Number of Texture Samples
     * @param ClearValue: Optimal ClearValue of the Texture
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITexture2DRef CreateTexture2D( ERHIFormat Format
                                            , ETextureUsageFlags UsageFlags
                                            , uint16 Width
                                            , uint16 Height
                                            , uint8 NumMips
                                            , uint8 NumSamples
                                            , const CTextureClearValue& ClearValue
                                            , EResourceAccess InitialState
                                            , const SResourceInitializer* InitalData) = 0;

    /**
     * @brief: Creates a Texture2DArray
     *
     * @param Format: Texture Format
     * @param UsageFlags: Texture usage flags
     * @param Width: Width of the Texture
     * @param Height: Height of the Texture
     * @param ArraySize: Number of slices in the Texture Array
     * @param NumMips: Number of Texture MipLevels
     * @param NumSamples: Number of Texture Samples
     * @param ClearValue: Optimal ClearValue of the Texture
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITexture2DArrayRef CreateTexture2DArray( ERHIFormat Format
                                                      , ETextureUsageFlags UsageFlags
                                                      , uint16 Width
                                                      , uint16 Height
                                                      , uint16 ArraySize
                                                      , uint8 NumMips
                                                      , uint8 NumSamples
                                                      , const CTextureClearValue& ClearValue
                                                      , EResourceAccess InitialState
                                                      , const SResourceInitializer* InitalData) = 0;

    /**
     * @brief: Creates a TextureCube
     *
     * @param Format: Texture Format
     * @param UsageFlags: Texture usage flags
     * @param Extent: Extent of one size of the TextureCube
     * @param ArraySize: Number of cubes in the Texture Cube
     * @param NumMips: Number of Texture MipLevels
     * @param NumSamples: Number of Texture Samples
     * @param ClearValue: Optimal ClearValue of the Texture
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITextureCubeRef CreateTextureCube( ERHIFormat Format
                                                , ETextureUsageFlags UsageFlags
                                                , uint16 Extent
                                                , uint16 ArraySize
                                                , uint8 NumMips
                                                , uint8 NumSamples
                                                , const CTextureClearValue& ClearValue
                                                , EResourceAccess InitialState
                                                , const SResourceInitializer* InitalData) = 0;

    /**
     * @brief: Creates a Texture3D
     *
     * @param Format: Texture Format
     * @param UsageFlags: Texture usage flags
     * @param Width: Width of the Texture
     * @param Height: Height of the Texture
     * @param Depth: Depth of the Texture
     * @param NumMips: Number of Texture MipLevels
     * @param ClearValue: Optimal ClearValue of the Texture
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITexture3DRef CreateTexture3D( ERHIFormat Format
                                            , ETextureUsageFlags UsageFlags
                                            , uint16 Width
                                            , uint16 Height
                                            , uint16 Depth
                                            , uint8 NumMips
                                            , const CTextureClearValue& ClearValue
                                            , EResourceAccess InitialState
                                            , const SResourceInitializer* InitalData) = 0;

    /**
     * @brief: Creates a Buffer
     * 
     * @param UsageFlags: UsageFlags of the Buffer
     * @param Size: Size of the Buffer
     * @param Stride: Stride of each element in the Buffer
     * @param InitialState: Initial ResurceState of the Buffer
     * @param InitialData: Initial data supplied to the Buffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIBufferRef CreateBuffer( EBufferUsageFlags UsageFlags
                                      , uint32 Size
                                      , uint32 Stride
                                      , EResourceAccess InitialState
                                      , const SResourceInitializer* InitalData) = 0;

    /**
     * @brief: Creates a Buffer
     *
     * @param UsageFlags: UsageFlags of the Buffer
     * @param Size: Size of the Buffer
     * @param Stride: Stride of each element in the Buffer
     * @param InitialState: Initial ResurceState of the Buffer
     * @param InitialData: Initial data supplied to the Buffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIConstantBufferRef CreateConstantBuffer( EBufferUsageFlags UsageFlags
                                                      , uint32 Size
                                                      , uint32 Stride
                                                      , EResourceAccess InitialState
                                                      , const SResourceInitializer* InitalData) = 0;

    /**
     * @brief: Create a SamplerState
     * 
     * @param CreateDesc: Structure with information about the SamplerState
     * @return: Returns the newly created SamplerState (Could be the same as a already created sampler state and a reference is added)
     */
    virtual CRHISamplerStateRef CreateSamplerState(const CRHISamplerStateInitializer& Initializer) = 0;

    /**
     * @brief: Create a new Ray tracing scene
     * 
     * @param Instances: Array of Geometry Instances to build the Scene from
     * @param NumInstances: Number of instances in the Array
     * @param Flags: Flags for the RayTracing scene
     * @return: Returns the newly created Ray tracing Scene
     */
    virtual CRHIRayTracingSceneRef CreateRayTracingScene(CRHIRayTracingGeometryInstance* Instances, uint32 NumInstances, ERayTracingInstanceFlag Flags) = 0;
    
    /**
     * @brief: Create a new Ray tracing geometry
     *
     * @param VertexBuffer: VertexBuffer for the Geometry-Structure
     * @param NumVertices: Number of vertices in the VertexBuffer
     * @param IndexBuffer: IndexBuffer for the Geometry-Structure
     * @param NumIndicies: Number of instances in the IndexBuffer
     * @param Flags: Flags for the GeometryStructure
     * @return: Returns the newly created Ray tracing Geometry
     */
    virtual CRHIRayTracingGeometryRef CreateRayTracingGeometry( CRHIBuffer* VertexBuffer
                                                              , uint32 NumVertices
                                                              , CRHIBuffer* Indexbuffer
                                                              , EIndexFormat IndexFormat
                                                              , uint32 NumIndicies
                                                              , ERayTracingStructureFlag Flags) = 0;

    /**
     * @brief: Create a new ShaderResourceView
     *
     * @param CreateDesc: Description for the new view to create
     * @return: Returns the newly created ShaderResourceView
     */
    virtual CRHIShaderResourceViewRef CreateShaderResourceView(const CRHITextureSRVInitializer& Initializer) = 0;

    /**
     * @brief: Create a new ShaderResourceView
     *
     * @param CreateDesc: Description for the new view to create
     * @return: Returns the newly created ShaderResourceView
     */
    virtual CRHIShaderResourceViewRef CreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer) = 0;
    
    /**
     * @brief: Create a new UnorderedAccessView for a texture
     *
     * @param CreateDesc: Description for the new view to create
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual CRHIUnorderedAccessViewRef CreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer) = 0;

    /**
     * @brief: Create a new UnorderedAccessView for a buffer
     *
     * @param CreateDesc: Description for the new view to create
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
    virtual class CRHIDepthStencilState* CreateDepthStencilState(const SDepthStencilStateDesc& Desc) = 0;

    /**
     * @brief: Create a new RasterizerState
     *
     * @param Desc: Info about a RasterizerState
     * @return: Returns the newly created RasterizerState
     */
    virtual class CRHIRasterizerState* CreateRasterizerState(const SRasterizerStateDesc& Desc) = 0;

    /**
     * @brief: Create a new BlendState
     *
     * @param Desc: Info about a BlendState
     * @return: Returns the newly created BlendState
     */
    virtual class CRHIBlendState* CreateBlendState(const SBlendStateDesc& Desc) = 0;

    /**
     * @brief: Create a new InputLayoutState
     *
     * @param Desc: Info about a InputLayoutState
     * @return: Returns the newly created InputLayoutState
     */
    virtual class CRHIVertexInputLayout* CreateInputLayout(const SVertexInputLayoutDesc& Desc) = 0;

    /**
     * @brief: Create a Graphics PipelineState
     * 
     * @param Desc: Info about the Graphics PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateCreateDesc& Desc) = 0;
    
    /**
     * @brief: Create a Compute PipelineState
     *
     * @param Desc: Info about the Compute PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIComputePipelineState* CreateComputePipelineState(const SRHIComputePipelineStateDesc& Desc) = 0;
    
    /**
     * @brief: Create a Ray-Tracing PipelineState
     *
     * @param Desc: Info about the Ray-Tracing PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateDesc& Desc) = 0;

    /**
     * @brief: Create a new Timestamp Query
     * 
     * @return: Returns the newly created Timestamp Query
     */
    virtual CRHITimeQueryRef CreateTimeQuery() = 0;

    /**
     * @brief: Create a new Viewport
     * 
     * @param Window: Window to bind to the viewport
     * @param Width: Width of the viewport
     * @param Height: Height of the viewport
     * @param ColorFormat: Format for the color
     * @param DepthFormat: Format for the depth
     * @return: Returns the newly created viewport
     */
    virtual CRHIViewportRef CreateViewport( PlatformWindowHandle WindowHandle
                                          , uint32 Width
                                          , uint32 Height
                                          , ERHIFormat ColorFormat
                                          , ERHIFormat DepthFormat) = 0;

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

    CRHIInstance(ERHIType InType)
        : Type(InType)
    { }

    virtual ~CRHIInstance() = default;

    ERHIType Type;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper functions

FORCEINLINE CRHITexture2DRef RHICreateTexture2D( ERHIFormat Format
                                               , ETextureUsageFlags UsageFlags
                                               , uint16 Width
                                               , uint16 Height
                                               , uint8 NumMips
                                               , uint8 NumSamples
                                               , const CTextureClearValue& ClearValue
                                               , EResourceAccess InitialState
                                               , const SResourceInitializer* InitalData)
{
    return GRHIInstance->CreateTexture2D(Format, UsageFlags, Width, Height, NumMips, NumSamples, ClearValue, InitialState, );
}

FORCEINLINE CRHIBufferRef RHICreateBuffer(const CRHIBufferCreateDesc& BufferDesc, EResourceAccess InitialState, const SResourceInitializer* InitialData)
{
    return GRHIInstance->CreateBuffer(BufferDesc, InitialState, InitialData);
}

FORCEINLINE CRHISamplerStateRef RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer)
{
    return GRHIInstance->CreateSamplerState(Initializer);
}

FORCEINLINE CRHIRayTracingSceneRef RHICreateRayTracingScene(const CRHIRayTracingSceneCreateDesc& SceneDesc)
{
    return GRHIInstance->CreateRayTracingScene(SceneDesc);
}

FORCEINLINE CRHIRayTracingGeometryRef RHICreateRayTracingGeometry(const CRHIRayTracingGeometryCreateDesc& GeometryDesc)
{
    return GRHIInstance->CreateRayTracingGeometry(GeometryDesc);
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

FORCEINLINE CRHIVertexInputLayout* RHICreateInputLayout(const SVertexInputLayoutDesc& Desc)
{
    return GRHIInstance->CreateInputLayout(Desc);
}

FORCEINLINE CRHIDepthStencilState* RHICreateDepthStencilState(const SDepthStencilStateDesc& Desc)
{
    return GRHIInstance->CreateDepthStencilState(Desc);
}

FORCEINLINE CRHIRasterizerState* RHICreateRasterizerState(const SRasterizerStateDesc& Desc)
{
    return GRHIInstance->CreateRasterizerState(Desc);
}

FORCEINLINE CRHIBlendState* RHICreateBlendState(const SBlendStateDesc& Desc)
{
    return GRHIInstance->CreateBlendState(Desc);
}

FORCEINLINE CRHIComputePipelineState* RHICreateComputePipelineState(const SRHIComputePipelineStateDesc& Desc)
{
    return GRHIInstance->CreateComputePipelineState(Desc);
}

FORCEINLINE CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const SRHIGraphicsPipelineStateCreateDesc& Desc)
{
    return GRHIInstance->CreateGraphicsPipelineState(Desc);
}

FORCEINLINE CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const SRHIRayTracingPipelineStateDesc& Desc)
{
    return GRHIInstance->CreateRayTracingPipelineState(Desc);
}

FORCEINLINE CRHITimeQueryRef RHICreateTimeQuery()
{
    return GRHIInstance->CreateTimeQuery();
}

FORCEINLINE CRHIViewportRef RHICreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, ERHIFormat ColorFormat, ERHIFormat DepthFormat)
{
    return GRHIInstance->CreateViewport(WindowHandle, Width, Height, ColorFormat, DepthFormat);
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
