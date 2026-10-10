#pragma once
#include "Core/Containers/String.h"
#include "Core/Templates/TypeHash.h"

struct FShaderCompilerIdentity
{
    NODISCARD bool IsValid() const
    {
        return !Name.IsEmpty();
    }

    NODISCARD uint64 GetHash() const
    {
        uint64 Hash = THash<String>::GetHash(Name);
        HashCombine(Hash, VersionMajor);
        HashCombine(Hash, VersionMinor);
        HashCombine(Hash, SettingsVersion);
        HashCombine(Hash, TranslatorVersion);
        return Hash;
    }

    String Name;
    uint32 VersionMajor      = 0;
    uint32 VersionMinor      = 0;

    /** Bumped by hand when the backend changes the arguments or flags it builds from an FShaderCompileInfo */
    uint32 SettingsVersion   = 0;

    /** The version of the backend's TranslateSource step, zero when it has none */
    uint32 TranslatorVersion = 0;
};
