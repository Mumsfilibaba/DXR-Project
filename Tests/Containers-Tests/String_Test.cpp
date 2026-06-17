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
        for ( int32 i = 0; i < Str.Length(); i++ ) \
        {                                          \
            std::cout << Str[i];                   \
        }                                          \
        std::cout << std::endl;                    \
    }

#define PrintWideString(Str) \
    { std::wcout << #Str << L"= " << Str.GetCString() << std::endl; }

#define PrintWideStringView(Str)                   \
    {                                              \
        std::wcout << #Str << L"= ";               \
        for ( int32 i = 0; i < Str.Length(); i++ ) \
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
        std::cout << std::endl << "----Testing String----" << std::endl << std::endl;

        StringView StringView("Hello StringView");

        String String0;
        TEST_CHECK_STRING(String0, "");
        TEST_CHECK(String0.Length()   == 0);
        TEST_CHECK(String0.Capacity() == 0);

        String String1 = "Hello String";
        TEST_CHECK_STRING(String1, "Hello String");
        TEST_CHECK(String1.Length()   == 12);
        TEST_CHECK(String1.Capacity() == 13);

        String String2 = String(Args, 7);
        TEST_CHECK_STRING_N(String2, Args, 7);
		TEST_CHECK(String2.Length()   == 7);
		TEST_CHECK(String2.Capacity() == 8);

        String String3 = String(StringView);
        TEST_CHECK_STRING(String3, "Hello StringView");
		TEST_CHECK(String3.Length() == 17);
		TEST_CHECK(String3.Capacity() == 18);

        String String4 = String1;
        TEST_CHECK_STRING(String4, "Hello String");
		TEST_CHECK(String4.Length()   == 12);
		TEST_CHECK(String4.Capacity() == 13);
        
        String String5 = ::Move(String2);
        TEST_CHECK_STRING(String2, "");
        TEST_CHECK_STRING_N(String5, Args, 7);
		TEST_CHECK(String5.Length()   == 7);
		TEST_CHECK(String5.Capacity() == 8);

        String0.Append("Appended String");
        TEST_CHECK_STRING(String0, "Appended String");
        String0.Append('_');
        TEST_CHECK_STRING(String0, "Appended String_");
        String5.Append(String0);
        const std::string ArgString = std::string(Args, 7) + "Appended String_";
        TEST_CHECK_STRING(String5, ArgString.c_str());

        String String6;
        String6.Format("Formatted String=%.4f", 0.004f);
        TEST_CHECK_STRING(String6, "Formatted String=0.0040");

        String6.Append('_');
        TEST_CHECK_STRING(String6, "Formatted String=0.0040_");

        String6.AppendFormat("Formatted String=%.4f", 0.0077f);
        TEST_CHECK_STRING(String6, "Formatted String=0.0040_Formatted String=0.0077");

        String LowerString6 = String6.ToLower();
        TEST_CHECK_STRING(LowerString6, "formatted string=0.0040_formatted string=0.0077");

        String UpperString6 = String6.ToUpper();
        TEST_CHECK_STRING(UpperString6, "FORMATTED STRING=0.0040_FORMATTED STRING=0.0077");

        String6.Clear();
        TEST_CHECK_STRING(String6, "");

        String6.Append("    Trimmable String    ");
        TEST_CHECK_STRING(String6, "    Trimmable String    ");

        String TrimmedString6 = String6.Trim();
        TrimmedString6.Append('*');
        TEST_CHECK_STRING(TrimmedString6, "Trimmable String*");

        String6.Clear();
        TEST_CHECK_STRING(String6, "");

        String6.Append("123456789");
        TEST_CHECK_STRING(String6, "123456789");

        String ReversedString6 = String6.Reverse();
        TEST_CHECK_STRING(ReversedString6, "987654321");

        String String7 = "NewString";
        TEST_CHECK_STRING(String7, "NewString");

        String7.Reset(12);
        TEST_CHECK(String7.Size()     == 12);
        TEST_CHECK(String7.Capacity() == 13);
        TEST_CHECK(TCString<CHAR>::Strcmp(String7.GetCString(), "") == 0);

        String SearchString = "0123MeSearch89Me89";
        TEST_CHECK_STRING(SearchString, "0123MeSearch89Me89");
        TEST_CHECK(SearchString.Find("Me")                                    == 4);
        TEST_CHECK(SearchString.FindChar('M')                                 == 4);
        TEST_CHECK(SearchString.Contains("Me")                                == true);
        TEST_CHECK(SearchString.Contains('M')                                 == true);
        TEST_CHECK(SearchString.StartsWith("0123Me")                          == true);
        TEST_CHECK(SearchString.StartsWith("0123ME", EStringCaseType::NoCase) == true);
        TEST_CHECK(SearchString.EndsWith("Me89")                              == true);
        TEST_CHECK(SearchString.EndsWith("ME89", EStringCaseType::NoCase)     == true);
        
        TEST_CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'e') || (Char == 'c');
        }) == 5);
        
        TEST_CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'M') || (Char == 'c');
        }) == 4);

        TEST_CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
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

        TEST_CHECK(SearchString.FindLast("Me") == 14);
        TEST_CHECK(SearchString.FindLastChar('M') == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'h') || (Char == 'M') || (Char == 'c');
        }) == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
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

        String CompareString0 = "COMPARE";
        TEST_CHECK_STRING(CompareString0, "COMPARE");

        String CompareString1 = "compare";
        TEST_CHECK_STRING(CompareString1, "compare");
        TEST_CHECK(CompareString0.Compare(CompareString1)                          != 0);
        TEST_CHECK(CompareString0.Compare(CompareString1, EStringCaseType::NoCase) == 0);

        CompareString1.Resize(7);
        TEST_CHECK(CString::Strcmp(CompareString1.GetCString(), "compare") == 0);

        CompareString1.Resize(20);
        TEST_CHECK(CString::Strcmp(CompareString1.GetCString(), "compare") == 0);

        CompareString1.Resize(7);
        TEST_CHECK(CString::Strcmp(CompareString1.GetCString(), "compare") == 0);

        CHAR Buffer[6];
        Buffer[5] = 0;
        CompareString1.CopyToBuffer(Buffer, 5, 2);
        TEST_CHECK(CString::Strcmp(Buffer, "mpare") == 0);

        CompareString0.Insert("lower", 4);
        TEST_CHECK_STRING(CompareString0, "COMPlowerARE");

        CompareString0.Replace("upper", 4);
        TEST_CHECK_STRING(CompareString0, "COMPupperARE");

        CompareString0.Replace('X', 0);
        TEST_CHECK_STRING(CompareString0, "XOMPupperARE");

        String CombinedString;
        CombinedString = CompareString0 + '5';
        TEST_CHECK_STRING(CombinedString, "XOMPupperARE5");

        CombinedString = '5' + CombinedString;
        TEST_CHECK_STRING(CombinedString, "5XOMPupperARE5");

        CombinedString = CombinedString + "Appended";
        TEST_CHECK_STRING(CombinedString, "5XOMPupperARE5Appended");

        CombinedString = "Inserted" + CombinedString;
        TEST_CHECK_STRING(CombinedString, "Inserted5XOMPupperARE5Appended");

        CombinedString = CombinedString + CombinedString;
        TEST_CHECK_STRING(CombinedString, "Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");

        String TestString = "Test";
        TEST_CHECK_STRING(TestString, "Test");

        TEST_CHECK(("Test" == TestString) == true);
        TEST_CHECK((TestString == "Test") == true);
        TEST_CHECK((CombinedString == CombinedString) == true);

        TEST_CHECK(("Test" != TestString) == false);
        TEST_CHECK((TestString != "Test") == false);
        TEST_CHECK((CombinedString != CombinedString) == false);

        TEST_CHECK(("Test" <= TestString) == true);
        TEST_CHECK((TestString <= "Test") == true);
        TEST_CHECK((CombinedString <= CombinedString) == true);

        TEST_CHECK(("Test" < TestString) == false);
        TEST_CHECK((TestString < "Test") == false);
        TEST_CHECK((CombinedString < CombinedString) == false);

        TEST_CHECK(("Test" >= TestString) == true);
        TEST_CHECK((TestString >= "Test") == true);
        TEST_CHECK((CombinedString >= CombinedString) == true);

        TEST_CHECK(("Test" > TestString) == false);
        TEST_CHECK((TestString > "Test") == false);
        TEST_CHECK((CombinedString > CombinedString) == false);

        for (CHAR C : TestString)
        {
            std::cout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.Length(); Index++)
        {
            std::cout << Index << '=' << TestString[Index] << std::endl;
        }

        WString WideCompareString = CharToWide(CombinedString);
        TEST_CHECK_STRING(WideCompareString, L"Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");
    }

    {
        std::cout << std::endl << "----Testing WString----" << std::endl << std::endl;

        WStringView StringView(L"Hello StringView");

        const WIDECHAR* SomeWideStringInsteadOfArgs = L"/Users/SomeFolder/Blabla/BlaBla";

        WString String0;
        TEST_CHECK_STRING(String0, L"");
        TEST_CHECK(String0.Length()   == 0);
        TEST_CHECK(String0.Capacity() == 0);

        WString String1 = L"Hello String";
        TEST_CHECK_STRING(String1, L"Hello String");
        TEST_CHECK(String1.Length()   == 12);
        TEST_CHECK(String1.Capacity() == 13);

        WString String2 = WString(SomeWideStringInsteadOfArgs, 7);
        TEST_CHECK_STRING_N(String2, SomeWideStringInsteadOfArgs, 7);

        WString String3 = WString(StringView);
        TEST_CHECK_STRING(String3, L"Hello StringView");
        WString String4 = String1;
        TEST_CHECK_STRING(String4, L"Hello String");
        WString String5 = Move(String2);
        TEST_CHECK_STRING(String2, L"");
        TEST_CHECK_STRING_N(String5, SomeWideStringInsteadOfArgs, 7);

        String0.Append(L"Appended String");
        TEST_CHECK_STRING(String0, L"Appended String");
        String0.Append(L'_');
        TEST_CHECK_STRING(String0, L"Appended String_");
        String5.Append(String0);
        const std::wstring ArgString = std::wstring(SomeWideStringInsteadOfArgs, 7) + L"Appended String_";
        TEST_CHECK_STRING(String5, ArgString.c_str());

        WString String6;
        String6.Format(L"Formatted String=%.4f", 0.004f);
        TEST_CHECK_STRING(String6, L"Formatted String=0.0040");

        String6.Append(L'_');
        TEST_CHECK_STRING(String6, L"Formatted String=0.0040_");

        String6.AppendFormat(L"Formatted String=%.4f", 0.0077f);
        TEST_CHECK_STRING(String6, L"Formatted String=0.0040_Formatted String=0.0077");

        WString LowerString6 = String6.ToLower();
        TEST_CHECK_STRING(LowerString6, L"formatted string=0.0040_formatted string=0.0077");

        WString UpperString6 = String6.ToUpper();
        TEST_CHECK_STRING(UpperString6, L"FORMATTED STRING=0.0040_FORMATTED STRING=0.0077");

        String6.Clear();
        TEST_CHECK_STRING(String6, L"");

        String6.Append(L"    Trimmable String    ");
        TEST_CHECK_STRING(String6, L"    Trimmable String    ");

        WString TrimmedString6 = String6.Trim();
        TrimmedString6.Append(L'*');
        TEST_CHECK_STRING(TrimmedString6, L"Trimmable String*");

        String6.Clear();
        TEST_CHECK_STRING(String6, L"");

        String6.Append(L"123456789");
        TEST_CHECK_STRING(String6, L"123456789");

        WString ReversedString6 = String6.Reverse();
        TEST_CHECK_STRING(ReversedString6, L"987654321");

        WString String7 = L"NewString";
        TEST_CHECK_STRING(String7, L"NewString");

        String7.Reset(12);
        TEST_CHECK(String7.Size()     == 12);
        TEST_CHECK(String7.Capacity() == 13);
        TEST_CHECK(TCString<WIDECHAR>::Strncmp(String7.GetCString(), L"", 0) == 0);

        WString SearchString = L"0123MeSearch89Me89";
        TEST_CHECK_STRING(SearchString, L"0123MeSearch89Me89");
        TEST_CHECK(SearchString.Find(L"Me")                                    == 4);
        TEST_CHECK(SearchString.FindChar(L'M')                                 == 4);
        TEST_CHECK(SearchString.Contains(L"Me")                                == true);
        TEST_CHECK(SearchString.Contains(L'M')                                 == true);
        TEST_CHECK(SearchString.StartsWith(L"0123Me")                          == true);
        TEST_CHECK(SearchString.StartsWith(L"0123ME", EStringCaseType::NoCase) == true);
        TEST_CHECK(SearchString.EndsWith(L"Me89")                              == true);
        TEST_CHECK(SearchString.EndsWith(L"ME89", EStringCaseType::NoCase)     == true);

        TEST_CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'e') || (Char == L'c');
        }) == 5);

        TEST_CHECK(SearchString.FindCharWithPredicate([](WIDECHAR  Char) -> bool
        {
            return (Char == L'M') || (Char == L'c');
        }) == 4);

        TEST_CHECK(SearchString.FindCharWithPredicate([](WIDECHAR  Char) -> bool
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

        TEST_CHECK(SearchString.FindLast(L"Me") == 14);
        TEST_CHECK(SearchString.FindLastChar(L'M') == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'h') || (Char == L'M') || (Char == L'c');
        }) == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR  Char) -> bool
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

        WString CompareString0 = L"COMPARE";
        TEST_CHECK_STRING(CompareString0, L"COMPARE");

        WString CompareString1 = L"compare";
        TEST_CHECK_STRING(CompareString1, L"compare");
        TEST_CHECK(CompareString0.Compare(CompareString1)                          != 0);
        TEST_CHECK(CompareString0.Compare(CompareString1, EStringCaseType::NoCase) == 0);

        CompareString1.Resize(7);
        TEST_CHECK(CStringWide::Strcmp(CompareString1.GetCString(), L"compare") == 0);

        CompareString1.Resize(20);
        TEST_CHECK(CStringWide::Strcmp(CompareString1.GetCString(), L"compare") == 0);

        CompareString1.Resize(7);
        TEST_CHECK(CStringWide::Strcmp(CompareString1.GetCString(), L"compare") == 0);

        WIDECHAR Buffer[6];
        Buffer[5] = 0;
        CompareString1.CopyToBuffer(Buffer, 5, 2);
        TEST_CHECK(CStringWide::Strcmp(Buffer, L"mpare") == 0);

        CompareString0.Insert(L"lower", 4);
        TEST_CHECK_STRING(CompareString0, L"COMPlowerARE");

        CompareString0.Replace(L"upper", 4);
        TEST_CHECK_STRING(CompareString0, L"COMPupperARE");

        CompareString0.Replace(L'X', 0);
        TEST_CHECK_STRING(CompareString0, L"XOMPupperARE");

        WString CombinedString = CompareString0 + L'5';
        TEST_CHECK_STRING(CombinedString, L"XOMPupperARE5");

        CombinedString = L'5' + CombinedString;
        TEST_CHECK_STRING(CombinedString, L"5XOMPupperARE5");

        CombinedString = CombinedString + L"Appended";
        TEST_CHECK_STRING(CombinedString, L"5XOMPupperARE5Appended");

        CombinedString = L"Inserted" + CombinedString;
        TEST_CHECK_STRING(CombinedString, L"Inserted5XOMPupperARE5Appended");

        CombinedString = CombinedString + CombinedString;
        TEST_CHECK_STRING(CombinedString, L"Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");

        WString TestString = L"Test";
        TEST_CHECK_STRING(TestString, L"Test");

        TEST_CHECK((L"Test" == TestString) == true);
        TEST_CHECK((TestString == L"Test") == true);
        TEST_CHECK((CombinedString == CombinedString) == true);

        TEST_CHECK((L"Test" != TestString) == false);
        TEST_CHECK((TestString != L"Test") == false);
        TEST_CHECK((CombinedString != CombinedString) == false);

        TEST_CHECK((L"Test" <= TestString) == true);
        TEST_CHECK((TestString <= L"Test") == true);
        TEST_CHECK((CombinedString <= CombinedString) == true);

        TEST_CHECK((L"Test" < TestString) == false);
        TEST_CHECK((TestString < L"Test") == false);
        TEST_CHECK((CombinedString < CombinedString) == false);

        TEST_CHECK((L"Test" >= TestString) == true);
        TEST_CHECK((TestString >= L"Test") == true);
        TEST_CHECK((CombinedString >= CombinedString) == true);

        TEST_CHECK((L"Test" > TestString) == false);
        TEST_CHECK((TestString > L"Test") == false);
        TEST_CHECK((CombinedString > CombinedString) == false);

        for (WIDECHAR C : TestString)
        {
            std::wcout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.Length(); Index++)
        {
            std::wcout << Index << L'=' << TestString[Index] << std::endl;
        }

        String WideCompareString = WideToChar(CombinedString);
        TEST_CHECK_STRING(WideCompareString, "Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");
    }

    SUCCESS();
}
#endif

