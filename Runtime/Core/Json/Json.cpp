#include "Core/Json/Json.h"
#include "Core/Filesystem/File.h"
#include "Core/Platform/PlatformFile.h"

bool Json::Parse(const StringView& Text, FJsonValue& OutValue, FJsonError* OutError, EJsonParseFlags Flags)
{
    FJsonReader Reader(Flags);
    return Reader.Parse(Text, OutValue, OutError);
}

bool Json::LoadFromFile(const String& Filename, FJsonValue& OutValue, FJsonError* OutError, EJsonParseFlags Flags)
{
    const auto ReportFailure = [OutError](const CHAR* Message)
    {
        if (OutError)
        {
            *OutError         = FJsonError();
            OutError->Message = Message;
        }

        return false;
    };

    TFileRef<IPlatformFile> SourceFile = FPlatformFile::OpenForRead(Filename);
    if (!SourceFile.IsValid())
    {
        return ReportFailure("the file could not be opened for reading");
    }

    TArray<CHAR> Text;
    if (!File::ReadTextFile(SourceFile.Get(), Text))
    {
        return ReportFailure("the file is empty or could not be read");
    }

    const int32 TextLength = CString::Strlen(Text.Data());
    return Json::Parse(StringView(Text.Data(), TextLength), OutValue, OutError, Flags);
}

bool Json::SaveToFile(const String& Filename, const FJsonValue& Value, EJsonWriteFlags Flags)
{
    const String Text = Json::ToString(Value, Flags);

    const String Directory = File::ExtractFilepath(Filename);
    if (!Directory.IsEmpty() && !File::CreateDirectoryTree(Directory))
    {
        return false;
    }

    const String TempFilename = Filename + ".tmp";

    {
        TFileRef<IPlatformFile> TempFile = FPlatformFile::OpenForWrite(TempFilename);
        if (!TempFile.IsValid())
        {
            return false;
        }

        if (!File::WriteTextFile(TempFile.Get(), Text))
        {
            TempFile.Close();
            FPlatformFile::DeleteFile(TempFilename.Data());
            return false;
        }
    }

    if (!FPlatformFile::MoveFile(TempFilename.Data(), Filename.Data()))
    {
        FPlatformFile::DeleteFile(TempFilename.Data());
        return false;
    }

    return true;
}

String Json::ToString(const FJsonValue& Value, EJsonWriteFlags Flags)
{
    FJsonWriter Writer(Flags);
    return Writer.ToString(Value);
}
