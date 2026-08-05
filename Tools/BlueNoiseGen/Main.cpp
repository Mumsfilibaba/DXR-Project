#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Filesystem/File.h>
#include <Core/Math/BlueNoiseGenerator.h>
#include <Core/Math/Math.h>
#include <Core/Memory/Malloc.h>
#include <Core/Memory/Memory.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Misc/IOutputDevice.h>
#include <Core/Misc/OutputDeviceLogger.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Tasks/TaskGraph.h>
#include <Core/Templates/CString.h>
#include <Core/Threading/ThreadManager.h>

DISABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING

#define TINYDDSLOADER_IMPLEMENTATION
#include <tinyddsloader.h>

ENABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING

#include <cstdio>

#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();

using tinyddsloader::DDSFile;

class FConsoleOutputDevice final : public IOutputDevice
{
public:
    virtual void Log(const String& Message) override
    {
        printf("%s\n", Message.Data());
    }

    virtual void Log(ELogSeverity Severity, const String& Message) override
    {
        const CHAR* Prefix = "";
        switch (Severity)
        {
            case ELogSeverity::Warning: Prefix = "[WARNING] "; break;
            case ELogSeverity::Error:   Prefix = "[ERROR] ";   break;
            default:                    Prefix = "";           break;
        }

        printf("%s%s\n", Prefix, Message.Data());
    }

    virtual void Flush() override
    {
        fflush(stdout);
    }
};

static void PrintUsage()
{
    printf(
        "BlueNoiseGen -- offline blue-noise mask generator\n"
        "\n"
        "  --type=<type>       vc_scalar   Void-and-cluster scalar mask (2D)\n"
        "                      vec2        Georgiev-Fajardo vector mask (2D)\n"
        "                      stbn_scalar Spatiotemporal scalar mask\n"
        "                      stbn_vec2   Spatiotemporal vector mask\n"
        "  --size=<n>          Width and height of a slice. Default 128.\n"
        "  --depth=<n>         Temporal slices. Forced to 1 for the non-stbn types. Default 64.\n"
        "  --channels=<n>      Values per pixel. Implied by the type when omitted.\n"
        "  --seed=<n>          Generator seed. Default 0.\n"
        "  --iterations=<n>    Swap iterations. Zero scales to the domain size.\n"
        "  --bits=<8|16>       Output channel width. Default 16.\n"
        "  --out=<path>        Destination .dds file.\n"
        "  --atlas             Lay the slices out as a 2D grid so the Texture2D-only importer\n"
        "                      can load them. Otherwise a Texture3D is written.\n"
        "  --checkpoint=<path> Periodically save swap-algorithm state, and resume from it if the\n"
        "                      file already exists.\n"
        "\n"
        "Example:\n"
        "  BlueNoiseGen --type=stbn_vec2 --size=128 --depth=64 --atlas \\\n"
        "           --out=Assets/Textures/Noise/STBN_Vec2_128x128x64_RG.dds\n");
}

static bool GetIntOption(const CHAR* Name, int64& OutValue)
{
    StringView Value;
    if (!CommandLine::FindOption(Name, Value) || (Value.Size() == 0))
    {
        return false;
    }

    const String Text(Value.Data(), Value.Size());
    OutValue = CString::Atoi64(Text.Data());
    return true;
}

static bool GetStringOption(const CHAR* Name, String& OutValue)
{
    StringView Value;
    if (!CommandLine::FindOption(Name, Value) || (Value.Size() == 0))
    {
        return false;
    }

    OutValue = String(Value.Data(), Value.Size());
    return true;
}

static DDSFile::DXGIFormat ResolveFormat(int32 Channels, int32 Bits)
{
    if (Bits == 8)
    {
        return (Channels >= 2) ? DDSFile::DXGIFormat::R8G8_UNorm : DDSFile::DXGIFormat::R8_UNorm;
    }

    return (Channels >= 2) ? DDSFile::DXGIFormat::R16G16_UNorm : DDSFile::DXGIFormat::R16_UNorm;
}

