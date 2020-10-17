#include "CommandList.h"

/*
* CommandListExecutor
*/

CommandListExecutor::CommandListExecutor()
	: CmdContext(nullptr)
{
}

CommandListExecutor::~CommandListExecutor()
{
}

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

	CmdList.Reset();
}