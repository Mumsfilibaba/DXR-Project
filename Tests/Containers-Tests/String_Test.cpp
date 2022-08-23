#include "String_Test.h"
#include "TestUtils.h"

#include <Core/Containers/StaticString.h>
#include <Core/Containers/String.h>

#include <iostream>

#define PrintString(Str) \
    { std::cout << #Str << "= " << Str.GetCString() << std::endl; }

#define PrintStringView(Str)                       \
    {                                              \
        std::cout << #Str << "= ";                 \
        for ( int32 i = 0; i < Str.GetLength(); i++ ) \
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
        for ( int32 i = 0; i < Str.GetLength(); i++ ) \
        {                                          \
            std::wcout << Str[i];                  \
        }                                          \
        std::wcout << std::endl;                   \
    }

#if RUN_TSTRING_TEST
static bool TString_Test_Internal(const CHAR* Args);
#endif
#if RUN_TSTATICSTRING_TEST
static bool TStaticString_Test_Internal(const CHAR* Args);
#endif
#if RUN_TSTRINGVIEW_TEST
static bool TStringView_Test_Internal(const CHAR* Args);
#endif

#if (RUN_TSTRING_TEST || RUN_TSTATICSTRING_TEST || RUN_TSTRINGVIEW_TEST)
void TString_Test(const CHAR* Args)
{
    UNREFERENCED_VARIABLE(Args);

#if RUN_TSTRING_TEST
    TString_Test_Internal(Args);
#endif
#if RUN_TSTATICSTRING_TEST
    TStaticString_Test_Internal(Args);
#endif
#if RUN_TSTRINGVIEW_TEST
    TStringView_Test_Internal(Args);
#endif
}
#endif

#if RUN_TSTRING_TEST
bool TString_Test_Internal(const CHAR* Args)
{
    {
        std::cout << std::endl << "----Testing FString----" << std::endl << std::endl;

        FStringView StringView("Hello FStringView");

        FString String0;
        CHECK_STRING(String0, "");
        CHECK(String0.GetLength()   == 0);
        CHECK(String0.GetCapacity() == 0);

        FString String1 = "Hello String";
        CHECK_STRING(String1, "Hello String");
        CHECK(String1.GetLength()   == 12);
        CHECK(String1.GetCapacity() == 13);

        FString String2 = FString(Args, 7);
        CHECK_STRING_N(String2, Args, 7);

        FString String3 = FString(StringView);
        CHECK_STRING(String3, "Hello FStringView");
        FString String4 = String1;
        CHECK_STRING(String4, "Hello String");
        FString String5 = Move(String2);
        CHECK_STRING(String2, "");
        CHECK_STRING_N(String5, Args, 7);

        String0.Append("Appended String");
        CHECK_STRING(String0, "Appended String");
        String0.Append('_');
        CHECK_STRING(String0, "Appended String_");
        String5.Append(String0);
        const std::string ArgString = std::string(Args, 7) + "Appended String_";
        CHECK_STRING(String5, ArgString.c_str());

        FString String6;
        String6.Format("Formatted String=%.4f", 0.004f);
        CHECK_STRING(String6, "Formatted String=0.0040");

        String6.Append('_');
        CHECK_STRING(String6, "Formatted String=0.0040_");

        String6.AppendFormat("Formatted String=%.4f", 0.0077f);
        CHECK_STRING(String6, "Formatted String=0.0040_Formatted String=0.0077");

        FString LowerString6 = String6.ToLower();
        CHECK_STRING(LowerString6, "formatted string=0.0040_formatted string=0.0077");

        FString UpperString6 = String6.ToUpper();
        CHECK_STRING(UpperString6, "FORMATTED STRING=0.0040_FORMATTED STRING=0.0077");

        String6.Clear();
        CHECK_STRING(String6, "");

        String6.Append("    Trimmable String    ");
        CHECK_STRING(String6, "    Trimmable String    ");

        FString TrimmedString6 = String6.Trim();
        TrimmedString6.Append('*');
        CHECK_STRING(TrimmedString6, "Trimmable String*");

        String6.Clear();
        CHECK_STRING(String6, "");

        String6.Append("123456789");
        CHECK_STRING(String6, "123456789");

        FString ReversedString6 = String6.Reverse();
        CHECK_STRING(ReversedString6, "987654321");

        FString String7 = "NewString";
        CHECK_STRING(String7, "NewString");

        String7.Reset(12);
        CHECK(String7.GetSize()     == 12);
        CHECK(String7.GetCapacity() == 13);
        CHECK_STRING(String7, "");

        String7.Reset(6, 'c');
        CHECK(String7.GetSize()     == 6);
        CHECK(String7.GetCapacity() == 13);
        CHECK_STRING(String7, "cccccc");

        FString SearchString = "0123MeSearch89Me89";
        CHECK_STRING(SearchString, "0123MeSearch89Me89");
        CHECK(SearchString.Find("Me")                 == 4);
        CHECK(SearchString.FindChar('M')                 == 4);
        CHECK(SearchString.Contains("Me")             == true);
        CHECK(SearchString.Contains('M')                 == true);
        CHECK(SearchString.StartsWith("0123Me")       == true);
        CHECK(SearchString.StartsWithNoCase("0123ME") == true);
        CHECK(SearchString.EndsWith("Me89")           == true);
        CHECK(SearchString.EndsWithNoCase("ME89")     == true);
        
        CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'e') || (Char == 'c');
        }) == 5);
        
        CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'M') || (Char == 'c');
        }) == 4);

        CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            const CHAR Buffer[] = "0123456789";
            for (CHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 4);

        CHECK(SearchString.FindLast("Me") == 14);
        CHECK(SearchString.FindLastChar('M') == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'h') || (Char == 'M') || (Char == 'c');
        }) == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
        {
            const CHAR Buffer[] = "0123456789";
            for (CHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 15);

        FString CompareString0 = "COMPARE";
        CHECK_STRING(CompareString0, "COMPARE");

        FString CompareString1 = "compare";
        CHECK_STRING(CompareString1, "compare");
        CHECK(CompareString0.Compare(CompareString1)       != 0);
        CHECK(CompareString0.CompareNoCase(CompareString1) == 0);

        CompareString1.Resize(20, 'A');
        CHECK_STRING(CompareString1, "compareAAAAAAAAAAAAA");

        CompareString1.Resize(16, 'A');
        CHECK_STRING(CompareString1, "compareAAAAAAAAA");

        CompareString1.Resize(7);
        CHECK_STRING(CompareString1, "compare");

        CompareString1.Resize(20);
        CHECK_STRING(CompareString1, "compare");

        CompareString1.Resize(7);
        CHECK_STRING(CompareString1, "compare");

        CompareString1.Resize(20, 'A');
        CHECK_STRING(CompareString1, "compareAAAAAAAAAAAAA");

        CHAR Buffer[6];
        Buffer[5] = 0;
        CompareString1.CopyToBuffer(Buffer, 5, 3);
        CHECK(FCString::Strcmp(Buffer, "pareA") == 0);

        CompareString0.Insert("lower", 4);
        CHECK_STRING(CompareString0, "COMPlowerARE");

        CompareString0.Replace("upper", 4);
        CHECK_STRING(CompareString0, "COMPupperARE");

        CompareString0.Replace('X', 0);
        CHECK_STRING(CompareString0, "XOMPupperARE");

        FString CombinedString = CompareString0 + '5';
        CHECK_STRING(CombinedString, "XOMPupperARE5");

        CombinedString = '5' + CombinedString;
        CHECK_STRING(CombinedString, "5XOMPupperARE5");

        CombinedString = CombinedString + "Appended";
        CHECK_STRING(CombinedString, "5XOMPupperARE5Appended");

        CombinedString = "Inserted" + CombinedString;
        CHECK_STRING(CombinedString, "Inserted5XOMPupperARE5Appended");

        CombinedString = CombinedString + CombinedString;
        CHECK_STRING(CombinedString, "Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");

        FString TestString = "Test";
        CHECK_STRING(TestString, "Test");

        CHECK(("Test" == TestString) == true);
        CHECK((TestString == "Test") == true);
        CHECK((CombinedString == CombinedString) == true);

        CHECK(("Test" != TestString) == false);
        CHECK((TestString != "Test") == false);
        CHECK((CombinedString != CombinedString) == false);

        CHECK(("Test" <= TestString) == true);
        CHECK((TestString <= "Test") == true);
        CHECK((CombinedString <= CombinedString) == true);

        CHECK(("Test" < TestString) == false);
        CHECK((TestString < "Test") == false);
        CHECK((CombinedString < CombinedString) == false);

        CHECK(("Test" >= TestString) == true);
        CHECK((TestString >= "Test") == true);
        CHECK((CombinedString >= CombinedString) == true);

        CHECK(("Test" > TestString) == false);
        CHECK((TestString > "Test") == false);
        CHECK((CombinedString > CombinedString) == false);

        for (CHAR C : TestString)
        {
            std::cout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.GetLength(); Index++)
        {
            std::cout << Index << '=' << TestString[Index] << std::endl;
        }

        FStringWide WideCompareString = CharToWide(CombinedString);
        CHECK_STRING(WideCompareString, L"Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");
    }

    {
        std::cout << std::endl << "----Testing FStringWide----" << std::endl << std::endl;

        FStringViewWide StringView(L"Hello FStringView");

        const WIDECHAR* SomeWideStringInsteadOfArgs = L"/Users/SomeFolder/Blabla/BlaBla";

        FStringWide String0;
        CHECK_STRING(String0, L"");
        CHECK(String0.GetLength()   == 0);
        CHECK(String0.GetCapacity() == 0);

        FStringWide String1 = L"Hello String";
        CHECK_STRING(String1, L"Hello String");
        CHECK(String1.GetLength()   == 12);
        CHECK(String1.GetCapacity() == 13);

        FStringWide String2 = FStringWide(SomeWideStringInsteadOfArgs, 7);
        CHECK_STRING_N(String2, SomeWideStringInsteadOfArgs, 7);

        FStringWide String3 = FStringWide(StringView);
        CHECK_STRING(String3, L"Hello FStringView");
        FStringWide String4 = String1;
        CHECK_STRING(String4, L"Hello String");
        FStringWide String5 = Move(String2);
        CHECK_STRING(String2, L"");
        CHECK_STRING_N(String5, SomeWideStringInsteadOfArgs, 7);

        String0.Append(L"Appended String");
        CHECK_STRING(String0, L"Appended String");
        String0.Append(L'_');
        CHECK_STRING(String0, L"Appended String_");
        String5.Append(String0);
        const std::wstring ArgString = std::wstring(SomeWideStringInsteadOfArgs, 7) + L"Appended String_";
        CHECK_STRING(String5, ArgString.c_str());

        FStringWide String6;
        String6.Format(L"Formatted String=%.4f", 0.004f);
        CHECK_STRING(String6, L"Formatted String=0.0040");

        String6.Append(L'_');
        CHECK_STRING(String6, L"Formatted String=0.0040_");

        String6.AppendFormat(L"Formatted String=%.4f", 0.0077f);
        CHECK_STRING(String6, L"Formatted String=0.0040_Formatted String=0.0077");

        FStringWide LowerString6 = String6.ToLower();
        CHECK_STRING(LowerString6, L"formatted string=0.0040_formatted string=0.0077");

        FStringWide UpperString6 = String6.ToUpper();
        CHECK_STRING(UpperString6, L"FORMATTED STRING=0.0040_FORMATTED STRING=0.0077");

        String6.Clear();
        CHECK_STRING(String6, L"");

        String6.Append(L"    Trimmable String    ");
        CHECK_STRING(String6, L"    Trimmable String    ");

        FStringWide TrimmedString6 = String6.Trim();
        TrimmedString6.Append(L'*');
        CHECK_STRING(TrimmedString6, L"Trimmable String*");

        String6.Clear();
        CHECK_STRING(String6, L"");

        String6.Append(L"123456789");
        CHECK_STRING(String6, L"123456789");

        FStringWide ReversedString6 = String6.Reverse();
        CHECK_STRING(ReversedString6, L"987654321");

        FStringWide String7 = L"NewString";
        CHECK_STRING(String7, L"NewString");

        String7.Reset(12);
        CHECK(String7.GetSize()     == 12);
        CHECK(String7.GetCapacity() == 13);
        CHECK_STRING(String7, L"");

        String7.Reset(6, L'c');
        CHECK(String7.GetSize()     == 6);
        CHECK(String7.GetCapacity() == 13);
        CHECK_STRING(String7, L"cccccc");

        FStringWide SearchString = L"0123MeSearch89Me89";
        CHECK_STRING(SearchString, L"0123MeSearch89Me89");
        CHECK(SearchString.Find(L"Me")                        == 4);
        CHECK(SearchString.FindChar(L'M')                 == 4);
        CHECK(SearchString.Contains(L"Me")                     == true);
        CHECK(SearchString.Contains(L'M')                 == true);
        CHECK(SearchString.StartsWith(L"0123Me")       == true);
        CHECK(SearchString.StartsWithNoCase(L"0123ME") == true);
        CHECK(SearchString.EndsWith(L"Me89")           == true);
        CHECK(SearchString.EndsWithNoCase(L"ME89")     == true);

        CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'e') || (Char == L'c');
        }) == 5);

        CHECK(SearchString.FindCharWithPredicate([](WIDECHAR  Char) -> bool
        {
            return (Char == L'M') || (Char == L'c');
        }) == 4);

        CHECK(SearchString.FindCharWithPredicate([](WIDECHAR  Char) -> bool
        {
            const WIDECHAR  Buffer[] = L"0123456789";
            for (WIDECHAR  Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 4);

        CHECK(SearchString.FindLast(L"Me") == 14);
        CHECK(SearchString.FindLastChar(L'M') == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'h') || (Char == L'M') || (Char == L'c');
        }) == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR  Char) -> bool
        {
            const WIDECHAR  Buffer[] = L"0123456789";
            for (WIDECHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 15);

        FStringWide CompareString0 = L"COMPARE";
        CHECK_STRING(CompareString0, L"COMPARE");

        FStringWide CompareString1 = L"compare";
        CHECK_STRING(CompareString1, L"compare");
        CHECK(CompareString0.Compare(CompareString1)       != 0);
        CHECK(CompareString0.CompareNoCase(CompareString1) == 0);

        CompareString1.Resize(20, L'A');
        CHECK_STRING(CompareString1, L"compareAAAAAAAAAAAAA");

        CompareString1.Resize(16, L'A');
        CHECK_STRING(CompareString1, L"compareAAAAAAAAA");

        CompareString1.Resize(7);
        CHECK_STRING(CompareString1, L"compare");

        CompareString1.Resize(20);
        CHECK_STRING(CompareString1, L"compare");

        CompareString1.Resize(7);
        CHECK_STRING(CompareString1, L"compare");

        CompareString1.Resize(20, L'A');
        CHECK_STRING(CompareString1, L"compareAAAAAAAAAAAAA");

        WIDECHAR Buffer[6];
        Buffer[5] = 0;
        CompareString1.CopyToBuffer(Buffer, 5, 3);
        CHECK(FCStringWide::Strcmp(Buffer, L"pareA") == 0);

        CompareString0.Insert(L"lower", 4);
        CHECK_STRING(CompareString0, L"COMPlowerARE");

        CompareString0.Replace(L"upper", 4);
        CHECK_STRING(CompareString0, L"COMPupperARE");

        CompareString0.Replace(L'X', 0);
        CHECK_STRING(CompareString0, L"XOMPupperARE");

        FStringWide CombinedString = CompareString0 + L'5';
        CHECK_STRING(CombinedString, L"XOMPupperARE5");

        CombinedString = L'5' + CombinedString;
        CHECK_STRING(CombinedString, L"5XOMPupperARE5");

        CombinedString = CombinedString + L"Appended";
        CHECK_STRING(CombinedString, L"5XOMPupperARE5Appended");

        CombinedString = L"Inserted" + CombinedString;
        CHECK_STRING(CombinedString, L"Inserted5XOMPupperARE5Appended");

        CombinedString = CombinedString + CombinedString;
        CHECK_STRING(CombinedString, L"Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");

        FStringWide TestString = L"Test";
        CHECK_STRING(TestString, L"Test");

        CHECK((L"Test" == TestString) == true);
        CHECK((TestString == L"Test") == true);
        CHECK((CombinedString == CombinedString) == true);

        CHECK((L"Test" != TestString) == false);
        CHECK((TestString != L"Test") == false);
        CHECK((CombinedString != CombinedString) == false);

        CHECK((L"Test" <= TestString) == true);
        CHECK((TestString <= L"Test") == true);
        CHECK((CombinedString <= CombinedString) == true);

        CHECK((L"Test" < TestString) == false);
        CHECK((TestString < L"Test") == false);
        CHECK((CombinedString < CombinedString) == false);

        CHECK((L"Test" >= TestString) == true);
        CHECK((TestString >= L"Test") == true);
        CHECK((CombinedString >= CombinedString) == true);

        CHECK((L"Test" > TestString) == false);
        CHECK((TestString > L"Test") == false);
        CHECK((CombinedString > CombinedString) == false);

        for (WIDECHAR C : TestString)
        {
            std::wcout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.GetLength(); Index++)
        {
            std::wcout << Index << L'=' << TestString[Index] << std::endl;
        }

        FString WideCompareString = WideToChar(CombinedString);
        CHECK_STRING(WideCompareString, "Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");
    }

    SUCCESS();
}
#endif

