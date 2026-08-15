#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/StaticString.h"

class FPlayerInput;

class ENGINE_API FInputName
{
    friend class FPlayerInput;

public:
    FInputName();

    template<typename StringType>
    FORCEINLINE FInputName(const StringType& InName) requires(TIsTStringType<StringType>::Value)
        : Name(InName.Data(), InName.Length())
        , Hash(HashInputName(InName.Data(), static_cast<uint32>(InName.Length())))
    {
    }

    FInputName(const CHAR* InName);

    uint32 GetHash() const
    {
        return Hash;
    }

    const String& ToString() const
    {
        return Name;
    }

    bool operator==(const FInputName& Other) const;
    bool operator!=(const FInputName& Other) const;

private:
    static uint32 HashInputName(const CHAR* Data, uint32 Length);

    template<typename StringType>
    FORCEINLINE static uint32 HashInputName(const StringType& InName) requires(TIsTStringType<StringType>::Value)
    {
        return HashInputName(InName.Data(), static_cast<uint32>(InName.Length()));
    }

    String Name;
    uint32 Hash;
};
