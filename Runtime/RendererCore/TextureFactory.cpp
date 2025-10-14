#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "RHI/ShaderCompiler.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/TextureResourceData.h"

FTextureFactory* FTextureFactory::GTextureFactory = nullptr;

FTextureFactory::FTextureFactory()
    : TextureCompressor()
    , LinearSampler(nullptr)
    , PanoramaPSO(nullptr)
    , PanoramCS(nullptr)
    , GenerateMipsTex2D_PSO(nullptr)
    , GenerateMipsTex2D_CS(nullptr)
    , GenerateMipsTexCube_PSO(nullptr)
    , GenerateMipsTexCube_CS(nullptr)
{
}

FTextureFactory::~FTextureFactory()
{
    // Samplers
    LinearSampler.Reset();

    // Panorama
    PanoramaPSO.Reset();
    PanoramCS.Reset();

    // GenerateMips Texture2D
    GenerateMipsTex2D_PSO.Reset();
    GenerateMipsTex2D_CS.Reset();

    // GenerateMips TextureCube
    GenerateMipsTexCube_PSO.Reset();
    GenerateMipsTexCube_CS.Reset();

    // Specular IrradianceGen
    SpecularCubeMapFilter_PSO.Reset();
    SpecularCubeMapFilter_CS.Reset();

    // Diffuse IrradianceGen
    DiffuseCubeMapFilter_PSO.Reset();
    DiffuseCubeMapFilter_CS.Reset();
}

bool FTextureFactory::Initialize()
{
    GTextureFactory = new FTextureFactory();
    if (!GTextureFactory->CreateResources())
    {
        return false;
    }

    return true;
}

void FTextureFactory::Release()
{
    SAFE_DELETE(GTextureFactory);
}