#if RUN_TSTATICSTRING_TEST
bool TStaticString_Test_Internal(const CHAR* Args)
{
    {
        std::cout << std::endl << "----Testing FStaticString----" << std::endl << std::endl;

        FStringView StringView("Hello FStringView");

        FStaticString<64> StaticString0;
        CHECK_STRING(StaticString0, "");
        FStaticString<64> StaticString1 = "Hello String";
        CHECK_STRING(StaticString1, "Hello String");
        FStaticString<64> StaticString2 = FStaticString<64>(Args, 7);
        CHECK_STRING_N(StaticString2, Args, 7);
        FStaticString<64> StaticString3 = FStaticString<64>(StringView);
        CHECK_STRING(StaticString3, "Hello FStringView");
        FStaticString<64> StaticString4 = StaticString1;
        CHECK_STRING(StaticString4, "Hello String");
        FStaticString<64> StaticString5 = Move(StaticString2);
        CHECK_STRING(StaticString2, "");
        CHECK_STRING_N(StaticString5, Args, 7);

        StaticString0.Append("Appended String");
        CHECK_STRING(StaticString0, "Appended String");
        StaticString0.Append('_');
        CHECK_STRING(StaticString0, "Appended String_");
        StaticString5.Append(StaticString0);
        const std::string ArgString = std::string(Args, 7) + "Appended String_";
        CHECK_STRING(StaticString5, ArgString.c_str());

        FStaticString<64> StaticString6;
        StaticString6.Format("Formatted String=%.4f", 0.004f);
        CHECK_STRING(StaticString6, "Formatted String=0.0040");

        StaticString6.Append('_');
        CHECK_STRING(StaticString6, "Formatted String=0.0040_");

        StaticString6.AppendFormat("Formatted String=%.4f", 0.0077f);
        CHECK_STRING(StaticString6, "Formatted String=0.0040_Formatted String=0.0077");

        FStaticString<64> LowerStaticString6 = StaticString6.ToLower();
        CHECK_STRING(LowerStaticString6, "formatted string=0.0040_formatted string=0.0077");

        FStaticString<64> UpperStaticString6 = StaticString6.ToUpper();
        CHECK_STRING(UpperStaticString6, "FORMATTED STRING=0.0040_FORMATTED STRING=0.0077");

        StaticString6.Clear();
        CHECK_STRING(StaticString6, "");

        StaticString6.Append("    Trimmable String    ");
        CHECK_STRING(StaticString6, "    Trimmable String    ");

        FStaticString<64> TrimmedStaticString6 = StaticString6.Trim();
        TrimmedStaticString6.Append('*');
        CHECK_STRING(TrimmedStaticString6, "Trimmable String*");

        StaticString6.Clear();
        CHECK_STRING(StaticString6, "");

        StaticString6.Append("123456789");
        CHECK_STRING(StaticString6, "123456789");

        FStaticString<64> ReversedStaticString6 = StaticString6.Reverse();
        CHECK_STRING(ReversedStaticString6, "987654321");

        FStaticString<64> SearchString = "0123MeSearch89Me89";
        CHECK_STRING(SearchString, "0123MeSearch89Me89");

        CHECK(SearchString.Find("Me")                 == 4);
        CHECK(SearchString.FindChar('M')                 == 4);
        CHECK(SearchString.Contains("Me")             == true);
        CHECK(SearchString.Contains('M')                 == true);
        CHECK(SearchString.StartsWith("0123Me")       == true);
        CHECK(SearchString.StartsWithNoCase("0123ME") == true);
        CHECK(SearchString.EndsWith("Me89")           == true);
        CHECK(SearchString.EndsWithNoCase("ME89")     == true);

        CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'e') || (Char == 'c');
        }) == 5);

        CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'M') || (Char == 'c');
        }) == 4);

        CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            const CHAR Buffer[] = "0123456789";
            for (CHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 4);

        CHECK(SearchString.FindLast("Me") == 14);
        CHECK(SearchString.FindLastChar('M') == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'h') || (Char == 'M') || (Char == 'c');
        }) == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
        {
            const CHAR Buffer[] = "0123456789";
            for (CHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 15);

        FStaticString<64> CompareString0 = "COMPARE";
        CHECK_STRING(CompareString0, "COMPARE");

        FStaticString<64> CompareString1 = "compare";
        CHECK_STRING(CompareString1, "compare");

        CHECK(CompareString0.Compare(CompareString1)       != 0);
        CHECK(CompareString0.CompareNoCase(CompareString1) == 0);

        CompareString1.Resize(20, 'A');
        CHECK_STRING(CompareString1, "compareAAAAAAAAAAAAA");

        CompareString1.Resize(16, 'A');
        CHECK_STRING(CompareString1, "compareAAAAAAAAA");

        CompareString1.Resize(7);
        CHECK_STRING(CompareString1, "compare");

        CompareString1.Resize(20);
        CHECK_STRING(CompareString1, "compare");

        CompareString1.Resize(7);
        CHECK_STRING(CompareString1, "compare");

        CompareString1.Resize(20, 'A');
        CHECK_STRING(CompareString1, "compareAAAAAAAAAAAAA");

        CHAR Buffer[6];
        Buffer[5] = 0;
        CompareString1.CopyToBuffer(Buffer, 5, 3);
        CHECK(FCString::Strcmp(Buffer, "pareA") == 0);

        CompareString0.Insert("lower", 4);
        CHECK_STRING(CompareString0, "COMPlowerARE");

        CompareString0.Replace("upper", 4);
        CHECK_STRING(CompareString0, "COMPupperARE");

        CompareString0.Replace('X', 0);
        CHECK_STRING(CompareString0, "XOMPupperARE");

        FStaticString<64> CombinedString = (CompareString0 + '5');
        CHECK_STRING(CombinedString, "XOMPupperARE5");

        CombinedString = '5' + CombinedString;
        CHECK_STRING(CombinedString, "5XOMPupperARE5");

        CombinedString = CombinedString + "Appended";
        CHECK_STRING(CombinedString, "5XOMPupperARE5Appended");

        CombinedString = "Inserted" + CombinedString;
        CHECK_STRING(CombinedString, "Inserted5XOMPupperARE5Appended");

        CombinedString = CombinedString + CombinedString;
        CHECK_STRING(CombinedString, "Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");

        FStaticString<64> TestString = "Test";
        CHECK_STRING(TestString, "Test");

        CHECK(("Test" == TestString) == true);
        CHECK((TestString == "Test") == true);
        CHECK((CombinedString == CombinedString) == true);

        CHECK(("Test" != TestString) == false);
        CHECK((TestString != "Test") == false);
        CHECK((CombinedString != CombinedString) == false);

        CHECK(("Test" <= TestString) == true);
        CHECK((TestString <= "Test") == true);
        CHECK((CombinedString <= CombinedString) == true);

        CHECK(("Test" < TestString) == false);
        CHECK((TestString < "Test") == false);
        CHECK((CombinedString < CombinedString) == false);

        CHECK(("Test" >= TestString) == true);
        CHECK((TestString >= "Test") == true);
        CHECK((CombinedString >= CombinedString) == true);

        CHECK(("Test" > TestString) == false);
        CHECK((TestString > "Test") == false);
        CHECK((CombinedString > CombinedString) == false);

        for (CHAR C : TestString)
        {
            std::cout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.GetLength(); Index++)
        {
            std::cout << Index << L'=' << TestString[Index] << std::endl;
        }

        FStaticStringWide<64> WideCompareString = CharToWide(CombinedString);
        CHECK_STRING(WideCompareString, L"Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");
    }

    {
        std::cout << std::endl << "----Testing FStaticStringWide----" << std::endl << std::endl;

        FStringViewWide StringView(L"Hello FStringView");

        const WIDECHAR* SomeWideStringInsteadOfArgs = L"/Users/SomeFolder/Blabla/BlaBla";

        FStaticStringWide<64> StaticString0;
        CHECK_STRING(StaticString0, L"");
        FStaticStringWide<64> StaticString1 = L"Hello String";
        CHECK_STRING(StaticString1, L"Hello String");
        FStaticStringWide<64> StaticString2 = FStaticStringWide<64>(SomeWideStringInsteadOfArgs, 7);
        CHECK_STRING_N(StaticString2, SomeWideStringInsteadOfArgs, 7);
        FStaticStringWide<64> StaticString3 = FStaticStringWide<64>(StringView);
        CHECK_STRING(StaticString3, L"Hello FStringView");
        FStaticStringWide<64> StaticString4 = StaticString1;
        CHECK_STRING(StaticString4, L"Hello String");
        FStaticStringWide<64> StaticString5 = Move(StaticString2);
        CHECK_STRING(StaticString2, L"");
        CHECK_STRING_N(StaticString5, SomeWideStringInsteadOfArgs, 7);

        StaticString0.Append(L"Appended String");
        CHECK_STRING(StaticString0, L"Appended String");
        StaticString0.Append(L'_');
        CHECK_STRING(StaticString0, L"Appended String_");
        StaticString5.Append(StaticString0);
        const std::wstring ArgString = std::wstring(SomeWideStringInsteadOfArgs, 7) + L"Appended String_";
        CHECK_STRING(StaticString5, ArgString.c_str());

        FStaticStringWide<64> StaticString6;
        StaticString6.Format(L"Formatted String=%.4f", 0.004f);
        CHECK_STRING(StaticString6, L"Formatted String=0.0040");

        StaticString6.Append(L'_');
        CHECK_STRING(StaticString6, L"Formatted String=0.0040_");

        StaticString6.AppendFormat(L"Formatted String=%.4f", 0.0077f);
        CHECK_STRING(StaticString6, L"Formatted String=0.0040_Formatted String=0.0077");

        FStaticStringWide<64> LowerStaticString6 = StaticString6.ToLower();
        CHECK_STRING(LowerStaticString6, L"formatted string=0.0040_formatted string=0.0077");

        FStaticStringWide<64> UpperStaticString6 = StaticString6.ToUpper();
        CHECK_STRING(UpperStaticString6, L"FORMATTED STRING=0.0040_FORMATTED STRING=0.0077");

        StaticString6.Clear();
        CHECK_STRING(StaticString6, L"");

        StaticString6.Append(L"    Trimmable String    ");
        CHECK_STRING(StaticString6, L"    Trimmable String    ");

        FStaticStringWide<64> TrimmedStaticString6 = StaticString6.Trim();
        TrimmedStaticString6.Append(L'*');
        CHECK_STRING(TrimmedStaticString6, L"Trimmable String*");

        StaticString6.Clear();
        CHECK_STRING(StaticString6, L"");

        StaticString6.Append(L"123456789");
        CHECK_STRING(StaticString6, L"123456789");

        FStaticStringWide<64> ReversedStaticString6 = StaticString6.Reverse();
        CHECK_STRING(ReversedStaticString6, L"987654321");

        FStaticStringWide<64> SearchString = L"0123MeSearch89Me89";
        CHECK_STRING(SearchString, L"0123MeSearch89Me89");

        CHECK(SearchString.Find(L"Me")                 == 4);
        CHECK(SearchString.FindChar(L'M')              == 4);
        CHECK(SearchString.Contains(L"Me")             == true);
        CHECK(SearchString.Contains(L'M')              == true);
        CHECK(SearchString.StartsWith(L"0123Me")       == true);
        CHECK(SearchString.StartsWithNoCase(L"0123ME") == true);
        CHECK(SearchString.EndsWith(L"Me89")           == true);
        CHECK(SearchString.EndsWithNoCase(L"ME89")     == true);

        CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'e') || (Char == L'c');
        }) == 5);

        CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'M') || (Char == L'c');
        }) == 4);

        CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            const WIDECHAR Buffer[] = L"0123456789";
            for (WIDECHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 4);

        CHECK(SearchString.FindLast(L"Me") == 14);
        CHECK(SearchString.FindLastChar(L'M') == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'h') || (Char == L'M') || (Char == L'c');
        }) == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR Char) -> bool
        {
            const WIDECHAR Buffer[] = L"0123456789";
            for (WIDECHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 15);

        FStaticStringWide<64> CompareString0 = L"COMPARE";
        CHECK_STRING(CompareString0, L"COMPARE");

        FStaticStringWide<64> CompareString1 = L"compare";
        CHECK_STRING(CompareString1, L"compare");

        CHECK(CompareString0.Compare(CompareString1)       != 0);
        CHECK(CompareString0.CompareNoCase(CompareString1) == 0);

        CompareString1.Resize(20, L'A');
        CHECK_STRING(CompareString1, L"compareAAAAAAAAAAAAA");

        CompareString1.Resize(16, L'A');
        CHECK_STRING(CompareString1, L"compareAAAAAAAAA");

        CompareString1.Resize(7);
        CHECK_STRING(CompareString1, L"compare");

        CompareString1.Resize(20);
        CHECK_STRING(CompareString1, L"compare");

        CompareString1.Resize(7);
        CHECK_STRING(CompareString1, L"compare");

        CompareString1.Resize(20, L'A');
        CHECK_STRING(CompareString1, L"compareAAAAAAAAAAAAA");

        WIDECHAR Buffer[6];
        Buffer[5] = 0;
        CompareString1.CopyToBuffer(Buffer, 5, 3);
        CHECK(FCStringWide::Strcmp(Buffer, L"pareA") == 0);

        CompareString0.Insert(L"lower", 4);
        CHECK_STRING(CompareString0, L"COMPlowerARE");

        CompareString0.Replace(L"upper", 4);
        CHECK_STRING(CompareString0, L"COMPupperARE");

        CompareString0.Replace(L'X', 0);
        CHECK_STRING(CompareString0, L"XOMPupperARE");

        FStaticStringWide<64> CombinedString = (CompareString0 + L'5');
        CHECK_STRING(CombinedString, L"XOMPupperARE5");

        CombinedString = L'5' + CombinedString;
        CHECK_STRING(CombinedString, L"5XOMPupperARE5");

        CombinedString = CombinedString + L"Appended";
        CHECK_STRING(CombinedString, L"5XOMPupperARE5Appended");

        CombinedString = L"Inserted" + CombinedString;
        CHECK_STRING(CombinedString, L"Inserted5XOMPupperARE5Appended");

        CombinedString = CombinedString + CombinedString;
        CHECK_STRING(CombinedString, L"Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");

        FStaticStringWide<64> TestString = L"Test";
        CHECK_STRING(TestString, L"Test");

        CHECK((L"Test" == TestString) == true);
        CHECK((TestString == L"Test") == true);
        CHECK((CombinedString == CombinedString) == true);

        CHECK((L"Test" != TestString) == false);
        CHECK((TestString != L"Test") == false);
        CHECK((CombinedString != CombinedString) == false);

        CHECK((L"Test" <= TestString) == true);
        CHECK((TestString <= L"Test") == true);
        CHECK((CombinedString <= CombinedString) == true);

        CHECK((L"Test" < TestString) == false);
        CHECK((TestString < L"Test") == false);
        CHECK((CombinedString < CombinedString) == false);

        CHECK((L"Test" >= TestString) == true);
        CHECK((TestString >= L"Test") == true);
        CHECK((CombinedString >= CombinedString) == true);

        CHECK((L"Test" > TestString) == false);
        CHECK((TestString > L"Test") == false);
        CHECK((CombinedString > CombinedString) == false);

        for (WIDECHAR C : TestString)
        {
            std::cout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.GetLength(); Index++)
        {
            std::cout << Index << '=' << TestString[Index] << std::endl;
        }

        FStaticString<64> WideCompareString = WideToChar(CombinedString);
        CHECK_STRING(WideCompareString, "Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");
    }
    
    SUCCESS();
}
#endif

