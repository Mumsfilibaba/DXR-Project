#include "AssetCore/AssetFile.h"
#include "Core/Math/Math.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/EnvironmentAssetFactory.h"
#include "Engine/Assets/TextureAssetLoader.h"
#include "RHI/RHI.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/TextureHelpers.h"
#include "RendererCore/TextureResourceData.h"

static constexpr uint64 ReadbackRowAlignment = 512;

static constexpr int32 HalfFloatTexelSize = 8;
static constexpr int32 MinSkySize         = 16;
static constexpr int32 MaxSkySize         = 8192;

static bool ReadCubeMap(FRHITexture* CubeMap, int32 NumMips, FEnvironmentCube& OutCube, FString& OutError)
{
    const int32 Size = static_cast<int32>(CubeMap->GetDesc().Extent.X);

    TArray<uint64> MipOffsets;
    TArray<uint64> RowStrides;
    uint64         ReadbackSize = 0;

    for (int32 Mip = 0; Mip < NumMips; ++Mip)
    {
        const int32  MipSize   = Math::Max(Size >> Mip, 1);
        const uint64 RowStride = Math::AlignUp<uint64>(static_cast<uint64>(MipSize) * HalfFloatTexelSize, ReadbackRowAlignment);

        MipOffsets.Add(ReadbackSize);
        RowStrides.Add(RowStride);
        ReadbackSize += RowStride * MipSize * EnvironmentAssets::NumCubeFaces;
    }

    FRHIFenceRef  Fence          = RHI::CreateFence();
    FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(ReadbackSize), ERHIResourceState::CopyDest, nullptr);

    if (!Fence || !ReadbackBuffer)
    {
        OutError = FString::Printf(TEXT("a %llu byte readback buffer couldn't be created"), ReadbackSize);
        return false;
    }

    FRHICommandList CommandList;
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(CubeMap, ERHIResourceState::ShaderResource, ERHIResourceState::CopySource));

    for (int32 Mip = 0; Mip < NumMips; ++Mip)
    {
        const int32 MipSize = Math::Max(Size >> Mip, 1);
        for (int32 Face = 0; Face < EnvironmentAssets::NumCubeFaces; ++Face)
        {
            for (int32 Row = 0; Row < MipSize; ++Row)
            {
                const uint64 Offset = MipOffsets[Mip] + RowStrides[Mip] * (static_cast<uint64>(Face) * MipSize + Row);
                CommandList.CopyTextureSubresourceToBuffer(ReadbackBuffer.Get(), Offset, CubeMap, FTextureRegion3D(MipSize, 1, 1, 0, Row, 0), Mip, Face);
            }
        }
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(CubeMap, ERHIResourceState::CopySource, ERHIResourceState::ShaderResource));
    CommandList.WriteFence(Fence.Get());
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    FRHICommandListExecutor::Get().WaitForCommands();

    if (!Fence->Wait())
    {
        OutError = FString(TEXT("the GPU never finished"));
        return false;
    }

    const uint8* Mapped = reinterpret_cast<const uint8*>(ReadbackBuffer->Map(0, ReadbackSize));
    if (!Mapped)
    {
        OutError = FString(TEXT("the readback buffer couldn't be mapped"));
        return false;
    }

    OutCube.Format = ETextureFormat::R16G16B16A16_Float;
    OutCube.Mips.Clear();

    for (int32 Mip = 0; Mip < NumMips; ++Mip)
    {
        const int32 MipSize  = Math::Max(Size >> Mip, 1);
        const int32 RowBytes = MipSize * HalfFloatTexelSize;

        FTextureMip& CubeMip = OutCube.Mips.Emplace();
        CubeMip.Width  = MipSize;
        CubeMip.Height = MipSize;
        CubeMip.Data.ResizeUninitialized(RowBytes * MipSize * EnvironmentAssets::NumCubeFaces);

        for (int32 Row = 0; Row < MipSize * EnvironmentAssets::NumCubeFaces; ++Row)
        {
            Memory::Memcpy(CubeMip.Data.Data() + static_cast<int64>(RowBytes) * Row, Mapped + MipOffsets[Mip] + RowStrides[Mip] * Row, RowBytes);
        }
    }

    ReadbackBuffer->Unmap(0, ReadbackSize);
    return true;
}

