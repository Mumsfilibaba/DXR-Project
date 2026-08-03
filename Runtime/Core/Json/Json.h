#pragma once
#include "Core/Json/JsonTypes.h"
#include "Core/Json/JsonValue.h"
#include "Core/Json/JsonReader.h"
#include "Core/Json/JsonWriter.h"

struct CORE_API Json
{
    static bool Parse(const StringView& Text, FJsonValue& OutValue, FJsonError* OutError = nullptr, EJsonParseFlags Flags = EJsonParseFlags::None);
    static bool LoadFromFile(const String& Filename, FJsonValue& OutValue, FJsonError* OutError = nullptr, EJsonParseFlags Flags = EJsonParseFlags::None);
    static bool SaveToFile(const String& Filename, const FJsonValue& Value, EJsonWriteFlags Flags = EJsonWriteFlags::Pretty);

    NODISCARD static String ToString(const FJsonValue& Value, EJsonWriteFlags Flags = EJsonWriteFlags::Pretty);
};
