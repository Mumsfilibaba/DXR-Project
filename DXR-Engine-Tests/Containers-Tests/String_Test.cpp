#include "String_Test.h"

#include <Core/Containers/FixedString.h>

#include <iostream>

#define PrintString(Str) \
    std::cout << #Str << "= " << Str.CStr() << std::endl;

void TString_Test( const char* Args )
{
    std::cout << std::endl << "----Testing FixedString----" << std::endl << std::endl;
    CStringView StringView( "Hello StringView" );

    CFixedString<64> FixedString0;
    PrintString( FixedString0 );
    CFixedString<64> FixedString1 = "Hello String";
    PrintString( FixedString1 );
    CFixedString<64> FixedString2 = CFixedString<64>( Args, 4 );
    PrintString( FixedString2 );
    CFixedString<64> FixedString3 = CFixedString<64>(StringView);
    PrintString( FixedString3 );
    CFixedString<64> FixedString4 = FixedString1;
    PrintString( FixedString4 );
    CFixedString<64> FixedString5 = Move(FixedString2);
    PrintString( FixedString5 );

    FixedString0.Append("Appended String");
    PrintString( FixedString0 );
    FixedString5.Append( FixedString0 );
    PrintString( FixedString5 );

    CFixedString<64> FixedString6;
    FixedString6.Format( "Formatted String=%.4f", 0.004f );
    PrintString( FixedString6 );

    FixedString6.Append( ' ' );
    PrintString( FixedString6 );

    FixedString6.AppendFormat( "Formatted String=%.4f", 0.0077f );
    PrintString( FixedString6 );

    CFixedString<64> LowerFixedString6 = FixedString6.ToLower();
    PrintString( LowerFixedString6 );

    CFixedString<64> UpperFixedString6 = FixedString6.ToUpper();
    PrintString( UpperFixedString6 );

    FixedString6.Clear();
    PrintString( FixedString6 );

    FixedString6.Append( "    Trimmable String    " );
    PrintString( FixedString6 );

    CFixedString<64> TrimmedFixedString6 = FixedString6.Trim();
    TrimmedFixedString6.Append( '*' );
    PrintString( TrimmedFixedString6 );

    FixedString6.Clear();
    PrintString( FixedString6 );

    FixedString6.Append( "123456789" );
    PrintString( FixedString6 );

    CFixedString<64> ReversedFixedString6 = FixedString6.Reverse();
    PrintString( ReversedFixedString6 );

    CFixedString<64> SearchString = "Search Me";
    PrintString( SearchString );

    std::cout << "Position=" << SearchString.Find( "Me" ) << std::endl;
    std::cout << "Position=" << SearchString.Find( 'M' ) << std::endl;

    std::cout << "Position=" << SearchString.FindOneOf( "ec" ) << std::endl;
    std::cout << "Position=" << SearchString.FindOneOf( "Mc" ) << std::endl;
}