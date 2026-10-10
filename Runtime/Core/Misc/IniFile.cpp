#include "Core/Misc/IniFile.h"
#include "Core/Misc/Parse.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Templates/CString.h"

struct FIniParseContext
{
    /** Directory the file being parsed lives in, empty for text that came from nowhere */
    String BaseDirectory;

    /** The files currently open, so a file that includes itself is caught before it recurses */
    TArray<String> IncludeStack;
};

constexpr int32 INI_MAX_INCLUDE_DEPTH = 8;

static void ParseIniText(FIniFile& OutFile, TArray<CHAR>& InText, FIniParseContext& Context);

static bool ReadIniFile(const String& Filename, TArray<CHAR>& OutText)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    if (!FileHandle)
    {
        return false;
    }

    return File::ReadTextFile(FileHandle.Get(), OutText);
}

static CHAR* ParseIncludeDirective(CHAR* Line)
{
    constexpr SIZE_T DirectiveLength = 7; // 'include'
    if (CString::Strnicmp(Line, "include", DirectiveLength) != 0)
    {
        return nullptr;
    }

    // The keyword has to be a word of its own, so 'includes' is still an ordinary key.
    CHAR* PathStart = Line + DirectiveLength;
    if (*PathStart != ' ' && *PathStart != '\"')
    {
        return nullptr;
    }

    Parse::ParseWhiteSpace(&PathStart);

    // A quoted path keeps its spaces, an unquoted one ends at the first space.
    if (*PathStart == '\"')
    {
        ++PathStart;
        if (CHAR* ClosingQuote = CString::Strchr(PathStart, '\"'))
        {
            *ClosingQuote = '\0';
        }
    }
    else if (CHAR* TrailingSpace = CString::Strchr(PathStart, ' '))
    {
        *TrailingSpace = '\0';
    }

    return (*PathStart != '\0') ? PathStart : nullptr;
}

static void ParseIniInclude(FIniFile& OutFile, const CHAR* IncludePath, FIniParseContext& Context)
{
    const String RelativePath(IncludePath);

    const bool bIsAbsolute = (RelativePath.Length() > 0 && RelativePath[0] == '/')
        || (RelativePath.Length() > 1 && RelativePath[1] == ':');

    const String ResolvedPath = (bIsAbsolute || Context.BaseDirectory.IsEmpty())
        ? RelativePath : String::Printf("%s/%s", *Context.BaseDirectory, *RelativePath);

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

    TArray<CHAR> FileContents;
    if (!ReadIniFile(ResolvedPath, FileContents))
    {
        LOG_WARNING("Failed to open included ini file '%s'", *ResolvedPath);
        return;
    }

    const String PreviousDirectory = Context.BaseDirectory;
    Context.BaseDirectory = File::GetDirectoryOf(ResolvedPath);
    Context.IncludeStack.Add(ResolvedPath);

    ParseIniText(OutFile, FileContents, Context);

    Context.IncludeStack.Pop();
    Context.BaseDirectory = PreviousDirectory;
}

