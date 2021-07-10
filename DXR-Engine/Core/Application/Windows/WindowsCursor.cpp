#include "WindowsCursor.h"

GenericCursor* WindowsCursor::Create( LPCSTR CursorName )
{
    TRef<WindowsCursor> NewCursor = DBG_NEW WindowsCursor();
    if ( !NewCursor->Init( CursorName ) )
    {
        return nullptr;
    }
    else
    {
        return NewCursor.ReleaseOwnership();
    }
}

WindowsCursor::WindowsCursor()
    : GenericCursor()
    , Cursor( 0 )
    , CursorName( nullptr )
{
}

WindowsCursor::~WindowsCursor()
{
    if ( !CursorName )
    {
        DestroyCursor( Cursor );
    }
}

bool WindowsCursor::Init( LPCSTR InCursorName )
{
    CursorName = InCursorName;
    if ( CursorName )
    {
        Cursor = LoadCursor( 0, CursorName );
        return true;
    }
    else
    {
        return false;
    }
}