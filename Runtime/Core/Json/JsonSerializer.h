#pragma once
#include "Core/Json/JsonArchive.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/Optional.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Templates/TypeTraits.h"

// Satisfied by a type that serializes itself in both directions through one function
template<typename T>
concept CJsonSerializable = requires(T& Value, FJsonArchive& Archive)
{
    Value.Serialize(Archive);
};

// Maps an enum onto the names it should read and write as. Unspecialized, an enum serializes as 
// its underlying integer. JSON_ENUM_NAMES_BEGIN gives it text instead, so a file says "Static" 
// rather than 1 and stays readable after the enum gains a value.
template<typename EnumType>
struct TJsonEnumNames
{
    static constexpr bool bHasNames = false;
};

#define JSON_ENUM_NAMES_BEGIN(EnumType)                                       \
    template<>                                                                \
    struct TJsonEnumNames<EnumType>                                           \
    {                                                                         \
        static constexpr bool bHasNames = true;                               \
                                                                              \
        struct FEntry                                                         \
        {                                                                     \
            EnumType    Value;                                                \
            const CHAR* Name;                                                 \
        };                                                                    \
                                                                              \
        static const FEntry* GetEntries(int32& OutNumEntries)                 \
        {                                                                     \
            static const FEntry Entries[] = {

#define JSON_ENUM_NAME(EnumType, Enumerator) { EnumType::Enumerator, #Enumerator },

#define JSON_ENUM_NAMES_END()                                                         \
            };                                                                        \
                                                                                      \
            OutNumEntries = static_cast<int32>(sizeof(Entries) / sizeof(Entries[0])); \
            return Entries;                                                           \
        }                                                                             \
    };

// The customization point that teaches the archive about a type. The primary template covers enums
// and any type with its own Serialize function. Everything else needs a specialization, and asking
// for one that does not exist is a compile error rather than a silently skipped field.
template<typename T>
struct TJsonSerializer
{
    static void Save(FJsonValue& OutValue, const T& InValue)
    {
        if constexpr (TIsEnum<T>::Value)
        {
            if constexpr (TJsonEnumNames<T>::bHasNames)
            {
                int32 NumEntries = 0;

                const auto* Entries = TJsonEnumNames<T>::GetEntries(NumEntries);
                for (int32 Index = 0; Index < NumEntries; ++Index)
                {
                    if (Entries[Index].Value == InValue)
                    {
                        OutValue = FJsonValue(Entries[Index].Name);
                        return;
                    }
                }

                // An enumerator missing from the table still has to survive the round trip
                OutValue = FJsonValue(static_cast<int64>(UnderlyingTypeValue(InValue)));
            }
            else
            {
                OutValue = FJsonValue(static_cast<int64>(UnderlyingTypeValue(InValue)));
            }
        }
        else if constexpr (CJsonSerializable<T>)
        {
            FJsonArchive Nested = FJsonArchive::Saver(OutValue);
            const_cast<T&>(InValue).Serialize(Nested);
        }
        else
        {
            static_assert(sizeof(T) == 0, "This type needs either a Serialize(FJsonArchive&) function or a TJsonSerializer specialization");
        }
    }

    static bool Load(const FJsonValue& InValue, T& OutValue, FJsonArchive& Archive)
    {
        if constexpr (TIsEnum<T>::Value)
        {
            typedef typename TUnderlyingType<T>::Type UnderlyingType;

            if constexpr (TJsonEnumNames<T>::bHasNames)
            {
                String Name;
                if (InValue.TryGetString(Name))
                {
                    int32 NumEntries = 0;

                    const auto* Entries = TJsonEnumNames<T>::GetEntries(NumEntries);
                    for (int32 Index = 0; Index < NumEntries; ++Index)
                    {
                        if (Name.Equals(Entries[Index].Name))
                        {
                            OutValue = Entries[Index].Value;
                            return true;
                        }
                    }

                    Archive.AddError("'%s' is not one of the values this field accepts", Name.Data());
                    return false;
                }
            }

            int64 Raw = 0;
            if (!InValue.TryGetInt64(Raw))
            {
                Archive.AddTypeError(InValue, TJsonEnumNames<T>::bHasNames ? "a string" : "a number");
                return false;
            }

            OutValue = static_cast<T>(static_cast<UnderlyingType>(Raw));
            return true;
        }
        else if constexpr (CJsonSerializable<T>)
        {
            if (!InValue.IsObject())
            {
                Archive.AddTypeError(InValue, "an object");
                return false;
            }

            FJsonArchive Nested = FJsonArchive::Loader(InValue);
            OutValue.Serialize(Nested);
            Archive.AppendErrors(Nested);

            // Whatever did load is kept. Reporting failure here would throw away every good field
            // in the object because one of them was mistyped.
            return true;
        }
        else
        {
            static_assert(sizeof(T) == 0, "This type needs either a Serialize(FJsonArchive&) function or a TJsonSerializer specialization");
            return false;
        }
    }
};


