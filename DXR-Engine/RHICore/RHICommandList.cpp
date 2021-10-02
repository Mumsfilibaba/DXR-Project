#include "RHICommandList.h"

CRHICommandQueue GCmdListExecutor;

void CRHICommandQueue::ExecuteCommandList( CRHICommandList& CmdList )
{
    GetContext().Begin();

    InternalExecuteCommandList( CmdList );

    GetContext().End();
}

void CRHICommandQueue::ExecuteCommandLists( CRHICommandList* const* CmdLists, uint32 NumCmdLists )
{
    GetContext().Begin();

    for ( uint32 i = 0; i < NumCmdLists; i++ )
    {
        CRHICommandList* CurrentCmdList = CmdLists[i];
        InternalExecuteCommandList( *CurrentCmdList );
    }

    GetContext().End();
}

void CRHICommandQueue::WaitForGPU()
{
    if ( CmdContext )
    {
        CmdContext->Flush();
    }
}

void CRHICommandQueue::InternalExecuteCommandList( CRHICommandList& CmdList )
{
    SRHIRenderCommand* Cmd = CmdList.First;
    while ( Cmd != nullptr )
    {
        SRHIRenderCommand* Old = Cmd;
        Cmd = Cmd->NextCmd;

        Old->Execute( GetContext() );
        Old->~SRHIRenderCommand();
    }

    CmdList.First = nullptr;
    CmdList.Last = nullptr;
    CmdList.Reset();
}
