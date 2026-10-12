#pragma once
#include "Core/Memory/MemoryProfilerTypes.h"
#include "Core/Threading/Spinlock.h"

class CORE_API FMemoryTagRegistry
{
public:

    /** @brief The most names the registry holds before handing out MEMORY_NAME_OVERFLOW */
    static constexpr int32 MaxNames = 4096;

    /** @brief The most scope nodes the registry holds before handing out MEMORY_SCOPE_OVERFLOW */
    static constexpr int32 MaxScopeNodes = 16384;

public:
    FMemoryTagRegistry(const FMemoryTagRegistry&) = delete;
    FMemoryTagRegistry& operator=(const FMemoryTagRegistry&) = delete;

    /** @brief Creates an empty registry whose storage is mapped from OS pages on first use */
    constexpr FMemoryTagRegistry() noexcept
        : Lock()
        , Storage(nullptr)
    {
    }

    /**
     * @brief Interns a name that lives as long as the program, such as a string literal. The pointer is looked up
     * first and the string second, since the same literal has a different address in every module.
     *
     * @param Name The name to intern
     * @return Returns the name's id, or MEMORY_NAME_OVERFLOW once the table is full
     */
    FMemoryNameId InternStatic(const CHAR* Name);

    /**
     * @brief Interns a name built at runtime, which is matched by its contents only and copied into the registry
     *
     * @param Name The name to intern, which may be released once this returns
     * @return Returns the name's id, or MEMORY_NAME_OVERFLOW once the table is full
     */
    FMemoryNameId InternDynamic(const CHAR* Name);

    /**
     * @brief Interns one step of a scope path, so the same name under the same parent is always the same node
     *
     * @param Parent The node the step is taken from, MEMORY_SCOPE_ROOT for a scope opened outside any other
     * @param Name The name of the step
     * @return Returns the node's id, or MEMORY_SCOPE_OVERFLOW once the tree is full
     */
    FMemoryScopeId InternScopeNode(FMemoryScopeId Parent, FMemoryNameId Name);

    /**
     * @brief Looks up the string of an interned name
     *
     * @param Id The name's id
     * @return Returns the registry's copy of the name, or the name of MEMORY_NAME_UNTAGGED for an unknown id
     */
    const CHAR* GetName(FMemoryNameId Id);

    /**
     * @brief Looks up the parent of a scope node
     *
     * @param Node The node's id
     * @return Returns the parent's id, which is MEMORY_SCOPE_ROOT for the root itself and for an unknown id
     */
    FMemoryScopeId GetScopeParent(FMemoryScopeId Node);

    /**
     * @brief Looks up the name a scope node was entered with
     *
     * @param Node The node's id
     * @return Returns the name's id, which is MEMORY_NAME_UNTAGGED for the root and for an unknown id
     */
    FMemoryNameId GetScopeName(FMemoryScopeId Node);

    /** @return Returns how many names have been interned, the reserved ones included */
    int32 GetNumNames();

    /** @return Returns how many scope nodes exist, the reserved ones included */
    int32 GetNumScopeNodes();

private:
    struct FStorage;

    bool EnsureStorage();
    FMemoryNameId InternNameLocked(const CHAR* Name, uint32 Hash);

    FSpinLock Lock;
    FStorage* Storage;
};
