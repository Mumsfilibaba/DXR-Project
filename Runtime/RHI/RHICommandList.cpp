#include "RHICommandList.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

CCommandAllocator::CCommandAllocator( uint32 StartSize )
    : CurrentMemory( nullptr )
    , Size( StartSize )
    , Offset( 0 )
    , DiscardedMemory()
{
    CurrentMemory = reinterpret_cast<uint8*>(CMemory::Malloc( Size ));
    Assert( CurrentMemory != nullptr );

    AverageMemoryUsage = Size;
}

CCommandAllocator::~CCommandAllocator()
{
    ReleaseDiscardedMemory();

    SafeDelete( CurrentMemory );
}

void* CCommandAllocator::Allocate( uint64 SizeInBytes, uint64 Alignment )
{
    Assert( CurrentMemory != nullptr );

    const uint64 AlignedSize = NMath::AlignUp( SizeInBytes, Alignment );
    const uint64 NewOffset = Offset + AlignedSize;
    if ( NewOffset <= Size )
    {
        void* Result = CurrentMemory + Offset;
        Offset = NewOffset;
        return Result;
    }
    else
    {
        // Discard the current pointer 
        DiscardedMemory.Emplace( CurrentMemory );

        // Allocate a new block of memory
        const uint64 NewSize = NMath::Max( Size + Size, AlignedSize );

        CurrentMemory = reinterpret_cast<uint8*>(CMemory::Malloc( NewSize ));
        Assert( CurrentMemory != nullptr );

        Size = NewSize;
        AverageMemoryUsage = Size;
        Offset = AlignedSize;

        // Return the newly allocated block
        return CurrentMemory;
    }
}

void CCommandAllocator::Reset()
{
    ReleaseDiscardedMemory();

    // Moving average for the memory usage
    const float Alpha = 0.2f;
    AverageMemoryUsage = uint64(Offset * Alpha) + uint64((1.0f - Alpha) * AverageMemoryUsage);

    // Resize if to much memory is used 
    const uint64 SlackSize = Size - AverageMemoryUsage;
    if ( NMemoryUtils::BytesToMegaBytes( SlackSize ) > 1 )
    {
        SafeDelete( CurrentMemory );

        const uint64 NewSize = AverageMemoryUsage + NMemoryUtils::MegaBytesToBytes( 1 );

        CurrentMemory = reinterpret_cast<uint8*>(CMemory::Malloc( NewSize ));
        Assert( CurrentMemory != nullptr );

        Size = NewSize;
        AverageMemoryUsage = Size;
    }

    // Reset
    Offset = 0;
}

void CCommandAllocator::ReleaseDiscardedMemory()
{
    for ( uint8* Memory : DiscardedMemory )
    {
        SafeDelete( Memory );
    }

    DiscardedMemory.Empty();
}

/* CommandQueue */

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
