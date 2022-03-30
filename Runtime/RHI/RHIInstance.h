#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHICommandList.h"
#include "RHIModule.h"
#include "RHISamplerState.h"
#include "RHIViewport.h"

#include "CoreApplication/Interface/PlatformWindow.h"

struct CRHIResourceData;
struct SRHIClearValue;
class CRHIRayTracingGeometry;
class CRHIRayTracingScene;

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
     * @brief: Creates a Texture
     *
     * @param TextureDesc: Texture description
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITextureRef CreateTexture(const SRHITextureCreateDesc& TextureDesc, EResourceAccess InitialState, const CRHIResourceData* InitalData) = 0;

    /**
     * @brief: Creates a new Buffer
     * 
     * @param BufferDesc: Buffer description
     * @param InitialState: Initial ResurceState of the Buffer
     * @param InitialData: Initial data supplied to the Buffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIBufferRef CreateBuffer(const SRHIBufferCreateDesc& BufferDesc, EResourceAccess InitialState, const CRHIResourceData* InitalData) = 0;

    /**
     * @brief: Create a SamplerState
     * 
     * @param Desc: Structure with information about the SamplerState
     * @return: Returns the newly created SamplerState (Could be the same as a already created sampler state and a reference is added)
     */
    virtual CRHISamplerStateRef CreateSamplerState(const struct SRHISamplerStateCreateDesc& Desc) = 0;

    /**
     * @brief: Create a new Ray tracing scene
     * 
     * @param Flags: Flags for the creation
     * @param Instances: Initial instances to create the acceleration structure with
     * @param NumInstances: Number of instances in the array
     * @return: Returns the newly created Ray tracing Scene
     */
    virtual CRHIRayTracingScene* CreateRayTracingScene(uint32 Flags, SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances) = 0;
    
    /**
     * @brief: Create a new Ray tracing geometry
     *
     * @param Flags: Flags for the creation
     * @param VertexBuffer: VertexBuffer to create the acceleration structure with
     * @param NumVertices: Number of vertices in the VertexBuffer
     * @param IndexBuffer: IndexBuffer to create the acceleration structure with
     * @param NumIndices: Number of indices in the IndexBuffer
     * @return: Returns the newly created Ray tracing Geometry
     */
    virtual CRHIRayTracingGeometry* CreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, uint32 NumVertices, ERHIIndexFormat IndexFormat, CRHIBuffer* IndexBuffer, uint32 NumIndices) = 0;

    /**
     * @brief: Create a new ShaderResourceView
     *
     * @return: Returns the newly created ShaderResourceView
     */
    virtual CRHIShaderResourceViewRef CreateShaderResourceView(CRHITexture* Texture, const SRHIShaderResourceViewDesc& Desc) = 0;

    /**
     * @brief: Create a new ShaderResourceView
     *
     * @return: Returns the newly created ShaderResourceView
     */
    virtual CRHIShaderResourceViewRef CreateShaderResourceView(CRHIBuffer* Buffer, uint32 FirstElement, uint32 NumElements, uint32 Stride) = 0;
    
    /**
     * @brief: Create a new UnorderedAccessView
     *
     * @param Desc: Info about the UnorderedAccessView
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual CRHIUnorderedAccessViewRef CreateUnorderedAccessView(CRHITexture* Texture, const SRHIUnorderedAccessViewDesc& Desc) = 0;
    
    /**
     * @brief: Create a new UnorderedAccessView
     *
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual CRHIUnorderedAccessViewRef CreateUnorderedAccessView(CRHIBuffer* Buffer, uint32 FirstElement, uint32 NumElements, uint32 Stride) = 0;

    /**
     * @brief: Create a new RenderTargetView
     *
     * @param Desc: Info about the RenderTargetView
     * @return: Returns the newly created RenderTargetView
     */
    virtual CRHIRenderTargetViewRef CreateRenderTargetView(CRHITexture* Texture, const SRHIRenderTargetViewDesc& Desc) = 0;
    
    /**
     * @brief: Create a new DepthStencilView
     *
     * @param Desc: Info about the DepthStencilView
     * @return: Returns the newly created DepthStencilView
     */
    virtual CRHIDepthStencilViewRef CreateDepthStencilView(CRHITexture* Texture, const SRHIDepthStencilViewDesc& Desc) = 0;

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
    virtual class CRHIDepthStencilState* CreateDepthStencilState(const SRHIDepthStencilStateDesc& Desc) = 0;

    /**
     * @brief: Create a new RasterizerState
     *
     * @param Desc: Info about a RasterizerState
     * @return: Returns the newly created RasterizerState
     */
    virtual class CRHIRasterizerState* CreateRasterizerState(const SRHIRasterizerStateDesc& Desc) = 0;

    /**
     * @brief: Create a new BlendState
     *
     * @param Desc: Info about a BlendState
     * @return: Returns the newly created BlendState
     */
    virtual class CRHIBlendState* CreateBlendState(const SRHIBlendStateDesc& Desc) = 0;

    /**
     * @brief: Create a new InputLayoutState
     *
     * @param Desc: Info about a InputLayoutState
     * @return: Returns the newly created InputLayoutState
     */
    virtual class CRHIInputLayoutState* CreateInputLayout(const SRHIInputLayoutStateDesc& Desc) = 0;

    /**
     * @brief: Create a Graphics PipelineState
     * 
     * @param Desc: Info about the Graphics PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateDesc& Desc) = 0;
    
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
    virtual class CRHITimestampQuery* CreateTimestampQuery() = 0;


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
    virtual CRHIViewportRef CreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, ERHIFormat ColorFormat, ERHIFormat DepthFormat) = 0;

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
    virtual void CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const = 0;

    /**
     * @brief: Check for Shading-rate support
     *
     * @param OutSupport: Struct containing the Shading-rate support for the system and current RHI
     */
    virtual void CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const = 0;

    /**
     * @brief: Check if the current RHI supports UnorderedAccessViews for the specified format
     * 
     * @param Format: Format to check
     * @return: Returns true if the current RHI supports UnorderedAccessViews with the specified format
     */
    virtual bool UAVSupportsFormat(ERHIFormat Format) const
    {
        UNREFERENCED_VARIABLE(Format);
        return false;
    }

    /**
     * @brief: retrieve the current API that is used
     * 
     * @return: Returns the current RHI's API
     */
    FORCEINLINE ERHIType GetApi() const { return CurrentRHI; }