bool FTextureFactory::CreateResources()
{
    if (!TextureCompressor.Initialize())
    {
        return false;
    }

    // Compile and create shader
    TArray<uint8> Code;

    // Compile "Cube-Map from Panorama" shader
    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/CubeMapGen.hlsl", CompileInfo, Code))
    {
        return false;
    }

    PanoramCS = FRHI::Get()->CreateComputeShader(Code);
    if (!PanoramCS)
    {
        return false;
    }

    // Create "Cube-Map from Panorama" pipeline
    FRHIComputePipelineStateInfo PanoramaPSOInfo;
    PanoramaPSOInfo.Shader = PanoramCS.Get();

    PanoramaPSO = FRHI::Get()->CreateComputePipelineState(PanoramaPSOInfo);
    if (PanoramaPSO)
    {
        PanoramaPSO->SetDebugName("Generate CubeMap RootSignature");
    }
    else
    {
        return false;
    }

    // Compile "GenerateMips Texure2D" shader
    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/GenerateMipsTex2D.hlsl", CompileInfo, Code))
    {
        return false;
    }

    GenerateMipsTex2D_CS = FRHI::Get()->CreateComputeShader(Code);
    if (!GenerateMipsTex2D_CS)
    {
        return false;
    }

    // Create "GenerateMips Texure2D" pipeline
	FRHIComputePipelineStateInfo GenerateMipsTex2D_PSOInfo;
    GenerateMipsTex2D_PSOInfo.Shader = GenerateMipsTex2D_CS.Get();

    GenerateMipsTex2D_PSO = FRHI::Get()->CreateComputePipelineState(GenerateMipsTex2D_PSOInfo);
    if (GenerateMipsTex2D_PSO)
    {
        GenerateMipsTex2D_PSO->SetDebugName("GenerateMips Texure2D PSO");
    }
    else
    {
        return false;
    }

    // Compile "GenerateMips TexureCube" shader
    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/GenerateMipsTexCube.hlsl", CompileInfo, Code))
    {
        return false;
    }

    GenerateMipsTexCube_CS = FRHI::Get()->CreateComputeShader(Code);
    if (!GenerateMipsTexCube_CS)
    {
        return false;
    }

    // Create "GenerateMips TexureCube" pipeline
	FRHIComputePipelineStateInfo GenerateMipsTexCube_PSOInfo;
    GenerateMipsTexCube_PSOInfo.Shader = GenerateMipsTexCube_CS.Get();

    GenerateMipsTexCube_PSO = FRHI::Get()->CreateComputePipelineState(GenerateMipsTexCube_PSOInfo);
    if (GenerateMipsTexCube_PSO)
    {
        GenerateMipsTexCube_PSO->SetDebugName("GenerateMips TexureCube PSO");
    }
    else
    {
        return false;
    }

    // Create "Diffuse cube-map filter" pipeline
    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/IrradianceGen.hlsl", CompileInfo, Code))
    {
        LOG_ERROR("Failed to compile IrradianceGen Shader");
    }

    DiffuseCubeMapFilter_CS = FRHI::Get()->CreateComputeShader(Code);
    if (!DiffuseCubeMapFilter_CS)
    {
        LOG_ERROR("Failed to create IrradianceGen Shader");
    }

	FRHIComputePipelineStateInfo DiffuseCubeMapFilter_PSOInfo;
    DiffuseCubeMapFilter_PSOInfo.Shader = DiffuseCubeMapFilter_CS.Get();

    DiffuseCubeMapFilter_PSO = FRHI::Get()->CreateComputePipelineState(DiffuseCubeMapFilter_PSOInfo);
    if (!DiffuseCubeMapFilter_PSO)
    {
        LOG_ERROR("Failed to create IrradianceGen PipelineState");
    }
    else
    {
        DiffuseCubeMapFilter_PSO->SetDebugName("Diffuse cube-map filter PSO");
    }

    // Create "Specular cube-map filter" pipeline
    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/SpecularIrradianceGen.hlsl", CompileInfo, Code))
    {
        LOG_ERROR("Failed to compile SpecularIrradianceGen Shader");
    }

    SpecularCubeMapFilter_CS = FRHI::Get()->CreateComputeShader(Code);
    if (!SpecularCubeMapFilter_CS)
    {
        LOG_ERROR("Failed to create Specular IrradianceGen Shader");
    }

	FRHIComputePipelineStateInfo SpecularCubeMapFilter_PSOInfo;
    SpecularCubeMapFilter_PSOInfo.Shader = SpecularCubeMapFilter_CS.Get();

    SpecularCubeMapFilter_PSO = FRHI::Get()->CreateComputePipelineState(SpecularCubeMapFilter_PSOInfo);
    if (!SpecularCubeMapFilter_PSO)
    {
        LOG_ERROR("Failed to create Specular IrradianceGen PipelineState");
    }
    else
    {
        SpecularCubeMapFilter_PSO->SetDebugName("Specular cube-map filter PSO");
    }

    // Sampler
    FRHISamplerStateInfo LinearSamplerInfo;
    LinearSamplerInfo.AddressU = ESamplerMode::Wrap;
    LinearSamplerInfo.AddressV = ESamplerMode::Wrap;
    LinearSamplerInfo.AddressW = ESamplerMode::Wrap;
    LinearSamplerInfo.Filter   = ESamplerFilter::MinMagMipLinear;
    LinearSamplerInfo.MinLOD   = 0.0f;
    LinearSamplerInfo.MaxLOD   = TNumericLimits<float>::Max();

    LinearSampler = FRHI::Get()->CreateSamplerState(LinearSamplerInfo);
    if (!LinearSampler)
    {
        return false;
    }

    FRHISamplerStateInfo CubeMapFilterSamplerInfo;
    CubeMapFilterSamplerInfo.AddressU = ESamplerMode::Wrap;
    CubeMapFilterSamplerInfo.AddressV = ESamplerMode::Wrap;
    CubeMapFilterSamplerInfo.AddressW = ESamplerMode::Wrap;
    CubeMapFilterSamplerInfo.Filter   = ESamplerFilter::MinMagMipLinear;

    CubeMapFilterSampler = FRHI::Get()->CreateSamplerState(CubeMapFilterSamplerInfo);
    if (!CubeMapFilterSampler)
    {
        return false;
    }

    return true;
}

