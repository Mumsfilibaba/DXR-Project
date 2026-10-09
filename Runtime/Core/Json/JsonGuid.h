#pragma once
#include "Core/Json/JsonSerializer.h"
#include "Core/Misc/Guid.h"

template<>
struct TJsonSerializer<FGuid>
{
    static void Save(FJsonValue& OutValue, const FGuid& InValue)
    {
        OutValue = FJsonValue(InValue.ToString());
    }

    static bool Load(const FJsonValue& InValue, FGuid& OutValue, FJsonArchive& Archive)
    {
        String Text;
        if (!InValue.TryGetString(Text))
        {
            Archive.AddTypeError(InValue, "a GUID string");
            return false;
        }

        if (!FGuid::Parse(Text, OutValue))
        {
            Archive.AddError("'%s' is not a GUID", *Text);
            return false;
        }

        return true;
    }
};
