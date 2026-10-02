#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Math/IntVector2.h"

class FUIDrawData;
class FVisualElement;

struct FSnapshotImage
{
    FSnapshotImage()
        : Width(0)
        , Height(0)
        , Pixels()
    {
    }

    void Initialize(int32 InWidth, int32 InHeight, uint32 ClearColor);

    NODISCARD bool IsValid() const
    {
        return Width > 0 && Height > 0 && Pixels.Size() == Width * Height;
    }

    NODISCARD uint32 GetPixel(int32 X, int32 Y) const;

    int32          Width;
    int32          Height;
    TArray<uint32> Pixels;
};

void RasterizeDrawData(const FUIDrawData& DrawData, FSnapshotImage& OutImage);

NODISCARD bool WriteSnapshotPng(const FSnapshotImage& Image, const String& Filename);
NODISCARD bool SnapshotElementToPng(const TSharedPtr<FVisualElement>& Element, const IntVector2& Size, uint32 ClearColor, const String& Filename);

bool EnsureSnapshotDirectory(const String& Directory);

NODISCARD String GetSnapshotDirectory(const String& SubDirectory);