#if RUN_TSTRINGVIEW_TEST
bool TStringView_Test_Internal(const CHAR* Args)
{
    UNREFERENCED_VARIABLE(Args);

    {
        std::cout << std::endl << "----Testing FStringView----" << std::endl << std::endl;

        const CHAR* LongString = "This is a long string";

        FStringView StringView0;
        CHECK_STRING(StringView0, "");
        FStringView StringView1 = LongString;
        CHECK_STRING(StringView1, "This is a long string");
        FStringView StringView2 = FStringView(LongString + 5, 4);
        CHECK_STRING(StringView2, "is a");

        CHAR Buffer[6] = { };
        Buffer[5] = 0;
        StringView1.CopyToBuffer(Buffer, 5, 3);
        CHECK(FCString::Strcmp(Buffer, "s is ") == 0);

        FStringView StringView3 = "    Trimmable String    ";
        CHECK_STRING(StringView3, "    Trimmable String    ");

        FStringView StringView4 = StringView3.Trim();
        CHECK_STRING(StringView4, "Trimmable String");

        FStringView StringView5 = FStringView("COMPAREPostfix", 7);
        CHECK_STRING(StringView5, "COMPARE");

        FStringView StringView6 = FStringView("comparePostfix", 7);
        CHECK_STRING(StringView6, "compare");

        std::cout << "Compare=" << StringView5.Compare(StringView6) << std::endl;
        std::cout << "CompareNoCase=" << StringView5.CompareNoCase(StringView6) << std::endl;

        CHECK(StringView5.Compare(StringView6)       != 0);
        CHECK(StringView5.CompareNoCase(StringView6) == 0);

        StringView6.Clear();
        PrintStringView(StringView6);

        FStringView SearchString = "0123MeSearch89Me89";
        CHECK_STRING(SearchString, "0123MeSearch89Me89");

        CHECK(SearchString.Find("Me")                 == 4);
        CHECK(SearchString.FindChar('M')              == 4);
        CHECK(SearchString.Contains("Me")             == true);
        CHECK(SearchString.Contains('M')              == true);
        CHECK(SearchString.StartsWith("0123Me")       == true);
        CHECK(SearchString.StartsWithNoCase("0123ME") == true);
        CHECK(SearchString.EndsWith("Me89")           == true);
        CHECK(SearchString.EndsWithNoCase("ME89")     == true);

        CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'e') || (Char == 'c');
        }) == 5);

        CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'M') || (Char == 'c');
        }) == 4);

        CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            const CHAR Buffer[] = "0123456789";
            for (CHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 4);

        CHECK(SearchString.FindLast("Me")    == 14);
        CHECK(SearchString.FindLastChar('M') == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'h') || (Char == 'M') || (Char == 'c');
        }) == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
        {
            const CHAR Buffer[] = "0123456789";
            for (CHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 15);

        FStringView TestString = "Test";
        CHECK_STRING(TestString, "Test");

        CHECK(("Test" == TestString) == true);
        CHECK((TestString == "Test") == true);

        CHECK(("Test" != TestString) == false);
        CHECK((TestString != "Test") == false);

        CHECK(("Test" <= TestString) == true);
        CHECK((TestString <= "Test") == true);

        CHECK(("Test" < TestString) == false);
        CHECK((TestString < "Test") == false);

        CHECK(("Test" >= TestString) == true);
        CHECK((TestString >= "Test") == true);

        CHECK(("Test" > TestString) == false);
        CHECK((TestString > "Test") == false);

        for (CHAR C : TestString)
        {
            std::cout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.GetLength(); Index++)
        {
            std::cout << Index << '=' << TestString[Index] << std::endl;
        }
    }

    {
        std::cout << std::endl << "----Testing FStringViewWide----" << std::endl << std::endl;

        const WIDECHAR* LongString = L"This is a long string";

        FStringViewWide StringView0;
        CHECK_STRING(StringView0, L"");
        FStringViewWide StringView1 = LongString;
        CHECK_STRING(StringView1, L"This is a long string");
        FStringViewWide StringView2 = FStringViewWide(LongString + 5, 4);
        CHECK_STRING(StringView2, L"is a");

        WIDECHAR Buffer[6] = { };
        Buffer[5] = 0;
        StringView1.CopyToBuffer(Buffer, 5, 3);
        CHECK(FCStringWide::Strcmp(Buffer, L"s is ") == 0);

        FStringViewWide StringView3 = L"    Trimmable String    ";
        CHECK_STRING(StringView3, L"    Trimmable String    ");

        FStringViewWide StringView4 = StringView3.Trim();
        CHECK_STRING(StringView4, L"Trimmable String");

        FStringViewWide StringView5 = FStringViewWide(L"COMPAREPostfix", 7);
        CHECK_STRING(StringView5, L"COMPARE");

        FStringViewWide StringView6 = FStringViewWide(L"comparePostfix", 7);
        CHECK_STRING(StringView6, L"compare");

        CHECK(StringView5.Compare(StringView6)       != 0);
        CHECK(StringView5.CompareNoCase(StringView6) == 0);

        StringView6.Clear();
        PrintStringView(StringView6);

        FStringViewWide SearchString = L"0123MeSearch89Me89";
        CHECK_STRING(SearchString, L"0123MeSearch89Me89");

        CHECK(SearchString.Find(L"Me")                 == 4);
        CHECK(SearchString.FindChar(L'M')              == 4);
        CHECK(SearchString.Contains(L"Me")             == true);
        CHECK(SearchString.Contains(L'M')              == true);
        CHECK(SearchString.StartsWith(L"0123Me")       == true);
        CHECK(SearchString.StartsWithNoCase(L"0123ME") == true);
        CHECK(SearchString.EndsWith(L"Me89")           == true);
        CHECK(SearchString.EndsWithNoCase(L"ME89")     == true);

        CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'e') || (Char == L'c');
        }) == 5);

        CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'M') || (Char == L'c');
        }) == 4);

        CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            const WIDECHAR Buffer[] = L"0123456789";
            for (WIDECHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 4);

        CHECK(SearchString.FindLast(L"Me") == 14);
        CHECK(SearchString.FindLastChar(L'M') == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'h') || (Char == L'M') || (Char == L'c');
        }) == 14);

        CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR Char) -> bool
        {
            const WIDECHAR Buffer[] = L"0123456789";
            for (WIDECHAR Current : Buffer)
            {
                if (Char == Current)
                {
                    return false;
                }
            }

            return true;
        }) == 15);

        FStringViewWide TestString = L"Test";
        CHECK_STRING(TestString, L"Test");

        CHECK((L"Test" == TestString) == true);
        CHECK((TestString == L"Test") == true);

        CHECK((L"Test" != TestString) == false);
        CHECK((TestString != L"Test") == false);

        CHECK((L"Test" <= TestString) == true);
        CHECK((TestString <= L"Test") == true);

        CHECK((L"Test" < TestString) == false);
        CHECK((TestString < L"Test") == false);

        CHECK((L"Test" >= TestString) == true);
        CHECK((TestString >= L"Test") == true);

        CHECK((L"Test" > TestString) == false);
        CHECK((TestString > L"Test") == false);

        for (WIDECHAR C : TestString)
        {
            std::wcout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.GetLength(); Index++)
        {
            std::wcout << Index << L'=' << TestString[Index] << std::endl;
        }
    }

    SUCCESS();
}
#endif