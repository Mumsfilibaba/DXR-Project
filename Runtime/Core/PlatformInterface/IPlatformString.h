#pragma once
#include "Core/Core.h"
#include <cctype>
#include <cwchar>
#include <cwctype>
#include <cstring>

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct IPlatformString
{
    /**
     * @brief Copy a wide format, turning every %s and %c without a length modifier into %ls and %lc. In the engine, %s and %c
     * take the format's own character type, which is what MSVC does. Standard C reads them as narrow in a wide format, so
     * platforms that follow it pass wide formats through this first.
     * @return Returns false if the result does not fit in OutFormatLength characters, including the null-terminator
     */
    static bool ConvertWideFormatToStandard(WIDECHAR* OutFormat, int32 OutFormatLength, const WIDECHAR* Format) noexcept
    {
        int32 NumWritten = 0;
        while (*Format != 0)
        {
            if (NumWritten + 3 > OutFormatLength)
            {
                return false;
            }

            const WIDECHAR Char = *Format++;
            OutFormat[NumWritten++] = Char;
            if (Char != L'%')
            {
                continue;
            }

            if (*Format == L'%')
            {
                OutFormat[NumWritten++] = *Format++;
                continue;
            }

            bool bHasLengthModifier = false;
            while (*Format != 0 && Strchr(L"-+ #0123456789.*$hljztLqI", *Format) != nullptr)
            {
                if (NumWritten + 3 > OutFormatLength)
                {
                    return false;
                }

                bHasLengthModifier = bHasLengthModifier || Strchr(L"hljztLqI", *Format) != nullptr;
                OutFormat[NumWritten++] = *Format++;
            }

            if (!bHasLengthModifier && (*Format == L's' || *Format == L'c'))
            {
                OutFormat[NumWritten++] = L'l';
            }
        }

        OutFormat[NumWritten] = 0;
        return true;
    }

    // -------------------------------------------------------------------------------------------
    // ANSICHAR
    // -------------------------------------------------------------------------------------------

    NODISCARD static FORCEINLINE ANSICHAR ToUpper(ANSICHAR Char)
    {
        return static_cast<ANSICHAR>(::toupper(static_cast<uint8>(Char)));
    }

    NODISCARD static FORCEINLINE ANSICHAR ToLower(ANSICHAR Char)
    {
        return static_cast<ANSICHAR>(::tolower(static_cast<uint8>(Char)));
    }

    NODISCARD static FORCEINLINE bool IsWhitespace(ANSICHAR Char)
    {
        return static_cast<bool>(::isspace(static_cast<uint8>(Char)));
    }

    NODISCARD static FORCEINLINE bool IsUpper(ANSICHAR Char)
    {
        return static_cast<bool>(::isupper(static_cast<uint8>(Char)));
    }

    NODISCARD static FORCEINLINE bool IsLower(ANSICHAR Char)
    {
        return static_cast<bool>(::islower(static_cast<uint8>(Char)));
    }

    NODISCARD static FORCEINLINE bool IsAlnum(ANSICHAR Char)
    {
        return static_cast<bool>(::isalnum(static_cast<uint8>(Char)));
    }

    NODISCARD static FORCEINLINE bool IsDigit(ANSICHAR Char)
    {
        return static_cast<bool>(::isdigit(static_cast<uint8>(Char)));
    }

    NODISCARD static FORCEINLINE bool IsAlpha(ANSICHAR Char)
    {
        return static_cast<bool>(::isalpha(static_cast<uint8>(Char)));
    }

    NODISCARD static FORCEINLINE bool IsPunct(ANSICHAR Char)
    {
        return static_cast<bool>(::ispunct(static_cast<uint8>(Char)));
    }

    NODISCARD static FORCEINLINE bool IsHexDigit(ANSICHAR Char)
    {
        return static_cast<bool>(::isxdigit(static_cast<uint8>(Char)));
    }

    template<typename... ArgTypes>
    static FORCEINLINE int32 Snprintf(ANSICHAR* Buffer, SIZE_T BufferSize, const ANSICHAR* Format, ArgTypes&&... Args) noexcept
    {
        // The format string is a literal at the call site, it cannot be one here.
        DISABLE_FORMAT_SECURITY_WARNING
        return static_cast<int32>(::snprintf(Buffer, BufferSize, Format, Forward<ArgTypes>(Args)...));
        ENABLE_FORMAT_SECURITY_WARNING
    }

    NODISCARD static FORCEINLINE ANSICHAR* Strstr(const ANSICHAR* String, const ANSICHAR* Find) noexcept
    {
        // NOTE: This is super strange looking at the docs this function should return 'char' but seems to return 'const char'
        // https://en.cppreference.com/w/c/string/byte/strstr
        return const_cast<ANSICHAR*>(static_cast<const ANSICHAR*>(::strstr(String, Find)));
    }

	NODISCARD static FORCEINLINE ANSICHAR* Stristr(const ANSICHAR* String, const ANSICHAR* Find) noexcept
	{
        if (!String || !Find)
        {
			return nullptr;
        }

        if (*Find == '\0')
        {
			return const_cast<ANSICHAR*>(String); // Empty needle matches at start (same as strstr)
        }

		for (const ANSICHAR* S = String; *S != '\0'; ++S)
		{
			const ANSICHAR* H = S;
			const ANSICHAR* N = Find;

			while (*H != '\0' && *N != '\0')
			{
				const ANSICHAR Hc = ToLower(*H);
				const ANSICHAR Nc = ToLower(*N);

                if (Hc != Nc)
                {
					break;
                }

				++H;
				++N;
			}

            if (*N == '\0')
            {
				return const_cast<ANSICHAR*>(S); // Matched whole needle
            }
		}

		return nullptr;
	}

    NODISCARD static FORCEINLINE ANSICHAR* Strpbrk(const ANSICHAR* String, const ANSICHAR* Find) noexcept
    {
        return const_cast<ANSICHAR*>(static_cast<const ANSICHAR*>(::strpbrk(String, Find)));
    }

    NODISCARD static FORCEINLINE ANSICHAR* Strchr(const ANSICHAR* String, ANSICHAR Char) noexcept
    {
        return const_cast<ANSICHAR*>(static_cast<const ANSICHAR*>(::strchr(String, Char)));
    }

    NODISCARD static FORCEINLINE ANSICHAR* Strrchr(const ANSICHAR* String, ANSICHAR Char) noexcept
    {
        return const_cast<ANSICHAR*>(static_cast<const ANSICHAR*>(::strrchr(String, Char)));
    }
    
    NODISCARD static FORCEINLINE int32 Strlen(const ANSICHAR* String) noexcept
    {
        return static_cast<int32>(::strlen(String));
    }

    NODISCARD static FORCEINLINE SIZE_T Strspn(const ANSICHAR* String, const ANSICHAR* Set) noexcept
    {
        return static_cast<SIZE_T>(::strspn(String, Set));
    }

    static FORCEINLINE ANSICHAR* Strcpy(ANSICHAR* Dst, const ANSICHAR* Src) noexcept
    {
        return static_cast<ANSICHAR*>(::strcpy(Dst, Src));
    }

    static FORCEINLINE ANSICHAR* Strncpy(ANSICHAR* Dst, const ANSICHAR* Src, SIZE_T InLength) noexcept
    {
        return static_cast<ANSICHAR*>(::strncpy(Dst, Src, InLength));
    }

    static FORCEINLINE ANSICHAR* Strcat(ANSICHAR* Dst, const ANSICHAR* Src) noexcept
    {
        return static_cast<ANSICHAR*>(::strcat(Dst, Src));
    }

    static FORCEINLINE ANSICHAR* Strncat(ANSICHAR* Dst, const ANSICHAR* Src, SIZE_T InLength) noexcept
    {
        return static_cast<ANSICHAR*>(::strncat(Dst, Src, InLength));
    }

    NODISCARD static FORCEINLINE int32 Strcmp(const ANSICHAR* String0, const ANSICHAR* String1) noexcept
    {
        return static_cast<int32>(::strcmp(String0, String1));
    }

    NODISCARD static FORCEINLINE int32 Strncmp(const ANSICHAR* String0, const ANSICHAR* String1, SIZE_T InLength) noexcept
    {
        return static_cast<int32>(::strncmp(String0, String1, InLength));
    }

    NODISCARD static FORCEINLINE int32 Strtoi(const ANSICHAR* String, ANSICHAR** End, int32 Base) noexcept
    {
        return static_cast<int32>(::strtol(String, End, Base));
    }

    NODISCARD static FORCEINLINE int64 Strtoi64(const ANSICHAR* String, ANSICHAR** End, int32 Base) noexcept
    {
        return static_cast<int64>(::strtoll(String, End, Base));
    }

    NODISCARD static FORCEINLINE uint32 Strtoui(const ANSICHAR* String, ANSICHAR** End, int32 Base) noexcept
    {
        return static_cast<uint32>(::strtoul(String, End, Base));
    }

    NODISCARD static FORCEINLINE uint64 Strtoui64(const ANSICHAR* String, ANSICHAR** End, int32 Base) noexcept
    {
        return static_cast<uint64>(::strtoll(String, End, Base));
    }

    NODISCARD static FORCEINLINE float Strtof(const ANSICHAR* String, ANSICHAR** End) noexcept
    {
        return static_cast<float>(::strtof(String, End));
    }

    NODISCARD static FORCEINLINE double Strtod(const ANSICHAR* String, ANSICHAR** End) noexcept
    {
        return static_cast<double>(::strtod(String, End));
    }

    NODISCARD static FORCEINLINE long double Strtold(const ANSICHAR* String, ANSICHAR** End) noexcept
    {
        return static_cast<long double>(::strtold(String, End));
    }
    
    NODISCARD static FORCEINLINE int32 Atoi(const ANSICHAR* String) noexcept
    {
        return static_cast<int32>(::atol(String));
    }

    NODISCARD static FORCEINLINE int64 Atoi64(const ANSICHAR* String) noexcept
    {
        return static_cast<int64>(::atoll(String));
    }

    NODISCARD static FORCEINLINE float Atof(const ANSICHAR* String) noexcept
    {
        return static_cast<float>(::atof(String));
    }

    NODISCARD static FORCEINLINE double Atod(const ANSICHAR* String) noexcept
    {
        return static_cast<double>(::atof(String));
    }

    // -------------------------------------------------------------------------------------------
    // WIDECHAR
    // -------------------------------------------------------------------------------------------

    NODISCARD static FORCEINLINE WIDECHAR ToUpper(WIDECHAR Char)
    {
        return static_cast<WIDECHAR>(::towupper(Char));
    }

    NODISCARD static FORCEINLINE WIDECHAR ToLower(WIDECHAR Char)
    {
        return static_cast<WIDECHAR>(::towlower(Char));
    }

    NODISCARD static FORCEINLINE bool IsWhitespace(WIDECHAR Char)
    {
        return static_cast<bool>(::iswspace(Char));
    }

    NODISCARD static FORCEINLINE bool IsUpper(WIDECHAR Char)
    {
        return static_cast<bool>(::iswupper(Char));
    }

    NODISCARD static FORCEINLINE bool IsLower(WIDECHAR Char)
    {
        return static_cast<bool>(::iswlower(Char));
    }

    NODISCARD static FORCEINLINE bool IsAlnum(WIDECHAR Char)
    {
        return static_cast<bool>(::iswalnum(Char));
    }

    NODISCARD static FORCEINLINE bool IsDigit(WIDECHAR Char)
    {
        return static_cast<bool>(::iswdigit(Char));
    }

    NODISCARD static FORCEINLINE bool IsAlpha(WIDECHAR Char)
    {
        return static_cast<bool>(::iswalpha(Char));
    }

    NODISCARD static FORCEINLINE bool IsPunct(WIDECHAR Char)
    {
        return static_cast<bool>(::iswpunct(Char));
    }

    NODISCARD static FORCEINLINE bool IsHexDigit(WIDECHAR Char)
    {
        return static_cast<bool>(::iswxdigit(Char));
    }

    template<typename... ArgTypes>
    static FORCEINLINE int32 Snprintf(WIDECHAR* Buffer, SIZE_T BufferSize, const WIDECHAR* Format, ArgTypes&&... Args) noexcept
    {
        return static_cast<int32>(::swprintf(Buffer, BufferSize, Format, Forward<ArgTypes>(Args)...));
    }

    NODISCARD static FORCEINLINE WIDECHAR* Strstr(const WIDECHAR* String, const WIDECHAR* Find) noexcept
    {
        return const_cast<WIDECHAR*>(static_cast<const WIDECHAR*>(::wcsstr(String, Find)));
    }

	NODISCARD static FORCEINLINE WIDECHAR* Stristr(const WIDECHAR* String, const WIDECHAR* Find) noexcept
	{
		if (!String || !Find)
		{
			return nullptr;
		}

		if (*Find == '\0')
		{
			return const_cast<WIDECHAR*>(String); // Empty needle matches at start (same as strstr)
		}

		for (const WIDECHAR* S = String; *S != '\0'; ++S)
		{
			const WIDECHAR* H = S;
			const WIDECHAR* N = Find;

			while (*H != '\0' && *N != '\0')
			{
				const WIDECHAR Hc = ToLower(*H);
				const WIDECHAR Nc = ToLower(*N);
				if (Hc != Nc)
				{
					break;
				}

				++H;
				++N;
			}

			if (*N == '\0')
			{
				return const_cast<WIDECHAR*>(S); // Matched whole needle
			}
		}

		return nullptr;
	}

    NODISCARD static FORCEINLINE WIDECHAR* Strpbrk(const WIDECHAR* String, const WIDECHAR* Find) noexcept
    {
        return const_cast<WIDECHAR*>(static_cast<const WIDECHAR*>(::wcspbrk(String, Find)));
    }

    NODISCARD static FORCEINLINE WIDECHAR* Strchr(const WIDECHAR* String, WIDECHAR Char) noexcept
    {
        return const_cast<WIDECHAR*>(static_cast<const WIDECHAR*>(::wcschr(String, Char)));
    }

    NODISCARD static FORCEINLINE WIDECHAR* Strrchr(const WIDECHAR* String, WIDECHAR Char) noexcept
    {
        return const_cast<WIDECHAR*>(static_cast<const WIDECHAR*>(::wcsrchr(String, Char)));
    }

    NODISCARD static FORCEINLINE int32 Strlen(const WIDECHAR* String) noexcept
    {
        return static_cast<int32>(::wcslen(String));
    }

    NODISCARD static FORCEINLINE int32 Strspn(const WIDECHAR* String, const WIDECHAR* Set) noexcept
    {
        return static_cast<int32>(::wcsspn(String, Set));
    }

    static FORCEINLINE WIDECHAR* Strcpy(WIDECHAR* Dst, const WIDECHAR* Src) noexcept
    {
        return static_cast<WIDECHAR*>(::wcscpy(Dst, Src));
    }

    static FORCEINLINE WIDECHAR* Strncpy(WIDECHAR* Dst, const WIDECHAR* Src, SIZE_T InLength) noexcept
    {
        return static_cast<WIDECHAR*>(::wcsncpy(Dst, Src, InLength));
    }

    static FORCEINLINE WIDECHAR* Strcat(WIDECHAR* Dst, const WIDECHAR* Src) noexcept
    {
        return static_cast<WIDECHAR*>(::wcscat(Dst, Src));
    }

    static FORCEINLINE WIDECHAR* Strncat(WIDECHAR* Dst, const WIDECHAR* Src, SIZE_T InLength) noexcept
    {
        return static_cast<WIDECHAR*>(::wcsncat(Dst, Src, InLength));
    }

    NODISCARD static FORCEINLINE int32 Strcmp(const WIDECHAR* String0, const WIDECHAR* String1) noexcept
    {
        return static_cast<int32>(::wcscmp(String0, String1));
    }

    NODISCARD static FORCEINLINE int32 Strncmp(const WIDECHAR* String0, const WIDECHAR* String1, SIZE_T InLength) noexcept
    {
        return static_cast<int32>(::wcsncmp(String0, String1, InLength));
    }

    NODISCARD static FORCEINLINE int32 Strtoi(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<int32>(::wcstol(String, End, Base));
    }
    
    NODISCARD static FORCEINLINE int64 Strtoi64(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<int64>(::wcstoll(String, End, Base));
    }
    
    NODISCARD static FORCEINLINE uint32 Strtoui(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<uint32>(::wcstoul(String, End, Base));
    }
    
    NODISCARD static FORCEINLINE uint64 Strtoui64(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<uint64>(::wcstoull(String, End, Base));
    }
    
    NODISCARD static FORCEINLINE float Strtof(const WIDECHAR* String, WIDECHAR** End) noexcept
    {
        return static_cast<float>(::wcstof(String, End));
    }

    NODISCARD static FORCEINLINE double Strtod(const WIDECHAR* String, WIDECHAR** End) noexcept
    {
        return static_cast<double>(::wcstod(String, End));
    }

    NODISCARD static FORCEINLINE long double Strtold(const WIDECHAR* String, WIDECHAR** End) noexcept
    {
        return static_cast<long double>(::wcstold(String, End));
    }

    NODISCARD static FORCEINLINE int32 Atoi(const WIDECHAR* String) noexcept
    {
        return static_cast<int32>(::wcstol(String, nullptr, 10));
    }

    NODISCARD static FORCEINLINE int64 Atoi64(const WIDECHAR* String) noexcept
    {
        return static_cast<int64>(::wcstoll(String, nullptr, 10));
    }

    NODISCARD static FORCEINLINE float Atof(const WIDECHAR* String) noexcept
    {
        return static_cast<float>(::wcstof(String, nullptr));
    }

    NODISCARD static FORCEINLINE double Atod(const WIDECHAR* String) noexcept
    {
        return static_cast<double>(::wcstod(String, nullptr));
    }

    // -------------------------------------------------------------------------------------------
    // UTF8CHAR
    // -------------------------------------------------------------------------------------------

    NODISCARD static FORCEINLINE UTF8CHAR ToUpper(UTF8CHAR Char)
    {
        return static_cast<UTF8CHAR>(ToUpper(static_cast<ANSICHAR>(Char)));
    }

    NODISCARD static FORCEINLINE UTF8CHAR ToLower(UTF8CHAR Char)
    {
        return static_cast<UTF8CHAR>(ToLower(static_cast<ANSICHAR>(Char)));
    }

    NODISCARD static FORCEINLINE bool IsWhitespace(UTF8CHAR Char)
    {
        return IsWhitespace(static_cast<ANSICHAR>(Char));
    }

    NODISCARD static FORCEINLINE bool IsUpper(UTF8CHAR Char)
    {
        return IsUpper(static_cast<ANSICHAR>(Char));
    }

    NODISCARD static FORCEINLINE bool IsLower(UTF8CHAR Char)
    {
        return IsLower(static_cast<ANSICHAR>(Char));
    }

    NODISCARD static FORCEINLINE bool IsAlnum(UTF8CHAR Char)
    {
        return IsAlnum(static_cast<ANSICHAR>(Char));
    }

    NODISCARD static FORCEINLINE bool IsDigit(UTF8CHAR Char)
    {
        return IsDigit(static_cast<ANSICHAR>(Char));
    }

    NODISCARD static FORCEINLINE bool IsAlpha(UTF8CHAR Char)
    {
        return IsAlpha(static_cast<ANSICHAR>(Char));
    }

    NODISCARD static FORCEINLINE bool IsPunct(UTF8CHAR Char)
    {
        return IsPunct(static_cast<ANSICHAR>(Char));
    }

    NODISCARD static FORCEINLINE bool IsHexDigit(UTF8CHAR Char)
    {
        return IsHexDigit(static_cast<ANSICHAR>(Char));
    }

    template<typename... ArgTypes>
    static FORCEINLINE int32 Snprintf(UTF8CHAR* Buffer, SIZE_T BufferSize, const UTF8CHAR* Format, ArgTypes&&... Args) noexcept
    {
        return Snprintf(AsAnsi(Buffer), BufferSize, AsAnsi(Format), Forward<ArgTypes>(Args)...);
    }

    NODISCARD static FORCEINLINE UTF8CHAR* Strstr(const UTF8CHAR* String, const UTF8CHAR* Find) noexcept
    {
        return AsUtf8(Strstr(AsAnsi(String), AsAnsi(Find)));
    }

    NODISCARD static FORCEINLINE UTF8CHAR* Stristr(const UTF8CHAR* String, const UTF8CHAR* Find) noexcept
    {
        return AsUtf8(Stristr(AsAnsi(String), AsAnsi(Find)));
    }

    NODISCARD static FORCEINLINE UTF8CHAR* Strpbrk(const UTF8CHAR* String, const UTF8CHAR* Find) noexcept
    {
        return AsUtf8(Strpbrk(AsAnsi(String), AsAnsi(Find)));
    }

    NODISCARD static FORCEINLINE UTF8CHAR* Strchr(const UTF8CHAR* String, UTF8CHAR Char) noexcept
    {
        return AsUtf8(Strchr(AsAnsi(String), static_cast<ANSICHAR>(Char)));
    }

    NODISCARD static FORCEINLINE UTF8CHAR* Strrchr(const UTF8CHAR* String, UTF8CHAR Char) noexcept
    {
        return AsUtf8(Strrchr(AsAnsi(String), static_cast<ANSICHAR>(Char)));
    }

    NODISCARD static FORCEINLINE int32 Strlen(const UTF8CHAR* String) noexcept
    {
        return Strlen(AsAnsi(String));
    }

    NODISCARD static FORCEINLINE SIZE_T Strspn(const UTF8CHAR* String, const UTF8CHAR* Set) noexcept
    {
        return Strspn(AsAnsi(String), AsAnsi(Set));
    }

    static FORCEINLINE UTF8CHAR* Strcpy(UTF8CHAR* Dst, const UTF8CHAR* Src) noexcept
    {
        return AsUtf8(Strcpy(AsAnsi(Dst), AsAnsi(Src)));
    }

    static FORCEINLINE UTF8CHAR* Strncpy(UTF8CHAR* Dst, const UTF8CHAR* Src, SIZE_T InLength) noexcept
    {
        return AsUtf8(Strncpy(AsAnsi(Dst), AsAnsi(Src), InLength));
    }

    static FORCEINLINE UTF8CHAR* Strcat(UTF8CHAR* Dst, const UTF8CHAR* Src) noexcept
    {
        return AsUtf8(Strcat(AsAnsi(Dst), AsAnsi(Src)));
    }

    static FORCEINLINE UTF8CHAR* Strncat(UTF8CHAR* Dst, const UTF8CHAR* Src, SIZE_T InLength) noexcept
    {
        return AsUtf8(Strncat(AsAnsi(Dst), AsAnsi(Src), InLength));
    }

    NODISCARD static FORCEINLINE int32 Strcmp(const UTF8CHAR* String0, const UTF8CHAR* String1) noexcept
    {
        return Strcmp(AsAnsi(String0), AsAnsi(String1));
    }

    NODISCARD static FORCEINLINE int32 Strncmp(const UTF8CHAR* String0, const UTF8CHAR* String1, SIZE_T InLength) noexcept
    {
        return Strncmp(AsAnsi(String0), AsAnsi(String1), InLength);
    }

    NODISCARD static FORCEINLINE int32 Strtoi(const UTF8CHAR* String, UTF8CHAR** End, int32 Base) noexcept
    {
        return Strtoi(AsAnsi(String), AsAnsi(End), Base);
    }

    NODISCARD static FORCEINLINE int64 Strtoi64(const UTF8CHAR* String, UTF8CHAR** End, int32 Base) noexcept
    {
        return Strtoi64(AsAnsi(String), AsAnsi(End), Base);
    }

    NODISCARD static FORCEINLINE uint32 Strtoui(const UTF8CHAR* String, UTF8CHAR** End, int32 Base) noexcept
    {
        return Strtoui(AsAnsi(String), AsAnsi(End), Base);
    }

    NODISCARD static FORCEINLINE uint64 Strtoui64(const UTF8CHAR* String, UTF8CHAR** End, int32 Base) noexcept
    {
        return Strtoui64(AsAnsi(String), AsAnsi(End), Base);
    }

    NODISCARD static FORCEINLINE float Strtof(const UTF8CHAR* String, UTF8CHAR** End) noexcept
    {
        return Strtof(AsAnsi(String), AsAnsi(End));
    }

    NODISCARD static FORCEINLINE double Strtod(const UTF8CHAR* String, UTF8CHAR** End) noexcept
    {
        return Strtod(AsAnsi(String), AsAnsi(End));
    }

    NODISCARD static FORCEINLINE long double Strtold(const UTF8CHAR* String, UTF8CHAR** End) noexcept
    {
        return Strtold(AsAnsi(String), AsAnsi(End));
    }

    NODISCARD static FORCEINLINE int32 Atoi(const UTF8CHAR* String) noexcept
    {
        return Atoi(AsAnsi(String));
    }

    NODISCARD static FORCEINLINE int64 Atoi64(const UTF8CHAR* String) noexcept
    {
        return Atoi64(AsAnsi(String));
    }

    NODISCARD static FORCEINLINE float Atof(const UTF8CHAR* String) noexcept
    {
        return Atof(AsAnsi(String));
    }

    NODISCARD static FORCEINLINE double Atod(const UTF8CHAR* String) noexcept
    {
        return Atod(AsAnsi(String));
    }

protected:
    static FORCEINLINE const ANSICHAR* AsAnsi(const UTF8CHAR* String) noexcept
    {
        return reinterpret_cast<const ANSICHAR*>(String);
    }

    static FORCEINLINE ANSICHAR* AsAnsi(UTF8CHAR* String) noexcept
    {
        return reinterpret_cast<ANSICHAR*>(String);
    }

    static FORCEINLINE ANSICHAR** AsAnsi(UTF8CHAR** End) noexcept
    {
        return reinterpret_cast<ANSICHAR**>(End);
    }

    static FORCEINLINE UTF8CHAR* AsUtf8(ANSICHAR* String) noexcept
    {
        return reinterpret_cast<UTF8CHAR*>(String);
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
