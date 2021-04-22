#include "TextureFactory.h"

#include "RenderLayer/CommandList.h"
#include "RenderLayer/PipelineState.h"
#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#ifdef min
    #undef min
#endif

#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct TextureFactoryData
{
    TRef<ComputePipelineState> PanoramaPSO;
    TRef<ComputeShader>        PanoramaCS;

    TRef<ComputePipelineState> CombineChannelsPSO;
    TRef<ComputeShader>        CombineChannelsCS;

    TRef<ComputePipelineState> AddAlphaPSO;
    TRef<ComputeShader>        AddAlphaCS;

    TRef<SamplerState> CombineSampler;

    CommandList CmdList;
};

static TextureFactoryData GlobalFactoryData;

Bool TextureFactory::Init()
{
    // Compile and create shaders
    TArray<UInt8> Code;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/CubeMapGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    GlobalFactoryData.PanoramaCS = CreateComputeShader(Code);
    if (!GlobalFactoryData.PanoramaCS)
    {
        Debug::DebugBreak();
        return false;
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/TextureCombine.hlsl", "AddAlpha_Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    GlobalFactoryData.AddAlphaCS = CreateComputeShader(Code);
    if (!GlobalFactoryData.AddAlphaCS)
    {
        Debug::DebugBreak();
        return false;
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/TextureCombine.hlsl", "CombineChannels_Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    GlobalFactoryData.CombineChannelsCS = CreateComputeShader(Code);
    if (!GlobalFactoryData.CombineChannelsCS)
    {
        Debug::DebugBreak();
        return false;
    }

    // Create pipelines
    ComputePipelineStateCreateInfo PanoramaPSODesc;
    PanoramaPSODesc.Shader = GlobalFactoryData.PanoramaCS.Get();

    GlobalFactoryData.PanoramaPSO = CreateComputePipelineState(PanoramaPSODesc);
    if (GlobalFactoryData.PanoramaPSO)
    {
        GlobalFactoryData.PanoramaPSO->SetName("Generate CubeMap Pipeline");
    }
    else
    {
        Debug::DebugBreak();
        return false;
    }

    ComputePipelineStateCreateInfo CombineChannelsPSODesc;
    CombineChannelsPSODesc.Shader = GlobalFactoryData.CombineChannelsCS.Get();

    GlobalFactoryData.CombineChannelsPSO = CreateComputePipelineState(CombineChannelsPSODesc);
    if (GlobalFactoryData.CombineChannelsPSO)
    {
        GlobalFactoryData.CombineChannelsPSO->SetName("Combine Channels Pipeline");
    }
    else
    {
        Debug::DebugBreak();
        return false;
    }

    ComputePipelineStateCreateInfo AddAlphaPSODesc;
    AddAlphaPSODesc.Shader = GlobalFactoryData.AddAlphaCS.Get();

    GlobalFactoryData.AddAlphaPSO = CreateComputePipelineState(AddAlphaPSODesc);
    if (GlobalFactoryData.AddAlphaPSO)
    {
        GlobalFactoryData.AddAlphaPSO->SetName("Add Alpha Pipeline");
    }
    else
    {
        Debug::DebugBreak();
        return false;
    }

    // SamplerState
    SamplerStateCreateInfo SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Wrap;
    SamplerDesc.AddressV = ESamplerMode::Wrap;
    SamplerDesc.AddressW = ESamplerMode::Wrap;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipLinear;

    GlobalFactoryData.CombineSampler = CreateSamplerState(SamplerDesc);
    if (GlobalFactoryData.CombineSampler)
    {
        GlobalFactoryData.CombineSampler->SetName("Combine Sampler");
    }
    else
    {
        Debug::DebugBreak();
        return false;
    }

    return true;
}

void TextureFactory::Release()
{
    GlobalFactoryData.PanoramaPSO.Reset();
    GlobalFactoryData.PanoramaCS.Reset();

    GlobalFactoryData.CombineChannelsCS.Reset();
    GlobalFactoryData.CombineChannelsPSO.Reset();

    GlobalFactoryData.AddAlphaCS.Reset();
    GlobalFactoryData.AddAlphaPSO.Reset();

    GlobalFactoryData.CombineSampler.Reset();
}

Texture2D* TextureFactory::LoadFromFile(const std::string& Filepath, UInt32 CreateFlags, EFormat Format)
{
    Int32 Width        = 0;
    Int32 Height       = 0;
    Int32 ChannelCount = 0;

    // Load based on format
    TUniquePtr<Byte> Pixels;
    if (Format == EFormat::R8G8B8A8_Unorm)
    {
        Pixels = TUniquePtr<Byte>(stbi_load(Filepath.c_str(), &Width, &Height, &ChannelCount, 4));
    }
    else if (Format == EFormat::R8_Unorm)
    {
        Pixels = TUniquePtr<Byte>(stbi_load(Filepath.c_str(), &Width, &Height, &ChannelCount, 1));
    }
    else if (Format == EFormat::R32G32B32A32_Float)
    {
        Pixels = TUniquePtr<Byte>(reinterpret_cast<Byte*>(stbi_loadf(Filepath.c_str(), &Width, &Height, &ChannelCount, 4)));
    }
    else
    {
        LOG_ERROR("[TextureFactory]: Format not supported");
        return nullptr;
    }

    // Check if succeeded
    if (!Pixels)
    {
        LOG_ERROR("[TextureFactory]: Failed to load image '" + Filepath + "'");
        return nullptr;
    }
    else
    {
        LOG_INFO("[TextureFactory]: Loaded image '" + Filepath + "'");
    }

    return LoadFromMemory(Pixels.Get(), Width, Height, CreateFlags, Format);
}

Texture2D* TextureFactory::LoadFromMemory(const Byte* Pixels, UInt32 Width, UInt32 Height, UInt32 CreateFlags, EFormat Format)
{
    if (Format != EFormat::R8_Unorm && Format != EFormat::R8G8B8A8_Unorm && Format != EFormat::R32G32B32A32_Float)
    {
        LOG_ERROR("[TextureFactory]: Format not supported");
        return nullptr;
    }

    const Bool GenerateMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    const UInt32 NumMips    = GenerateMips ? std::max(UInt32(std::min(std::log2(Width), std::log2(Height))), 1u) : 1;

    Assert(NumMips != 0);

    const UInt32 Stride   = GetByteStrideFromFormat(Format);
    const UInt32 RowPitch = Width * Stride;
    
    Assert(RowPitch > 0);
    
    ResourceData InitalData = ResourceData(Pixels, Format, Width);
    TRef<Texture2D> Texture = CreateTexture2D(Format, Width, Height, NumMips, 1, TextureFlag_SRV, EResourceState::PixelShaderResource, &InitalData);
    if (!Texture)
    {
        Debug::DebugBreak();
        return nullptr;
    }

    if (GenerateMips && NumMips > 1)
    {
        CommandList& CmdList = GlobalFactoryData.CmdList;
        CmdList.Begin();
        CmdList.TransitionTexture(Texture.Get(), EResourceState::PixelShaderResource, EResourceState::CopyDest);
        CmdList.GenerateMips(Texture.Get());
        CmdList.TransitionTexture(Texture.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource);
        CmdList.End();
        gCmdListExecutor.ExecuteCommandList(CmdList);
    }

    return Texture.ReleaseOwnership();
}

TextureCube* TextureFactory::CreateTextureCubeFromPanorma(Texture2D* PanoramaSource, UInt32 CubeMapSize, UInt32 CreateFlags, EFormat Format)
{
    Assert(PanoramaSource->IsSRV());

    const Bool GenerateNumMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    const UInt16 NumMips = (GenerateNumMips) ? static_cast<UInt16>(std::log2(CubeMapSize)) : 1U;

    TRef<TextureCube> StagingTexture = CreateTextureCube(Format, CubeMapSize, NumMips, TextureFlag_UAV, EResourceState::Common, nullptr);
    if (!StagingTexture)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("TextureCube From Panorama StagingTexture");
    }

    TRef<UnorderedAccessView> StagingTextureUAV = CreateUnorderedAccessView(StagingTexture.Get(), Format, 0);
    if (!StagingTextureUAV)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("TextureCube From Panorama StagingTexture UAV");
    }

    TRef<TextureCube> Texture = CreateTextureCube(Format, CubeMapSize, NumMips, TextureFlag_SRV, EResourceState::Common, nullptr);
    if (!Texture)
    {
        return nullptr;
    }

    CommandList& CmdList = GlobalFactoryData.CmdList;
    CmdList.Begin();
    
    CmdList.TransitionTexture(PanoramaSource, EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::Common, EResourceState::UnorderedAccess);

    CmdList.SetComputePipelineState(GlobalFactoryData.PanoramaPSO.Get());

    struct ConstantBuffer
    {
        UInt32 CubeMapSize;
    } CB0;
    CB0.CubeMapSize = CubeMapSize;

    CmdList.Set32BitShaderConstants(GlobalFactoryData.PanoramaCS.Get(), &CB0, 1);
    CmdList.SetUnorderedAccessView(GlobalFactoryData.PanoramaCS.Get(), StagingTextureUAV.Get(), 0);

    ShaderResourceView* PanoramaSourceView = PanoramaSource->GetShaderResourceView();
    CmdList.SetShaderResourceView(GlobalFactoryData.PanoramaCS.Get(), PanoramaSourceView, 0);

    constexpr UInt32 LocalWorkGroupCount = 16;
    const UInt32 ThreadsX = Math::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
    const UInt32 ThreadsY = Math::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
    CmdList.Dispatch(ThreadsX, ThreadsY, 6);

    CmdList.TransitionTexture(PanoramaSource, EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::UnorderedAccess, EResourceState::CopySource);
    CmdList.TransitionTexture(Texture.Get(), EResourceState::Common, EResourceState::CopyDest);

    CmdList.CopyTexture(Texture.Get(), StagingTexture.Get());

    if (GenerateNumMips)
    {
        CmdList.GenerateMips(Texture.Get());
    }

    CmdList.TransitionTexture(Texture.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource);
    CmdList.End();
    gCmdListExecutor.ExecuteCommandList(CmdList);

    return Texture.ReleaseOwnership();
}

