#include "Core/Json/Json.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Memory/Memory.h"
#include "ShaderCompiler/ShaderCompilerIdentity.h"
#include "ShaderCompiler/Remote/RemoteShaderCompilerProtocol.h"

static bool IsKnownMessageType(uint32 Type)
{
    return Type >= static_cast<uint32>(RemoteShaderCompilerProtocol::EMessageType::Hello) && Type <= static_cast<uint32>(RemoteShaderCompilerProtocol::EMessageType::Pong);
}

void RemoteShaderCompilerProtocol::EncodeFrame(const FFrame& Frame, TArray<uint8>& OutBytes)
{
    const FString                             HeaderJson = Frame.Header.IsNull() ? FString() : Json::ToString(Frame.Header, EJsonWriteFlags::Compact);
    const TStringConversion<ANSICHAR, CHAR_T> HeaderText = StringCast<ANSICHAR>(HeaderJson);

    FFrameHeader FrameHeader;
    FrameHeader.Type        = Frame.Type;
    FrameHeader.RequestId   = Frame.RequestId;
    FrameHeader.HeaderSize  = static_cast<uint32>(HeaderText.Length());
    FrameHeader.PayloadSize = static_cast<uint32>(Frame.Payload.Size());

    const int32 Offset = OutBytes.Size();
    OutBytes.AppendUninitialized(static_cast<int32>(sizeof(FFrameHeader) + FrameHeader.HeaderSize + FrameHeader.PayloadSize));

    uint8* Destination = OutBytes.Data() + Offset;
    Memory::Memcpy(Destination, &FrameHeader, sizeof(FFrameHeader));
    Destination += sizeof(FFrameHeader);

    if (FrameHeader.HeaderSize > 0)
    {
        Memory::Memcpy(Destination, HeaderText.Get(), FrameHeader.HeaderSize);
        Destination += FrameHeader.HeaderSize;
    }

    if (FrameHeader.PayloadSize > 0)
    {
        Memory::Memcpy(Destination, Frame.Payload.Data(), FrameHeader.PayloadSize);
    }
}

bool RemoteShaderCompilerProtocol::TryDecodeFrame(TArray<uint8>& InOutBuffer, FFrame& OutFrame, bool& bOutHasFrame)
{
    bOutHasFrame = false;

    constexpr int32 FrameHeaderSize = static_cast<int32>(sizeof(FFrameHeader));
    if (InOutBuffer.Size() < FrameHeaderSize)
    {
        return true;
    }

    FFrameHeader FrameHeader;
    Memory::Memcpy(&FrameHeader, InOutBuffer.Data(), sizeof(FFrameHeader));

    if (FrameHeader.Magic != Magic || !IsKnownMessageType(static_cast<uint32>(FrameHeader.Type)) || FrameHeader.HeaderSize > MaxHeaderSize || FrameHeader.PayloadSize > MaxPayloadSize)
    {
        return false;
    }

    const int64 FrameSize = static_cast<int64>(FrameHeaderSize) + FrameHeader.HeaderSize + FrameHeader.PayloadSize;
    if (static_cast<int64>(InOutBuffer.Size()) < FrameSize)
    {
        return true;
    }

    FFrame Frame;
    Frame.Type      = FrameHeader.Type;
    Frame.RequestId = FrameHeader.RequestId;

    const ANSICHAR* HeaderBytes = reinterpret_cast<const ANSICHAR*>(InOutBuffer.Data() + FrameHeaderSize);
    if (FrameHeader.HeaderSize > 0)
    {
        const TStringConversion<CHAR_T, ANSICHAR> HeaderText = StringCast<CHAR_T>(HeaderBytes, static_cast<int32>(FrameHeader.HeaderSize));
        if (!Json::Parse(FStringView(HeaderText.Get(), HeaderText.Length()), Frame.Header) || !Frame.Header.IsObject())
        {
            return false;
        }
    }
    else
    {
        Frame.Header = FJsonValue::MakeObject();
    }

    if (FrameHeader.PayloadSize > 0)
    {
        Frame.Payload.Append(InOutBuffer.Data() + FrameHeaderSize + FrameHeader.HeaderSize, static_cast<int32>(FrameHeader.PayloadSize));
    }

    InOutBuffer.RemoveAt(0, static_cast<int32>(FrameSize));

    OutFrame     = ::Move(Frame);
    bOutHasFrame = true;
    return true;
}