template<>
struct TJsonSerializer<bool>
{
    static void Save(FJsonValue& OutValue, const bool& InValue)
    {
        OutValue = FJsonValue(InValue);
    }

    static bool Load(const FJsonValue& InValue, bool& OutValue, FJsonArchive& Archive)
    {
        if (!InValue.TryGetBool(OutValue))
        {
            Archive.AddTypeError(InValue, "a bool");
            return false;
        }

        return true;
    }
};

template<typename T>
struct TJsonIntegerSerializer
{
    static void Save(FJsonValue& OutValue, const T& InValue)
    {
        OutValue = FJsonValue(static_cast<int64>(InValue));
    }

    static bool Load(const FJsonValue& InValue, T& OutValue, FJsonArchive& Archive)
    {
        int64 Raw = 0;
        if (!InValue.TryGetInt64(Raw))
        {
            Archive.AddTypeError(InValue, "a number");
            return false;
        }

        if constexpr (TIsSame<T, uint64>::Value)
        {
            if (Raw < 0)
            {
                Archive.AddError("%lld is negative and this field is unsigned", Raw);
                return false;
            }
        }
        else if constexpr (!TIsSame<T, int64>::Value)
        {
            if ((Raw < static_cast<int64>(TNumericLimits<T>::Min())) || (Raw > static_cast<int64>(TNumericLimits<T>::Max())))
            {
                Archive.AddError("%lld does not fit in this field", Raw);
                return false;
            }
        }

        OutValue = static_cast<T>(Raw);
        return true;
    }
};

template<> struct TJsonSerializer<int8>   : public TJsonIntegerSerializer<int8>   { };
template<> struct TJsonSerializer<int16>  : public TJsonIntegerSerializer<int16>  { };
template<> struct TJsonSerializer<int32>  : public TJsonIntegerSerializer<int32>  { };
template<> struct TJsonSerializer<int64>  : public TJsonIntegerSerializer<int64>  { };
template<> struct TJsonSerializer<uint8>  : public TJsonIntegerSerializer<uint8>  { };
template<> struct TJsonSerializer<uint16> : public TJsonIntegerSerializer<uint16> { };
template<> struct TJsonSerializer<uint32> : public TJsonIntegerSerializer<uint32> { };
template<> struct TJsonSerializer<uint64> : public TJsonIntegerSerializer<uint64> { };

template<typename T>
struct TJsonRealSerializer
{
    static void Save(FJsonValue& OutValue, const T& InValue)
    {
        OutValue = FJsonValue(InValue);
    }

    static bool Load(const FJsonValue& InValue, T& OutValue, FJsonArchive& Archive)
    {
        double Raw = 0.0;
        if (!InValue.TryGetDouble(Raw))
        {
            Archive.AddTypeError(InValue, "a number");
            return false;
        }

        OutValue = static_cast<T>(Raw);
        return true;
    }
};

template<> struct TJsonSerializer<float>  : public TJsonRealSerializer<float>  { };
template<> struct TJsonSerializer<double> : public TJsonRealSerializer<double> { };

template<>
struct TJsonSerializer<String>
{
    static void Save(FJsonValue& OutValue, const String& InValue)
    {
        OutValue = FJsonValue(InValue);
    }

    static bool Load(const FJsonValue& InValue, String& OutValue, FJsonArchive& Archive)
    {
        if (!InValue.TryGetString(OutValue))
        {
            Archive.AddTypeError(InValue, "a string");
            return false;
        }

        return true;
    }
};

template<>
struct TJsonSerializer<StringView>
{
    static void Save(FJsonValue& OutValue, const StringView& InValue)
    {
        OutValue = FJsonValue(InValue);
    }

    static bool Load(const FJsonValue&, StringView&, FJsonArchive& Archive)
    {
        // A view owns nothing, so there is nowhere to put the characters
        Archive.AddError("a string view can be written but not read back into");
        return false;
    }
};

template<>
struct TJsonSerializer<const CHAR*>
{
    static void Save(FJsonValue& OutValue, const CHAR* const& InValue)
    {
        OutValue = FJsonValue(InValue);
    }

    static bool Load(const FJsonValue&, const CHAR*&, FJsonArchive& Archive)
    {
        Archive.AddError("a raw string pointer can be written but not read back into");
        return false;
    }
};