Texture2D* TextureFactory::AddAlphaChannel(Texture2D* Source, Texture2D* Alpha)
{
    Assert(Source->IsSRV() && !Source->IsMultiSampled());
    Assert(Alpha->IsSRV() && !Source->IsMultiSampled());

    UInt32 Width   = Source->GetWidth();
    UInt32 Height  = Source->GetHeight();
    UInt32 NumMips = Source->GetNumMips();
    UInt32 Flags   = Source->GetFlags();

    //TODO: More formats?

    TRef<Texture2D> StagingTexture = CreateTexture2D(EFormat::R8G8B8A8_Unorm, Width, Height, NumMips, 1, TextureFlag_UAV, EResourceState::UnorderedAccess, nullptr);
    if (!StagingTexture)
    {
        Debug::DebugBreak();
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("AddAlphaChannel StagingTexture");
    }

    TRef<Texture2D> Texture = CreateTexture2D(EFormat::R8G8B8A8_Unorm, Width, Height, NumMips, 1, Flags, EResourceState::CopyDest, nullptr);
    if (!Texture)
    {
        Debug::DebugBreak();
        return nullptr;
    }

    CommandList& CmdList = GlobalFactoryData.CmdList;
    CmdList.Begin();

    CmdList.TransitionTexture(Source, EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(Alpha, EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);

    CmdList.SetComputePipelineState(GlobalFactoryData.AddAlphaPSO.Get());

    CmdList.SetUnorderedAccessView(GlobalFactoryData.AddAlphaCS.Get(), StagingTexture->GetUnorderedAccessView(), 0);

    CmdList.SetShaderResourceView(GlobalFactoryData.AddAlphaCS.Get(), Source->GetShaderResourceView(), 0);
    CmdList.SetShaderResourceView(GlobalFactoryData.AddAlphaCS.Get(), Alpha->GetShaderResourceView(), 1);

    CmdList.SetSamplerState(GlobalFactoryData.AddAlphaCS.Get(), GlobalFactoryData.CombineSampler.Get(), 0);

    constexpr UInt32 LocalWorkGroupCount = 16;
    const UInt32 ThreadsX = Math::DivideByMultiple(Width, LocalWorkGroupCount);
    const UInt32 ThreadsY = Math::DivideByMultiple(Height, LocalWorkGroupCount);
    CmdList.Dispatch(ThreadsX, ThreadsY, 6);

    CmdList.TransitionTexture(Source, EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    CmdList.TransitionTexture(Alpha, EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::UnorderedAccess, EResourceState::CopySource);

    CmdList.CopyTexture(Texture.Get(), StagingTexture.Get());

    if (NumMips > 1)
    {
        CmdList.GenerateMips(Texture.Get());
    }

    CmdList.TransitionTexture(Texture.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource);
    CmdList.End();
    gCmdListExecutor.ExecuteCommandList(CmdList);

    return Texture.ReleaseOwnership();
}

Texture2D* TextureFactory::CombineTextureChannels(Texture2D* Source0, Texture2D* Source1, Texture2D* Source2, Texture2D* Source3)
{
    // TODO: Do min max
    UInt32 Width = 1;
    if (Source0)
    {
        Width = Math::Max(Width, Source0->GetWidth());
    }
    if (Source1)
    {
        Width = Math::Max(Width, Source1->GetWidth());
    }
    if (Source2)
    {
        Width = Math::Max(Width, Source2->GetWidth());
    }
    if (Source3)
    {
        Width = Math::Max(Width, Source3->GetWidth());
    }

    UInt32 Height = 1;
    if (Source0)
    {
        Height = Math::Max(Height, Source0->GetHeight());
    }
    if (Source1)
    {
        Height = Math::Max(Height, Source1->GetHeight());
    }
    if (Source2)
    {
        Height = Math::Max(Height, Source2->GetHeight());
    }
    if (Source3)
    {
        Height = Math::Max(Height, Source3->GetHeight());
    }

    UInt32 NumMips = 1;
    if (Source0)
    {
        NumMips = Math::Max(NumMips, Source0->GetNumMips());
    }
    if (Source1)
    {
        NumMips = Math::Max(NumMips, Source1->GetNumMips());
    }
    if (Source2)
    {
        NumMips = Math::Max(NumMips, Source2->GetNumMips());
    }
    if (Source3)
    {
        NumMips = Math::Max(NumMips, Source3->GetNumMips());
    }

    UInt32 Flags = 0;
    if (Source0)
    {
        Flags |= Source0->GetFlags();
    }
    if (Source1)
    {
        Flags |= Source1->GetFlags();
    }
    if (Source2)
    {
        Flags |= Source2->GetFlags();
    }
    if (Source3)
    {
        Flags |= Source3->GetFlags();
    }

    //TODO: More formats?

    TRef<Texture2D> StagingTexture = CreateTexture2D(EFormat::R8G8B8A8_Unorm, Width, Height, NumMips, 1, TextureFlag_UAV, EResourceState::UnorderedAccess, nullptr);
    if (!StagingTexture)
    {
        Debug::DebugBreak();
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("CombineChannels StagingTexture");
    }

    TRef<Texture2D> Texture = CreateTexture2D(EFormat::R8G8B8A8_Unorm, Width, Height, NumMips, 1, Flags, EResourceState::CopyDest, nullptr);
    if (!Texture)
    {
        Debug::DebugBreak();
        return nullptr;
    }

    CommandList& CmdList = GlobalFactoryData.CmdList;
    CmdList.Begin();

    if (Source0)
    {
        CmdList.TransitionTexture(Source0, EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);
    }
    if (Source1)
    {
        CmdList.TransitionTexture(Source1, EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);
    }
    if (Source2)
    {
        CmdList.TransitionTexture(Source2, EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);
    }
    if (Source3)
    {
        CmdList.TransitionTexture(Source3, EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);
    }

    CmdList.SetComputePipelineState(GlobalFactoryData.CombineChannelsPSO.Get());
    CmdList.SetUnorderedAccessView(GlobalFactoryData.CombineChannelsCS.Get(), StagingTexture->GetUnorderedAccessView(), 0);

    if (Source0)
    {
        CmdList.SetShaderResourceView(GlobalFactoryData.CombineChannelsCS.Get(), Source0->GetShaderResourceView(), 0);
    }
    else
    {
        CmdList.SetShaderResourceView(GlobalFactoryData.CombineChannelsCS.Get(), nullptr, 0);
    }
    
    if (Source1)
    {
        CmdList.SetShaderResourceView(GlobalFactoryData.CombineChannelsCS.Get(), Source1->GetShaderResourceView(), 1);
    }
    else
    {
        CmdList.SetShaderResourceView(GlobalFactoryData.CombineChannelsCS.Get(), nullptr, 1);
    }
    
    if (Source2)
    {
        CmdList.SetShaderResourceView(GlobalFactoryData.CombineChannelsCS.Get(), Source2->GetShaderResourceView(), 2);
    }
    else
    {
        CmdList.SetShaderResourceView(GlobalFactoryData.CombineChannelsCS.Get(), nullptr, 2);
    }

    if (Source3)
    {
        CmdList.SetShaderResourceView(GlobalFactoryData.CombineChannelsCS.Get(), Source3->GetShaderResourceView(), 3);
    }
    else
    {
        CmdList.SetShaderResourceView(GlobalFactoryData.CombineChannelsCS.Get(), nullptr, 3);
    }

    CmdList.SetSamplerState(GlobalFactoryData.CombineChannelsCS.Get(), GlobalFactoryData.CombineSampler.Get(), 0);

    struct CombineChannelsSettings
    {
        Int32 EnableSource0;
        Int32 EnableSource1;
        Int32 EnableSource2;
        Int32 EnableSource3;
    } Settings;

    Settings.EnableSource0 = Source0 ? 1 : 0;
    Settings.EnableSource1 = Source1 ? 1 : 0;
    Settings.EnableSource2 = Source2 ? 1 : 0;
    Settings.EnableSource3 = Source3 ? 1 : 0;

    CmdList.Set32BitShaderConstants(GlobalFactoryData.CombineChannelsCS.Get(), &Settings, 4);

    constexpr UInt32 LocalWorkGroupCount = 16;
    const UInt32 ThreadsX = Math::DivideByMultiple(Width, LocalWorkGroupCount);
    const UInt32 ThreadsY = Math::DivideByMultiple(Height, LocalWorkGroupCount);
    CmdList.Dispatch(ThreadsX, ThreadsY, 6);

    if (Source0)
    {
        CmdList.TransitionTexture(Source0, EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    }
    if (Source1)
    {
        CmdList.TransitionTexture(Source1, EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    }
    if (Source2)
    {
        CmdList.TransitionTexture(Source2, EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    }
    if (Source3)
    {
        CmdList.TransitionTexture(Source3, EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    }

    CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::UnorderedAccess, EResourceState::CopySource);

    CmdList.CopyTexture(Texture.Get(), StagingTexture.Get());

    if (NumMips > 1)
    {
        CmdList.GenerateMips(Texture.Get());
    }

    CmdList.TransitionTexture(Texture.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource);
    CmdList.End();
    gCmdListExecutor.ExecuteCommandList(CmdList);

    return Texture.ReleaseOwnership();
}
