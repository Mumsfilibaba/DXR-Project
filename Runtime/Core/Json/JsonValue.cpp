#include "Core/Json/JsonValue.h"

String FJsonError::ToString() const
{
    if (Line <= 0)
    {
        return Message;
    }

    return String::Printf("line %d, column %d: %s", Line, Column, Message.Data());
}


FJsonValue::FJsonValue()
    : Type(EJsonType::Null)
    , bIsIntegral(false)
    , IntValue(0)
    , StringValue()
    , ArrayValues()
    , Members()
{
}

FJsonValue::FJsonValue(const FJsonValue& Other) = default;

FJsonValue::FJsonValue(FJsonValue&& Other) = default;

FJsonValue::~FJsonValue() = default;

FJsonValue& FJsonValue::operator=(const FJsonValue& Other) = default;

FJsonValue& FJsonValue::operator=(FJsonValue&& Other) = default;

FJsonValue::FJsonValue(bool bValue)
    : FJsonValue()
{
    Type      = EJsonType::Bool;
    BoolValue = bValue;
}

FJsonValue::FJsonValue(int8 Value)   : FJsonValue() { SetIntegral(static_cast<int64>(Value)); }
FJsonValue::FJsonValue(int16 Value)  : FJsonValue() { SetIntegral(static_cast<int64>(Value)); }
FJsonValue::FJsonValue(int32 Value)  : FJsonValue() { SetIntegral(static_cast<int64>(Value)); }
FJsonValue::FJsonValue(int64 Value)  : FJsonValue() { SetIntegral(Value); }
FJsonValue::FJsonValue(uint8 Value)  : FJsonValue() { SetIntegral(static_cast<int64>(Value)); }
FJsonValue::FJsonValue(uint16 Value) : FJsonValue() { SetIntegral(static_cast<int64>(Value)); }
FJsonValue::FJsonValue(uint32 Value) : FJsonValue() { SetIntegral(static_cast<int64>(Value)); }

FJsonValue::FJsonValue(uint64 Value)
    : FJsonValue()
{
    // Values past int64 lose exactness here, but JSON has no wider integer to write them to
    SetIntegral(static_cast<int64>(Value));
}

FJsonValue::FJsonValue(float Value)  : FJsonValue() { SetReal(static_cast<double>(Value)); }
FJsonValue::FJsonValue(double Value) : FJsonValue() { SetReal(Value); }

FJsonValue::FJsonValue(const CHAR* Value)
    : FJsonValue()
{
    Type        = EJsonType::String;
    StringValue = Value ? String(Value) : String();
}

FJsonValue::FJsonValue(const String& Value)
    : FJsonValue()
{
    Type        = EJsonType::String;
    StringValue = Value;
}

FJsonValue::FJsonValue(const StringView& Value)
    : FJsonValue()
{
    Type        = EJsonType::String;
    StringValue = String(Value.Data(), Value.Length());
}

void FJsonValue::SetIntegral(int64 Value)
{
    Type        = EJsonType::Number;
    bIsIntegral = true;
    IntValue    = Value;
}

void FJsonValue::SetReal(double Value)
{
    Type        = EJsonType::Number;
    bIsIntegral = false;
    DoubleValue = Value;
}

FJsonValue FJsonValue::MakeArray()
{
    FJsonValue NewValue;
    NewValue.Type = EJsonType::Array;
    return NewValue;
}

FJsonValue FJsonValue::MakeObject()
{
    FJsonValue NewValue;
    NewValue.Type = EJsonType::Object;
    return NewValue;
}

void FJsonValue::Reset()
{
    Type        = EJsonType::Null;
    bIsIntegral = false;
    IntValue    = 0;
    StringValue.Clear(true);
    ArrayValues.Clear(true);
    Members.Clear(true);
}

bool FJsonValue::TryGetBool(bool& OutValue) const
{
    if (Type != EJsonType::Bool)
    {
        return false;
    }

    OutValue = BoolValue;
    return true;
}

bool FJsonValue::TryGetInt64(int64& OutValue) const
{
    if (Type != EJsonType::Number)
    {
        return false;
    }

    OutValue = bIsIntegral ? IntValue : static_cast<int64>(DoubleValue);
    return true;
}

bool FJsonValue::TryGetDouble(double& OutValue) const
{
    if (Type != EJsonType::Number)
    {
        return false;
    }

    OutValue = bIsIntegral ? static_cast<double>(IntValue) : DoubleValue;
    return true;
}

bool FJsonValue::TryGetString(String& OutValue) const
{
    if (Type != EJsonType::String)
    {
        return false;
    }

    OutValue = StringValue;
    return true;
}

bool FJsonValue::GetBoolOr(bool bDefault) const
{
    bool bResult = bDefault;
    return TryGetBool(bResult) ? bResult : bDefault;
}

int64 FJsonValue::GetInt64Or(int64 Default) const
{
    int64 Result = Default;
    return TryGetInt64(Result) ? Result : Default;
}

