#include "Core/Misc/IniFile.h"
#include "Core/Misc/Parse.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Templates/CString.h"

struct FIniParseContext
{
    /** Directory the file being parsed lives in, empty for text that came from nowhere */
    FString BaseDirectory;

    /** The files currently open, so a file that includes itself is caught before it recurses */
    TArray<FString> IncludeStack;
};

constexpr int32 INI_MAX_INCLUDE_DEPTH = 8;

static void ParseIniText(FIniFile& OutFile, TArray<CHAR_T>& InText, FIniParseContext& Context);

static bool ReadIniFile(const FString& Filename, TArray<CHAR_T>& OutText)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    if (!FileHandle)
    {
        return false;
    }

    return File::ReadTextFile(FileHandle.Get(), OutText);
}

static CHAR_T* ParseIncludeDirective(CHAR_T* Line)
{
    constexpr SIZE_T DirectiveLength = 7; // 'include'
    if (CString::Strnicmp(Line, TEXT("include"), DirectiveLength) != 0)
    {
        return nullptr;
    }

    // The keyword has to be a word of its own, so 'includes' is still an ordinary key.
    CHAR_T* PathStart = Line + DirectiveLength;
    if (*PathStart != ' ' && *PathStart != '\"')
    {
        return nullptr;
    }

    Parse::ParseWhiteSpace(&PathStart);

    // A quoted path keeps its spaces, an unquoted one ends at the first space.
    if (*PathStart == '\"')
    {
        ++PathStart;
        if (CHAR_T* ClosingQuote = CString::Strchr(PathStart, '\"'))
        {
            *ClosingQuote = '\0';
        }
    }
    else if (CHAR_T* TrailingSpace = CString::Strchr(PathStart, ' '))
    {
        *TrailingSpace = '\0';
    }

    return (*PathStart != '\0') ? PathStart : nullptr;
}

static void ParseIniInclude(FIniFile& OutFile, const CHAR_T* IncludePath, FIniParseContext& Context)
{
    const FString RelativePath(IncludePath);

    const bool bIsAbsolute = (RelativePath.Length() > 0 && RelativePath[0] == '/')
        || (RelativePath.Length() > 1 && RelativePath[1] == ':');

    const FString ResolvedPath = (bIsAbsolute || Context.BaseDirectory.IsEmpty())
        ? RelativePath : FString::Printf(TEXT("%s/%s"), *Context.BaseDirectory, *RelativePath);

    if (Context.IncludeStack.Contains(ResolvedPath))
    {
        LOG_ERROR("Ini file '%s' includes itself, the include is ignored", *ResolvedPath);
        return;
    }

    if (Context.IncludeStack.Size() >= INI_MAX_INCLUDE_DEPTH)
    {
        LOG_ERROR("Ini includes are nested deeper than %d files at '%s'", INI_MAX_INCLUDE_DEPTH, *ResolvedPath);
        return;
    }

    TArray<CHAR_T> FileContents;
    if (!ReadIniFile(ResolvedPath, FileContents))
    {
        LOG_WARNING("Failed to open included ini file '%s'", *ResolvedPath);
        return;
    }

    const FString PreviousDirectory = Context.BaseDirectory;
    Context.BaseDirectory = File::GetDirectoryOf(ResolvedPath);
    Context.IncludeStack.Add(ResolvedPath);

    ParseIniText(OutFile, FileContents, Context);

    Context.IncludeStack.Pop();
    Context.BaseDirectory = PreviousDirectory;
}