#if RUN_TSTATICSTRING_TEST
bool TStaticString_Test_Internal(const CHAR* Args)
{
    {
        std::cout << std::endl << "----Testing StaticString----" << std::endl << std::endl;

        StringView StringView("Hello StringView");

        StaticString<64> StaticString0;
        TEST_CHECK_STRING(StaticString0, "");
        StaticString<64> StaticString1 = "Hello String";
        TEST_CHECK_STRING(StaticString1, "Hello String");
        StaticString<64> StaticString2 = StaticString<64>(Args, 7);
        TEST_CHECK_STRING_N(StaticString2, Args, 7);
        StaticString<64> StaticString3 = StaticString<64>(StringView);
        TEST_CHECK_STRING(StaticString3, "Hello StringView");
        StaticString<64> StaticString4 = StaticString1;
        TEST_CHECK_STRING(StaticString4, "Hello String");
        StaticString<64> StaticString5 = Move(StaticString2);
        TEST_CHECK_STRING(StaticString2, "");
        TEST_CHECK_STRING_N(StaticString5, Args, 7);

        StaticString0.Append("Appended String");
        TEST_CHECK_STRING(StaticString0, "Appended String");
        StaticString0.Append('_');
        TEST_CHECK_STRING(StaticString0, "Appended String_");
        StaticString5.Append(StaticString0);
        const std::string ArgString = std::string(Args, 7) + "Appended String_";
        TEST_CHECK_STRING(StaticString5, ArgString.c_str());

        StaticString<64> StaticString6;
        StaticString6.Format("Formatted String=%.4f", 0.004f);
        TEST_CHECK_STRING(StaticString6, "Formatted String=0.0040");

        StaticString6.Append('_');
        TEST_CHECK_STRING(StaticString6, "Formatted String=0.0040_");

        StaticString6.AppendFormat("Formatted String=%.4f", 0.0077f);
        TEST_CHECK_STRING(StaticString6, "Formatted String=0.0040_Formatted String=0.0077");

        StaticString<64> LowerStaticString6 = StaticString6.ToLower();
        TEST_CHECK_STRING(LowerStaticString6, "formatted string=0.0040_formatted string=0.0077");

        StaticString<64> UpperStaticString6 = StaticString6.ToUpper();
        TEST_CHECK_STRING(UpperStaticString6, "FORMATTED STRING=0.0040_FORMATTED STRING=0.0077");

        StaticString6.Reset();
        TEST_CHECK_STRING(StaticString6, "");

        StaticString6.Append("    Trimmable String    ");
        TEST_CHECK_STRING(StaticString6, "    Trimmable String    ");

        StaticString<64> TrimmedStaticString6 = StaticString6.Trim();
        TEST_CHECK_STRING(TrimmedStaticString6, "Trimmable String");
        TrimmedStaticString6.Append('*');
        TEST_CHECK_STRING(TrimmedStaticString6, "Trimmable String*");

        StaticString6.Reset();
        TEST_CHECK_STRING(StaticString6, "");

        StaticString6.Append("123456789");
        TEST_CHECK_STRING(StaticString6, "123456789");

        StaticString<64> ReversedStaticString6 = StaticString6.Reverse();
        TEST_CHECK_STRING(ReversedStaticString6, "987654321");

        StaticString<64> SearchString = "0123MeSearch89Me89";
        TEST_CHECK_STRING(SearchString, "0123MeSearch89Me89");

        TEST_CHECK(SearchString.Find("Me")                                    == 4);
        TEST_CHECK(SearchString.FindChar('M')                                 == 4);
        TEST_CHECK(SearchString.Contains("Me")                                == true);
        TEST_CHECK(SearchString.Contains('M')                                 == true);
        TEST_CHECK(SearchString.StartsWith("0123Me")                          == true);
        TEST_CHECK(SearchString.StartsWith("0123ME", EStringCaseType::NoCase) == true);
        TEST_CHECK(SearchString.EndsWith("Me89")                              == true);
        TEST_CHECK(SearchString.EndsWith("ME89", EStringCaseType::NoCase)     == true);

        TEST_CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'e') || (Char == 'c');
        }) == 5);

        TEST_CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'M') || (Char == 'c');
        }) == 4);

        TEST_CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
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

        TEST_CHECK(SearchString.FindLast("Me") == 14);
        TEST_CHECK(SearchString.FindLastChar('M') == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'h') || (Char == 'M') || (Char == 'c');
        }) == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
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

        StaticString<64> CompareString0 = "COMPARE";
        TEST_CHECK_STRING(CompareString0, "COMPARE");

        StaticString<64> CompareString1 = "compare";
        TEST_CHECK_STRING(CompareString1, "compare");

        TEST_CHECK(CompareString0.Compare(CompareString1)                          != 0);
        TEST_CHECK(CompareString0.Compare(CompareString1, EStringCaseType::NoCase) == 0);

        CompareString1.Resize(7);
        TEST_CHECK(CString::Strcmp(CompareString1.GetCString(), "compare") == 0);

        CompareString1.Resize(20);
        TEST_CHECK(CString::Strcmp(CompareString1.GetCString(), "compare") == 0);

        CompareString1.Resize(7);
        TEST_CHECK(CString::Strcmp(CompareString1.GetCString(), "compare") == 0);

        CHAR Buffer[6];
        Buffer[5] = 0;
        CompareString1.CopyToBuffer(Buffer, 5, 2);
        TEST_CHECK(CString::Strcmp(Buffer, "mpare") == 0);

        CompareString0.Insert("lower", 4);
        TEST_CHECK_STRING(CompareString0, "COMPlowerARE");

        CompareString0.Replace("upper", 4);
        TEST_CHECK_STRING(CompareString0, "COMPupperARE");

        CompareString0.Replace('X', 0);
        TEST_CHECK_STRING(CompareString0, "XOMPupperARE");

        StaticString<64> CombinedString = (CompareString0 + '5');
        TEST_CHECK_STRING(CombinedString, "XOMPupperARE5");

        CombinedString = '5' + CombinedString;
        TEST_CHECK_STRING(CombinedString, "5XOMPupperARE5");

        CombinedString = CombinedString + "Appended";
        TEST_CHECK_STRING(CombinedString, "5XOMPupperARE5Appended");

        CombinedString = "Inserted" + CombinedString;
        TEST_CHECK_STRING(CombinedString, "Inserted5XOMPupperARE5Appended");

        CombinedString = CombinedString + CombinedString;
        TEST_CHECK_STRING(CombinedString, "Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");

        StaticString<64> TestString = "Test";
        TEST_CHECK_STRING(TestString, "Test");

        TEST_CHECK(("Test" == TestString) == true);
        TEST_CHECK((TestString == "Test") == true);
        TEST_CHECK((CombinedString == CombinedString) == true);

        TEST_CHECK(("Test" != TestString) == false);
        TEST_CHECK((TestString != "Test") == false);
        TEST_CHECK((CombinedString != CombinedString) == false);

        TEST_CHECK(("Test" <= TestString) == true);
        TEST_CHECK((TestString <= "Test") == true);
        TEST_CHECK((CombinedString <= CombinedString) == true);

        TEST_CHECK(("Test" < TestString) == false);
        TEST_CHECK((TestString < "Test") == false);
        TEST_CHECK((CombinedString < CombinedString) == false);

        TEST_CHECK(("Test" >= TestString) == true);
        TEST_CHECK((TestString >= "Test") == true);
        TEST_CHECK((CombinedString >= CombinedString) == true);

        TEST_CHECK(("Test" > TestString) == false);
        TEST_CHECK((TestString > "Test") == false);
        TEST_CHECK((CombinedString > CombinedString) == false);

        for (CHAR C : TestString)
        {
            std::cout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.Length(); Index++)
        {
            std::cout << Index << '=' << TestString[Index] << std::endl;
        }

        WStaticString<64> WideCompareString = CharToWide(CombinedString);
        TEST_CHECK_STRING(WideCompareString, L"Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");
    }

    {
        std::cout << std::endl << "----Testing WStaticString----" << std::endl << std::endl;

        WStringView StringView(L"Hello StringView");

        const WIDECHAR* SomeWideStringInsteadOfArgs = L"/Users/SomeFolder/Blabla/BlaBla";

        WStaticString<64> StaticString0;
        TEST_CHECK_STRING(StaticString0, L"");
        WStaticString<64> StaticString1 = L"Hello String";
        TEST_CHECK_STRING(StaticString1, L"Hello String");
        WStaticString<64> StaticString2 = WStaticString<64>(SomeWideStringInsteadOfArgs, 7);
        TEST_CHECK_STRING_N(StaticString2, SomeWideStringInsteadOfArgs, 7);
        WStaticString<64> StaticString3 = WStaticString<64>(StringView);
        TEST_CHECK_STRING(StaticString3, L"Hello StringView");
        WStaticString<64> StaticString4 = StaticString1;
        TEST_CHECK_STRING(StaticString4, L"Hello String");
        WStaticString<64> StaticString5 = Move(StaticString2);
        TEST_CHECK_STRING(StaticString2, L"");
        TEST_CHECK_STRING_N(StaticString5, SomeWideStringInsteadOfArgs, 7);

        StaticString0.Append(L"Appended String");
        TEST_CHECK_STRING(StaticString0, L"Appended String");
        StaticString0.Append(L'_');
        TEST_CHECK_STRING(StaticString0, L"Appended String_");
        StaticString5.Append(StaticString0);
        const std::wstring ArgString = std::wstring(SomeWideStringInsteadOfArgs, 7) + L"Appended String_";
        TEST_CHECK_STRING(StaticString5, ArgString.c_str());

        WStaticString<64> StaticString6;
        StaticString6.Format(L"Formatted String=%.4f", 0.004f);
        TEST_CHECK_STRING(StaticString6, L"Formatted String=0.0040");

        StaticString6.Append(L'_');
        TEST_CHECK_STRING(StaticString6, L"Formatted String=0.0040_");

        StaticString6.AppendFormat(L"Formatted String=%.4f", 0.0077f);
        TEST_CHECK_STRING(StaticString6, L"Formatted String=0.0040_Formatted String=0.0077");

        WStaticString<64> LowerStaticString6 = StaticString6.ToLower();
        TEST_CHECK_STRING(LowerStaticString6, L"formatted string=0.0040_formatted string=0.0077");

        WStaticString<64> UpperStaticString6 = StaticString6.ToUpper();
        TEST_CHECK_STRING(UpperStaticString6, L"FORMATTED STRING=0.0040_FORMATTED STRING=0.0077");

        StaticString6.Reset();
        TEST_CHECK_STRING(StaticString6, L"");

        StaticString6.Append(L"    Trimmable String    ");
        TEST_CHECK_STRING(StaticString6, L"    Trimmable String    ");

        WStaticString<64> TrimmedStaticString6 = StaticString6.Trim();
        TrimmedStaticString6.Append(L'*');
        TEST_CHECK_STRING(TrimmedStaticString6, L"Trimmable String*");

        StaticString6.Reset();
        TEST_CHECK_STRING(StaticString6, L"");

        StaticString6.Append(L"123456789");
        TEST_CHECK_STRING(StaticString6, L"123456789");

        WStaticString<64> ReversedStaticString6 = StaticString6.Reverse();
        TEST_CHECK_STRING(ReversedStaticString6, L"987654321");

        WStaticString<64> SearchString = L"0123MeSearch89Me89";
        TEST_CHECK_STRING(SearchString, L"0123MeSearch89Me89");

        TEST_CHECK(SearchString.Find(L"Me")                                    == 4);
        TEST_CHECK(SearchString.FindChar(L'M')                                 == 4);
        TEST_CHECK(SearchString.Contains(L"Me")                                == true);
        TEST_CHECK(SearchString.Contains(L'M')                                 == true);
        TEST_CHECK(SearchString.StartsWith(L"0123Me")                          == true);
        TEST_CHECK(SearchString.StartsWith(L"0123ME", EStringCaseType::NoCase) == true);
        TEST_CHECK(SearchString.EndsWith(L"Me89")                              == true);
        TEST_CHECK(SearchString.EndsWith(L"ME89", EStringCaseType::NoCase)     == true);

        TEST_CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'e') || (Char == L'c');
        }) == 5);

        TEST_CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'M') || (Char == L'c');
        }) == 4);

        TEST_CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
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

        TEST_CHECK(SearchString.FindLast(L"Me") == 14);
        TEST_CHECK(SearchString.FindLastChar(L'M') == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'h') || (Char == L'M') || (Char == L'c');
        }) == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR Char) -> bool
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

        WStaticString<64> CompareString0 = L"COMPARE";
        TEST_CHECK_STRING(CompareString0, L"COMPARE");

        WStaticString<64> CompareString1 = L"compare";
        TEST_CHECK_STRING(CompareString1, L"compare");

        TEST_CHECK(CompareString0.Compare(CompareString1)                          != 0);
        TEST_CHECK(CompareString0.Compare(CompareString1, EStringCaseType::NoCase) == 0);

        CompareString1.Resize(7);
        TEST_CHECK(CStringWide::Strcmp(CompareString1.GetCString(), L"compare") == 0);

        CompareString1.Resize(20);
        TEST_CHECK(CStringWide::Strcmp(CompareString1.GetCString(), L"compare") == 0);

        CompareString1.Resize(7);
        TEST_CHECK(CStringWide::Strcmp(CompareString1.GetCString(), L"compare") == 0);

        WIDECHAR Buffer[6];
        Buffer[5] = 0;
        CompareString1.CopyToBuffer(Buffer, 5, 2);
        TEST_CHECK(CStringWide::Strcmp(Buffer, L"mpare") == 0);

        CompareString0.Insert(L"lower", 4);
        TEST_CHECK_STRING(CompareString0, L"COMPlowerARE");

        CompareString0.Replace(L"upper", 4);
        TEST_CHECK_STRING(CompareString0, L"COMPupperARE");

        CompareString0.Replace(L'X', 0);
        TEST_CHECK_STRING(CompareString0, L"XOMPupperARE");

        WStaticString<64> CombinedString = (CompareString0 + L'5');
        TEST_CHECK_STRING(CombinedString, L"XOMPupperARE5");

        CombinedString = L'5' + CombinedString;
        TEST_CHECK_STRING(CombinedString, L"5XOMPupperARE5");

        CombinedString = CombinedString + L"Appended";
        TEST_CHECK_STRING(CombinedString, L"5XOMPupperARE5Appended");

        CombinedString = L"Inserted" + CombinedString;
        TEST_CHECK_STRING(CombinedString, L"Inserted5XOMPupperARE5Appended");

        CombinedString = CombinedString + CombinedString;
        TEST_CHECK_STRING(CombinedString, L"Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");

        WStaticString<64> TestString = L"Test";
        TEST_CHECK_STRING(TestString, L"Test");

        TEST_CHECK((L"Test" == TestString) == true);
        TEST_CHECK((TestString == L"Test") == true);
        TEST_CHECK((CombinedString == CombinedString) == true);

        TEST_CHECK((L"Test" != TestString) == false);
        TEST_CHECK((TestString != L"Test") == false);
        TEST_CHECK((CombinedString != CombinedString) == false);

        TEST_CHECK((L"Test" <= TestString) == true);
        TEST_CHECK((TestString <= L"Test") == true);
        TEST_CHECK((CombinedString <= CombinedString) == true);

        TEST_CHECK((L"Test" < TestString) == false);
        TEST_CHECK((TestString < L"Test") == false);
        TEST_CHECK((CombinedString < CombinedString) == false);

        TEST_CHECK((L"Test" >= TestString) == true);
        TEST_CHECK((TestString >= L"Test") == true);
        TEST_CHECK((CombinedString >= CombinedString) == true);

        TEST_CHECK((L"Test" > TestString) == false);
        TEST_CHECK((TestString > L"Test") == false);
        TEST_CHECK((CombinedString > CombinedString) == false);

        for (WIDECHAR C : TestString)
        {
            std::wcout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.Length(); Index++)
        {
            std::wcout << Index << L'=' << TestString[Index] << std::endl;
        }

        StaticString<64> WideCompareString = WideToChar(CombinedString);
        TEST_CHECK_STRING(WideCompareString, "Inserted5XOMPupperARE5AppendedInserted5XOMPupperARE5Appended");
    }
    
    SUCCESS();
}
#endif

