#include "CommandList.h"

CommandListExecutor GCmdListExecutor;

void CommandListExecutor::ExecuteCommandList(CommandList& CmdList)
{
    Assert(CmdList.IsRecording == false);

    RenderCommand* Cmd = CmdList.First;
    while (Cmd != nullptr)
    {
        RenderCommand* Old = Cmd;
        Cmd = Cmd->NextCmd;

        Old->Execute(GetContext());
        Old->~RenderCommand();
    }

    CmdList.First = nullptr;
    CmdList.Last  = nullptr;
    CmdList.Reset();
}

void CommandListExecutor::WaitForGPU()
{
    CmdContext->Flush();
}