template<typename T, typename AllocatorType>
struct TJsonSerializer<TArray<T, AllocatorType>>
{
    typedef TArray<T, AllocatorType> ArrayType;

    static void Save(FJsonValue& OutValue, const ArrayType& InValue)
    {
        OutValue = FJsonValue::MakeArray();
        for (int32 Index = 0; Index < InValue.Size(); ++Index)
        {
            FJsonValue Element;
            TJsonSerializer<T>::Save(Element, InValue[Index]);
            OutValue.Add(Move(Element));
        }
    }

    static bool Load(const FJsonValue& InValue, ArrayType& OutValue, FJsonArchive& Archive)
    {
        if (!InValue.IsArray())
        {
            Archive.AddTypeError(InValue, "an array");
            return false;
        }

        OutValue.Reset(InValue.Num());
        for (int32 Index = 0; Index < InValue.Num(); ++Index)
        {
            TJsonSerializer<T>::Load(InValue[Index], OutValue[Index], Archive);
        }

        return true;
    }
};

template<typename T, int32 NUM_ELEMENTS>
struct TJsonSerializer<TStaticArray<T, NUM_ELEMENTS>>
{
    typedef TStaticArray<T, NUM_ELEMENTS> ArrayType;

    static void Save(FJsonValue& OutValue, const ArrayType& InValue)
    {
        OutValue = FJsonValue::MakeArray();
        for (int32 Index = 0; Index < NUM_ELEMENTS; ++Index)
        {
            FJsonValue Element;
            TJsonSerializer<T>::Save(Element, InValue[Index]);
            OutValue.Add(Move(Element));
        }
    }

    static bool Load(const FJsonValue& InValue, ArrayType& OutValue, FJsonArchive& Archive)
    {
        if (!InValue.IsArray())
        {
            Archive.AddTypeError(InValue, "an array");
            return false;
        }

        if (InValue.Num() != NUM_ELEMENTS)
        {
            Archive.AddError("expected %d elements, found %d", NUM_ELEMENTS, InValue.Num());
            return false;
        }

        for (int32 Index = 0; Index < NUM_ELEMENTS; ++Index)
        {
            TJsonSerializer<T>::Load(InValue[Index], OutValue[Index], Archive);
        }

        return true;
    }
};

template<typename T>
struct TJsonSerializer<TOptional<T>>
{
    static void Save(FJsonValue& OutValue, const TOptional<T>& InValue)
    {
        if (InValue.HasValue())
        {
            TJsonSerializer<T>::Save(OutValue, InValue.GetValue());
        }
        else
        {
            OutValue = FJsonValue();
        }
    }

    static bool Load(const FJsonValue& InValue, TOptional<T>& OutValue, FJsonArchive& Archive)
    {
        if (InValue.IsNull())
        {
            OutValue.Reset();
            return true;
        }

        T Loaded{};
        if (!TJsonSerializer<T>::Load(InValue, Loaded, Archive))
        {
            return false;
        }

        OutValue.Emplace(Move(Loaded));
        return true;
    }
};

template<typename T>
struct TJsonOmitEmpty<TOptional<T>>
{
    static bool IsEmpty(const TOptional<T>& Value)
    {
        return !Value.HasValue();
    }
};

// Writes a map as a JSON object with its keys in alphabetical order. TMap is backed by
// std::unordered_map, so its own iteration order changes between runs and between builds, and
// sorting is what makes two saves of the same data produce the same bytes.
template<typename T>
struct TJsonSerializer<TMap<String, T>>
{
    typedef TMap<String, T> MapType;

    static void Save(FJsonValue& OutValue, const MapType& InValue)
    {
        TArray<String> Keys = InValue.GetKeys();
        Keys.SortWithPredicate([](const String& LHS, const String& RHS) { return LHS.Compare(RHS) < 0; });

        OutValue = FJsonValue::MakeObject();
        for (int32 Index = 0; Index < Keys.Size(); ++Index)
        {
            FJsonValue Element;
            TJsonSerializer<T>::Save(Element, *InValue.Find(Keys[Index]));
            OutValue.AddMember(Keys[Index].Data(), Move(Element));
        }
    }

    static bool Load(const FJsonValue& InValue, MapType& OutValue, FJsonArchive& Archive)
    {
        if (!InValue.IsObject())
        {
            Archive.AddTypeError(InValue, "an object");
            return false;
        }

        OutValue.Clear();
        for (int32 Index = 0; Index < InValue.NumMembers(); ++Index)
        {
            T Loaded{};
            if (TJsonSerializer<T>::Load(InValue.GetMemberValue(Index), Loaded, Archive))
            {
                OutValue.Add(InValue.GetMemberName(Index), Move(Loaded));
            }
        }

        return true;
    }
};