#if RUN_TSTRINGVIEW_TEST
bool TStringView_Test_Internal(const CHAR* Args)
{
    UNREFERENCED_VARIABLE(Args);

    {
        std::cout << std::endl << "----Testing StringView----" << std::endl << std::endl;

        const CHAR* LongString = "This is a long string";

        StringView StringView0;
        TEST_CHECK_STRING(StringView0, "");
        StringView StringView1 = LongString;
        TEST_CHECK_STRING(StringView1, "This is a long string");
        StringView StringView2 = StringView(LongString, 4, 5);
        TEST_CHECK_STRING(StringView2, "is a");

        CHAR Buffer[6] = { };
        Buffer[5] = 0;
        StringView1.CopyToBuffer(Buffer, 5, 3);
        TEST_CHECK(CString::Strcmp(Buffer, "s is ") == 0);

        StringView StringView3 = "    Trimmable String    ";
        TEST_CHECK_STRING(StringView3, "    Trimmable String    ");

        StringView StringView4 = StringView3.Trim();
        TEST_CHECK_STRING(StringView4, "Trimmable String");

        StringView StringView5 = StringView("COMPAREPostfix", 7);
        TEST_CHECK_STRING(StringView5, "COMPARE");

        StringView StringView6 = StringView("comparePostfix", 7);
        TEST_CHECK_STRING(StringView6, "compare");

        TEST_CHECK(StringView5.Compare(StringView6)                          != 0);
        TEST_CHECK(StringView5.Compare(StringView6, EStringCaseType::NoCase) == 0);

        StringView6.Clear();
        PrintStringView(StringView6);

        StringView SearchString = "0123MeSearch89Me89";
        TEST_CHECK_STRING(SearchString, "0123MeSearch89Me89");

        TEST_CHECK(SearchString.Find("Me")                                    == 4);
        TEST_CHECK(SearchString.FindChar('M')                                 == 4);
        TEST_CHECK(SearchString.Contains("Me")                                == true);
        TEST_CHECK(SearchString.Contains('M')                                 == true);
        TEST_CHECK(SearchString.StartsWith("0123Me")                          == true);
        TEST_CHECK(SearchString.StartsWith("0123ME", EStringCaseType::NoCase) == true);
        TEST_CHECK(SearchString.EndsWith("Me89")                              == true);
        TEST_CHECK(SearchString.EndsWith("ME89", EStringCaseType::NoCase)     == true);

        TEST_CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'e') || (Char == 'c');
        }) == 5);

        TEST_CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'M') || (Char == 'c');
        }) == 4);

        TEST_CHECK(SearchString.FindCharWithPredicate([](CHAR Char) -> bool
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

        TEST_CHECK(SearchString.FindLast("Me")    == 14);
        TEST_CHECK(SearchString.FindLastChar('M') == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
        {
            return (Char == 'h') || (Char == 'M') || (Char == 'c');
        }) == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](CHAR Char) -> bool
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

        StringView TestString = "Test";
        TEST_CHECK_STRING(TestString, "Test");

        TEST_CHECK(("Test" == TestString) == true);
        TEST_CHECK((TestString == "Test") == true);

        TEST_CHECK(("Test" != TestString) == false);
        TEST_CHECK((TestString != "Test") == false);

        TEST_CHECK(("Test" <= TestString) == true);
        TEST_CHECK((TestString <= "Test") == true);

        TEST_CHECK(("Test" < TestString) == false);
        TEST_CHECK((TestString < "Test") == false);

        TEST_CHECK(("Test" >= TestString) == true);
        TEST_CHECK((TestString >= "Test") == true);

        TEST_CHECK(("Test" > TestString) == false);
        TEST_CHECK((TestString > "Test") == false);

        for (CHAR C : TestString)
        {
            std::cout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.Length(); Index++)
        {
            std::cout << Index << '=' << TestString[Index] << std::endl;
        }
    }

    {
        std::cout << std::endl << "----Testing WStringView----" << std::endl << std::endl;

        const WIDECHAR* LongString = L"This is a long string";

        WStringView StringView0;
        TEST_CHECK_STRING(StringView0, L"");
        WStringView StringView1 = LongString;
        TEST_CHECK_STRING(StringView1, L"This is a long string");
        WStringView StringView2 = WStringView(LongString, 4, 5);
        TEST_CHECK_STRING(StringView2, L"is a");

        WIDECHAR Buffer[6] = { };
        Buffer[5] = 0;
        StringView1.CopyToBuffer(Buffer, 5, 3);
        TEST_CHECK(CStringWide::Strcmp(Buffer, L"s is ") == 0);

        WStringView StringView3 = L"    Trimmable String    ";
        TEST_CHECK_STRING(StringView3, L"    Trimmable String    ");

        WStringView StringView4 = StringView3.Trim();
        TEST_CHECK_STRING(StringView4, L"Trimmable String");

        WStringView StringView5 = WStringView(L"COMPAREPostfix", 7);
        TEST_CHECK_STRING(StringView5, L"COMPARE");

        WStringView StringView6 = WStringView(L"comparePostfix", 7);
        TEST_CHECK_STRING(StringView6, L"compare");

        TEST_CHECK(StringView5.Compare(StringView6)                          != 0);
        TEST_CHECK(StringView5.Compare(StringView6, EStringCaseType::NoCase) == 0);

        StringView6.Clear();
        PrintWideStringView(StringView6);

        WStringView SearchString = L"0123MeSearch89Me89";
        TEST_CHECK_STRING(SearchString, L"0123MeSearch89Me89");

        TEST_CHECK(SearchString.Find(L"Me")                                    == 4);
        TEST_CHECK(SearchString.FindChar(L'M')                                 == 4);
        TEST_CHECK(SearchString.Contains(L"Me")                                == true);
        TEST_CHECK(SearchString.Contains(L'M')                                 == true);
        TEST_CHECK(SearchString.StartsWith(L"0123Me")                          == true);
        TEST_CHECK(SearchString.StartsWith(L"0123ME", EStringCaseType::NoCase) == true);
        TEST_CHECK(SearchString.EndsWith(L"Me89")                              == true);
        TEST_CHECK(SearchString.EndsWith(L"ME89", EStringCaseType::NoCase)     == true);

        TEST_CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'e') || (Char == L'c');
        }) == 5);

        TEST_CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'M') || (Char == L'c');
        }) == 4);

        TEST_CHECK(SearchString.FindCharWithPredicate([](WIDECHAR Char) -> bool
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

        TEST_CHECK(SearchString.FindLast(L"Me") == 14);
        TEST_CHECK(SearchString.FindLastChar(L'M') == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR Char) -> bool
        {
            return (Char == L'h') || (Char == L'M') || (Char == L'c');
        }) == 14);

        TEST_CHECK(SearchString.FindLastCharWithPredicate([](WIDECHAR Char) -> bool
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

        WStringView TestString = L"Test";
        TEST_CHECK_STRING(TestString, L"Test");

        TEST_CHECK((L"Test" == TestString) == true);
        TEST_CHECK((TestString == L"Test") == true);

        TEST_CHECK((L"Test" != TestString) == false);
        TEST_CHECK((TestString != L"Test") == false);

        TEST_CHECK((L"Test" <= TestString) == true);
        TEST_CHECK((TestString <= L"Test") == true);

        TEST_CHECK((L"Test" < TestString) == false);
        TEST_CHECK((TestString < L"Test") == false);

        TEST_CHECK((L"Test" >= TestString) == true);
        TEST_CHECK((TestString >= L"Test") == true);

        TEST_CHECK((L"Test" > TestString) == false);
        TEST_CHECK((TestString > L"Test") == false);

        for (WIDECHAR C : TestString)
        {
            std::wcout << C << std::endl;
        }

        for (int32 Index = 0; Index < TestString.Length(); Index++)
        {
            std::wcout << Index << L'=' << TestString[Index] << std::endl;
        }
    }

    SUCCESS();
}
#endif