protected:

    CRHIInstance(ERHIType InCurrentRHI)
        : CurrentRHI(InCurrentRHI)
    { }

    virtual ~CRHIInstance() = default;

    ERHIType CurrentRHI;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper functions

FORCEINLINE CRHITextureRef RHICreateTexture(const SRHITextureCreateDesc& TextureDesc, EResourceAccess InitialState, const CRHIResourceData* InitialData = nullptr)
{
    return GRHIInstance->CreateTexture(TextureDesc, InitialState, InitialData);
}

FORCEINLINE CRHIBufferRef RHICreateBuffer(const SRHIBufferCreateDesc& BufferDesc, EResourceAccess InitialState, const CRHIResourceData* InitialData)
{
    return GRHIInstance->CreateBuffer(BufferDesc, InitialState, InitialData);
}

FORCEINLINE CRHIConstantBufferRef RHICreateConstantBuffer(uint32 Size, uint32 Flags, const CRHIResourceData* InitialData)
{
    return GRHIInstance->CreateConstantBuffer(Size, Flags, InitialData);
}

FORCEINLINE CRHISamplerStateRef RHICreateSamplerState(const class SRHISamplerStateCreateDesc& Desc)
{
    return GRHIInstance->CreateSamplerState(Desc);
}

FORCEINLINE CRHIRayTracingScene* RHICreateRayTracingScene(uint32 Flags, SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances)
{
    return GRHIInstance->CreateRayTracingScene(Flags, Instances, NumInstances);
}

FORCEINLINE CRHIRayTracingGeometry* RHICreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, uint32 NumVertices, ERHIIndexFormat IndexFormat, CRHIBuffer* IndexBuffer, uint32 NumIndices)
{
    return GRHIInstance->CreateRayTracingGeometry(Flags, VertexBuffer, NumVertices, IndexFormat, IndexBuffer, NumIndices);
}

