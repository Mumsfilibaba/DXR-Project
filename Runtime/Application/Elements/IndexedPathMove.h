#pragma once
#include "Core/Containers/Array.h"

struct FIndexedPathMove
{
    /** @return True when Prefix is Path, or Path continues past Prefix. */
    NODISCARD static bool IsPrefix(const TArray<int32>& Prefix, const TArray<int32>& Path)
    {
        if (Path.Size() < Prefix.Size())
        {
            return false;
        }

        for (int32 Depth = 0; Depth < Prefix.Size(); ++Depth)
        {
            if (Path[Depth] != Prefix[Depth])
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Splits children into those that can move into TargetPath and those that cannot.
     *
     * Same-parent drops are all illegal. A folder cannot land on itself or inside its own tree.
     */
    static void Classify(const TArray<int32>& SourceParentPath, const TArray<int32>& ChildIndices, const TArray<int32>& TargetPath,
        TArray<int32>& OutLegal, TArray<int32>& OutIllegal)
    {
        OutLegal.Clear();
        OutIllegal.Clear();

        if (ChildIndices.IsEmpty() || SourceParentPath == TargetPath)
        {
            OutIllegal = ChildIndices;
            return;
        }

        for (const int32 ChildIndex : ChildIndices)
        {
            TArray<int32> SourcePath = SourceParentPath;
            SourcePath.Add(ChildIndex);

            if (IsPrefix(SourcePath, TargetPath))
            {
                OutIllegal.Add(ChildIndex);
            }
            else
            {
                OutLegal.Add(ChildIndex);
            }
        }
    }

    /** @return True when at least one child can move into TargetPath. */
    NODISCARD static bool CanMoveAny(const TArray<int32>& SourceParentPath, const TArray<int32>& ChildIndices, const TArray<int32>& TargetPath)
    {
        TArray<int32> Legal;
        TArray<int32> Illegal;
        Classify(SourceParentPath, ChildIndices, TargetPath, Legal, Illegal);
        return !Legal.IsEmpty();
    }
};