bool RemoteShaderCompilerProtocol::IsValidSourcePath(const FString& Path)
{
    if (Path.IsEmpty() || Path.Contains('\\') || Path.Contains(':') || Path[0] == '/')
    {
        return false;
    }

    int32 SegmentStart = 0;
    for (int32 Index = 0; Index <= Path.Length(); ++Index)
    {
        if (Index < Path.Length() && Path[Index] != '/')
        {
            continue;
        }

        const int32 SegmentLength = Index - SegmentStart;
        if (SegmentLength == 0)
        {
            return false;
        }

        if ((SegmentLength == 1 && Path[SegmentStart] == '.') || (SegmentLength == 2 && Path[SegmentStart] == '.' && Path[SegmentStart + 1] == '.'))
        {
            return false;
        }

        SegmentStart = Index + 1;
    }

    return true;
}

FString RemoteShaderCompilerProtocol::CollapsePath(const FString& Path)
{
    TArray<FString> Segments;

    int32 SegmentStart = 0;
    for (int32 Index = 0; Index <= Path.Length(); ++Index)
    {
        if (Index < Path.Length() && Path[Index] != '/' && Path[Index] != '\\')
        {
            continue;
        }

        const FString Segment = Path.SubString(SegmentStart, Index - SegmentStart);
        SegmentStart = Index + 1;

        if (Segment == TEXT("."))
        {
            continue;
        }

        if (Segment == TEXT("..") && !Segments.IsEmpty() && Segments.Last() != TEXT("..") && !Segments.Last().IsEmpty())
        {
            Segments.RemoveAt(Segments.Size() - 1);
            continue;
        }

        Segments.Add(Segment);
    }

    FString Result;
    for (int32 Index = 0; Index < Segments.Size(); ++Index)
    {
        if (Index > 0)
        {
            Result += '/';
        }

        Result += Segments[Index];
    }

    return Result;
}

FJsonValue RemoteShaderCompilerProtocol::IdentityToJson(const FShaderCompilerIdentity& Identity)
{
    FJsonValue Value = FJsonValue::MakeObject();
    Value.AddMember(TEXT("name"), FJsonValue(Identity.Name));
    Value.AddMember(TEXT("major"), FJsonValue(Identity.VersionMajor));
    Value.AddMember(TEXT("minor"), FJsonValue(Identity.VersionMinor));
    Value.AddMember(TEXT("settings"), FJsonValue(Identity.SettingsVersion));
    Value.AddMember(TEXT("translator"), FJsonValue(Identity.TranslatorVersion));
    return Value;
}

bool RemoteShaderCompilerProtocol::IdentityFromJson(const FJsonValue& Value, FShaderCompilerIdentity& OutIdentity)
{
    if (!Value.IsObject())
    {
        return false;
    }

    const FJsonValue* NameValue = Value.Find(TEXT("name"));
    if (!NameValue || !NameValue->TryGetString(OutIdentity.Name) || OutIdentity.Name.IsEmpty())
    {
        return false;
    }

    const FJsonValue* MajorValue      = Value.Find(TEXT("major"));
    const FJsonValue* MinorValue      = Value.Find(TEXT("minor"));
    const FJsonValue* SettingsValue   = Value.Find(TEXT("settings"));
    const FJsonValue* TranslatorValue = Value.Find(TEXT("translator"));

    OutIdentity.VersionMajor      = MajorValue      ? static_cast<uint32>(MajorValue->GetInt64Or(0))      : 0;
    OutIdentity.VersionMinor      = MinorValue      ? static_cast<uint32>(MinorValue->GetInt64Or(0))      : 0;
    OutIdentity.SettingsVersion   = SettingsValue   ? static_cast<uint32>(SettingsValue->GetInt64Or(0))   : 0;
    OutIdentity.TranslatorVersion = TranslatorValue ? static_cast<uint32>(TranslatorValue->GetInt64Or(0)) : 0;
    return true;
}

const CHAR_T* RemoteShaderCompilerProtocol::GetPlatformName()
{
#if PLATFORM_WINDOWS
    return TEXT("Windows");
#elif PLATFORM_MACOS
    return TEXT("macOS");
#else
    return TEXT("Unknown");
#endif
}
