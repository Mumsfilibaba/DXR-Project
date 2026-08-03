#pragma once
#include "Core/Json/JsonTypes.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"

struct FJsonMember;

// A single value in a JSON document. Object members keep insertion order rather than living in a 
// TMap, whose iteration order would make saved files churn between runs. Integers are stored 
// separately from doubles so that counts write as "3" rather than "3.0", and so values beyond the
// 2^53 a double holds exactly still round-trip.
class CORE_API FJsonValue
{
public:
    FJsonValue();
    FJsonValue(const FJsonValue& Other);
    FJsonValue(FJsonValue&& Other);
    ~FJsonValue();

    FJsonValue& operator=(const FJsonValue& Other);
    FJsonValue& operator=(FJsonValue&& Other);

    explicit FJsonValue(bool bValue);
    explicit FJsonValue(int8 Value);
    explicit FJsonValue(int16 Value);
    explicit FJsonValue(int32 Value);
    explicit FJsonValue(int64 Value);
    explicit FJsonValue(uint8 Value);
    explicit FJsonValue(uint16 Value);
    explicit FJsonValue(uint32 Value);
    explicit FJsonValue(uint64 Value);
    explicit FJsonValue(float Value);
    explicit FJsonValue(double Value);
    explicit FJsonValue(const CHAR* Value);
    explicit FJsonValue(const String& Value);
    explicit FJsonValue(const StringView& Value);

    NODISCARD static FJsonValue MakeArray();
    NODISCARD static FJsonValue MakeObject();

    NODISCARD EJsonType GetType() const { return Type; }

    NODISCARD bool IsNull()     const { return Type == EJsonType::Null; }
    NODISCARD bool IsBool()     const { return Type == EJsonType::Bool; }
    NODISCARD bool IsNumber()   const { return Type == EJsonType::Number; }
    NODISCARD bool IsString()   const { return Type == EJsonType::String; }
    NODISCARD bool IsArray()    const { return Type == EJsonType::Array; }
    NODISCARD bool IsObject()   const { return Type == EJsonType::Object; }
    NODISCARD bool IsIntegral() const { return (Type == EJsonType::Number) && bIsIntegral; }

    // Discards any contents and turns this into a null value
    void Reset();

    // Typed access. Each returns false and leaves Out untouched when the type does not match.
    NODISCARD bool TryGetBool(bool& OutValue) const;
    NODISCARD bool TryGetInt64(int64& OutValue) const;
    NODISCARD bool TryGetDouble(double& OutValue) const;
    NODISCARD bool TryGetString(String& OutValue) const;

    // Typed access with a fallback, for call sites that have a sensible default.
    NODISCARD bool   GetBoolOr(bool bDefault) const;
    NODISCARD int64  GetInt64Or(int64 Default) const;
    NODISCARD double GetDoubleOr(double Default) const;
    NODISCARD String GetStringOr(const CHAR* Default) const;

    NODISCARD FJsonValue*       Find(const CHAR* Name);
    NODISCARD const FJsonValue* Find(const CHAR* Name) const;

    // Returns the named member, adding a null one when it does not exist yet
    NODISCARD FJsonValue& FindOrAdd(const CHAR* Name);

    // Array access. Calling these on a non-array is a programming error.
    FJsonValue& Add(FJsonValue&& InValue);

    // Adds or replaces a member, preserving the position of an existing one
    FJsonValue& AddMember(const CHAR* Name, FJsonValue&& InValue);

    bool RemoveMember(const CHAR* Name);

    // Compares type and contents, with object members having to match in order
    NODISCARD bool Equals(const FJsonValue& Other) const;

    NODISCARD int32 Num() const;
    NODISCARD int32 NumMembers() const;

    NODISCARD const String&     GetMemberName(int32 Index) const;
    NODISCARD FJsonValue&       GetMemberValue(int32 Index);
    NODISCARD const FJsonValue& GetMemberValue(int32 Index) const;

    NODISCARD FJsonValue&       operator[](int32 Index);
    NODISCARD const FJsonValue& operator[](int32 Index) const;

private:
    void SetIntegral(int64 Value);
    void SetReal(double Value);

    EJsonType Type;
    bool      bIsIntegral;

    union
    {
        bool   BoolValue;
        int64  IntValue;
        double DoubleValue;
    };

    String              StringValue;
    TArray<FJsonValue>  ArrayValues;
    TArray<FJsonMember> Members;
};

struct FJsonMember
{
    FJsonMember() = default;

    FJsonMember(const CHAR* InName, FJsonValue&& InValue)
        : Name(InName)
        , Value(Move(InValue))
    {
    }

    String     Name;
    FJsonValue Value;
};

NODISCARD FORCEINLINE bool operator==(const FJsonValue& LHS, const FJsonValue& RHS)
{
    return LHS.Equals(RHS);
}

NODISCARD FORCEINLINE bool operator!=(const FJsonValue& LHS, const FJsonValue& RHS)
{
    return !LHS.Equals(RHS);
}
