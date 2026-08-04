#pragma once
#include "Core/Json/JsonValue.h"
#include "Core/Templates/Utility/NonCopyable.h"

template<typename T>
struct TJsonSerializer;

template<typename T>
struct TJsonOmitEmpty
{
    static bool IsEmpty(const T&)
    {
        return false;
    }
};

// A type writes a single Serialize(FJsonArchive&) that names its fields, and the same code both saves
// and loads. Three behaviours make this pleasant to live with as scene files age:
//  - A field that still holds its default is left out of the output, so a file records only what
//    differs and adding a property does not rewrite every scene.
//  - A field missing from the input takes its default without complaint, which is what makes files
//    written by an older or newer build still load.
//  - Errors accumulate with the path that produced them instead of aborting, so one bad hand-edit
//    reports as Actors[12].Components[0].Intensity and the rest of the scene still loads.
class CORE_API FJsonArchive
{
public:

    // Descends into a nested object for the lifetime of the scope. The unnamed form takes the next
    // element of the enclosing array, appending one when saving and stepping through the existing
    // ones when loading, so the same loop body works either way.
    struct FObjectScope : public FNonCopyable
    {
        FObjectScope(FJsonArchive& InArchive, const CHAR* Name);
        explicit FObjectScope(FJsonArchive& InArchive);
        ~FObjectScope();

        // False when loading a document where this object is absent.
        NODISCARD bool IsValid() const;

    private:
        FJsonArchive& Archive;
    };

    struct FArrayScope : public FNonCopyable
    {
        FArrayScope(FJsonArchive& InArchive, const CHAR* Name);
        ~FArrayScope();

        // False when loading a document where this array is absent.
        NODISCARD bool IsValid() const;

        // Elements present when loading, or written so far when saving.
        NODISCARD int32 Num() const;

    private:
        FJsonArchive& Archive;
    };

public:
    NODISCARD static FJsonArchive Saver(FJsonValue& Root);
    NODISCARD static FJsonArchive Loader(const FJsonValue& Root);

public:
    ~FJsonArchive() = default;

    NODISCARD bool IsSaving()  const { return bIsSaving; }
    NODISCARD bool IsLoading() const { return !bIsSaving; }

    // The "Version" member of the document, or 0 when it has none.
    NODISCARD int32 GetVersion() const;

    void SetVersion(int32 InVersion);

    template<typename T>
    FJsonArchive& Field(const CHAR* Name, T& Value);

    // Serializes a field that is left out of the output while it still equals Default, 
    // which is also what it falls back to when the document has no such member.
    template<typename T>
    FJsonArchive& Field(const CHAR* Name, T& Value, const T& Default);

    // Serializes a field, reporting whether the document actually carried one. Always true on save.
    template<typename T>
    NODISCARD bool OptionalField(const CHAR* Name, T& Value);

    // Records a failure against the field currently being serialized
    void AddError(const CHAR* Format, ...);

    // Records that the document held the wrong type for the field being serialized
    void AddTypeError(const FJsonValue& Found, const CHAR* Expected);

    // Adopts the errors of a nested archive, rewriting their paths as relative to this one. A type
    // that serializes itself runs on its own archive rooted at its own value, so its errors arrive
    // with paths that mean nothing to the caller until they are prefixed here.
    void AppendErrors(const FJsonArchive& Nested);

    // Every error joined by newlines, for logging in one go
    NODISCARD String GetErrorString() const;

    // The field currently being serialized, such as "Actors[12].Intensity"
    NODISCARD String GetPath() const;

    // The name JSON uses for a type, for error messages
    NODISCARD static const CHAR* GetTypeName(EJsonType Type);

    NODISCARD bool HasErrors() const
    {
        return !Errors.IsEmpty();
    }
    
    NODISCARD const TArray<String>& GetErrors() const
    {
        return Errors;
    }

private:
    struct FScope
    {
        FJsonValue*       SaveValue     = nullptr;
        const FJsonValue* LoadValue     = nullptr;
        int32             ElementCursor = 0;
    };

    FJsonArchive(FJsonValue* InSaveRoot, const FJsonValue* InLoadRoot);

    NODISCARD FScope&       CurrentScope()       { return Scopes.Last(); }
    NODISCARD const FScope& CurrentScope() const { return Scopes.Last(); }

    void PushNamedScope(const CHAR* Name, EJsonType ContainerType);
    void PushElementScope(EJsonType ContainerType);
    void PopScope();

    // Null when the member is absent or the enclosing scope is unreachable
    NODISCARD const FJsonValue* FindForLoad(const CHAR* Name) const;
    NODISCARD FJsonValue*       GetObjectForSave() const;

    bool              bIsSaving;
    FJsonValue*       SaveRoot;
    const FJsonValue* LoadRoot;
    TArray<FScope>    Scopes;
    TArray<String>    PathSegments;
    TArray<String>    Errors;
};

template<typename T>
FJsonArchive& FJsonArchive::Field(const CHAR* Name, T& Value)
{
    PathSegments.Emplace(Name);

    if (bIsSaving)
    {
        if (FJsonValue* Object = GetObjectForSave())
        {
            if (!TJsonOmitEmpty<T>::IsEmpty(Value))
            {
                FJsonValue Written;
                TJsonSerializer<T>::Save(Written, Value);
                Object->AddMember(Name, Move(Written));
            }
        }
    }
    else if (const FJsonValue* Found = FindForLoad(Name))
    {
        TJsonSerializer<T>::Load(*Found, Value, *this);
    }

    PathSegments.Pop();
    return *this;
}

template<typename T>
FJsonArchive& FJsonArchive::Field(const CHAR* Name, T& Value, const T& Default)
{
    PathSegments.Emplace(Name);

    if (bIsSaving)
    {
        if (FJsonValue* Object = GetObjectForSave())
        {
            if (!(Value == Default) && !TJsonOmitEmpty<T>::IsEmpty(Value))
            {
                FJsonValue Written;
                TJsonSerializer<T>::Save(Written, Value);
                Object->AddMember(Name, Move(Written));
            }
        }
    }
    else if (const FJsonValue* Found = FindForLoad(Name))
    {
        if (!TJsonSerializer<T>::Load(*Found, Value, *this))
        {
            Value = Default;
        }
    }
    else
    {
        Value = Default;
    }

    PathSegments.Pop();
    return *this;
}

template<typename T>
bool FJsonArchive::OptionalField(const CHAR* Name, T& Value)
{
    PathSegments.Emplace(Name);

    bool bResult = false;
    if (bIsSaving)
    {
        if (FJsonValue* Object = GetObjectForSave())
        {
            if (!TJsonOmitEmpty<T>::IsEmpty(Value))
            {
                FJsonValue Written;
                TJsonSerializer<T>::Save(Written, Value);
                Object->AddMember(Name, Move(Written));
                bResult = true;
            }
        }
    }
    else if (const FJsonValue* Found = FindForLoad(Name))
    {
        bResult = TJsonSerializer<T>::Load(*Found, Value, *this);
    }

    PathSegments.Pop();
    return bResult;
}