static void ParseIniText(FIniFile& OutFile, TArray<CHAR_T>& InText, FIniParseContext& Context)
{
    InText.Remove('\r');

    FIniSection* CurrentSection = nullptr;

    CHAR_T* Start = InText.Data();
    while (Start && *Start)
    {
        // Skip newline chars
        while (*Start == '\n')
        {
            ++Start;
        }

        CHAR_T* LineStart = Start;
        Parse::ParseLine(&Start);

        // End string at the end of line
        if (*Start == '\n')
        {
            *(Start++) = '\0';
        }

        // Skip any spaces at the beginning of the line
        Parse::ParseWhiteSpace(&LineStart);

        // This is a section
        if (*LineStart == '[')
        {
            if (CHAR_T* SectionEnd = CString::Strchr(++LineStart, ']'))
            {
                CHAR_T* SectionStart = LineStart;
                *SectionEnd = '\0';
                
                FIniSection& Section = OutFile.FindOrAddSection(SectionStart);
                CurrentSection = &Section;
            }
        }
        else if (*LineStart != ';') // Check if this is a comment line
        {
            if (CHAR_T* EqualSign = CString::Strchr(LineStart, '='))
            {
                *EqualSign = '\0';

                CHAR_T* KeyEnd = EqualSign - 1;
                while (*KeyEnd == ' ')
                {
                    *(KeyEnd--) = '\0';
                }

                // The parsed key
                CHAR_T* Key = LineStart;
                LineStart = EqualSign + 1;

                Parse::ParseWhiteSpace(&LineStart);

                // Find the end of the value, the line is already null-terminated so the
                // value ends at the line terminator unless something closes it earlier.
                CHAR_T* Value = LineStart;

                // Special case for string-values, these end at the closing quote and keep inner spaces.
                if (*Value == '\"')
                {
                    Value++;

                    // An unterminated quote simply runs to the end of the line
                    if (CHAR_T* ClosingQuote = CString::Strchr(Value, '\"'))
                    {
                        *ClosingQuote = '\0';
                    }
                }
                else if (CHAR_T* TrailingSpace = CString::Strchr(Value, ' '))
                {
                    // Unquoted values end at the first space
                    *TrailingSpace = '\0';
                }

                // If there are no section, use the global one
                if (!CurrentSection)
                {
                    FIniSection& Section = OutFile.FindOrAddSection(TEXT(""));
                    CurrentSection = &Section;
                }

                // The parsed value
                if (FIniValue* CurrentValue = CurrentSection->Values.Find(Key))
                {
                    *CurrentValue = FIniValue(Value);
                }
                else
                {
                    CurrentSection->Values.Add(Key, FIniValue(Value));
                }
            }
            else if (CHAR_T* IncludePath = ParseIncludeDirective(LineStart))
            {
                // The included file parses into this same file, starting at the global section,
                // so it can neither see nor change the section this line sits in.
                ParseIniInclude(OutFile, IncludePath, Context);
            }
        }
    }
}

FIniFile::FIniFile()
    : Filename()
    , Sections()
{
}

FIniFile::~FIniFile() = default;

FIniSection::FIniSection()
    : Name()
    , Values()
{
}

FIniSection::FIniSection(const CHAR_T* InName)
    : Name(InName)
    , Values()
{
}

FIniSection::~FIniSection() = default;

void FIniSection::Restore()
{
    for (TPair<const FString&, FIniValue&> Value : Values)
    {
        Value.Second.Restore();
    }
}

void FIniSection::DumpToString(FString& OutString)
{
    for (TPair<const FString&, FIniValue&> ValuePair : Values)
    {
        // Values containing spaces have to be quoted, the parser otherwise stops at the first space
        const FString& Value = ValuePair.Second.CurrentValue;
        if (Value.Contains(' '))
        {
            OutString.AppendPrintf(TEXT("%s=\"%s\"\n"), *ValuePair.First, *Value);
        }
        else
        {
            OutString.AppendPrintf(TEXT("%s=%s\n"), *ValuePair.First, *Value);
        }
    }
}

bool FIniFile::SetString(const CHAR_T* SectionName, const CHAR_T* Name, const FString& NewValue)
{
    if (FIniValue* Value = FindValue(SectionName, Name))
    {
        Value->CurrentValue = NewValue;
        return true;
    }
    else
    {
        LOG_ERROR("Failed to set config value '%s' in section '%s'", Name, SectionName);
        return false;
    }
}

bool FIniFile::SetInt(const CHAR_T* SectionName, const CHAR_T* Name, int32 NewValue)
{
    return SetString(SectionName, Name, TTypeToString<int32>::ToString(NewValue));
}

bool FIniFile::SetFloat(const CHAR_T* SectionName, const CHAR_T* Name, float NewValue)
{
    return SetString(SectionName, Name, TTypeToString<float>::ToString(NewValue));
}

bool FIniFile::SetBool(const CHAR_T* SectionName, const CHAR_T* Name, bool bNewValue)
{
    return SetString(SectionName, Name, TTypeToString<bool>::ToString(bNewValue));
}

FIniSection& FIniFile::FindOrAddSection(const CHAR_T* SectionName)
{
    FIniSection& Section = Sections.FindOrAdd(SectionName);
    Section.Name = SectionName;
    return Section;
}

void FIniFile::SetOrAddString(const CHAR_T* SectionName, const CHAR_T* Name, const FString& NewValue)
{
    FIniSection& Section = FindOrAddSection(SectionName);
    if (FIniValue* Value = Section.Values.Find(Name))
    {
        Value->CurrentValue = NewValue;
    }
    else
    {
        Section.Values.Add(Name, FIniValue(NewValue));
    }
}

