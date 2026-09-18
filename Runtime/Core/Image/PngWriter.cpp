#include "Core/Image/PngWriter.h"
#include "Core/Misc/CRC.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Filesystem/File.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformFile.h"

// Deflate's window is 32 KiB and a match runs from three to 258 bytes.
constexpr int32 WINDOW_SIZE = 32768;
constexpr int32 MIN_MATCH   = 3;
constexpr int32 MAX_MATCH   = 258;
constexpr int32 MAX_CHAIN_LENGTH = 32;

constexpr int32 HASH_BITS = 15;
constexpr int32 HASH_SIZE = 1 << HASH_BITS;

constexpr uint16 GLengthBase[29] =
{
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};

constexpr uint8 GLengthExtraBits[29] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

constexpr uint16 GDistanceBase[30] =
{
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};

constexpr uint8 GDistanceExtraBits[30] =
{
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

static void ReserveAtLeast(TArray<uint8>& Array, int32 Capacity)
{
    if (Array.Capacity() < Capacity)
    {
        Array.Reserve(Capacity);
    }
}

FPngWriter::FPngWriter()
    : FilteredRows()
    , Compressed()
    , FileBytes()
    , SwappedPixels()
    , HashHeads()
    , ChainPrevious()
    , BitBuffer(0)
    , BitCount(0)
{
}

FPngWriter::~FPngWriter() = default;

bool FPngWriter::Encode(const FImageView& Image, TArray<uint8>& OutBytes)
{
    if (!Image.IsValid())
    {
        LOG_ERROR("[FPngWriter]: Cannot encode a %d x %d image with a pitch of %d",
            Image.Width, Image.Height, Image.RowPitch);
        return false;
    }

    FilterScanlines(Image);

    // A zlib stream: deflate at the default window, no preset dictionary, then the Adler-32.
    Compressed.Clear();
    ReserveAtLeast(Compressed, FilteredRows.Size() / 2);
    Compressed.Add(0x78);
    Compressed.Add(0x01);

    Deflate();
    AppendBigEndian32(Compressed, ComputeAdler32(FilteredRows));

    OutBytes.Clear();
    ReserveAtLeast(OutBytes, Compressed.Size() + 128);

    const uint8 Signature[] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    OutBytes.Append(Signature, ARRAY_COUNT(Signature));

    TArray<uint8> Header;
    AppendBigEndian32(Header, static_cast<uint32>(Image.Width));
    AppendBigEndian32(Header, static_cast<uint32>(Image.Height));
    Header.Add(8); // Bits a channel
    Header.Add(6); // Truecolour with alpha
    Header.Add(0); // Deflate, the only compression PNG has
    Header.Add(0); // Adaptive filtering, which is the per-row choice made above
    Header.Add(0); // Not interlaced

    AppendChunk(OutBytes, "IHDR", Header);
    AppendChunk(OutBytes, "IDAT", Compressed);
    AppendChunk(OutBytes, "IEND", TArray<uint8>());

    return true;
}

bool FPngWriter::WriteToFile(const String& Filename, const FImageView& Image)
{
    if (!Encode(Image, FileBytes))
    {
        LOG_ERROR("[FPngWriter]: Cannot write '%s' from a %d x %d image", *Filename, Image.Width, Image.Height);
        return false;
    }

    const String Directory = File::ExtractFilepath(Filename);
    if (!Directory.IsEmpty() && !File::CreateDirectoryTree(Directory))
    {
        LOG_ERROR("[FPngWriter]: Failed to create '%s' to write '%s' into", *Directory, *Filename);
        return false;
    }

    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    if (!FileHandle)
    {
        LOG_ERROR("[FPngWriter]: Failed to open '%s' for writing", *Filename);
        return false;
    }

    if (FileHandle->Write(FileBytes.Data(), static_cast<uint32>(FileBytes.Size())) != FileBytes.Size())
    {
        LOG_ERROR("[FPngWriter]: Failed to write all %d bytes of '%s'", FileBytes.Size(), *Filename);
        return false;
    }

    return true;
}

void FPngWriter::BuildRedBlueSwappedCopy(const FImageView& Image)
{
    const int32 RowBytes = Image.GetRowBytes();

    SwappedPixels.Clear();
    ReserveAtLeast(SwappedPixels, Image.Height * RowBytes);
    SwappedPixels.Resize(Image.Height * RowBytes);

    for (int32 Y = 0; Y < Image.Height; ++Y)
    {
        const uint8* Source      = Image.GetRow(Y);
        uint8*       Destination = SwappedPixels.Data() + (static_cast<int64>(Y) * RowBytes);

        for (int32 Index = 0; Index < RowBytes; Index += FImageView::BytesPerPixel)
        {
            Destination[Index + 0] = Source[Index + 2];
            Destination[Index + 1] = Source[Index + 1];
            Destination[Index + 2] = Source[Index + 0];
            Destination[Index + 3] = Source[Index + 3];
        }
    }
}

void FPngWriter::FilterScanlines(const FImageView& Image)
{
    FImageView Source = Image;
    if (Image.NeedsRedBlueSwap())
    {
        BuildRedBlueSwappedCopy(Image);
        Source = FImageView(SwappedPixels.Data(), Image.Width, Image.Height);
    }

    const int32 RowBytes = Source.GetRowBytes();

    FilteredRows.Clear();
    ReserveAtLeast(FilteredRows, Source.Height * (RowBytes + 1));

    for (int32 Y = 0; Y < Source.Height; ++Y)
    {
        const uint8* Row         = Source.GetRow(Y);
        const uint8* PreviousRow = (Y > 0) ? Source.GetRow(Y - 1) : nullptr;

        // The usual sum-of-absolute-values heuristic over the three cheap filters.
        uint32 NoneScore = 0;
        uint32 SubScore  = 0;
        uint32 UpScore   = 0;

        for (int32 Index = 0; Index < RowBytes; ++Index)
        {
            const uint8 Left  = (Index >= FImageView::BytesPerPixel) ? Row[Index - FImageView::BytesPerPixel] : 0;
            const uint8 Above = PreviousRow ? PreviousRow[Index] : 0;

            NoneScore += Row[Index];
            SubScore  += static_cast<uint8>(Row[Index] - Left);
            UpScore   += static_cast<uint8>(Row[Index] - Above);
        }

        uint8 Filter = 0;
        if (SubScore < NoneScore && SubScore <= UpScore)
        {
            Filter = 1;
        }
        else if (UpScore < NoneScore)
        {
            Filter = 2;
        }

        FilteredRows.Add(Filter);

        for (int32 Index = 0; Index < RowBytes; ++Index)
        {
            uint8 Predictor = 0;
            if (Filter == 1)
            {
                Predictor = (Index >= FImageView::BytesPerPixel) ? Row[Index - FImageView::BytesPerPixel] : 0;
            }
            else if (Filter == 2)
            {
                Predictor = PreviousRow ? PreviousRow[Index] : 0;
            }

            FilteredRows.Add(static_cast<uint8>(Row[Index] - Predictor));
        }
    }
}

void FPngWriter::ResetMatchTables(int32 InputSize)
{
    // Minus one is "no entry", so both tables start there rather than at a valid position.
    HashHeads.Resize(HASH_SIZE);
    for (int32 Index = 0; Index < HashHeads.Size(); ++Index)
    {
        HashHeads[Index] = -1;
    }

    ChainPrevious.Resize(Math::Max(InputSize, 1));
    for (int32 Index = 0; Index < ChainPrevious.Size(); ++Index)
    {
        ChainPrevious[Index] = -1;
    }
}

int32 FPngWriter::HashAt(int32 Position) const
{
    const uint8* Data = FilteredRows.Data();

    const uint32 Value = (static_cast<uint32>(Data[Position])     << 16) |
                         (static_cast<uint32>(Data[Position + 1]) <<  8) |
                          static_cast<uint32>(Data[Position + 2]);

    return static_cast<int32>((Value * 2654435761u) >> (32 - HASH_BITS));
}

void FPngWriter::InsertHash(int32 Position)
{
    const int32 Hash = HashAt(Position);

    ChainPrevious[Position] = HashHeads[Hash];
    HashHeads[Hash]         = Position;
}

int32 FPngWriter::FindLongestMatch(int32 Position, int32& OutDistance) const
{
    const uint8* Data  = FilteredRows.Data();
    const int32  Count = FilteredRows.Size();
    const int32  Limit = Math::Min(MAX_MATCH, Count - Position);

    int32 BestLength = 0;
    int32 Candidate  = HashHeads[HashAt(Position)];
    int32 Steps      = 0;

    while (Candidate >= 0 && Steps < MAX_CHAIN_LENGTH)
    {
        const int32 Distance = Position - Candidate;
        if (Distance <= 0 || Distance > WINDOW_SIZE)
        {
            break;
        }

        int32 Length = 0;
        while (Length < Limit && Data[Candidate + Length] == Data[Position + Length])
        {
            ++Length;
        }

        if (Length > BestLength)
        {
            BestLength  = Length;
            OutDistance = Distance;

            // Nothing longer exists, so stop rather than walk the rest of the chain.
            if (BestLength >= Limit)
            {
                break;
            }
        }

        Candidate = ChainPrevious[Candidate];
        ++Steps;
    }

    return BestLength;
}

void FPngWriter::Deflate()
{
    BitBuffer = 0;
    BitCount  = 0;

    // One final block, compressed with the fixed code tables.
    WriteBits(1, 1);
    WriteBits(1, 2);

    const int32 Count = FilteredRows.Size();
    ResetMatchTables(Count);

    int32 Position = 0;
    while (Position < Count)
    {
        int32 BestLength   = 0;
        int32 BestDistance = 0;

        if (Position + MIN_MATCH <= Count)
        {
            BestLength = FindLongestMatch(Position, BestDistance);
            InsertHash(Position);
        }

        if (BestLength >= MIN_MATCH)
        {
            WriteFixedMatch(BestLength, BestDistance);

            // Every position a match covers still has to enter the chains, or the next search would
            // be blind to everything inside it.
            for (int32 Offset = 1; Offset < BestLength; ++Offset)
            {
                const int32 Inner = Position + Offset;
                if (Inner + MIN_MATCH > Count)
                {
                    break;
                }

                InsertHash(Inner);
            }

            Position += BestLength;
        }
        else
        {
            WriteFixedLiteral(FilteredRows[Position]);
            ++Position;
        }
    }

    // End of block, which is symbol 256 and therefore seven zero bits.
    WriteCode(0, 7);
    FlushBits();
}

void FPngWriter::PushBit(uint32 Bit)
{
    BitBuffer |= static_cast<uint8>(Bit << BitCount);
    ++BitCount;

    if (BitCount == 8)
    {
        Compressed.Add(BitBuffer);
        BitBuffer = 0;
        BitCount  = 0;
    }
}

void FPngWriter::WriteBits(uint32 Value, int32 NumBits)
{
    for (int32 Index = 0; Index < NumBits; ++Index)
    {
        PushBit((Value >> Index) & 1u);
    }
}

void FPngWriter::WriteCode(uint32 Code, int32 NumBits)
{
    for (int32 Index = NumBits - 1; Index >= 0; --Index)
    {
        PushBit((Code >> Index) & 1u);
    }
}

void FPngWriter::FlushBits()
{
    if (BitCount > 0)
    {
        Compressed.Add(BitBuffer);
        BitBuffer = 0;
        BitCount  = 0;
    }
}

void FPngWriter::WriteFixedLiteral(uint8 Literal)
{
    // The fixed table splits the literals in two: the first 144 are eight bits, the rest are nine.
    if (Literal < 144)
    {
        WriteCode(0x30u + Literal, 8);
    }
    else
    {
        WriteCode(0x190u + (Literal - 144), 9);
    }
}

void FPngWriter::WriteFixedMatch(int32 Length, int32 Distance)
{
    int32 LengthCode = 28;
    while (LengthCode > 0 && GLengthBase[LengthCode] > Length)
    {
        --LengthCode;
    }

    // Length codes 257 to 279 are seven bits wide, 280 upwards are eight.
    const int32 Symbol = 257 + LengthCode;
    if (Symbol < 280)
    {
        WriteCode(static_cast<uint32>(Symbol - 256), 7);
    }
    else
    {
        WriteCode(0xC0u + static_cast<uint32>(Symbol - 280), 8);
    }

    WriteBits(static_cast<uint32>(Length - GLengthBase[LengthCode]), GLengthExtraBits[LengthCode]);

    int32 DistanceCode = 29;
    while (DistanceCode > 0 && GDistanceBase[DistanceCode] > Distance)
    {
        --DistanceCode;
    }

    WriteCode(static_cast<uint32>(DistanceCode), 5);
    WriteBits(static_cast<uint32>(Distance - GDistanceBase[DistanceCode]), GDistanceExtraBits[DistanceCode]);
}

uint32 FPngWriter::ComputeAdler32(const TArray<uint8>& Data)
{
    uint32 A = 1;
    uint32 B = 0;

    for (int32 Index = 0; Index < Data.Size(); ++Index)
    {
        A = (A + Data[Index]) % 65521u;
        B = (B + A) % 65521u;
    }

    return (B << 16) | A;
}

void FPngWriter::AppendBigEndian32(TArray<uint8>& Out, uint32 Value)
{
    Out.Add(static_cast<uint8>((Value >> 24) & 0xFF));
    Out.Add(static_cast<uint8>((Value >> 16) & 0xFF));
    Out.Add(static_cast<uint8>((Value >> 8)  & 0xFF));
    Out.Add(static_cast<uint8>((Value >> 0)  & 0xFF));
}

void FPngWriter::AppendChunk(TArray<uint8>& Out, const CHAR* Type, const TArray<uint8>& Payload)
{
    AppendBigEndian32(Out, static_cast<uint32>(Payload.Size()));

    const int32 TypeStart = Out.Size();
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Out.Add(static_cast<uint8>(Type[Index]));
    }

    Out.Append(Payload.Data(), Payload.Size());

    // The CRC covers the type and the payload, but not the length written in front of them.
    AppendBigEndian32(Out, CRC32::Generate(Out.Data() + TypeStart, static_cast<uint64>(Out.Size() - TypeStart)));
}
