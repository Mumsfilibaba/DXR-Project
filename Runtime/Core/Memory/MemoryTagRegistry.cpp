#include "Core/Memory/MemoryTagRegistry.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformMemory.h"
#include "Core/Templates/CString.h"
#include "Core/Threading/ScopedLock.h"

static constexpr int32  NAME_TABLE_SIZE    = FMemoryTagRegistry::MaxNames * 2;
static constexpr int32  SCOPE_TABLE_SIZE   = FMemoryTagRegistry::MaxScopeNodes * 2;
static constexpr uint64 STRING_CHUNK_BYTES = 64 * 1024;

static const CHAR* const UNTAGGED_NAME = "Untagged";
static const CHAR* const OVERFLOW_NAME = "Overflow";

struct FMemoryTagRegistry::FStorage
{
    struct FPointerSlot
    {
        const CHAR*   Key;
        FMemoryNameId Id;
    };

    const CHAR*    Names[MaxNames];
    uint32         NameHashes[MaxNames];
    FPointerSlot   PointerTable[NAME_TABLE_SIZE];
    uint16         StringTable[NAME_TABLE_SIZE];
    FMemoryScopeId ScopeParents[MaxScopeNodes];
    FMemoryNameId  ScopeNames[MaxScopeNodes];
    uint16         ScopeTable[SCOPE_TABLE_SIZE];
    int32          NumNames;
    int32          NumPointers;
    int32          NumScopeNodes;
    CHAR*          StringCursor;
    uint64         StringBytesLeft;
};

static uint32 HashString(const CHAR* String)
{
    uint32 Hash = 2166136261u;
    for (const CHAR* Current = String; *Current != '\0'; ++Current)
    {
        Hash = (Hash ^ static_cast<uint8>(*Current)) * 16777619u;
    }

    return Hash;
}

static uint32 HashPointer(const void* Pointer)
{
    const uint64 Value = static_cast<uint64>(reinterpret_cast<UPTR_INT>(Pointer));
    return static_cast<uint32>((Value >> 3) * 0x9E3779B97F4A7C15ull >> 32);
}

static uint32 HashScopeKey(FMemoryScopeId Parent, FMemoryNameId Name)
{
    const uint32 Key = (static_cast<uint32>(Parent) << 16) | static_cast<uint32>(Name);
    return Key * 2654435761u;
}

bool FMemoryTagRegistry::EnsureStorage()
{
    if (Storage)
    {
        return true;
    }

    Storage = static_cast<FStorage*>(FPlatformMemory::AllocatePages(sizeof(FStorage)));
    if (!Storage)
    {
        return false;
    }

    InternNameLocked(UNTAGGED_NAME, HashString(UNTAGGED_NAME));
    InternNameLocked(OVERFLOW_NAME, HashString(OVERFLOW_NAME));

    Storage->ScopeParents[MEMORY_SCOPE_ROOT]     = MEMORY_SCOPE_ROOT;
    Storage->ScopeNames[MEMORY_SCOPE_ROOT]       = MEMORY_NAME_UNTAGGED;
    Storage->ScopeParents[MEMORY_SCOPE_OVERFLOW] = MEMORY_SCOPE_ROOT;
    Storage->ScopeNames[MEMORY_SCOPE_OVERFLOW]   = MEMORY_NAME_OVERFLOW;
    Storage->NumScopeNodes = 2;
    return true;
}

FMemoryNameId FMemoryTagRegistry::InternNameLocked(const CHAR* Name, uint32 Hash)
{
    for (uint32 Probe = Hash & (NAME_TABLE_SIZE - 1);; Probe = (Probe + 1) & (NAME_TABLE_SIZE - 1))
    {
        const uint16 Entry = Storage->StringTable[Probe];
        if (Entry == 0)
        {
            if (Storage->NumNames >= MaxNames)
            {
                return MEMORY_NAME_OVERFLOW;
            }

            const uint64 Length = static_cast<uint64>(CString::Strlen(Name)) + 1;
            if (Length > Storage->StringBytesLeft)
            {
                const uint64 ChunkBytes = Length > STRING_CHUNK_BYTES ? Length : STRING_CHUNK_BYTES;

                CHAR* Chunk = static_cast<CHAR*>(FPlatformMemory::AllocatePages(ChunkBytes));
                if (!Chunk)
                {
                    return MEMORY_NAME_OVERFLOW;
                }

                Storage->StringCursor    = Chunk;
                Storage->StringBytesLeft = ChunkBytes;
            }

            CHAR* Copy = Storage->StringCursor;
            Memory::Memcpy(Copy, Name, Length);
            Storage->StringCursor    += Length;
            Storage->StringBytesLeft -= Length;

            const FMemoryNameId NewId = static_cast<FMemoryNameId>(Storage->NumNames++);
            Storage->Names[NewId]      = Copy;
            Storage->NameHashes[NewId] = Hash;
            Storage->StringTable[Probe] = static_cast<uint16>(NewId + 1);
            return NewId;
        }

        const FMemoryNameId Id = static_cast<FMemoryNameId>(Entry - 1);
        if (Storage->NameHashes[Id] == Hash && CString::Strcmp(Storage->Names[Id], Name) == 0)
        {
            return Id;
        }
    }
}

