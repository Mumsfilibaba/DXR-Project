#include "Core/Json/Json.h"
#include "Core/Memory/Memory.h"
#include "ShaderCompiler/ShaderCompilerIdentity.h"
#include "ShaderCompiler/Remote/RemoteShaderCompilerProtocol.h"

static bool IsKnownMessageType(uint32 Type)
{
    return Type >= static_cast<uint32>(RemoteShaderCompilerProtocol::EMessageType::Hello) && Type <= static_cast<uint32>(RemoteShaderCompilerProtocol::EMessageType::Pong);
}

void RemoteShaderCompilerProtocol::EncodeFrame(const FFrame& Frame, TArray<uint8>& OutBytes)
{
    const String HeaderText = Frame.Header.IsNull() ? String() : Json::ToString(Frame.Header, EJsonWriteFlags::Compact);

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
        Memory::Memcpy(Destination, HeaderText.Data(), FrameHeader.HeaderSize);
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

    const CHAR* HeaderText = reinterpret_cast<const CHAR*>(InOutBuffer.Data() + FrameHeaderSize);
    if (FrameHeader.HeaderSize > 0)
    {
        if (!Json::Parse(StringView(HeaderText, static_cast<int32>(FrameHeader.HeaderSize)), Frame.Header) || !Frame.Header.IsObject())
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

bool RemoteShaderCompilerProtocol::IsValidSourcePath(const String& Path)
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

String RemoteShaderCompilerProtocol::CollapsePath(const String& Path)
{
    TArray<String> Segments;

    int32 SegmentStart = 0;
    for (int32 Index = 0; Index <= Path.Length(); ++Index)
    {
        if (Index < Path.Length() && Path[Index] != '/' && Path[Index] != '\\')
        {
            continue;
        }

        const String Segment = Path.SubString(SegmentStart, Index - SegmentStart);
        SegmentStart = Index + 1;

        if (Segment == ".")
        {
            continue;
        }

        if (Segment == ".." && !Segments.IsEmpty() && Segments.Last() != ".." && !Segments.Last().IsEmpty())
        {
            Segments.RemoveAt(Segments.Size() - 1);
            continue;
        }

        Segments.Add(Segment);
    }

    String Result;
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
    Value.AddMember("name", FJsonValue(Identity.Name));
    Value.AddMember("major", FJsonValue(Identity.VersionMajor));
    Value.AddMember("minor", FJsonValue(Identity.VersionMinor));
    Value.AddMember("settings", FJsonValue(Identity.SettingsVersion));
    Value.AddMember("translator", FJsonValue(Identity.TranslatorVersion));
    return Value;
}

bool RemoteShaderCompilerProtocol::IdentityFromJson(const FJsonValue& Value, FShaderCompilerIdentity& OutIdentity)
{
    if (!Value.IsObject())
    {
        return false;
    }

    const FJsonValue* NameValue = Value.Find("name");
    if (!NameValue || !NameValue->TryGetString(OutIdentity.Name) || OutIdentity.Name.IsEmpty())
    {
        return false;
    }

    const FJsonValue* MajorValue      = Value.Find("major");
    const FJsonValue* MinorValue      = Value.Find("minor");
    const FJsonValue* SettingsValue   = Value.Find("settings");
    const FJsonValue* TranslatorValue = Value.Find("translator");

    OutIdentity.VersionMajor      = MajorValue      ? static_cast<uint32>(MajorValue->GetInt64Or(0))      : 0;
    OutIdentity.VersionMinor      = MinorValue      ? static_cast<uint32>(MinorValue->GetInt64Or(0))      : 0;
    OutIdentity.SettingsVersion   = SettingsValue   ? static_cast<uint32>(SettingsValue->GetInt64Or(0))   : 0;
    OutIdentity.TranslatorVersion = TranslatorValue ? static_cast<uint32>(TranslatorValue->GetInt64Or(0)) : 0;
    return true;
}

const CHAR* RemoteShaderCompilerProtocol::GetPlatformName()
{
#if PLATFORM_WINDOWS
    return "Windows";
#elif PLATFORM_MACOS
    return "macOS";
#else
    return "Unknown";
#endif
}
