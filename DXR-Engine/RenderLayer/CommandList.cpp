#include "CommandList.h"

CommandListExecutor GCmdListExecutor;

void CommandListExecutor::ExecuteCommandList( CommandList& CmdList )
{
    GetContext().Begin();

    InternalExecuteCommandList( CmdList );

    GetContext().End();
}

void CommandListExecutor::ExecuteCommandLists( CommandList* const* CmdLists, uint32 NumCmdLists )
{
    GetContext().Begin();

    for ( uint32 i = 0; i < NumCmdLists; i++ )
    {
        CommandList* CurrentCmdList = CmdLists[i];
        InternalExecuteCommandList( *CurrentCmdList );
    }

    GetContext().End();
}

void CommandListExecutor::WaitForGPU()
{
    CmdContext->Flush();
}

void CommandListExecutor::InternalExecuteCommandList( CommandList& CmdList )
{
    RenderCommand* Cmd = CmdList.First;
    while ( Cmd != nullptr )
    {
        RenderCommand* Old = Cmd;
        Cmd = Cmd->NextCmd;

        Old->Execute( GetContext() );
        Old->~RenderCommand();
    }

    CmdList.First = nullptr;
    CmdList.Last = nullptr;
    CmdList.Reset();
}