void FIniFile::SetOrAddInt(const CHAR_T* SectionName, const CHAR_T* Name, int32 NewValue)
{
    SetOrAddString(SectionName, Name, TTypeToString<int32>::ToString(NewValue));
}

void FIniFile::SetOrAddFloat(const CHAR_T* SectionName, const CHAR_T* Name, float NewValue)
{
    SetOrAddString(SectionName, Name, TTypeToString<float>::ToString(NewValue));
}

void FIniFile::SetOrAddBool(const CHAR_T* SectionName, const CHAR_T* Name, bool bNewValue)
{
    SetOrAddString(SectionName, Name, TTypeToString<bool>::ToString(bNewValue));
}

bool FIniFile::GetString(const CHAR_T* SectionName, const CHAR_T* Name, FString& OutValue)
{
    if (FIniValue* Value = FindValue(SectionName, Name))
    {
        OutValue = Value->CurrentValue;
        return true;
    }
    else
    {
        return false;
    }
}

bool FIniFile::GetInt(const CHAR_T* SectionName, const CHAR_T* Name, int32& OutValue)
{
    if (FIniValue* Value = FindValue(SectionName, Name))
    {
        return TTypeFromString<int32>::FromString(Value->CurrentValue, OutValue);
    }
    else
    {
        return false;
    }
}

bool FIniFile::GetFloat(const CHAR_T* SectionName, const CHAR_T* Name, float& OutValue)
{
    if (FIniValue* Value = FindValue(SectionName, Name))
    {
        return TTypeFromString<float>::FromString(Value->CurrentValue, OutValue);
    }
    else
    {
        return false;
    }
}

bool FIniFile::GetBool(const CHAR_T* SectionName, const CHAR_T* Name, bool& bOutValue)
{
    if (FIniValue* Value = FindValue(SectionName, Name))
    {
        return TTypeFromString<bool>::FromString(Value->CurrentValue, bOutValue);
    }
    else
    {
        return false;
    }
}

// NOTE: Why this warning is a thing is a mystery to me
DISABLE_UNREACHABLE_CODE_WARNING

FIniValue* FIniFile::FindValue(const CHAR_T* Key)
{
    for (TPair<const FString&, FIniSection&> CurrentSection : Sections)
    {
        if (FIniValue* Value = CurrentSection.Second.Values.Find(Key))
        {
            return Value;
        }
    }
    
    return nullptr;
}

ENABLE_UNREACHABLE_CODE_WARNING

FIniValue* FIniFile::FindValue(const CHAR_T* SectionName, const CHAR_T* Name)
{
    const bool bIsGlobal = !SectionName || CString::Strcmp(SectionName, TEXT("")) == 0;
    if (bIsGlobal)
    {
        return FindValue(Name);
    }
    else if (FIniSection* Section = Sections.Find(SectionName))
    {
        return Section->Values.Find(Name);
    }

    return nullptr;
}

bool FIniFile::WriteToFile()
{
    FString ConfigString;
    DumpToString(ConfigString);

    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    if (!FileHandle)
    {
        return false;
    }

    return File::WriteTextFile(FileHandle.Get(), ConfigString);
}

void FIniFile::DumpToString(FString& OutString)
{
    // The global section has no header of its own, so it has to be written before any '[Section]'
    // line, otherwise it would be read back as part of whichever section precedes it.
    if (FIniSection* GlobalSection = Sections.Find(TEXT("")))
    {
        GlobalSection->DumpToString(OutString);
        OutString += '\n';
    }

    for (TPair<const FString&, FIniSection&> CurrentSection : Sections)
    {
        if (CurrentSection.First.IsEmpty())
        {
            continue;
        }

        OutString.AppendPrintf(TEXT("[%s]\n"), *CurrentSection.First);
        CurrentSection.Second.DumpToString(OutString);
        OutString += '\n';
    }
}

bool FIniFile::LoadFromFile(const FString& InFilename)
{
    TArray<CHAR_T> FileContents;
    if (!ReadIniFile(InFilename, FileContents))
    {
        return false;
    }

    Filename = InFilename;

    FIniParseContext Context;
    Context.BaseDirectory = File::GetDirectoryOf(InFilename);
    Context.IncludeStack.Add(InFilename);

    ParseIniText(*this, FileContents, Context);
    return true;
}

void FIniFile::ParseFromText(TArray<CHAR_T>& InText)
{
    // Text with no file behind it resolves its includes against the working directory
    FIniParseContext Context;
    ParseIniText(*this, InText, Context);
}
