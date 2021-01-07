#include "CommandList.h"

/*
* CommandListExecutor
*/

ICommandContext* CommandListExecutor::CmdContext = nullptr;

void CommandListExecutor::ExecuteCommandList(CommandList& CmdList)
{
	RenderCommand* Cmd = CmdList.First;
	while (Cmd != nullptr)
	{
		RenderCommand* Old = Cmd;
		Cmd = Cmd->NextCmd;

		Old->Execute(GetContext());
		Old->~RenderCommand();
	}

	CmdList.First	= nullptr;
	CmdList.Last	= nullptr;
	CmdList.Reset();
}

void CommandListExecutor::WaitForGPU()
{
	GetContext().Flush();
}