FRHITexture* FTextureFactory::LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, ETextureFactoryFlags Flags, EFormat Format)
{
    CHECK(Pixels != nullptr);

    const bool bGenerateMips = IsEnumFlagSet(Flags, ETextureFactoryFlags::GenerateMips);

    const uint32 NumMiplevels = bGenerateMips ? FTextureFactoryHelpers::TextureSizeToMiplevels(Math::Max<uint32>(Width, Height)) : 1u;
    CHECK(NumMiplevels != 0);

    const uint32 Stride   = GetByteStrideFromFormat(Format);
    const uint32 RowPitch = Width * Stride;
    CHECK(RowPitch > 0);

    FTextureResourceData InitalData;
    InitalData.InitMipData(Pixels, RowPitch, RowPitch * Height);

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(Format, Width, Height, NumMiplevels, 1, ETextureUsageFlags::ShaderResourceTexture);
    FRHITextureRef Texture = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::PixelShaderResource, &InitalData);
    if (!Texture)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    if (bGenerateMips && NumMiplevels > 1)
    {
        GenerateMiplevels(Texture.Get());
    }

    return Texture.ReleaseOwnership();
}

bool FTextureFactory::TextureCubeFromPanorma(FRHITexture* Source, FRHITexture* Dest, ETextureFactoryFlags Flags)
{
    CHECK(IsTextureCube(Dest->GetDimension()));
    CHECK(IsEnumFlagSet(Source->GetFlags(), ETextureUsageFlags::ShaderResourceTexture));
    CHECK(IsEnumFlagSet(Dest->GetFlags(), ETextureUsageFlags::ShaderResourceTexture));

    const bool bGenerateMips   = IsEnumFlagSet(Flags, ETextureFactoryFlags::GenerateMips);
    const bool bDestSupportUAV = IsEnumFlagSet(Dest->GetFlags(), ETextureUsageFlags::UnorderedAccessTexture);

    // If the destination does not support UAVs, create a staging texture that does
    FRHITextureRef StagingTexture;
    if (!bDestSupportUAV)
    {
        FRHITextureInfo TextureInfo = Dest->GetInfo();
        TextureInfo.UsageFlags |= ETextureUsageFlags::UnorderedAccessTexture;

        StagingTexture = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::Common, nullptr);
        if (!StagingTexture)
        {
            return false;
        }
        else
        {
            StagingTexture->SetDebugName("TextureCube From Panorama StagingTexture");
        }
    }
    else
    {
        StagingTexture = MakeSharedRef<FRHITexture>(Dest);
    }

    // Create UAV for the staging-texture
    FRHITextureUAVInfo UAVInfo(StagingTexture.Get(), StagingTexture->GetFormat(), 0, 0, 1);

    FRHIUnorderedAccessViewRef StagingTextureUAV = FRHI::Get()->CreateUnorderedAccessView(UAVInfo);
    if (!StagingTextureUAV)
    {
        return false;
    }

    // Schedule work on the GPU
    {
        FRHICommandList CommandList;
        CommandList.TransitionTexture(Source, FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));
        CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::UnorderedAccess));

        CommandList.SetComputePipelineState(PanoramaPSO.Get());

        struct FCubeMapGenConstants
        {
            uint32 CubeMapSize;
        } ShaderConstantData;

        ShaderConstantData.CubeMapSize = StagingTexture->GetExtent().X;

        CommandList.Set32BitShaderConstants(PanoramCS.Get(), &ShaderConstantData, 1);
        CommandList.SetUnorderedAccessView(PanoramCS.Get(), StagingTextureUAV.Get(), 0);

        FRHIShaderResourceView* PanoramaSourceView = Source->GetShaderResourceView();
        CommandList.SetShaderResourceView(PanoramCS.Get(), PanoramaSourceView, 0);

        CommandList.SetSamplerState(PanoramCS.Get(), LinearSampler.Get(), 0);

        constexpr uint32 LocalWorkGroupCount = 16;
        const uint32 ThreadsX = Math::DivideByMultiple(ShaderConstantData.CubeMapSize, LocalWorkGroupCount);
        const uint32 ThreadsY = Math::DivideByMultiple(ShaderConstantData.CubeMapSize, LocalWorkGroupCount);
        CommandList.Dispatch(ThreadsX, ThreadsY, 6);

        CommandList.TransitionTexture(Source, FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));

        if (!bDestSupportUAV)
        {
            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));
            CommandList.TransitionTexture(Dest, FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::CopyDest));

            CommandList.CopyTexture(Dest, StagingTexture.Get());

            CommandList.TransitionTexture(Dest, FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::PixelShaderResource));
        }
        else
        {
            CommandList.TransitionTexture(Dest, FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource));
        }

        if (bGenerateMips)
        {
            GenerateMiplevels(CommandList, Dest);
        }

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    }

    return true;
}