static bool BakeCubeMaps(FRHITexture* Panorama, int32 SkySize, FRHITextureRef& OutSkyCubeMap, FRHITextureRef& OutSpecularCubeMap, FRHITextureRef& OutDiffuseCubeMap, FString& OutError)
{
    const ETextureUsageFlags CubeFlags  = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopySource;
    const uint32             NumSkyMips = TextureHelpers::TextureSizeToMiplevels(SkySize);

    OutSkyCubeMap      = RHI::CreateTexture(FRHITextureDesc::CreateTextureCube(EFormat::R16G16B16A16_Float, SkySize, NumSkyMips, 1, CubeFlags));
    OutSpecularCubeMap = RHI::CreateTexture(FRHITextureDesc::CreateTextureCube(EFormat::R16G16B16A16_Float, FEnvironmentAssetFactory::SpecularSize, FEnvironmentAssetFactory::NumSpecularMips, 1, CubeFlags), ERHIResourceState::ShaderResource);
    OutDiffuseCubeMap  = RHI::CreateTexture(FRHITextureDesc::CreateTextureCube(EFormat::R16G16B16A16_Float, FEnvironmentAssetFactory::DiffuseSize, 1, 1, CubeFlags), ERHIResourceState::ShaderResource);

    if (!OutSkyCubeMap || !OutSpecularCubeMap || !OutDiffuseCubeMap)
    {
        OutError = FString(TEXT("the textures to bake it with couldn't be created"));
        return false;
    }

    if (!FTextureFactory::Get().TextureCubeFromPanorma(Panorama, OutSkyCubeMap.Get(), ETextureFactoryFlags::GenerateMips))
    {
        OutError = FString(TEXT("the panorama couldn't be turned into a cube map"));
        return false;
    }

    FRHICommandList CommandList;
    if (!FTextureFactory::Get().FilterSpecularCubeMap(CommandList, OutSkyCubeMap.Get(), OutSpecularCubeMap.Get(), FEnvironmentAssetFactory::NumSpecularMips) ||
        !FTextureFactory::Get().FilterDiffuseCubeMap(CommandList, OutSkyCubeMap.Get(), OutDiffuseCubeMap.Get()))
    {
        OutError = FString(TEXT("the sky couldn't be filtered"));
        return false;
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    OutSkyCubeMap->SetDebugName(TEXT("Environment Sky"));
    OutSpecularCubeMap->SetDebugName(TEXT("Environment Specular"));
    OutDiffuseCubeMap->SetDebugName(TEXT("Environment Diffuse"));
    return true;
}

static bool BakeEnvironment(const FTextureMip& Panorama, int32 SkySize, FEnvironmentAssetData& OutEnvironment, FString& OutError)
{
    FTextureResourceData PanoramaData;
    PanoramaData.InitMipData(Panorama.Data.Data(), static_cast<int64>(Panorama.Width) * HalfFloatTexelSize, Panorama.Data.Size());

    const FRHITextureDesc PanoramaDesc    = FRHITextureDesc::CreateTexture2D(EFormat::R16G16B16A16_Float, Panorama.Width, Panorama.Height, 1, 1, ETextureUsageFlags::ShaderResourceTexture);
    FRHITextureRef        PanoramaTexture = RHI::CreateTexture(PanoramaDesc, ERHIResourceState::ShaderResource, &PanoramaData);

    FRHITextureRef SkyCubeMap;
    FRHITextureRef SpecularCubeMap;
    FRHITextureRef DiffuseCubeMap;

    if (!PanoramaTexture)
    {
        OutError = FString(TEXT("the panorama couldn't be uploaded"));
        return false;
    }

    if (!BakeCubeMaps(PanoramaTexture.Get(), SkySize, SkyCubeMap, SpecularCubeMap, DiffuseCubeMap, OutError))
    {
        return false;
    }

    OutEnvironment.FilterVersion = FEnvironmentAssetFactory::FilterVersion;
    return ReadCubeMap(SkyCubeMap.Get(), SkyCubeMap->GetDesc().NumMipLevels, OutEnvironment.Sky, OutError)
        && ReadCubeMap(SpecularCubeMap.Get(), FEnvironmentAssetFactory::NumSpecularMips, OutEnvironment.Specular, OutError)
        && ReadCubeMap(DiffuseCubeMap.Get(), 1, OutEnvironment.Diffuse, OutError);
}

static bool CompressCubeMap(ITextureBlockCompressor& Compressor, FEnvironmentCube& Cube, FString& OutError)
{
    TArray<FTextureMip> CompressedMips;
    for (const FTextureMip& Mip : Cube.Mips)
    {
        FTextureMip& CompressedMip = CompressedMips.Emplace();
        CompressedMip.Width  = Mip.Width;
        CompressedMip.Height = Mip.Height;
    }

    for (int32 Face = 0; Face < EnvironmentAssets::NumCubeFaces; ++Face)
    {
        TArray<FTextureMip> FaceMips;
        for (const FTextureMip& Mip : Cube.Mips)
        {
            const int32 FaceSize = Mip.Data.Size() / EnvironmentAssets::NumCubeFaces;

            FTextureMip& FaceMip = FaceMips.Emplace();
            FaceMip.Width  = Mip.Width;
            FaceMip.Height = Mip.Height;
            FaceMip.Data.Reset(Mip.Data.Data() + static_cast<int64>(FaceSize) * Face, FaceSize);
        }

        TArray<FTextureMip> FaceBlocks;
        if (!Compressor.Compress(Cube.Format, FaceMips, ETextureFormat::BC6H_UF16, FaceBlocks, OutError) || FaceBlocks.Size() != FaceMips.Size())
        {
            return false;
        }

        for (int32 Mip = 0; Mip < FaceBlocks.Size(); ++Mip)
        {
            CompressedMips[Mip].Data.Append(FaceBlocks[Mip].Data);
        }
    }

    Cube.Format = ETextureFormat::BC6H_UF16;
    Cube.Mips   = ::Move(CompressedMips);
    return true;
}

class FCubeMapData final : public IRHITextureData
{
public:
    explicit FCubeMapData(const FEnvironmentCube& InCube)
        : Cube(InCube)
    {
    }

    virtual int64 GetMipRowPitch(uint32 MipLevel) const override
    {
        return TextureAssets::GetRowPitch(Cube.Format, Cube.Mips[MipLevel].Width);
    }

    virtual int64 GetMipSlicePitch(uint32 MipLevel) const override
    {
        return Cube.Mips[MipLevel].Data.Size() / EnvironmentAssets::NumCubeFaces;
    }

    virtual void* GetMipData(uint32 MipLevel) const override
    {
        return const_cast<uint8*>(Cube.Mips[MipLevel].Data.Data());
    }

private:
    const FEnvironmentCube& Cube;
};

static FRHITextureRef CreateCubeMap(const FEnvironmentCube& Cube, const CHAR_T* DebugName)
{
    FCubeMapData CubeData(Cube);

    FRHITextureDesc Desc = FRHITextureDesc::CreateTextureCube(TextureAssetLoader::GetRHIFormat(Cube.Format, false), Cube.Mips[0].Width, Cube.Mips.Size(), 1, ETextureUsageFlags::ShaderResourceTexture);
    Desc.TrackingMode = ERHIResourceStateTrackingMode::Static;

    FRHITextureRef CubeMap = RHI::CreateTexture(Desc, ERHIResourceState::ShaderResource, &CubeData);
    if (CubeMap)
    {
        CubeMap->SetDebugName(DebugName);
    }

    return CubeMap;
}

static bool ReadEnvironment(const FString& DiskPath, FEnvironmentAssetData& OutEnvironment, FString& OutError)
{
    FAssetFileReader Reader;
    TArray<uint8>    Payload;
    return Reader.Open(DiskPath, OutError) && Reader.ReadChunk(AssetChunk::Data, Payload, OutError) && EnvironmentAssets::Read(Payload, OutEnvironment, OutError);
}

bool FEnvironmentAssetFactory::ReadSettings(const FJsonValue& Value, FEnvironmentImportSettings& OutSettings, FString& OutError)
{
    if (!Value.IsNull() && !Value.IsObject())
    {
        OutError = FString(TEXT("they aren't a JSON object"));
        return false;
    }

    FEnvironmentImportSettings Settings;

    if (const FJsonValue* SkySize = Value.IsObject() ? Value.Find(TEXT("SkySize")) : nullptr)
    {
        int64 Size = 0;
        if (!SkySize->TryGetInt64(Size) || Size < MinSkySize || Size > MaxSkySize || (Size % TextureAssets::BlockSize) != 0)
        {
            OutError = FString::Printf(TEXT("SkySize isn't a multiple of 4 from %d to %d"), MinSkySize, MaxSkySize);
            return false;
        }

        Settings.SkySize = static_cast<int32>(Size);
    }

    if (const FJsonValue* Compress = Value.IsObject() ? Value.Find(TEXT("Compress")) : nullptr)
    {
        if (!Compress->TryGetBool(Settings.bCompress))
        {
            OutError = FString(TEXT("Compress isn't true or false"));
            return false;
        }
    }

    OutSettings = Settings;
    return true;
}

FJsonValue FEnvironmentAssetFactory::WriteSettings(const FEnvironmentImportSettings& Settings)
{
    FJsonValue Value = FJsonValue::MakeObject();
    Value.AddMember(TEXT("SkySize"), FJsonValue(Settings.SkySize));
    Value.AddMember(TEXT("Compress"), FJsonValue(Settings.bCompress));
    return Value;
}

TSharedPtr<FEnvironment> FEnvironmentAssetFactory::CreateFromPanorama(FRHITexture* Panorama, int32 SkySize, FString& OutError)
{
    FRHITextureRef SkyCubeMap;
    FRHITextureRef SpecularCubeMap;
    FRHITextureRef DiffuseCubeMap;

    if (!Panorama || !BakeCubeMaps(Panorama, SkySize, SkyCubeMap, SpecularCubeMap, DiffuseCubeMap, OutError))
    {
        return nullptr;
    }

    FRHITextureRef CompressedSkyCubeMap;
    if (FTextureFactory::Get().GetTextureCompressor().CompressCubeMapBC6(SkyCubeMap, CompressedSkyCubeMap) && CompressedSkyCubeMap)
    {
        CompressedSkyCubeMap->SetDebugName(TEXT("Environment Sky"));
        SkyCubeMap = CompressedSkyCubeMap;
    }

    return MakeSharedPtr<FEnvironment>(SkyCubeMap, SpecularCubeMap, DiffuseCubeMap);
}

TSharedPtr<FEnvironment> FEnvironmentAssetFactory::Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError)
{
    FEnvironmentAssetData Environment;
    if (!ReadEnvironment(DiskPath, Environment, OutError))
    {
        OutError = FString::Printf(TEXT("The environment data of '%s' can't be read: %s"), *Asset.Path, *OutError);
        return nullptr;
    }

    if (Environment.FilterVersion != FilterVersion)
    {
        LOG_INFO("[FEnvironmentAssetFactory]: '%s' was baked by filter version %u, so it is rebuilt with version %u", *Asset.Path, Environment.FilterVersion, FilterVersion);

        const FAssetImportResult Result = FAssetManager::Get().GetAssetImporter().Reimport(Asset.Path);
        if (Result.Error != EAssetImportError::None || !ReadEnvironment(DiskPath, Environment, OutError))
        {
            LOG_WARNING("[FEnvironmentAssetFactory]: '%s' couldn't be rebuilt, so the old bake is used: %s", *Asset.Path, Result.Error != EAssetImportError::None ? *Result.Message : *OutError);
        }
    }

    FRHITextureRef SkyCubeMap      = CreateCubeMap(Environment.Sky, TEXT("Environment Sky"));
    FRHITextureRef SpecularCubeMap = CreateCubeMap(Environment.Specular, TEXT("Environment Specular"));
    FRHITextureRef DiffuseCubeMap  = CreateCubeMap(Environment.Diffuse, TEXT("Environment Diffuse"));

    if (!SkyCubeMap || !SpecularCubeMap || !DiffuseCubeMap)
    {
        OutError = FString::Printf(TEXT("The RHI cube maps for '%s' couldn't be created"), *Asset.Path);
        return nullptr;
    }

    return MakeSharedPtr<FEnvironment>(SkyCubeMap, SpecularCubeMap, DiffuseCubeMap);
}

