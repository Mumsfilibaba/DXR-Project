#include "RHICommandList.h"

#include "Core/Debug/Profiler.h"

void* operator new( size_t Size, CCommandAllocator& Allocator )
{
    return Allocator.Allocate( static_cast<uint32>(Size), 16 );
}

void* operator new[]( size_t Size, CCommandAllocator& Allocator )
{
    return Allocator.Allocate( static_cast<uint32>(Size), 16 );
}

void operator delete ( void*, CCommandAllocator& )
{
}

void operator delete[]( void*, CCommandAllocator& )
{
}

CCommandAllocator::CCommandAllocator( uint32 StartSize )
    : Memory( nullptr )
    , Size( StartSize )
    , Offset( 0 )
{
    Memory = CMemory::Realloc( Memory, Size );
}

CCommandAllocator::~CCommandAllocator()
{
    SafeDelete( Memory );
}

void* CCommandAllocator::Allocate( uint32 SizeInBytes, uint32 Alignment )
{
    Assert( Memory != nullptr );

    const uint32 AlignedSize = NMath::AlignUp( SizeInBytes, Alignment );
    const uint32 NewOffset   = Offset + AlignedSize;
    if ( NewOffset >= Size )
    {
        void* Result = Memory + Offset;
        Offset = NewOffset;
        return Result;
    }
    else
    {
        const uint32 NewSize = Size + Size;
        Memory = CMemory::Realloc( Memory, NewSize );
        Size = NewSize;
        
        void* Result = Memory + Offset;
        Offset = NewOffset;
        return Result;
    }
}

void CCommandAllocator::Reset()
{
    // TODO: Calculate the average amount needed and resize the allocator to a smaller size to prevent to much waste
    Offset = 0;
}

CRHICommandQueue CRHICommandQueue::Instance;

CRHICommandQueue::CRHICommandQueue()
    : CmdContext( nullptr )
    , NumDrawCalls( 0 )
    , NumDispatchCalls( 0 )
    , NumCommands( 0 )
{
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
    if ( CmdList.First )
    {
        SRHIRenderCommand* CurrentCmd = CmdList.First;
        while ( CurrentCmd != nullptr )
        {
            SRHIRenderCommand* PreviousCmd = CurrentCmd;
            CurrentCmd = CurrentCmd->NextCmd;

            PreviousCmd->Execute( GetContext() );
            PreviousCmd->~SRHIRenderCommand();
        }

        NumDrawCalls += CmdList.GetNumDrawCalls();
        NumDispatchCalls += CmdList.GetNumDispatchCalls();
        NumCommands += CmdList.GetNumCommands();

        CmdList.First = nullptr;
        CmdList.Last = nullptr;
        CmdList.Reset();
    }
}