static TArray<uint8> PackChannels(const TArray<float>& Values, int64 NumPixels, int32 SourceChannels, int32 DestChannels, int32 Bits)
{
    const int32 BytesPerChannel = Bits / 8;

    TArray<uint8> Packed;
    Packed.Resize(static_cast<int32>(NumPixels * DestChannels * BytesPerChannel));

    uint8* Cursor = Packed.Data();
    for (int64 Pixel = 0; Pixel < NumPixels; ++Pixel)
    {
        for (int32 Channel = 0; Channel < DestChannels; ++Channel)
        {
            const float Value     = (Channel < SourceChannels) ? Values[static_cast<int32>(Pixel * SourceChannels) + Channel] : 0.0f;
            const float Saturated = Math::Clamp(Value, 0.0f, 1.0f);

            if (Bits == 8)
            {
                *Cursor = static_cast<uint8>(Saturated * 255.0f + 0.5f);
            }
            else
            {
                const uint16 Quantised = static_cast<uint16>(Saturated * 65535.0f + 0.5f);
                Memory::Memcpy(Cursor, &Quantised, sizeof(Quantised));
            }

            Cursor += BytesPerChannel;
        }
    }

    return Packed;
}

static String ExtractDirectory(const String& Filename)
{
    const int32 LastForward = Filename.FindLastChar('/');
    const int32 LastBack    = Filename.FindLastChar('\\');

    int32 LastSeparator = String::InvalidIndex;
    if (LastForward != String::InvalidIndex)
    {
        LastSeparator = LastForward;
    }

    if ((LastBack != String::InvalidIndex) && (LastBack > LastSeparator))
    {
        LastSeparator = LastBack;
    }

    if (LastSeparator == String::InvalidIndex)
    {
        return String();
    }

    return String(Filename.Data(), LastSeparator);
}

static void ReportQuality(const TArray<float>& Values, const FBlueNoiseParams& Params)
{
    constexpr int32 NumBins             = 32;
    constexpr float LowFrequencyCutoff  = 0.25f;
    constexpr float LowFrequencyCeiling = 0.02f;

    const int32 MiddleSlice = Params.Depth / 2;

    for (int32 Channel = 0; Channel < Params.Channels; ++Channel)
    {
        const TArray<float> Spectrum   = FBlueNoiseGenerator::ComputeRadialPowerSpectrum(Values, Params, MiddleSlice, Channel, NumBins);
        const float         LowEnergy  = FBlueNoiseGenerator::ComputeLowFrequencyEnergy(Spectrum, LowFrequencyCutoff);
        const int64         PixelCount = Params.GetPixelCount();

        double Sum = 0.0;
        for (int64 Pixel = 0; Pixel < PixelCount; ++Pixel)
        {
            Sum += Values[static_cast<int32>(Pixel) * Params.Channels + Channel];
        }

        const double Mean = Sum / static_cast<double>(Math::Max<int64>(PixelCount, 1));

        LOG_INFO("[BlueNoiseGen] Channel %d: low-frequency energy %.5f, mean %.4f", Channel, LowEnergy, Mean);

        if (LowEnergy > LowFrequencyCeiling)
        {
            LOG_WARNING("[BlueNoiseGen] Channel %d has %.1f%% of its energy below quarter-Nyquist; this is not blue noise", Channel, LowEnergy * 100.0f);
        }
    }
}

static int32 ChooseSlicesPerRow(int32 Depth)
{
    int32 SlicesPerRow = 1;
    while ((SlicesPerRow * SlicesPerRow) < Depth)
    {
        ++SlicesPerRow;
    }

    return SlicesPerRow;
}

