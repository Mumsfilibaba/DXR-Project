#include "String_Test.h"

#include <Core/Containers/StaticString.h>
#include <Core/Containers/String.h>

#include <iostream>

#define PrintString(Str) \
	{ std::cout << #Str << "= " << Str.CStr() << std::endl; }

#define PrintWideString(Str) \
	{ std::wcout << #Str << "= " << Str.CStr() << std::endl; }

void TString_Test( const char* Args )
{
	{
		std::cout << std::endl << "----Testing CStaticString----" << std::endl << std::endl;
		
		CStringView StringView( "Hello StringView" );

		CStaticString<64> StaticString0;
		PrintString( StaticString0 );
		CStaticString<64> StaticString1 = "Hello String";
		PrintString( StaticString1 );
		CStaticString<64> StaticString2 = CStaticString<64>( Args, 7 );
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

		CStaticString<64> SearchString = "0123Search89Me";
		PrintString( SearchString );

		std::cout << "Position=" << SearchString.Find( "Me" ) << std::endl;
		std::cout << "Position=" << SearchString.Find( 'M' ) << std::endl;

		std::cout << "Position=" << SearchString.FindOneOf( "ec" ) << std::endl;
		std::cout << "Position=" << SearchString.FindOneOf( "Mc" ) << std::endl;
		
		std::cout << "Position=" << SearchString.FindOneNotOf( "0123456789" ) << std::endl;
		
		CStaticString<64> CompareString0 = "COMPARE";
		PrintString( CompareString0 );
		
		CStaticString<64> CompareString1 = "compare";
		PrintString( CompareString1 );
		
		std::cout << "Compare="       << CompareString0.Compare(CompareString1)       << std::endl;
		std::cout << "CompareNoCase=" << CompareString0.CompareNoCase(CompareString1) << std::endl;
		
		CompareString1.Resize( 20, 'A' );
		PrintString( CompareString1 );
		
		char Buffer[6];
		Buffer[5] = 0;
		CompareString1.Copy(Buffer, 5, 3);
		std::cout << "Buffer=" << Buffer << std::endl;
		
		CompareString0.Insert( "lower", 4 );
		PrintString( CompareString0 );
		
		CompareString0.Replace( "upper", 4 );
		PrintString( CompareString0 );
		
		CStaticString<64> CombinedString = CompareString0 + '5';
		PrintString( CombinedString );
		
		CombinedString = '5' + CombinedString;
		PrintString( CombinedString );
		
		CombinedString = CombinedString + "Appended";
		PrintString( CombinedString );
		
		CombinedString = "Inserted" + CombinedString;
		PrintString( CombinedString );
		
		CombinedString = CombinedString + CombinedString;
		PrintString( CombinedString );
		
		CStaticString<64> TestString = "Test";
		PrintString( TestString );
		
		std::cout << "operator== : " << std::boolalpha << ("Test"         == TestString)     << std::endl;
		std::cout << "operator== : " << std::boolalpha << (TestString     == "Test" )        << std::endl;
		std::cout << "operator== : " << std::boolalpha << (CombinedString == CombinedString) << std::endl;
		
		std::cout << "operator!= : " << std::boolalpha << ("Test"         != TestString)     << std::endl;
		std::cout << "operator!= : " << std::boolalpha << (TestString     != "Test" )        << std::endl;
		std::cout << "operator!= : " << std::boolalpha << (CombinedString != CombinedString) << std::endl;
		
		std::cout << "operator<= : " << std::boolalpha << ("Test"         <= TestString)     << std::endl;
		std::cout << "operator<= : " << std::boolalpha << (TestString     <= "Test" )        << std::endl;
		std::cout << "operator<= : " << std::boolalpha << (CombinedString <= CombinedString) << std::endl;
		
		std::cout << "operator< : "  << std::boolalpha << ("Test"         <  TestString)     << std::endl;
		std::cout << "operator< : "  << std::boolalpha << (TestString     <  "Test" )        << std::endl;
		std::cout << "operator< : "  << std::boolalpha << (CombinedString <  CombinedString) << std::endl;
		
		std::cout << "operator>= : " << std::boolalpha << ("Test"         >= TestString)     << std::endl;
		std::cout << "operator>= : " << std::boolalpha << (TestString     >= "Test" )        << std::endl;
		std::cout << "operator>= : " << std::boolalpha << (CombinedString >= CombinedString) << std::endl;
		
		std::cout << "operator> : "  << std::boolalpha << ("Test"         >  TestString)     << std::endl;
		std::cout << "operator> : "  << std::boolalpha << (TestString     >  "Test" )        << std::endl;
		std::cout << "operator> : "  << std::boolalpha << (CombinedString >  CombinedString) << std::endl;
		
		for (char C : TestString)
		{
			std::cout << C << std::endl;
		}
		
		for (int32 Index = 0; Index < TestString.Length(); Index++)
		{
			std::cout << Index << '=' << TestString[Index] << std::endl;
		}
		
		WStaticString<64> WideCompareString = CharToWide<64>(CompareString0);
		PrintWideString(WideCompareString);
	}
	
	{
		std::cout << std::endl << "----Testing WStaticString----" << std::endl << std::endl;
		
		WStringView StringView( L"Hello StringView" );
		
		const wchar_t* SomeWideStringInsteadOfArgs = L"/Users/SomeFolder/Blabla/BlaBla";

		WStaticString<64> StaticString0;
		PrintWideString( StaticString0 );
		WStaticString<64> StaticString1 = L"Hello String";
		PrintWideString( StaticString1 );
		WStaticString<64> StaticString2 = WStaticString<64>( SomeWideStringInsteadOfArgs, 7 );
		PrintWideString( StaticString2 );
		WStaticString<64> StaticString3 = WStaticString<64>( StringView );
		PrintWideString( StaticString3 );
		WStaticString<64> StaticString4 = StaticString1;
		PrintWideString( StaticString4 );
		WStaticString<64> StaticString5 = Move(StaticString2);
		PrintWideString( StaticString5 );

		StaticString0.Append(L"Appended String");
		PrintWideString( StaticString0 );
		StaticString5.Append( StaticString0 );
		PrintWideString( StaticString5 );

		WStaticString<64> StaticString6;
		StaticString6.Format( L"Formatted String=%.4f", 0.004f );
		PrintWideString( StaticString6 );

		StaticString6.Append( ' ' );
		PrintWideString( StaticString6 );

		StaticString6.AppendFormat( L"Formatted String=%.4f", 0.0077f );
		PrintWideString( StaticString6 );

		WStaticString<64> LowerStaticString6 = StaticString6.ToLower();
		PrintWideString( LowerStaticString6 );

		WStaticString<64> UpperStaticString6 = StaticString6.ToUpper();
		PrintWideString( UpperStaticString6 );

		StaticString6.Clear();
		PrintWideString( StaticString6 );

		StaticString6.Append( L"    Trimmable String    " );
		PrintWideString( StaticString6 );

		WStaticString<64> TrimmedStaticString6 = StaticString6.Trim();
		TrimmedStaticString6.Append( '*' );
		PrintWideString( TrimmedStaticString6 );

		StaticString6.Clear();
		PrintWideString( StaticString6 );

		StaticString6.Append( L"123456789" );
		PrintWideString( StaticString6 );

		WStaticString<64> ReversedStaticString6 = StaticString6.Reverse();
		PrintWideString( ReversedStaticString6 );

		WStaticString<64> SearchString = L"0123Search89Me";
		PrintWideString( SearchString );

		std::cout << "Position=" << SearchString.Find( L"Me" ) << std::endl;
		std::cout << "Position=" << SearchString.Find( L'M' ) << std::endl;

		std::cout << "Position=" << SearchString.FindOneOf( L"ec" ) << std::endl;
		std::cout << "Position=" << SearchString.FindOneOf( L"Mc" ) << std::endl;
		
		std::cout << "Position=" << SearchString.FindOneNotOf( L"0123456789" ) << std::endl;
		
		WStaticString<64> CompareString0 = L"COMPARE";
		PrintWideString( CompareString0 );
		
		WStaticString<64> CompareString1 = L"compare";
		PrintWideString( CompareString1 );
		
		std::cout << "Compare="       << CompareString0.Compare(CompareString1)       << std::endl;
		std::cout << "CompareNoCase=" << CompareString0.CompareNoCase(CompareString1) << std::endl;
		
		CompareString1.Resize( 20, 'A' );
		PrintWideString( CompareString1 );
		
		wchar_t Buffer[6];
		Buffer[5] = 0;
		CompareString1.Copy(Buffer, 5, 3);
		std::wcout << L"Buffer=" << Buffer << std::endl;
		
		CompareString0.Insert( L"lower", 4 );
		PrintWideString( CompareString0 );
		
		CompareString0.Replace( L"upper", 4 );
		PrintWideString( CompareString0 );
		
		WStaticString<64> CombinedString = CompareString0 + L'5';
		PrintWideString( CombinedString );
		
		CombinedString = L'5' + CombinedString;
		PrintWideString( CombinedString );
		
		CombinedString = CombinedString + L"Appended";
		PrintWideString( CombinedString );
		
		CombinedString = L"Inserted" + CombinedString;
		PrintWideString( CombinedString );
		
		CombinedString = CombinedString + CombinedString;
		PrintWideString( CombinedString );
		
		WStaticString<64> TestString = L"Test";
		PrintWideString( TestString );
		
		std::cout << "operator== : " << std::boolalpha << (L"Test"        == TestString)     << std::endl;
		std::cout << "operator== : " << std::boolalpha << (TestString     == L"Test" )       << std::endl;
		std::cout << "operator== : " << std::boolalpha << (CombinedString == CombinedString) << std::endl;
		
		std::cout << "operator!= : " << std::boolalpha << (L"Test"        != TestString)     << std::endl;
		std::cout << "operator!= : " << std::boolalpha << (TestString     != L"Test" )       << std::endl;
		std::cout << "operator!= : " << std::boolalpha << (CombinedString != CombinedString) << std::endl;
		
		std::cout << "operator<= : " << std::boolalpha << (L"Test"        <= TestString)     << std::endl;
		std::cout << "operator<= : " << std::boolalpha << (TestString     <= L"Test" )       << std::endl;
		std::cout << "operator<= : " << std::boolalpha << (CombinedString <= CombinedString) << std::endl;
		
		std::cout << "operator< : "  << std::boolalpha << (L"Test"        <  TestString)     << std::endl;
		std::cout << "operator< : "  << std::boolalpha << (TestString     <  L"Test" )       << std::endl;
		std::cout << "operator< : "  << std::boolalpha << (CombinedString <  CombinedString) << std::endl;
		
		std::cout << "operator>= : " << std::boolalpha << (L"Test"        >= TestString)     << std::endl;
		std::cout << "operator>= : " << std::boolalpha << (TestString     >= L"Test" )       << std::endl;
		std::cout << "operator>= : " << std::boolalpha << (CombinedString >= CombinedString) << std::endl;
		
		std::cout << "operator> : "  << std::boolalpha << (L"Test"        >  TestString)     << std::endl;
		std::cout << "operator> : "  << std::boolalpha << (TestString     >  L"Test" )       << std::endl;
		std::cout << "operator> : "  << std::boolalpha << (CombinedString >  CombinedString) << std::endl;
		
		for (char C : TestString)
		{
			std::cout << C << std::endl;
		}
		
		for (int32 Index = 0; Index < TestString.Length(); Index++)
		{
			std::cout << Index << '=' << TestString[Index] << std::endl;
		}
		
		CStaticString<64> CharCompareString = WideToChar<64>(CompareString0);
		PrintString(CharCompareString);
	}
}
