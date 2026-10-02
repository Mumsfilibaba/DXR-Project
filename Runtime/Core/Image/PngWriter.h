#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Image/ImageView.h"

class CORE_API FPngWriter
{
public:
    FPngWriter();
    ~FPngWriter();

    /**
     * @brief Encodes an image and writes it out, creating any directories the path names.
     *
     * @param Filename Where to write, either absolute or relative to the working directory.
     * @param Image    The image to encode.
     * @return True when the whole file was written.
     */
    bool WriteToFile(const String& Filename, const FImageView& Image);

    /**
     * @brief Encodes an image into memory.
     *
     * Kept apart from writing so a capture can be encoded without a file to put it in, and so the
     * encoder can be tested without touching the filesystem.
     *
     * @param Image    The image to encode.
     * @param OutBytes Receives the file, replacing whatever it held.
     * @return True when the image was encoded.
     */
    bool Encode(const FImageView& Image, TArray<uint8>& OutBytes);

private:
    static void AppendBigEndian32(TArray<uint8>& Out, uint32 Value);
    static void AppendChunk(TArray<uint8>& Out, const CHAR* Type, const TArray<uint8>& Payload);

    NODISCARD static uint32 ComputeAdler32(const TArray<uint8>& Data);

    void FilterScanlines(const FImageView& Image);
    void BuildRedBlueSwappedCopy(const FImageView& Image);
    void Deflate();
    void ResetMatchTables(int32 InputSize);
    void InsertHash(int32 Position);
    void PushBit(uint32 Bit);
    void WriteBits(uint32 Value, int32 NumBits);
    void WriteCode(uint32 Code, int32 NumBits);
    void FlushBits();
    void WriteFixedLiteral(uint8 Literal);
    void WriteFixedMatch(int32 Length, int32 Distance);

    NODISCARD int32 HashAt(int32 Position) const;
    NODISCARD int32 FindLongestMatch(int32 Position, int32& OutDistance) const;

    TArray<uint8> FilteredRows;
    TArray<uint8> Compressed;
    TArray<uint8> FileBytes;
    TArray<uint8> SwappedPixels;
    TArray<int32> HashHeads;
    TArray<int32> ChainPrevious;
    uint8         BitBuffer;
    int32         BitCount;
};