bool FTextureFactory::GenerateMiplevels(FRHITexture* Texture)
{
    // Schedule miplevel generation without an existing CommandList
    FRHICommandList CommandList;

    const bool bResult = GenerateMiplevels(CommandList, Texture);
    if (!bResult)
    {
        return false;
    }

    // Then execute immediately
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureFactory::GenerateMiplevels(FRHICommandList& CommandList, FRHITexture* Texture)
{
    CHECK(IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::ShaderResourceTexture));

    if (Texture->GetNumMipLevels() < 2)
    {
        LOG_ERROR("Texture needs to have a full mip-chain allocated");
        return false;
    }

    // Determine if we need a staging resource
    const bool bIsTextureCube  = IsTextureCube(Texture->GetDimension());
    const bool bDestSupportUAV = IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::UnorderedAccessTexture);

    // If the destination does not support UAVs, create a staging texture that does
    FRHITextureRef StagingTexture;
    if (!bDestSupportUAV)
    {
        FRHITextureInfo TextureInfo = Texture->GetInfo();
        TextureInfo.UsageFlags |= ETextureUsageFlags::UnorderedAccessTexture;

        StagingTexture = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::Common, nullptr);
        if (!StagingTexture)
        {
            return false;
        }
        else
        {
            StagingTexture->SetDebugName("GenerateMiplevels StagingTexture");
        }
    }
    else
    {
        StagingTexture = MakeSharedRef<FRHITexture>(Texture);
    }

    // Calculate how many compute-dispatches that we need
    constexpr uint32 MipLevelsPerDispatch = 4;
    const uint32 NumMipLevels  = StagingTexture->GetNumMipLevels();
    const uint32 NumDispatches = Math::AlignUp<uint32>(NumMipLevels, MipLevelsPerDispatch) / MipLevelsPerDispatch;

    // Create a SRV for source mips
    FRHITextureSRVInfo SRVInfo;
    SRVInfo.Texture         = StagingTexture.Get();
    SRVInfo.Format          = StagingTexture->GetFormat();
    SRVInfo.FirstArraySlice = 0;
    SRVInfo.NumSlices       = 1;
    SRVInfo.MinLODClamp     = 0;
    SRVInfo.NumMips         = 1;

    TArray<FRHIShaderResourceViewRef> ShaderResourceViews;
    ShaderResourceViews.Reserve(NumDispatches);

    for (uint32 MipLevel = 0; MipLevel < NumMipLevels; MipLevel += MipLevelsPerDispatch)
    {
        SRVInfo.FirstMipLevel = static_cast<uint8>(MipLevel);

        FRHIShaderResourceView* ShaderResourceView = FRHI::Get()->CreateShaderResourceView(SRVInfo);
        ShaderResourceViews.Emplace(ShaderResourceView);
    }

    // Create UAV for each miplevel
    FRHITextureUAVInfo UAVInfo;
    UAVInfo.Texture         = StagingTexture.Get();
    UAVInfo.Format          = StagingTexture->GetFormat();
    UAVInfo.FirstArraySlice = 0;
    UAVInfo.NumSlices       = 1;

    // Skip the first mip since that will only be used as a source
    TArray<FRHIUnorderedAccessViewRef> UnorderedAccessViews;
    UnorderedAccessViews.Reserve(NumMipLevels - 1);

    for (uint32 MipLevel = 1; MipLevel < NumMipLevels; MipLevel++)
    {
        UAVInfo.MipLevel = static_cast<uint8>(MipLevel);

        FRHIUnorderedAccessView* UnorderedAccessView = FRHI::Get()->CreateUnorderedAccessView(UAVInfo);
        UnorderedAccessViews.Emplace(UnorderedAccessView);
    }

    // Copy the texture over to the staging-resource
    if (!bDestSupportUAV)
    {
        CommandList.TransitionTexture(Texture, FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::CopySource));
        CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::CopyDest));

        CommandList.CopyTexture(StagingTexture.Get(), Texture);

        CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::NonPixelShaderResource));
    }
    else
    {
        CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));
    }

    // Determine which compute-shader and pipeline-state to use
    FRHIComputeShaderRef        ComputeShader = bIsTextureCube ? GenerateMipsTexCube_CS  : GenerateMipsTex2D_CS;
    FRHIComputePipelineStateRef PipelineState = bIsTextureCube ? GenerateMipsTexCube_PSO : GenerateMipsTex2D_PSO;
    CommandList.SetComputePipelineState(PipelineState.Get());

    struct FGenMipsConstants
    {
        uint32   SrcMipLevel;  // MipLevel to read from
        uint32   NumMipLevels; // Number of MipLevels we want to create (Up to 4)
        FVector2 TexelSize;    // Size of the first destination miplevel
    } ShaderConstantData;

    // Start with size of the destination to be divided by 2 since we are starting with mip 1
    const FIntVector3 TextureExtent = StagingTexture->GetExtent();
    uint32 DstWidth  = static_cast<uint32>(TextureExtent.X) / 2;
    uint32 DstHeight = static_cast<uint32>(TextureExtent.Y) / 2;
    ShaderConstantData.SrcMipLevel = 0;

    constexpr uint32 NumThreadsTexture2D   = 1;
    constexpr uint32 NumThreadsTextureCube = 6;
    const uint32 ThreadsZ = bIsTextureCube ? NumThreadsTextureCube : NumThreadsTexture2D;

    // First miplevel is already finished and will be used as source
    uint32 RemainingMiplevels = NumMipLevels - 1;
    for (uint32 DispatchIndex = 0; DispatchIndex < NumDispatches; DispatchIndex++)
    {
        ShaderConstantData.TexelSize = FVector2(1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight));

        const uint32 NumMipLevelsThisBatch = Math::Min<uint32>(MipLevelsPerDispatch, RemainingMiplevels);
        ShaderConstantData.NumMipLevels = NumMipLevelsThisBatch;

        constexpr uint32 NumConstants = sizeof(FGenMipsConstants) / sizeof(uint32);
        CommandList.Set32BitShaderConstants(ComputeShader.Get(), &ShaderConstantData, NumConstants);

        // Bind the original texture
        CommandList.SetShaderResourceView(ComputeShader.Get(), ShaderResourceViews[DispatchIndex].Get(), 0);

        // This is the first miplevel to process, the first miplevel is skipped since that is just used for reading
        const uint32 FirstMipLevelThisBatch = DispatchIndex * MipLevelsPerDispatch;

        // Bind all the UAVs that needs processing this batch
        for (uint32 MipIndex = 0; MipIndex < NumMipLevelsThisBatch; MipIndex++)
        {
            const uint32 CurrentMipLevel = FirstMipLevelThisBatch + MipIndex;
            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::MakePartial(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess, CurrentMipLevel + 1));
            CommandList.SetUnorderedAccessView(ComputeShader.Get(), UnorderedAccessViews[CurrentMipLevel].Get(), MipIndex);
        }

        // Bind null UAVs for all other bindings
        for (uint32 MipIndex = NumMipLevelsThisBatch; MipIndex < MipLevelsPerDispatch; MipIndex++)
        {
            CommandList.SetUnorderedAccessView(ComputeShader.Get(), nullptr, MipIndex);
        }

        // Dispatch work
        constexpr uint32 ThreadCount = 8;
        const uint32 ThreadsX = Math::DivideByMultiple(DstWidth, ThreadCount);
        const uint32 ThreadsY = Math::DivideByMultiple(DstHeight, ThreadCount);
        CommandList.Dispatch(ThreadsX, ThreadsY, ThreadsZ);

        // Transition all resources back
        for (uint32 MipIndex = 0; MipIndex < NumMipLevelsThisBatch; MipIndex++)
        {
            const uint32 CurrentMipLevel = FirstMipLevelThisBatch + MipIndex;
            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::MakePartial(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource, CurrentMipLevel + 1));
        }

        // Bind null UAVs for all other bindings
        for (uint32 MipIndex = NumMipLevelsThisBatch; MipIndex < MipLevelsPerDispatch; MipIndex++)
        {
            CommandList.SetUnorderedAccessView(ComputeShader.Get(), nullptr, MipIndex);
        }

        DstWidth  = DstWidth / 16;
        DstHeight = DstHeight / 16;

        RemainingMiplevels -= MipLevelsPerDispatch;
    }

    // Copy the staging-resource to the texture
    if (!bDestSupportUAV)
    {
        CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::CopySource));
        CommandList.TransitionTexture(Texture, FRHITextureTransition::Make(EResourceAccess::CopySource, EResourceAccess::CopyDest));

        CommandList.CopyTexture(Texture, StagingTexture.Get());

        CommandList.TransitionTexture(Texture, FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
    }
    else
    {
        CommandList.TransitionTexture(Texture, FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));
    }

    return true;
}

