#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

struct FContentMount
{
    /** @brief The virtual root, a name between slashes such as "/Project/" */
    FString Root;

    /** @brief The absolute, normalized folder the root maps to */
    FString Directory;
};

class ASSETCORE_API FContentMounts
{
public:
    static constexpr const CHAR_T* EngineRoot  = TEXT("/Engine/");
    static constexpr const CHAR_T* ProjectRoot = TEXT("/Project/");

    /** @return Returns mounts with /Engine/ at the engine's Content directory and /Project/ at the project's */
    NODISCARD static FContentMounts CreateDefault();

    /**
     * @brief Mount Root at Directory, replacing any mount of the same root
     * @param Root A single name between slashes, of letters, digits, '_' and '-', such as "/Project/"
     * @param Directory The folder, made absolute and normalized
     * @return Returns false if Root isn't a valid root or Directory is empty
     */
    bool Mount(const FString& Root, const FString& Directory);

    /** @return Returns false if nothing was mounted at Root */
    bool Unmount(const FString& Root);

    /**
     * @brief Convert a virtual path to the path on disk. '\' is taken as '/', and "/Project" and "/Project/" are the mount's
     * folder itself.
     * @return Returns false if the path isn't under a mount, or leaves its mount with ".." or a drive
     */
    bool ToDiskPath(const FString& VirtualPath, FString& OutDiskPath) const;

    /**
     * @brief Convert a path on disk, absolute or relative to the working directory, to its virtual path under the mount
     * holding it, the deepest one when mounts are nested. A mount's folder itself becomes its root, such as "/Project/".
     * @return Returns false if no mount holds the path
     */
    bool ToVirtualPath(const FString& DiskPath, FString& OutVirtualPath) const;

    /** @return Returns the mount a virtual path is under, or nullptr */
    NODISCARD const FContentMount* FindMount(const FString& VirtualPath) const;

    FORCEINLINE const TArray<FContentMount>& GetMounts() const
    {
        return Mounts;
    }

private:
    TArray<FContentMount> Mounts;
};