FMemoryNameId FMemoryTagRegistry::InternStatic(const CHAR* Name)
{
    SCOPED_LOCK(Lock);

    if (!EnsureStorage())
    {
        return MEMORY_NAME_UNTAGGED;
    }

    uint32 PointerProbe = HashPointer(Name) & (NAME_TABLE_SIZE - 1);
    for (;; PointerProbe = (PointerProbe + 1) & (NAME_TABLE_SIZE - 1))
    {
        FStorage::FPointerSlot& Slot = Storage->PointerTable[PointerProbe];
        if (Slot.Key == nullptr)
        {
            break;
        }

        if (Slot.Key == Name)
        {
            if (CString::Strcmp(Storage->Names[Slot.Id], Name) == 0)
            {
                return Slot.Id;
            }

            const FMemoryNameId Id = InternNameLocked(Name, HashString(Name));
            Slot.Id = Id;
            return Id;
        }
    }

    const FMemoryNameId Id = InternNameLocked(Name, HashString(Name));
    if (Id != MEMORY_NAME_OVERFLOW && Storage->NumPointers < MaxNames)
    {
        Storage->PointerTable[PointerProbe].Key = Name;
        Storage->PointerTable[PointerProbe].Id  = Id;
        Storage->NumPointers++;
    }

    return Id;
}

FMemoryNameId FMemoryTagRegistry::InternDynamic(const CHAR* Name)
{
    SCOPED_LOCK(Lock);

    if (!EnsureStorage())
    {
        return MEMORY_NAME_UNTAGGED;
    }

    return InternNameLocked(Name, HashString(Name));
}

FMemoryScopeId FMemoryTagRegistry::InternScopeNode(FMemoryScopeId Parent, FMemoryNameId Name)
{
    SCOPED_LOCK(Lock);

    if (!EnsureStorage())
    {
        return MEMORY_SCOPE_ROOT;
    }

    for (uint32 Probe = HashScopeKey(Parent, Name) & (SCOPE_TABLE_SIZE - 1);; Probe = (Probe + 1) & (SCOPE_TABLE_SIZE - 1))
    {
        const uint16 Entry = Storage->ScopeTable[Probe];
        if (Entry == 0)
        {
            if (Storage->NumScopeNodes >= MaxScopeNodes)
            {
                return MEMORY_SCOPE_OVERFLOW;
            }

            const FMemoryScopeId NewNode = static_cast<FMemoryScopeId>(Storage->NumScopeNodes++);
            Storage->ScopeParents[NewNode] = Parent;
            Storage->ScopeNames[NewNode]   = Name;
            Storage->ScopeTable[Probe]     = static_cast<uint16>(NewNode + 1);
            return NewNode;
        }

        const FMemoryScopeId Node = static_cast<FMemoryScopeId>(Entry - 1);
        if (Storage->ScopeParents[Node] == Parent && Storage->ScopeNames[Node] == Name)
        {
            return Node;
        }
    }
}

const CHAR* FMemoryTagRegistry::GetName(FMemoryNameId Id)
{
    SCOPED_LOCK(Lock);
    return (EnsureStorage() && Id < Storage->NumNames) ? Storage->Names[Id] : UNTAGGED_NAME;
}

FMemoryScopeId FMemoryTagRegistry::GetScopeParent(FMemoryScopeId Node)
{
    SCOPED_LOCK(Lock);
    return (EnsureStorage() && Node < Storage->NumScopeNodes) ? Storage->ScopeParents[Node] : MEMORY_SCOPE_ROOT;
}

FMemoryNameId FMemoryTagRegistry::GetScopeName(FMemoryScopeId Node)
{
    SCOPED_LOCK(Lock);
    return (EnsureStorage() && Node < Storage->NumScopeNodes) ? Storage->ScopeNames[Node] : MEMORY_NAME_UNTAGGED;
}

int32 FMemoryTagRegistry::GetNumNames()
{
    SCOPED_LOCK(Lock);
    return EnsureStorage() ? Storage->NumNames : 0;
}

int32 FMemoryTagRegistry::GetNumScopeNodes()
{
    SCOPED_LOCK(Lock);
    return EnsureStorage() ? Storage->NumScopeNodes : 0;
}