bool FTextureFactory::FilterSpecularCubeMap(FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap, uint32 NumMipLevels)
{
    // Schedule miplevel generation without an existing CommandList
    FRHICommandList CommandList;

    const bool bResult = FilterSpecularCubeMap(CommandList, SrcCubeMap, DstCubeMap, NumMipLevels);
    if (!bResult)
    {
        return false;
    }

    // Then execute immediately
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureFactory::FilterSpecularCubeMap(FRHICommandList& CommandList, FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap, uint32 NumMipLevels)
{
    if (!SrcCubeMap)
    {
        DEBUG_BREAK();
        return false;
    }

    if (!DstCubeMap)
    {
        DEBUG_BREAK();
        return false;
    }

    TArray<FRHIUnorderedAccessViewRef> SpecularIrradianceMapUAVs;

    const int32 SpecularIrradianceMiplevels = DstCubeMap->GetNumMipLevels();
    for (int32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++)
    {
        FRHITextureUAVInfo UAVInfo = FRHITextureUAVInfo(DstCubeMap, DstCubeMap->GetFormat(), MipLevel, 0, 1);
        FRHIUnorderedAccessViewRef UAV = FRHI::Get()->CreateUnorderedAccessView(UAVInfo);
        if (UAV)
        {
            SpecularIrradianceMapUAVs.Emplace(UAV);
        }
        else
        {
            DEBUG_BREAK();
            return false;
        }
    }

    CommandList.TransitionTexture(SrcCubeMap, FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));
    CommandList.TransitionTexture(DstCubeMap, FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::UnorderedAccess));

    CommandList.SetComputePipelineState(SpecularCubeMapFilter_PSO.Get());
    
    CommandList.SetShaderResourceView(SpecularCubeMapFilter_CS.Get(), SrcCubeMap->GetShaderResourceView(), 0);
    CommandList.SetSamplerState(SpecularCubeMapFilter_CS.Get(), CubeMapFilterSampler.Get(), 0);

    const uint32 SpecularCubeMapSize = DstCubeMap->GetWidth();
    const uint32 NumMiplevels        = Math::Clamp<uint32>(DstCubeMap->GetNumMipLevels(), 1, NumMipLevels);
    const uint32 SkyboxWidth         = SrcCubeMap->GetWidth();
    const float  RoughnessDelta      = 1.0f / (NumMiplevels - 1);

    float  Roughness    = 0.0f;
    uint32 CurrentWidth = SpecularCubeMapSize;
    for (uint32 Mip = 0; Mip < NumMiplevels; Mip++)
    {
        struct FSpecularIrradianceGenConstants
        {
            float  Roughness;
            uint32 SourceFaceResolution;
            uint32 CurrentFaceResolution;
        } Constants;

        Constants.Roughness             = Roughness;
        Constants.SourceFaceResolution  = SkyboxWidth;
        Constants.CurrentFaceResolution = CurrentWidth;

        constexpr uint32 NumConstants = sizeof(FSpecularIrradianceGenConstants) / sizeof(uint32);
        CommandList.Set32BitShaderConstants(SpecularCubeMapFilter_CS.Get(), &Constants, NumConstants);

        FRHIUnorderedAccessView* UnorderedAccessView = SpecularIrradianceMapUAVs[Mip].Get();
        CommandList.SetUnorderedAccessView(SpecularCubeMapFilter_CS.Get(), UnorderedAccessView, 0);

        constexpr uint32 NumThreads = 16;
        constexpr uint32 ThreadsZ   = 6;

        const uint32 ThreadWidth  = Math::DivideByMultiple(CurrentWidth, NumThreads);
        const uint32 ThreadHeight = Math::DivideByMultiple(CurrentWidth, NumThreads);
        CommandList.Dispatch(ThreadWidth, ThreadHeight, ThreadsZ);

        CommandList.UnorderedAccessTextureBarrier(DstCubeMap);

        CurrentWidth = Math::Max<uint32>(CurrentWidth / 2, 1U);
        Roughness += RoughnessDelta;
    }

    CommandList.TransitionTexture(SrcCubeMap, FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));
    CommandList.TransitionTexture(DstCubeMap, FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource));
    return true;
}

