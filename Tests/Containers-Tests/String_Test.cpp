#include "String_Test.h"

#include <Core/Containers/StaticString.h>
#include <Core/Containers/String.h>

#include <iostream>

#define PrintString(Str) \
    { std::cout << #Str << "= " << Str.GetCString() << std::endl; }

#define PrintStringView(Str)                       \
    {                                              \
        std::cout << #Str << "= ";                 \
        for ( int32 i = 0; i < Str.Length(); i++ ) \
        {                                          \
            std::cout << Str[i];                   \
        }                                          \
        std::cout << std::endl;                    \
    }

#define PrintWideString(Str) \
    { std::wcout << #Str << "= " << Str.GetCString() << std::endl; }

#define PrintWideStringView(Str)                   \
    {                                              \
        std::wcout << #Str << "= ";                \
        for ( int32 i = 0; i < Str.Length(); i++ ) \
        {                                          \
            std::wcout << Str[i];                  \
        }                                          \
        std::wcout << std::endl;                   \
    }

void TString_Test( const char* Args )
{
    {
        std::cout << std::endl << "----Testing FStaticString----" << std::endl << std::endl;

        FStringView FStringView( "Hello FStringView" );

        FStaticString<64> StaticString0;
        PrintString( StaticString0 );
        FStaticString<64> StaticString1 = "Hello String";
        PrintString( StaticString1 );
        FStaticString<64> StaticString2 = FStaticString<64>( Args, 7 );
        PrintString( StaticString2 );
        FStaticString<64> StaticString3 = FStaticString<64>( FStringView );
        PrintString( StaticString3 );
        FStaticString<64> StaticString4 = StaticString1;
        PrintString( StaticString4 );
        FStaticString<64> StaticString5 = Move( StaticString2 );
        PrintString( StaticString5 );

        StaticString0.Append( "Appended String" );
        PrintString( StaticString0 );
        StaticString0.Append( '_' );
        PrintString( StaticString0 );
        StaticString5.Append( StaticString0 );
        PrintString( StaticString5 );

        FStaticString<64> StaticString6;
        StaticString6.Format( "Formatted String=%.4f", 0.004f );
        PrintString( StaticString6 );

        StaticString6.Append( '_' );
        PrintString( StaticString6 );

        StaticString6.AppendFormat( "Formatted String=%.4f", 0.0077f );
        PrintString( StaticString6 );

        FStaticString<64> LowerStaticString6 = StaticString6.ToLower();
        PrintString( LowerStaticString6 );

        FStaticString<64> UpperStaticString6 = StaticString6.ToUpper();
        PrintString( UpperStaticString6 );

        StaticString6.Clear();
        PrintString( StaticString6 );

        StaticString6.Append( "    Trimmable String    " );
        PrintString( StaticString6 );

        FStaticString<64> TrimmedStaticString6 = StaticString6.Trim();
        TrimmedStaticString6.Append( '*' );
        PrintString( TrimmedStaticString6 );

        StaticString6.Clear();
        PrintString( StaticString6 );

        StaticString6.Append( "123456789" );
        PrintString( StaticString6 );

        FStaticString<64> ReversedStaticString6 = StaticString6.Reverse();
        PrintString( ReversedStaticString6 );

        FStaticString<64> SearchString = "0123MeSearch89Me89";
        PrintString( SearchString );

        std::cout << "Position=" << SearchString.Find( "Me" ) << std::endl;
        std::cout << "Position=" << SearchString.Find( 'M' ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneOf( "ec" ) << std::endl;
        std::cout << "Position=" << SearchString.FindOneOf( "Mc" ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneNotOf( "0123456789" ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFind( "Me" ) << std::endl;
        std::cout << "Position=" << SearchString.ReverseFind( 'M' ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFindOneOf( "hMc" ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFindOneNotOf( "0123456789" ) << std::endl;

        FStaticString<64> CompareString0 = "COMPARE";
        PrintString( CompareString0 );

        FStaticString<64> CompareString1 = "compare";
        PrintString( CompareString1 );

        std::cout << "Compare=" << CompareString0.Compare( CompareString1 ) << std::endl;
        std::cout << "CompareNoCase=" << CompareString0.CompareNoCase( CompareString1 ) << std::endl;

        CompareString1.Resize( 20, 'A' );
        PrintString( CompareString1 );

        char Buffer[6];
        Buffer[5] = 0;
        CompareString1.Copy( Buffer, 5, 3 );
        std::cout << "Buffer=" << Buffer << std::endl;

        CompareString0.Insert( "lower", 4 );
        PrintString( CompareString0 );

        CompareString0.Replace( "upper", 4 );
        PrintString( CompareString0 );

        CompareString0.Replace( 'X', 0 );
        PrintString( CompareString0 );

        FStaticString<64> CombinedString = CompareString0 + '5';
        PrintString( CombinedString );

        CombinedString = '5' + CombinedString;
        PrintString( CombinedString );

        CombinedString = CombinedString + "Appended";
        PrintString( CombinedString );

        CombinedString = "Inserted" + CombinedString;
        PrintString( CombinedString );

        CombinedString = CombinedString + CombinedString;
        PrintString( CombinedString );

        FStaticString<64> TestString = "Test";
        PrintString( TestString );

        std::cout << "operator== : " << std::boolalpha << ("Test" == TestString) << std::endl;
        std::cout << "operator== : " << std::boolalpha << (TestString == "Test") << std::endl;
        std::cout << "operator== : " << std::boolalpha << (CombinedString == CombinedString) << std::endl;

        std::cout << "operator!= : " << std::boolalpha << ("Test" != TestString) << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (TestString != "Test") << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (CombinedString != CombinedString) << std::endl;

        std::cout << "operator<= : " << std::boolalpha << ("Test" <= TestString) << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (TestString <= "Test") << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (CombinedString <= CombinedString) << std::endl;

        std::cout << "operator< : " << std::boolalpha << ("Test" < TestString) << std::endl;
        std::cout << "operator< : " << std::boolalpha << (TestString < "Test") << std::endl;
        std::cout << "operator< : " << std::boolalpha << (CombinedString < CombinedString) << std::endl;

        std::cout << "operator>= : " << std::boolalpha << ("Test" >= TestString) << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (TestString >= "Test") << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (CombinedString >= CombinedString) << std::endl;

        std::cout << "operator> : " << std::boolalpha << ("Test" > TestString) << std::endl;
        std::cout << "operator> : " << std::boolalpha << (TestString > "Test") << std::endl;
        std::cout << "operator> : " << std::boolalpha << (CombinedString > CombinedString) << std::endl;

        for ( char C : TestString )
        {
            std::cout << C << std::endl;
        }

        for ( int32 Index = 0; Index < TestString.Length(); Index++ )
        {
            std::cout << Index << '=' << TestString[Index] << std::endl;
        }

        FStaticStringWide<64> WideCompareString = CharToWide<64>( CompareString0 );
        PrintWideString( WideCompareString );
    }

    {
        std::cout << std::endl << "----Testing FStaticStringWide----" << std::endl << std::endl;

        FStringViewWide FStringView( L"Hello FStringView" );

        const wchar_t* SomeWideStringInsteadOfArgs = L"/Users/SomeFolder/Blabla/BlaBla";

        FStaticStringWide<64> StaticString0;
        PrintWideString( StaticString0 );
        FStaticStringWide<64> StaticString1 = L"Hello String";
        PrintWideString( StaticString1 );
        FStaticStringWide<64> StaticString2 = FStaticStringWide<64>( SomeWideStringInsteadOfArgs, 7 );
        PrintWideString( StaticString2 );
        FStaticStringWide<64> StaticString3 = FStaticStringWide<64>( FStringView );
        PrintWideString( StaticString3 );
        FStaticStringWide<64> StaticString4 = StaticString1;
        PrintWideString( StaticString4 );
        FStaticStringWide<64> StaticString5 = Move( StaticString2 );
        PrintWideString( StaticString5 );

        StaticString0.Append( L"Appended String" );
        PrintWideString( StaticString0 );
        StaticString0.Append( L'_' );
        PrintWideString( StaticString0 );
        StaticString5.Append( StaticString0 );
        PrintWideString( StaticString5 );

        FStaticStringWide<64> StaticString6;
        StaticString6.Format( L"Formatted String=%.4f", 0.004f );
        PrintWideString( StaticString6 );

        StaticString6.Append( '_' );
        PrintWideString( StaticString6 );

        StaticString6.AppendFormat( L"Formatted String=%.4f", 0.0077f );
        PrintWideString( StaticString6 );

        FStaticStringWide<64> LowerStaticString6 = StaticString6.ToLower();
        PrintWideString( LowerStaticString6 );

        FStaticStringWide<64> UpperStaticString6 = StaticString6.ToUpper();
        PrintWideString( UpperStaticString6 );

        StaticString6.Clear();
        PrintWideString( StaticString6 );

        StaticString6.Append( L"    Trimmable String    " );
        PrintWideString( StaticString6 );

        FStaticStringWide<64> TrimmedStaticString6 = StaticString6.Trim();
        TrimmedStaticString6.Append( '*' );
        PrintWideString( TrimmedStaticString6 );

        StaticString6.Clear();
        PrintWideString( StaticString6 );

        StaticString6.Append( L"123456789" );
        PrintWideString( StaticString6 );

        FStaticStringWide<64> ReversedStaticString6 = StaticString6.Reverse();
        PrintWideString( ReversedStaticString6 );

        FStaticStringWide<64> SearchString = L"0123MeSearch89Me89";
        PrintWideString( SearchString );

        std::cout << "Position=" << SearchString.Find( L"Me" ) << std::endl;
        std::cout << "Position=" << SearchString.Find( L'M' ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneOf( L"ec" ) << std::endl;
        std::cout << "Position=" << SearchString.FindOneOf( L"Mc" ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneNotOf( L"0123456789" ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFind( L"Me" ) << std::endl;
        std::cout << "Position=" << SearchString.ReverseFind( L'M' ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFindOneOf( L"hMc" ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFindOneNotOf( L"0123456789" ) << std::endl;

        FStaticStringWide<64> CompareString0 = L"COMPARE";
        PrintWideString( CompareString0 );

        FStaticStringWide<64> CompareString1 = L"compare";
        PrintWideString( CompareString1 );

        std::cout << "Compare=" << CompareString0.Compare( CompareString1 ) << std::endl;
        std::cout << "CompareNoCase=" << CompareString0.CompareNoCase( CompareString1 ) << std::endl;

        CompareString1.Resize( 20, 'A' );
        PrintWideString( CompareString1 );

        wchar_t Buffer[6];
        Buffer[5] = 0;
        CompareString1.Copy( Buffer, 5, 3 );
        std::wcout << L"Buffer=" << Buffer << std::endl;

        CompareString0.Insert( L"lower", 4 );
        PrintWideString( CompareString0 );

        CompareString0.Replace( L"upper", 4 );
        PrintWideString( CompareString0 );

        CompareString0.Replace( L'X', 0 );
        PrintWideString( CompareString0 );

        FStaticStringWide<64> CombinedString = CompareString0 + L'5';
        PrintWideString( CombinedString );

        CombinedString = L'5' + CombinedString;
        PrintWideString( CombinedString );

        CombinedString = CombinedString + L"Appended";
        PrintWideString( CombinedString );

        CombinedString = L"Inserted" + CombinedString;
        PrintWideString( CombinedString );

        CombinedString = CombinedString + CombinedString;
        PrintWideString( CombinedString );

        FStaticStringWide<64> TestString = L"Test";
        PrintWideString( TestString );

        std::cout << "operator== : " << std::boolalpha << (L"Test" == TestString) << std::endl;
        std::cout << "operator== : " << std::boolalpha << (TestString == L"Test") << std::endl;
        std::cout << "operator== : " << std::boolalpha << (CombinedString == CombinedString) << std::endl;

        std::cout << "operator!= : " << std::boolalpha << (L"Test" != TestString) << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (TestString != L"Test") << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (CombinedString != CombinedString) << std::endl;

        std::cout << "operator<= : " << std::boolalpha << (L"Test" <= TestString) << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (TestString <= L"Test") << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (CombinedString <= CombinedString) << std::endl;

        std::cout << "operator< : " << std::boolalpha << (L"Test" < TestString) << std::endl;
        std::cout << "operator< : " << std::boolalpha << (TestString < L"Test") << std::endl;
        std::cout << "operator< : " << std::boolalpha << (CombinedString < CombinedString) << std::endl;

        std::cout << "operator>= : " << std::boolalpha << (L"Test" >= TestString) << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (TestString >= L"Test") << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (CombinedString >= CombinedString) << std::endl;

        std::cout << "operator> : " << std::boolalpha << (L"Test" > TestString) << std::endl;
        std::cout << "operator> : " << std::boolalpha << (TestString > L"Test") << std::endl;
        std::cout << "operator> : " << std::boolalpha << (CombinedString > CombinedString) << std::endl;

        for ( wchar_t C : TestString )
        {
            std::wcout << C << std::endl;
        }

        for ( int32 Index = 0; Index < TestString.Length(); Index++ )
        {
            std::wcout << Index << L'=' << TestString[Index] << std::endl;
        }

        FStaticString<64> CharCompareString = WideToChar<64>( CompareString0 );
        PrintString( CharCompareString );
    }

    {
        std::cout << std::endl << "----Testing String----" << std::endl << std::endl;

        FStringView FStringView( "Hello FStringView" );

        FString String0;
        PrintString( String0 );
        FString String1 = "Hello String";
        PrintString( String1 );
        FString String2 = FString( Args, 7 );
        PrintString( String2 );
        FString String3 = FString( FStringView );
        PrintString( String3 );
        FString String4 = String1;
        PrintString( String4 );
        FString String5 = Move( String2 );
        PrintString( String5 );

        String0.Append( "Appended String" );
        PrintString( String0 );
        String0.Append( '_' );
        PrintString( String0 );
        String5.Append( String0 );
        PrintString( String5 );

        FString String6;
        String6.Format( "Formatted String=%.4f", 0.004f );
        PrintString( String6 );

        String6.Append( '_' );
        PrintString( String6 );

        String6.AppendFormat( "Formatted String=%.4f", 0.0077f );
        PrintString( String6 );

        FString LowerString6 = String6.ToLower();
        PrintString( LowerString6 );

        FString UpperString6 = String6.ToUpper();
        PrintString( UpperString6 );

        String6.Clear();
        PrintString( String6 );

        String6.Append( "    Trimmable String    " );
        PrintString( String6 );

        FString TrimmedString6 = String6.Trim();
        TrimmedString6.Append( '*' );
        PrintString( TrimmedString6 );

        String6.Clear();
        PrintString( String6 );

        String6.Append( "123456789" );
        PrintString( String6 );

        FString ReversedString6 = String6.Reverse();
        PrintString( ReversedString6 );

        FString SearchString = "0123MeSearch89Me89";
        PrintString( SearchString );

        std::cout << "Position=" << SearchString.Find( "Me" ) << std::endl;
        std::cout << "Position=" << SearchString.Find( 'M' ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneOf( "ec" ) << std::endl;
        std::cout << "Position=" << SearchString.FindOneOf( "Mc" ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneNotOf( "0123456789" ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFind( "Me" ) << std::endl;
        std::cout << "Position=" << SearchString.ReverseFind( 'M' ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFindOneOf( "hMc" ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFindOneNotOf( "0123456789" ) << std::endl;

        FString CompareString0 = "COMPARE";
        PrintString( CompareString0 );

        FString CompareString1 = "compare";
        PrintString( CompareString1 );

        std::cout << "Compare=" << CompareString0.Compare( CompareString1 ) << std::endl;
        std::cout << "CompareNoCase=" << CompareString0.CompareNoCase( CompareString1 ) << std::endl;

        CompareString1.Resize( 20, 'A' );
        PrintString( CompareString1 );

        char Buffer[6];
        Buffer[5] = 0;
        CompareString1.Copy( Buffer, 5, 3 );
        std::cout << "Buffer=" << Buffer << std::endl;

        CompareString0.Insert( "lower", 4 );
        PrintString( CompareString0 );

        CompareString0.Replace( "upper", 4 );
        PrintString( CompareString0 );

        CompareString0.Replace( 'X', 0 );
        PrintString( CompareString0 );

        FString CombinedString = CompareString0 + '5';
        PrintString( CombinedString );

        CombinedString = '5' + CombinedString;
        PrintString( CombinedString );

        CombinedString = CombinedString + "Appended";
        PrintString( CombinedString );

        CombinedString = "Inserted" + CombinedString;
        PrintString( CombinedString );

        CombinedString = CombinedString + CombinedString;
        PrintString( CombinedString );

        FString TestString = "Test";
        PrintString( TestString );

        std::cout << "operator== : " << std::boolalpha << ("Test" == TestString) << std::endl;
        std::cout << "operator== : " << std::boolalpha << (TestString == "Test") << std::endl;
        std::cout << "operator== : " << std::boolalpha << (CombinedString == CombinedString) << std::endl;

        std::cout << "operator!= : " << std::boolalpha << ("Test" != TestString) << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (TestString != "Test") << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (CombinedString != CombinedString) << std::endl;

        std::cout << "operator<= : " << std::boolalpha << ("Test" <= TestString) << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (TestString <= "Test") << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (CombinedString <= CombinedString) << std::endl;

        std::cout << "operator< : " << std::boolalpha << ("Test" < TestString) << std::endl;
        std::cout << "operator< : " << std::boolalpha << (TestString < "Test") << std::endl;
        std::cout << "operator< : " << std::boolalpha << (CombinedString < CombinedString) << std::endl;

        std::cout << "operator>= : " << std::boolalpha << ("Test" >= TestString) << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (TestString >= "Test") << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (CombinedString >= CombinedString) << std::endl;

        std::cout << "operator> : " << std::boolalpha << ("Test" > TestString) << std::endl;
        std::cout << "operator> : " << std::boolalpha << (TestString > "Test") << std::endl;
        std::cout << "operator> : " << std::boolalpha << (CombinedString > CombinedString) << std::endl;

        for ( char C : TestString )
        {
            std::cout << C << std::endl;
        }

        for ( int32 Index = 0; Index < TestString.Length(); Index++ )
        {
            std::cout << Index << '=' << TestString[Index] << std::endl;
        }

        FStringWide WideCompareString = CharToWide( CompareString0 );
        PrintWideString( WideCompareString );
    }

    {
        std::cout << std::endl << "----Testing FStringWide----" << std::endl << std::endl;

        FStringViewWide FStringView( L"Hello FStringView" );

        const wchar_t* SomeWideStringInsteadOfArgs = L"/Users/SomeFolder/Blabla/BlaBla";

        FStringWide String0;
        PrintWideString( String0 );
        FStringWide String1 = L"Hello String";
        PrintWideString( String1 );
        FStringWide String2 = FStringWide( SomeWideStringInsteadOfArgs, 7 );
        PrintWideString( String2 );
        FStringWide String3 = FStringWide( FStringView );
        PrintWideString( String3 );
        FStringWide String4 = String1;
        PrintWideString( String4 );
        FStringWide String5 = Move( String2 );
        PrintWideString( String5 );

        String0.Append( L"Appended String" );
        PrintWideString( String0 );
        String0.Append( L'_' );
        PrintWideString( String0 );
        String5.Append( String0 );
        PrintWideString( String5 );

        FStringWide String6;
        String6.Format( L"Formatted String=%.4f", 0.004f );
        PrintWideString( String6 );

        String6.Append( '_' );
        PrintWideString( String6 );

        String6.AppendFormat( L"Formatted String=%.4f", 0.0077f );
        PrintWideString( String6 );

        FStringWide LowerString6 = String6.ToLower();
        PrintWideString( LowerString6 );

        FStringWide UpperString6 = String6.ToUpper();
        PrintWideString( UpperString6 );

        String6.Clear();
        PrintWideString( String6 );

        String6.Append( L"    Trimmable String    " );
        PrintWideString( String6 );

        FStringWide TrimmedString6 = String6.Trim();
        TrimmedString6.Append( L'*' );
        PrintWideString( TrimmedString6 );

        String6.Clear();
        PrintWideString( String6 );

        String6.Append( L"123456789" );
        PrintWideString( String6 );

        FStringWide ReversedString6 = String6.Reverse();
        PrintWideString( ReversedString6 );

        FStringWide SearchString = L"0123MeSearch89Me89";
        PrintWideString( SearchString );

        std::cout << "Position=" << SearchString.Find( L"Me" ) << std::endl;
        std::cout << "Position=" << SearchString.Find( L'M' ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneOf( L"ec" ) << std::endl;
        std::cout << "Position=" << SearchString.FindOneOf( L"Mc" ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneNotOf( L"0123456789" ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFind( L"Me" ) << std::endl;
        std::cout << "Position=" << SearchString.ReverseFind( L'M' ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFindOneOf( L"hMc" ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFindOneNotOf( L"0123456789" ) << std::endl;

        FStringWide CompareString0 = L"COMPARE";
        PrintWideString( CompareString0 );

        FStringWide CompareString1 = L"compare";
        PrintWideString( CompareString1 );

        std::cout << "Compare=" << CompareString0.Compare( CompareString1 ) << std::endl;
        std::cout << "CompareNoCase=" << CompareString0.CompareNoCase( CompareString1 ) << std::endl;

        CompareString1.Resize( 20, 'A' );
        PrintWideString( CompareString1 );

        wchar_t Buffer[6];
        Buffer[5] = 0;
        CompareString1.Copy( Buffer, 5, 3 );
        std::wcout << "Buffer=" << Buffer << std::endl;

        CompareString0.Insert( L"lower", 4 );
        PrintWideString( CompareString0 );

        CompareString0.Replace( L"upper", 4 );
        PrintWideString( CompareString0 );

        CompareString0.Replace( 'X', 0 );
        PrintWideString( CompareString0 );

        FStringWide CombinedString = CompareString0 + L'5';
        PrintWideString( CombinedString );

        CombinedString = L'5' + CombinedString;
        PrintWideString( CombinedString );

        CombinedString = CombinedString + L"Appended";
        PrintWideString( CombinedString );

        CombinedString = L"Inserted" + CombinedString;
        PrintWideString( CombinedString );

        CombinedString = CombinedString + CombinedString;
        PrintWideString( CombinedString );

        FStringWide TestString = L"Test";
        PrintWideString( TestString );

        std::cout << "operator== : " << std::boolalpha << (L"Test" == TestString) << std::endl;
        std::cout << "operator== : " << std::boolalpha << (TestString == L"Test") << std::endl;
        std::cout << "operator== : " << std::boolalpha << (CombinedString == CombinedString) << std::endl;

        std::cout << "operator!= : " << std::boolalpha << (L"Test" != TestString) << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (TestString != L"Test") << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (CombinedString != CombinedString) << std::endl;

        std::cout << "operator<= : " << std::boolalpha << (L"Test" <= TestString) << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (TestString <= L"Test") << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (CombinedString <= CombinedString) << std::endl;

        std::cout << "operator< : " << std::boolalpha << (L"Test" < TestString) << std::endl;
        std::cout << "operator< : " << std::boolalpha << (TestString < L"Test") << std::endl;
        std::cout << "operator< : " << std::boolalpha << (CombinedString < CombinedString) << std::endl;

        std::cout << "operator>= : " << std::boolalpha << (L"Test" >= TestString) << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (TestString >= L"Test") << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (CombinedString >= CombinedString) << std::endl;

        std::cout << "operator> : " << std::boolalpha << (L"Test" > TestString) << std::endl;
        std::cout << "operator> : " << std::boolalpha << (TestString > L"Test") << std::endl;
        std::cout << "operator> : " << std::boolalpha << (CombinedString > CombinedString) << std::endl;

        for ( wchar_t C : TestString )
        {
            std::wcout << C << std::endl;
        }

        for ( int32 Index = 0; Index < TestString.Length(); Index++ )
        {
            std::wcout << Index << L'=' << TestString[Index] << std::endl;
        }

        FString CharCompareString = WideToChar( CompareString0 );
        PrintString( CharCompareString );
    }

    {
        std::cout << std::endl << "----Testing FStringView----" << std::endl << std::endl;

        const char* LongString = "This is a long string";

        FStringView StringView0;
        PrintStringView( StringView0 );
        FStringView StringView1 = LongString;
        PrintStringView( StringView1 );
        FStringView StringView2 = FStringView( LongString + 5, 4 );
        PrintStringView( StringView2 );

        char Buffer[6] = { };
        Buffer[5] = 0;
        StringView1.Copy( Buffer, 5, 4 );
        std::cout << "Buffer=" << Buffer << std::endl;

        FStringView StringView3 = "    Trimmable String    ";
        PrintStringView( StringView3 );

        FStringView StringView4 = StringView3.Trim();
        PrintStringView( StringView4 );

        FStringView StringView5 = FStringView( "COMPAREPostfix", 7 );
        PrintStringView( StringView5 );

        FStringView StringView6 = FStringView( "comparePostfix", 7 );
        PrintStringView( StringView6 );

        std::cout << "Compare=" << StringView5.Compare( StringView6 ) << std::endl;
        std::cout << "CompareNoCase=" << StringView5.CompareNoCase( StringView6 ) << std::endl;

        StringView6.Clear();
        PrintStringView( StringView6 );

        FStringView SearchString = "0123MeSearch89Me89";
        PrintStringView( SearchString );

        std::cout << "Position=" << SearchString.Find( "Me" ) << std::endl;
        std::cout << "Position=" << SearchString.Find( 'M' ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneOf( "ec" ) << std::endl;
        std::cout << "Position=" << SearchString.FindOneOf( "Mc" ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneNotOf( "0123456789" ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFind( "Me" ) << std::endl;
        std::cout << "Position=" << SearchString.ReverseFind( 'M' ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFindOneOf( "hMc" ) << std::endl;
        std::cout << "Position=" << SearchString.ReverseFindOneNotOf( "0123456789" ) << std::endl;

        FStringView TestString = "Test";
        PrintStringView( TestString );

        std::cout << "operator== : " << std::boolalpha << ("Test" == TestString) << std::endl;
        std::cout << "operator== : " << std::boolalpha << (TestString == "Test") << std::endl;
        std::cout << "operator== : " << std::boolalpha << (TestString == TestString) << std::endl;

        std::cout << "operator!= : " << std::boolalpha << ("Test" != TestString) << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (TestString != "Test") << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (TestString != TestString) << std::endl;

        std::cout << "operator<= : " << std::boolalpha << ("Test" <= TestString) << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (TestString <= "Test") << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (TestString <= TestString) << std::endl;

        std::cout << "operator< : " << std::boolalpha << ("Test" < TestString) << std::endl;
        std::cout << "operator< : " << std::boolalpha << (TestString < "Test") << std::endl;
        std::cout << "operator< : " << std::boolalpha << (TestString < TestString) << std::endl;

        std::cout << "operator>= : " << std::boolalpha << ("Test" >= TestString) << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (TestString >= "Test") << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (TestString >= TestString) << std::endl;

        std::cout << "operator> : " << std::boolalpha << ("Test" > TestString) << std::endl;
        std::cout << "operator> : " << std::boolalpha << (TestString > "Test") << std::endl;
        std::cout << "operator> : " << std::boolalpha << (TestString > TestString) << std::endl;

        for ( char C : TestString )
        {
            std::cout << C << std::endl;
        }

        for ( int32 Index = 0; Index < TestString.Length(); Index++ )
        {
            std::cout << Index << '=' << TestString[Index] << std::endl;
        }
    }

    {
        std::cout << std::endl << "----Testing FStringViewWide----" << std::endl << std::endl;

        const wchar_t* LongString = L"This is a long string";

        FStringViewWide StringView0;
        PrintWideStringView( StringView0 );
        FStringViewWide StringView1 = LongString;
        PrintWideStringView( StringView1 );
        FStringViewWide StringView2 = FStringViewWide( LongString + 5, 4 );
        PrintWideStringView( StringView2 );

        wchar_t Buffer[6] = { };
        Buffer[5] = 0;
        StringView1.Copy( Buffer, 5, 4 );
        std::cout << "Buffer=" << Buffer << std::endl;

        FStringViewWide StringView3 = L"    Trimmable String    ";
        PrintWideStringView( StringView3 );

        FStringViewWide StringView4 = StringView3.Trim();
        PrintWideStringView( StringView4 );

        FStringViewWide StringView5 = FStringViewWide( L"COMPAREPostfix", 7 );
        PrintWideStringView( StringView5 );

        FStringViewWide StringView6 = FStringViewWide( L"comparePostfix", 7 );
        PrintWideStringView( StringView6 );

        std::cout << "Compare=" << StringView5.Compare( StringView6 ) << std::endl;
        std::cout << "CompareNoCase=" << StringView5.CompareNoCase( StringView6 ) << std::endl;

        StringView6.Clear();
        PrintWideStringView( StringView6 );

        FStringViewWide SearchString = L"0123MeSearch89Me89";
        PrintWideStringView( SearchString );

        std::cout << "Position=" << SearchString.Find( L"Me" ) << std::endl;
        std::cout << "Position=" << SearchString.Find( L'M' ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneOf( L"ec" ) << std::endl;
        std::cout << "Position=" << SearchString.FindOneOf( L"Mc" ) << std::endl;

        std::cout << "Position=" << SearchString.FindOneNotOf( L"0123456789" ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFind( L"Me" ) << std::endl;
        std::cout << "Position=" << SearchString.ReverseFind( L'M' ) << std::endl;

        std::cout << "Position=" << SearchString.ReverseFindOneOf( L"hMc" ) << std::endl;
        std::cout << "Position=" << SearchString.ReverseFindOneNotOf( L"0123456789" ) << std::endl;

        FStringViewWide TestString = L"Test";
        PrintWideStringView( TestString );

        std::cout << "operator== : " << std::boolalpha << (L"Test" == TestString) << std::endl;
        std::cout << "operator== : " << std::boolalpha << (TestString == L"Test") << std::endl;
        std::cout << "operator== : " << std::boolalpha << (TestString == TestString) << std::endl;

        std::cout << "operator!= : " << std::boolalpha << (L"Test" != TestString) << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (TestString != L"Test") << std::endl;
        std::cout << "operator!= : " << std::boolalpha << (TestString != TestString) << std::endl;

        std::cout << "operator<= : " << std::boolalpha << (L"Test" <= TestString) << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (TestString <= L"Test") << std::endl;
        std::cout << "operator<= : " << std::boolalpha << (TestString <= TestString) << std::endl;

        std::cout << "operator< : " << std::boolalpha << (L"Test" < TestString) << std::endl;
        std::cout << "operator< : " << std::boolalpha << (TestString < L"Test") << std::endl;
        std::cout << "operator< : " << std::boolalpha << (TestString < TestString) << std::endl;

        std::cout << "operator>= : " << std::boolalpha << (L"Test" >= TestString) << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (TestString >= L"Test") << std::endl;
        std::cout << "operator>= : " << std::boolalpha << (TestString >= TestString) << std::endl;

        std::cout << "operator> : " << std::boolalpha << (L"Test" > TestString) << std::endl;
        std::cout << "operator> : " << std::boolalpha << (TestString > L"Test") << std::endl;
        std::cout << "operator> : " << std::boolalpha << (TestString > TestString) << std::endl;

        for ( wchar_t C : TestString )
        {
            std::wcout << C << std::endl;
        }

        for ( int32 Index = 0; Index < TestString.Length(); Index++ )
        {
            std::wcout << Index << '=' << TestString[Index] << std::endl;
        }
    }
}
