#include "CommandList.h"

/*
* CommandMemoryArena
*/

VoidPtr CommandMemoryArena::Allocate(Uint64 InSizeInBytes)
{
	VALIDATE(ReservedSize() > InSizeInBytes);

	VoidPtr Allocated = reinterpret_cast<VoidPtr>(Mem + Offset);
	Offset += InSizeInBytes;
	return Allocated;
}

/*
* CommandAllocator
*/

CommandAllocator::CommandAllocator()
	: ArenaIndex(0)
	, CurrentArena(nullptr)
	, Arenas()
{
	CurrentArena = &Arenas.EmplaceBack();
}

VoidPtr CommandAllocator::Allocate(Uint64 SizeInBytes)
{
	VALIDATE(CurrentArena != nullptr);

	if (CurrentArena->ReservedSize() > SizeInBytes)
	{
		return CurrentArena->Allocate(SizeInBytes);
	}

	VALIDATE(Arenas.IsEmpty() == false);

	if (ArenaIndex < (Arenas.Size() - 1))
	{
		CurrentArena = &Arenas[++ArenaIndex];
	}
	else
	{
		CurrentArena = &Arenas.EmplaceBack();
	}

	VALIDATE(CurrentArena != nullptr);
	return CurrentArena->Allocate(SizeInBytes);
}

void CommandAllocator::Reset()
{
	VALIDATE(Arenas.IsEmpty() == false);
	Arenas.Front().Reset();

	CurrentArena = &Arenas.Front();
	VALIDATE(CurrentArena != nullptr);
}

/*
* CommandList
*/

CommandList::CommandList()
	: CmdAllocator()
	, CmdContext()
{
}

CommandList::~CommandList()
{
}

bool CommandList::Initialize()
{
	return false;
}

/*
* CommandExecutor
*/

void CommandExecutor::ExecuteCommandList(const CommandList& CmdList)
{
	const TArray<RenderCommand*>& Commands = CmdList.Commands;
	for (RenderCommand* Cmd : Commands)
	{
		Cmd->Execute(CmdList.GetContext());
	}
}