FEnvironmentAssetFactory::FEnvironmentAssetFactory(const TSharedPtr<FTextureAssetFactory>& InTextureFactory, const TSharedPtr<ITextureBlockCompressor>& InCompressor)
    : TextureFactory(InTextureFactory)
    , Compressor(InCompressor)
{
}

FEnvironmentAssetFactory::~FEnvironmentAssetFactory() = default;

const CHAR_T* FEnvironmentAssetFactory::GetName() const
{
    return TEXT("Environment");
}

TArray<FString> FEnvironmentAssetFactory::GetExtensions() const
{
    return { FString(TEXT(".hdr")) };
}

TArray<EAssetType> FEnvironmentAssetFactory::GetAssetTypes() const
{
    return { EAssetType::Environment };
}

bool FEnvironmentAssetFactory::Import(FAssetImportContext& Context, TArray<FImportedAsset>& OutAssets, FJsonValue& OutSettings, FString& OutError)
{
    FEnvironmentImportSettings Settings;
    if (!ReadSettings(Context.GetSettings(), Settings, OutError))
    {
        OutError = FString::Printf(TEXT("the settings aren't valid: %s"), *OutError);
        return false;
    }

    FTextureImportSettings PanoramaSettings;
    PanoramaSettings.Usage         = ETextureUsage::HDR;
    PanoramaSettings.bSRGB         = false;
    PanoramaSettings.bCompress     = false;
    PanoramaSettings.bGenerateMips = false;

    TArray<FTextureBuildJob> Jobs;
    FTextureBuildJob& Job = Jobs.Emplace();
    Job.SourcePath = Context.GetSourcePath();
    Job.Settings   = FTextureAssetFactory::WriteSettings(PanoramaSettings);

    FTextureAssetData Panorama;
    if (!TextureFactory->BuildTextures(Jobs) || !TextureAssets::Read(Jobs[0].Data, Panorama, OutError))
    {
        OutError = Jobs[0].Error.IsEmpty() ? OutError : Jobs[0].Error;
        return false;
    }

    FEnvironmentAssetData Environment;
    if (!BakeEnvironment(Panorama.Mips[0], Settings.SkySize, Environment, OutError))
    {
        OutError = FString::Printf(TEXT("it couldn't be baked: %s"), *OutError);
        return false;
    }

    if (Settings.bCompress && Compressor)
    {
        FEnvironmentCube* Cubes[] = { &Environment.Sky, &Environment.Specular, &Environment.Diffuse };
        for (FEnvironmentCube* Cube : Cubes)
        {
            if (!CompressCubeMap(*Compressor, *Cube, OutError))
            {
                OutError = FString::Printf(TEXT("it couldn't be compressed to BC6H: %s"), *OutError);
                return false;
            }
        }
    }

    FImportedAsset& Asset = OutAssets.Emplace();
    Asset.Name      = Context.GetImportName();
    Asset.Type      = EAssetType::Environment;
    Asset.Guid      = Context.GetGuid(Asset.Name, Asset.Type);
    Asset.Thumbnail = ::Move(Jobs[0].Thumbnail);
    EnvironmentAssets::Write(Environment, Asset.Data);

    OutSettings = WriteSettings(Settings);
    return true;
}