FORCEINLINE CRHIShaderResourceViewRef RHICreateShaderResourceView(CRHITexture* Texture, const SRHIShaderResourceViewDesc& Desc)
{
    return GRHIInstance->CreateShaderResourceView(Texture, Desc);
}

FORCEINLINE CRHIShaderResourceViewRef RHICreateShaderResourceView(CRHIBuffer* Buffer, uint32 FirstElement, uint32 NumElements, uint32 Stride)
{
    return GRHIInstance->CreateShaderResourceView(Buffer, FirstElement, NumElements, Stride);
}

FORCEINLINE CRHIUnorderedAccessViewRef RHICreateUnorderedAccessView(CRHITexture* Texture, const SRHIUnorderedAccessViewDesc& Desc)
{
    return GRHIInstance->CreateUnorderedAccessView(Texture, Desc);
}

FORCEINLINE CRHIUnorderedAccessViewRef RHICreateUnorderedAccessView(CRHIBuffer* Buffer, uint32 FirstElement, uint32 NumElements, uint32 Stride)
{
    return GRHIInstance->CreateUnorderedAccessView(Buffer, FirstElement, NumElements, Stride);
}

FORCEINLINE CRHIRenderTargetViewRef RHICreateRenderTargetView(CRHITexture* Texture, const SRHIRenderTargetViewDesc& Desc)
{
    return GRHIInstance->CreateRenderTargetView(Texture, Desc);
}

FORCEINLINE CRHIDepthStencilViewRef RHICreateDepthStencilView(CRHITexture* Texture, const SRHIDepthStencilViewDesc& Desc)
{
    return GRHIInstance->CreateDepthStencilView(Texture, Desc);
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

FORCEINLINE CRHIInputLayoutState* RHICreateInputLayout(const SRHIInputLayoutStateDesc& Desc)
{
    return GRHIInstance->CreateInputLayout(Desc);
}

FORCEINLINE CRHIDepthStencilState* RHICreateDepthStencilState(const SRHIDepthStencilStateDesc& Desc)
{
    return GRHIInstance->CreateDepthStencilState(Desc);
}

FORCEINLINE CRHIRasterizerState* RHICreateRasterizerState(const SRHIRasterizerStateDesc& Desc)
{
    return GRHIInstance->CreateRasterizerState(Desc);
}

FORCEINLINE CRHIBlendState* RHICreateBlendState(const SRHIBlendStateDesc& Desc)
{
    return GRHIInstance->CreateBlendState(Desc);
}

FORCEINLINE CRHIComputePipelineState* RHICreateComputePipelineState(const SRHIComputePipelineStateDesc& Desc)
{
    return GRHIInstance->CreateComputePipelineState(Desc);
}

FORCEINLINE CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const SRHIGraphicsPipelineStateDesc& Desc)
{
    return GRHIInstance->CreateGraphicsPipelineState(Desc);
}

FORCEINLINE CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const SRHIRayTracingPipelineStateDesc& Desc)
{
    return GRHIInstance->CreateRayTracingPipelineState(Desc);
}

FORCEINLINE class CRHITimestampQuery* RHICreateTimestampQuery()
{
    return GRHIInstance->CreateTimestampQuery();
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

FORCEINLINE void RHICheckShadingRateSupport(SRHIShadingRateSupport& OutSupport)
{
    GRHIInstance->CheckShadingRateSupport(OutSupport);
}

FORCEINLINE void RHICheckRayTracingSupport(SRHIRayTracingSupport& OutSupport)
{
    GRHIInstance->CheckRayTracingSupport(OutSupport);
}

FORCEINLINE bool RHISupportsRayTracing()
{
    SRHIRayTracingSupport Support;
    RHICheckRayTracingSupport(Support);

    return (Support.Tier != ERayTracingTier::NotSupported);
}

FORCEINLINE bool RHISupportsVariableRateShading()
{
    SRHIShadingRateSupport Support;
    RHICheckShadingRateSupport(Support);

    return (Support.Tier != EShadingRateTier::NotSupported);
}
