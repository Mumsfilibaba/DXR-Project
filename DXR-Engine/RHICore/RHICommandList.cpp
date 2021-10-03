#include "RHICommandList.h"

#include "Core/Debug/Profiler.h"

CRHICommandQueue GCommandQueue;

CRHICommandQueue::CRHICommandQueue()
    : CmdContext( nullptr )
    , NumDrawCalls( 0 )
    , NumDispatchCalls( 0 )
    , NumCommands( 0 )
{
}

CRHICommandQueue::~CRHICommandQueue()
{
    // Empty for now
}

void CRHICommandQueue::ExecuteCommandList( CRHICommandList& CmdList )
{
    // Execute
    GetContext().Begin();

    {
        TRACE_FUNCTION_SCOPE();

        // The statistics are only valid for the last call to execute command-list 
        ResetStatistics();

        InternalExecuteCommandList( CmdList );
    }

    GetContext().End();
}

void CRHICommandQueue::ExecuteCommandLists( CRHICommandList* const* CmdLists, uint32 NumCmdLists )
{
    // Execute
    GetContext().Begin();

    {
        TRACE_FUNCTION_SCOPE();

        // The statistics are only valid for the last call to execute commandlist 
        ResetStatistics();

        for ( uint32 i = 0; i < NumCmdLists; i++ )
        {
            CRHICommandList* CurrentCmdList = CmdLists[i];
            InternalExecuteCommandList( *CurrentCmdList );
        }
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

    NumDrawCalls     += CmdList.GetNumDrawCalls();
    NumDispatchCalls += CmdList.GetNumDispatchCalls();
    NumCommands      += CmdList.GetNumCommands();

    CmdList.First = nullptr;
    CmdList.Last = nullptr;
    CmdList.Reset();
}