static TArray<float> BuildAtlas(const TArray<float>& Values, int32 Width, int32 Height, int32 Depth, int32 Channels, int32 SlicesPerRow, int32& OutAtlasWidth, int32& OutAtlasHeight)
{
    const int32 SliceRows = Math::DivideByMultiple(Depth, static_cast<uint32>(SlicesPerRow));

    OutAtlasWidth  = SlicesPerRow * Width;
    OutAtlasHeight = SliceRows * Height;

    TArray<float> Atlas;
    Atlas.Resize(OutAtlasWidth * OutAtlasHeight * Channels);
    Atlas.Fill(0.0f);

    for (int32 Slice = 0; Slice < Depth; ++Slice)
    {
        const int32 OriginX = (Slice % SlicesPerRow) * Width;
        const int32 OriginY = (Slice / SlicesPerRow) * Height;

        for (int32 Y = 0; Y < Height; ++Y)
        {
            for (int32 X = 0; X < Width; ++X)
            {
                const int32 Source      = ((Slice * Width * Height) + (Y * Width) + X) * Channels;
                const int32 Destination = (((OriginY + Y) * OutAtlasWidth) + (OriginX + X)) * Channels;

                for (int32 Channel = 0; Channel < Channels; ++Channel)
                {
                    Atlas[Destination + Channel] = Values[Source + Channel];
                }
            }
        }
    }

    return Atlas;
}

struct FCheckpointFile
{
    static constexpr uint32 Magic   = 0x314e4247; // "GBN1"
    static constexpr uint32 Version = 1;

    struct FHeader
    {
        uint32 Magic;
        uint32 Version;
        int32  Width;
        int32  Height;
        int32  Depth;
        int32  Channels;
        int64  Iteration;
    };

    static bool Save(const String& Path, const FBlueNoiseParams& Params, const TArray<float>& Values, int64 Iteration)
    {
        const String TempPath = Path + ".tmp";

        {
            TFileRef<IPlatformFile> OutputFile = FPlatformFile::OpenForWrite(TempPath);
            if (!OutputFile.IsValid())
            {
                return false;
            }

            FHeader Header;
            Header.Magic     = Magic;
            Header.Version   = Version;
            Header.Width     = Params.Width;
            Header.Height    = Params.Height;
            Header.Depth     = Params.Depth;
            Header.Channels  = Params.Channels;
            Header.Iteration = Iteration;

            if (OutputFile->Write(reinterpret_cast<const uint8*>(&Header), sizeof(Header)) != sizeof(Header))
            {
                return false;
            }

            const uint32 PayloadSize = static_cast<uint32>(Values.Size() * sizeof(float));
            if (OutputFile->Write(reinterpret_cast<const uint8*>(Values.Data()), PayloadSize) != static_cast<int32>(PayloadSize))
            {
                return false;
            }
        }

        return FPlatformFile::MoveFile(TempPath.Data(), Path.Data());
    }

    static bool Load(const String& Path, const FBlueNoiseParams& Params, TArray<float>& OutValues, int64& OutIteration)
    {
        TFileRef<IPlatformFile> InputFile = FPlatformFile::OpenForRead(Path);
        if (!InputFile.IsValid())
        {
            return false;
        }

        FHeader Header;
        if (InputFile->Read(reinterpret_cast<uint8*>(&Header), sizeof(Header)) != sizeof(Header))
        {
            return false;
        }

        if ((Header.Magic != Magic) || (Header.Version != Version) ||
            (Header.Width != Params.Width) || (Header.Height != Params.Height) ||
            (Header.Depth != Params.Depth) || (Header.Channels != Params.Channels))
        {
            LOG_WARNING("[BlueNoiseGen] Checkpoint '%s' does not match the requested mask; starting over", Path.Data());
            return false;
        }

        OutValues.Resize(static_cast<int32>(Params.GetValueCount()));

        const uint32 PayloadSize = static_cast<uint32>(OutValues.Size() * sizeof(float));
        if (InputFile->Read(reinterpret_cast<uint8*>(OutValues.Data()), PayloadSize) != static_cast<int32>(PayloadSize))
        {
            return false;
        }

        OutIteration = Header.Iteration;
        return true;
    }
};