double FJsonValue::GetDoubleOr(double Default) const
{
    double Result = Default;
    return TryGetDouble(Result) ? Result : Default;
}

String FJsonValue::GetStringOr(const CHAR* Default) const
{
    String Result;
    return TryGetString(Result) ? Result : String(Default ? Default : "");
}

FJsonValue* FJsonValue::Find(const CHAR* Name)
{
    const FJsonValue* Result = const_cast<const FJsonValue*>(this)->Find(Name);
    return const_cast<FJsonValue*>(Result);
}

const FJsonValue* FJsonValue::Find(const CHAR* Name) const
{
    if ((Type != EJsonType::Object) || (Name == nullptr))
    {
        return nullptr;
    }

    for (int32 Index = 0; Index < Members.Size(); ++Index)
    {
        if (Members[Index].Name.Equals(Name))
        {
            return &Members[Index].Value;
        }
    }

    return nullptr;
}

FJsonValue& FJsonValue::FindOrAdd(const CHAR* Name)
{
    CHECK(Type == EJsonType::Object);

    if (FJsonValue* Existing = Find(Name))
    {
        return *Existing;
    }

    return Members.Emplace(Name, FJsonValue()).Value;
}

FJsonValue& FJsonValue::AddMember(const CHAR* Name, FJsonValue&& InValue)
{
    CHECK(Type == EJsonType::Object);

    if (FJsonValue* Existing = Find(Name))
    {
        *Existing = Move(InValue);
        return *Existing;
    }

    return Members.Emplace(Name, Move(InValue)).Value;
}

bool FJsonValue::RemoveMember(const CHAR* Name)
{
    if ((Type != EJsonType::Object) || (Name == nullptr))
    {
        return false;
    }

    for (int32 Index = 0; Index < Members.Size(); ++Index)
    {
        if (Members[Index].Name.Equals(Name))
        {
            Members.RemoveAt(Index);
            return true;
        }
    }

    return false;
}

int32 FJsonValue::NumMembers() const
{
    return (Type == EJsonType::Object) ? Members.Size() : 0;
}

const String& FJsonValue::GetMemberName(int32 Index) const
{
    CHECK(Type == EJsonType::Object);
    return Members[Index].Name;
}

FJsonValue& FJsonValue::GetMemberValue(int32 Index)
{
    CHECK(Type == EJsonType::Object);
    return Members[Index].Value;
}

const FJsonValue& FJsonValue::GetMemberValue(int32 Index) const
{
    CHECK(Type == EJsonType::Object);
    return Members[Index].Value;
}

FJsonValue& FJsonValue::Add(FJsonValue&& InValue)
{
    CHECK(Type == EJsonType::Array);
    return ArrayValues.Add(Move(InValue));
}

int32 FJsonValue::Num() const
{
    return (Type == EJsonType::Array) ? ArrayValues.Size() : 0;
}

FJsonValue& FJsonValue::operator[](int32 Index)
{
    CHECK(Type == EJsonType::Array);
    return ArrayValues[Index];
}

const FJsonValue& FJsonValue::operator[](int32 Index) const
{
    CHECK(Type == EJsonType::Array);
    return ArrayValues[Index];
}

bool FJsonValue::Equals(const FJsonValue& Other) const
{
    if (Type != Other.Type)
    {
        return false;
    }

    switch (Type)
    {
        case EJsonType::Null:
        {
            return true;
        }

        case EJsonType::Bool:
        {
            return BoolValue == Other.BoolValue;
        }

        case EJsonType::Number:
        {
            // An integer and a real holding the same quantity compare equal, so that a document
            // that survived a write/read round-trip still matches the one it came from.
            if (bIsIntegral && Other.bIsIntegral)
            {
                return IntValue == Other.IntValue;
            }

            const double This  = bIsIntegral ? static_cast<double>(IntValue) : DoubleValue;
            const double That  = Other.bIsIntegral ? static_cast<double>(Other.IntValue) : Other.DoubleValue;
            return This == That;
        }

        case EJsonType::String:
        {
            return StringValue.Equals(Other.StringValue);
        }

        case EJsonType::Array:
        {
            if (ArrayValues.Size() != Other.ArrayValues.Size())
            {
                return false;
            }

            for (int32 Index = 0; Index < ArrayValues.Size(); ++Index)
            {
                if (!ArrayValues[Index].Equals(Other.ArrayValues[Index]))
                {
                    return false;
                }
            }

            return true;
        }

        case EJsonType::Object:
        {
            if (Members.Size() != Other.Members.Size())
            {
                return false;
            }

            for (int32 Index = 0; Index < Members.Size(); ++Index)
            {
                if (!Members[Index].Name.Equals(Other.Members[Index].Name))
                {
                    return false;
                }

                if (!Members[Index].Value.Equals(Other.Members[Index].Value))
                {
                    return false;
                }
            }

            return true;
        }
    }

    return false;
}