static void ParseIniText(FIniFile& OutFile, TArray<CHAR>& InText, FIniParseContext& Context)
{
    InText.Remove('\r');

    FIniSection* CurrentSection = nullptr;

    CHAR* Start = InText.Data();
    while (Start && *Start)
    {
        // Skip newline chars
        while (*Start == '\n')
        {
            ++Start;
        }

        CHAR* LineStart = Start;
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
            if (CHAR* SectionEnd = CString::Strchr(++LineStart, ']'))
            {
                CHAR* SectionStart = LineStart;
                *SectionEnd = '\0';
                
                FIniSection& Section = OutFile.FindOrAddSection(SectionStart);
                CurrentSection = &Section;
            }
        }
        else if (*LineStart != ';') // Check if this is a comment line
        {
            if (CHAR* EqualSign = CString::Strchr(LineStart, '='))
            {
                *EqualSign = '\0';

                CHAR* KeyEnd = EqualSign - 1;
                while (*KeyEnd == ' ')
                {
                    *(KeyEnd--) = '\0';
                }

                // The parsed key
                CHAR* Key = LineStart;
                LineStart = EqualSign + 1;

                Parse::ParseWhiteSpace(&LineStart);

                // Find the end of the value, the line is already null-terminated so the
                // value ends at the line terminator unless something closes it earlier.
                CHAR* Value = LineStart;

                // Special case for string-values, these end at the closing quote and keep inner spaces.
                if (*Value == '\"')
                {
                    Value++;

                    // An unterminated quote simply runs to the end of the line
                    if (CHAR* ClosingQuote = CString::Strchr(Value, '\"'))
                    {
                        *ClosingQuote = '\0';
                    }
                }
                else if (CHAR* TrailingSpace = CString::Strchr(Value, ' '))
                {
                    // Unquoted values end at the first space
                    *TrailingSpace = '\0';
                }

                // If there are no section, use the global one
                if (!CurrentSection)
                {
                    FIniSection& Section = OutFile.FindOrAddSection("");
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
            else if (CHAR* IncludePath = ParseIncludeDirective(LineStart))
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

FIniSection::FIniSection(const CHAR* InName)
    : Name(InName)
    , Values()
{
}

FIniSection::~FIniSection() = default;

void FIniSection::Restore()
{
    for (auto Value : Values)
    {
        Value.Second.Restore();
    }
}

void FIniSection::DumpToString(String& OutString)
{
    for (auto ValuePair : Values)
    {
        // Values containing spaces have to be quoted, the parser otherwise stops at the first space
        const String& Value = ValuePair.Second.CurrentValue;
        if (Value.Contains(' '))
        {
            OutString.AppendPrintf("%s=\"%s\"\n", *ValuePair.First, *Value);
        }
        else
        {
            OutString.AppendPrintf("%s=%s\n", *ValuePair.First, *Value);
        }
    }
}

bool FIniFile::SetString(const CHAR* SectionName, const CHAR* Name, const String& NewValue)
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

bool FIniFile::SetInt(const CHAR* SectionName, const CHAR* Name, int32 NewValue)
{
    return SetString(SectionName, Name, TTypeToString<int32>::ToString(NewValue));
}

bool FIniFile::SetFloat(const CHAR* SectionName, const CHAR* Name, float NewValue)
{
    return SetString(SectionName, Name, TTypeToString<float>::ToString(NewValue));
}

bool FIniFile::SetBool(const CHAR* SectionName, const CHAR* Name, bool bNewValue)
{
    return SetString(SectionName, Name, TTypeToString<bool>::ToString(bNewValue));
}

FIniSection& FIniFile::FindOrAddSection(const CHAR* SectionName)
{
    FIniSection& Section = Sections.FindOrAdd(SectionName);
    Section.Name = SectionName;
    return Section;
}

void FIniFile::SetOrAddString(const CHAR* SectionName, const CHAR* Name, const String& NewValue)
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

void FIniFile::SetOrAddInt(const CHAR* SectionName, const CHAR* Name, int32 NewValue)
{
    SetOrAddString(SectionName, Name, TTypeToString<int32>::ToString(NewValue));
}

void FIniFile::SetOrAddFloat(const CHAR* SectionName, const CHAR* Name, float NewValue)
{
    SetOrAddString(SectionName, Name, TTypeToString<float>::ToString(NewValue));
}

void FIniFile::SetOrAddBool(const CHAR* SectionName, const CHAR* Name, bool bNewValue)
{
    SetOrAddString(SectionName, Name, TTypeToString<bool>::ToString(bNewValue));
}

bool FIniFile::GetString(const CHAR* SectionName, const CHAR* Name, String& OutValue)
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

bool FIniFile::GetInt(const CHAR* SectionName, const CHAR* Name, int32& OutValue)
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

bool FIniFile::GetFloat(const CHAR* SectionName, const CHAR* Name, float& OutValue)
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

bool FIniFile::GetBool(const CHAR* SectionName, const CHAR* Name, bool& bOutValue)
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

FIniValue* FIniFile::FindValue(const CHAR* Key)
{
    for (auto CurrentSection : Sections)
    {
        if (FIniValue* Value = CurrentSection.Second.Values.Find(Key))
        {
            return Value;
        }
    }
    
    return nullptr;
}

ENABLE_UNREACHABLE_CODE_WARNING

FIniValue* FIniFile::FindValue(const CHAR* SectionName, const CHAR* Name)
{
    const bool bIsGlobal = !SectionName || CString::Strcmp(SectionName, "") == 0;
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
    String ConfigString;
    DumpToString(ConfigString);

    {
        TFileRef<IPlatformFile> File = FPlatformFile::OpenForWrite(Filename);
        if (!File)
        {
            return false;
        }

        File->Write((const uint8*)ConfigString.Data(), ConfigString.SizeInBytes());
    }

    return true;
}

void FIniFile::DumpToString(String& OutString)
{
    // The global section has no header of its own, so it has to be written before any '[Section]'
    // line, otherwise it would be read back as part of whichever section precedes it.
    if (FIniSection* GlobalSection = Sections.Find(""))
    {
        GlobalSection->DumpToString(OutString);
        OutString += '\n';
    }

    for (auto CurrentSection : Sections)
    {
        if (CurrentSection.First.IsEmpty())
        {
            continue;
        }

        OutString.AppendPrintf("[%s]\n", *CurrentSection.First);
        CurrentSection.Second.DumpToString(OutString);
        OutString += '\n';
    }
}

bool FIniFile::LoadFromFile(const String& InFilename)
{
    TArray<CHAR> FileContents;
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

void FIniFile::ParseFromText(TArray<CHAR>& InText)
{
    // Text with no file behind it resolves its includes against the working directory
    FIniParseContext Context;
    ParseIniText(*this, InText, Context);
}
