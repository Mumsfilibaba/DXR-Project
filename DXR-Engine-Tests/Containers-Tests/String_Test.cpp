#include "String_Test.h"

#include <Core/Containers/StaticString.h>
#include <Core/Containers/String.h>

#include <iostream>

#define PrintString(Str) \
    std::cout << #Str << "= " << Str.CStr() << std::endl;

void TString_Test( const char* Args )
{
    std::cout << std::endl << "----Testing StaticString----" << std::endl << std::endl;
	
    CStringView StringView( "Hello StringView" );

    CStaticString<64> StaticString0;
    PrintString( StaticString0 );
    CStaticString<64> StaticString1 = "Hello String";
    PrintString( StaticString1 );
    CStaticString<64> StaticString2 = CStaticString<64>( Args, 4 );
    PrintString( StaticString2 );
    CStaticString<64> StaticString3 = CStaticString<64>( StringView );
    PrintString( StaticString3 );
    CStaticString<64> StaticString4 = StaticString1;
    PrintString( StaticString4 );
    CStaticString<64> StaticString5 = Move(StaticString2);
    PrintString( StaticString5 );

    StaticString0.Append("Appended String");
    PrintString( StaticString0 );
    StaticString5.Append( StaticString0 );
    PrintString( StaticString5 );

    CStaticString<64> StaticString6;
    StaticString6.Format( "Formatted String=%.4f", 0.004f );
    PrintString( StaticString6 );

    StaticString6.Append( ' ' );
    PrintString( StaticString6 );

    StaticString6.AppendFormat( "Formatted String=%.4f", 0.0077f );
    PrintString( StaticString6 );

    CStaticString<64> LowerStaticString6 = StaticString6.ToLower();
    PrintString( LowerStaticString6 );

    CStaticString<64> UpperStaticString6 = StaticString6.ToUpper();
    PrintString( UpperStaticString6 );

    StaticString6.Clear();
    PrintString( StaticString6 );

    StaticString6.Append( "    Trimmable String    " );
    PrintString( StaticString6 );

    CStaticString<64> TrimmedStaticString6 = StaticString6.Trim();
    TrimmedStaticString6.Append( '*' );
    PrintString( TrimmedStaticString6 );

    StaticString6.Clear();
    PrintString( StaticString6 );

    StaticString6.Append( "123456789" );
    PrintString( StaticString6 );

    CStaticString<64> ReversedStaticString6 = StaticString6.Reverse();
    PrintString( ReversedStaticString6 );

    CStaticString<64> SearchString = "Search Me";
    PrintString( SearchString );

    std::cout << "Position=" << SearchString.Find( "Me" ) << std::endl;
    std::cout << "Position=" << SearchString.Find( 'M' ) << std::endl;

    std::cout << "Position=" << SearchString.FindOneOf( "ec" ) << std::endl;
    std::cout << "Position=" << SearchString.FindOneOf( "Mc" ) << std::endl;
}
