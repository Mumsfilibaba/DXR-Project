#pragma once
#include "Core/Json/JsonTypes.h"
#include "Core/Json/JsonValue.h"
#include "Core/Json/JsonReader.h"
#include "Core/Json/JsonWriter.h"

struct CORE_API Json
{
    static bool Parse(const FStringView& Text, FJsonValue& OutValue, FJsonError* OutError = nullptr, EJsonParseFlags Flags = EJsonParseFlags::None);
    static bool LoadFromFile(const FString& Filename, FJsonValue& OutValue, FJsonError* OutError = nullptr, EJsonParseFlags Flags = EJsonParseFlags::None);
    static bool SaveToFile(const FString& Filename, const FJsonValue& Value, EJsonWriteFlags Flags = EJsonWriteFlags::Pretty);

    NODISCARD static FString ToString(const FJsonValue& Value, EJsonWriteFlags Flags = EJsonWriteFlags::Pretty);
};