bool FTextureFactory::FilterDiffuseCubeMap(FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap)
{
    // Schedule miplevel generation without an existing CommandList
    FRHICommandList CommandList;

    const bool bResult = FilterDiffuseCubeMap(CommandList, SrcCubeMap, DstCubeMap);
    if (!bResult)
    {
        return false;
    }

    // Then execute immediately
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureFactory::FilterDiffuseCubeMap(FRHICommandList& CommandList, FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap)
{
    if (!SrcCubeMap)
    {
        DEBUG_BREAK();
        return false;
    }

    if (!DstCubeMap)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHITextureUAVInfo UAVInfo(DstCubeMap, DstCubeMap->GetFormat(), 0, 0, 1);
    FRHIUnorderedAccessViewRef DstCubeMapUAV = FRHI::Get()->CreateUnorderedAccessView(UAVInfo);
    if (!DstCubeMapUAV)
    {
        DEBUG_BREAK();
        return false;
    }

    CommandList.TransitionTexture(SrcCubeMap, FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));
    CommandList.TransitionTexture(DstCubeMap, FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::UnorderedAccess));

    CommandList.SetComputePipelineState(DiffuseCubeMapFilter_PSO.Get());

    CommandList.SetShaderResourceView(DiffuseCubeMapFilter_CS.Get(), SrcCubeMap->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(DiffuseCubeMapFilter_CS.Get(), DstCubeMapUAV.Get(), 0);
    CommandList.SetSamplerState(DiffuseCubeMapFilter_CS.Get(), CubeMapFilterSampler.Get(), 0);

    constexpr uint32 NumThreads = 16;
    constexpr uint32 ThreadsZ   = 6;

    const uint32 DiffuseCubeMapSize = static_cast<uint32>(DstCubeMap->GetWidth());

    const uint32 ThreadWidth  = Math::DivideByMultiple(DiffuseCubeMapSize, NumThreads);
    const uint32 ThreadHeight = Math::DivideByMultiple(DiffuseCubeMapSize, NumThreads);
    CommandList.Dispatch(ThreadWidth, ThreadHeight, ThreadsZ);

    CommandList.UnorderedAccessTextureBarrier(DstCubeMap);

    CommandList.TransitionTexture(DstCubeMap, FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource));
    CommandList.TransitionTexture(SrcCubeMap, FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));
    return true;
}