int main(int Argc, const CHAR* Argv[])
{
    GIsUnattended = true;

    FConsoleOutputDevice ConsoleDevice;
    FOutputDeviceLogger::Get()->RegisterOutputDevice(&ConsoleDevice);

    CommandLine::Initialize(Argv, Argc);

    if ((Argc <= 1) || CommandLine::FindOption("help") || CommandLine::FindOption("h"))
    {
        PrintUsage();
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
        return 0;
    }

    String Type;
    if (!GetStringOption("type", Type))
    {
        LOG_ERROR("[BlueNoiseGen] --type is required");
        
        PrintUsage();
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
        return 1;
    }

    FBlueNoiseParams Params;

    bool bSpatiotemporal = false;
    if (Type == "vc_scalar")
    {
        Params.Algorithm = EBlueNoiseAlgorithm::VoidAndCluster;
        Params.Channels  = 1;
    }
    else if (Type == "vec2")
    {
        Params.Algorithm = EBlueNoiseAlgorithm::VectorSwap;
        Params.Channels  = 2;
    }
    else if (Type == "stbn_scalar")
    {
        Params.Algorithm = EBlueNoiseAlgorithm::VoidAndCluster;
        Params.Channels  = 1;
        bSpatiotemporal  = true;
    }
    else if (Type == "stbn_vec2")
    {
        Params.Algorithm = EBlueNoiseAlgorithm::VectorSwap;
        Params.Channels  = 2;
        bSpatiotemporal  = true;
    }
    else
    {
        LOG_ERROR("[BlueNoiseGen] Unknown --type '%s'", Type.Data());
        PrintUsage();
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
        return 1;
    }

    int64 Size       = 128;
    int64 Depth      = 64;
    int64 Channels   = Params.Channels;
    int64 Seed       = 0;
    int64 Iterations = 0;
    int64 Bits       = 16;

    GetIntOption("size", Size);
    GetIntOption("depth", Depth);
    GetIntOption("channels", Channels);
    GetIntOption("seed", Seed);
    GetIntOption("iterations", Iterations);
    GetIntOption("bits", Bits);

    Params.Width          = static_cast<int32>(Size);
    Params.Height         = static_cast<int32>(Size);
    Params.Depth          = bSpatiotemporal ? static_cast<int32>(Depth) : 1;
    Params.Channels       = static_cast<int32>(Channels);
    Params.Seed           = static_cast<uint32>(Seed);
    Params.NumIterations  = Iterations;

    if ((Params.Width <= 0) || (Params.Depth <= 0) || (Params.Channels <= 0))
    {
        LOG_ERROR("[BlueNoiseGen] Size, depth and channels must all be positive");
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
        return 1;
    }

    if ((Params.Algorithm == EBlueNoiseAlgorithm::VoidAndCluster) && (Params.Channels != 1))
    {
        LOG_ERROR("[BlueNoiseGen] Void-and-cluster produces a scalar rank ordering, so it needs --channels=1");
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
        return 1;
    }

    String OutputPath;
    if (!GetStringOption("out", OutputPath))
    {
        LOG_ERROR("[BlueNoiseGen] --out is required");
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
        return 1;
    }

    const bool bAtlas = CommandLine::FindOption("atlas");

    String CheckpointPath;
    const bool bCheckpoint = GetStringOption("checkpoint", CheckpointPath);

    FThreadManager::Initialize();
    if (!FTaskGraph::Initialize())
    {
        LOG_ERROR("[BlueNoiseGen] Failed to initialize the task graph");

        FThreadManager::Release();
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
        return 1;
    }

    if (bCheckpoint)
    {
        Params.CheckpointInterval = Math::Max<int64>(static_cast<int64>(Params.GetPixelCount()), 1);

        TArray<float> ResumeValues;
        
        int64 ResumeIteration = 0;
        if (FCheckpointFile::Load(CheckpointPath, Params, ResumeValues, ResumeIteration))
        {
            Params.InitialValues  = Move(ResumeValues);
            Params.StartIteration = ResumeIteration;
            LOG_INFO("[BlueNoiseGen] Resuming from '%s' at iteration %lld", CheckpointPath.Data(), ResumeIteration);
        }
    }

    LOG_INFO("[BlueNoiseGen] Generating %s %dx%dx%d, %d channel(s), seed %u", Type.Data(), Params.Width, Params.Height, Params.Depth, Params.Channels, Params.Seed);

    FBlueNoiseCallbacks Callbacks;

    float LastReported = -1.0f;
    Callbacks.Progress = [&LastReported](float Fraction, const CHAR* Phase)
    {
        if ((Fraction - LastReported) < 0.01f)
        {
            return;
        }

        LastReported = Fraction;
        printf("  %-10s %5.1f%%\r", Phase, Fraction * 100.0f);
        fflush(stdout);
    };

    if (bCheckpoint)
    {
        Callbacks.Checkpoint = [&CheckpointPath, &Params](const TArray<float>& Values, int64 Iteration)
        {
            if (!FCheckpointFile::Save(CheckpointPath, Params, Values, Iteration))
            {
                LOG_WARNING("[BlueNoiseGen] Failed to write checkpoint '%s'", CheckpointPath.Data());
            }
        };
    }

    TArray<float> Values = FBlueNoiseGenerator::Generate(Params, Move(Callbacks));
    printf("\n");

    ReportQuality(Values, Params);

    int32 OutputWidth  = Params.Width;
    int32 OutputHeight = Params.Height;
    int32 OutputDepth  = Params.Depth;

    if (bAtlas && (Params.Depth > 1))
    {
        const int32 SlicesPerRow = ChooseSlicesPerRow(Params.Depth);
        Values      = BuildAtlas(Values, Params.Width, Params.Height, Params.Depth, Params.Channels, SlicesPerRow, OutputWidth, OutputHeight);
        OutputDepth = 1;

        LOG_INFO("[BlueNoiseGen] Atlas layout: %d slices, %d per row, %dx%d texels", Params.Depth, SlicesPerRow, OutputWidth, OutputHeight);
    }

    const int32 OutputChannels = Math::Min<int32>(Params.Channels, 2);
    if (Params.Channels > OutputChannels)
    {
        LOG_WARNING("[BlueNoiseGen] Only the first %d of %d channels will be written", OutputChannels, Params.Channels);
    }

    const int64   NumPixels = static_cast<int64>(OutputWidth) * OutputHeight * OutputDepth;
    TArray<uint8> Packed    = PackChannels(Values, NumPixels, Params.Channels, OutputChannels, static_cast<int32>(Bits));

    DDSFile::SaveInfo Info;
    Info.m_format = ResolveFormat(OutputChannels, static_cast<int32>(Bits));
    Info.m_texDim = (OutputDepth > 1) ? DDSFile::TextureDimension::Texture3D : DDSFile::TextureDimension::Texture2D;
    Info.m_width  = static_cast<uint32>(OutputWidth);
    Info.m_height = static_cast<uint32>(OutputHeight);
    Info.m_depth  = static_cast<uint32>(OutputDepth);

    DDSFile::ImageData Image = {};
    Image.m_width  = Info.m_width;
    Image.m_height = Info.m_height;
    Image.m_depth  = Info.m_depth;
    Image.m_mem    = Packed.Data();

    FTaskGraph::Release();
    FThreadManager::Release();

    const String Directory = ExtractDirectory(OutputPath);
    if (!Directory.IsEmpty() && !File::CreateDirectoryTree(Directory))
    {
        LOG_ERROR("[BlueNoiseGen] Failed to create directory '%s'", Directory.Data());
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
        return 1;
    }

    const tinyddsloader::Result SaveResult = DDSFile::Save(OutputPath.Data(), Info, &Image);
    if (SaveResult != tinyddsloader::Result::Success)
    {
        LOG_ERROR("[BlueNoiseGen] Failed to write '%s' (tinyddsloader result %d)", OutputPath.Data(), static_cast<int32>(SaveResult));
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
        return 1;
    }

    LOG_INFO("[BlueNoiseGen] Wrote '%s' (%dx%dx%d, %d channel(s), %d bits)", OutputPath.Data(), OutputWidth, OutputHeight, OutputDepth, OutputChannels, static_cast<int32>(Bits));
    LOG_INFO("[BlueNoiseGen] Done");

    FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
    return 0;
}
