#include "Core/Misc/IniFile.h"
#include "Core/Misc/Parse.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Templates/CString.h"

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
        const String& Value = ValuePair.Second.CurrentValue;

        // Values containing spaces have to be quoted, the parser otherwise stops at the first space
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
    // The global section has no header of its own, so it has to be written before any
    // '[Section]' line, otherwise it would be read back as part of whichever section precedes it
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

    {
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(InFilename);
        if (!FileHandle)
        {
            return false;
        }

        // Read the full file
        if (!File::ReadTextFile(FileHandle.Get(), FileContents))
        {
            return false;
        }
    }

    Filename = InFilename;
    ParseFromText(FileContents);
    return true;
}

void FIniFile::ParseFromText(TArray<CHAR>& InText)
{
    // Remove all carriage returns if there are any (Easier to process)
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
                
                FIniSection& Section = Sections.FindOrAdd(SectionStart, FIniSection(SectionStart));
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
                // value ends at the line terminator unless something closes it earlier
                CHAR* Value = LineStart;

                // Special case for string-values, these end at the closing quote and keep inner spaces
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
                    FIniSection& Section = Sections.FindOrAdd("");
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
        }
    }
}
