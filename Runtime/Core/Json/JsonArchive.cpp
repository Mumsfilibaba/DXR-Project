#include "Core/Json/JsonArchive.h"

#include <cstdarg>
#include <cstdio>

static constexpr int32 GMaxErrorMessageLength = 512;

FJsonArchive::FJsonArchive(FJsonValue* InSaveRoot, const FJsonValue* InLoadRoot)
    : bIsSaving(InSaveRoot != nullptr)
    , SaveRoot(InSaveRoot)
    , LoadRoot(InLoadRoot)
    , Scopes()
    , PathSegments()
    , Errors()
{
    FScope& RootScope = Scopes.AddDefault();
    RootScope.SaveValue = InSaveRoot;
    RootScope.LoadValue = InLoadRoot;
}

FJsonArchive FJsonArchive::Saver(FJsonValue& Root)
{
    Root = FJsonValue::MakeObject();
    return FJsonArchive(&Root, nullptr);
}

FJsonArchive FJsonArchive::Loader(const FJsonValue& Root)
{
    return FJsonArchive(nullptr, &Root);
}

const CHAR* FJsonArchive::GetTypeName(EJsonType Type)
{
    switch (Type)
    {
        case EJsonType::Null:   return "null";
        case EJsonType::Bool:   return "a bool";
        case EJsonType::Number: return "a number";
        case EJsonType::String: return "a string";
        case EJsonType::Array:  return "an array";
        case EJsonType::Object: return "an object";
    }

    return "an unknown type";
}

int32 FJsonArchive::GetVersion() const
{
    const FJsonValue* Root = bIsSaving ? SaveRoot : LoadRoot;
    if (Root == nullptr)
    {
        return 0;
    }

    const FJsonValue* Version = Root->Find("Version");
    return Version ? static_cast<int32>(Version->GetInt64Or(0)) : 0;
}

void FJsonArchive::SetVersion(int32 InVersion)
{
    if (bIsSaving && SaveRoot)
    {
        SaveRoot->AddMember("Version", FJsonValue(InVersion));
    }
}

FJsonValue* FJsonArchive::GetObjectForSave() const
{
    FJsonValue* Container = CurrentScope().SaveValue;
    if ((Container == nullptr) || !Container->IsObject())
    {
        return nullptr;
    }

    return Container;
}

const FJsonValue* FJsonArchive::FindForLoad(const CHAR* Name) const
{
    const FJsonValue* Container = CurrentScope().LoadValue;
    if ((Container == nullptr) || !Container->IsObject())
    {
        return nullptr;
    }

    return Container->Find(Name);
}

void FJsonArchive::PushNamedScope(const CHAR* Name, EJsonType ContainerType)
{
    PathSegments.Emplace(Name);

    FScope NewScope;
    if (bIsSaving)
    {
        if (FJsonValue* Object = GetObjectForSave())
        {
            FJsonValue Container = (ContainerType == EJsonType::Array) ? FJsonValue::MakeArray() : FJsonValue::MakeObject();
            NewScope.SaveValue   = &Object->AddMember(Name, Move(Container));
        }
    }
    else if (const FJsonValue* Found = FindForLoad(Name))
    {
        if (Found->GetType() == ContainerType)
        {
            NewScope.LoadValue = Found;
        }
        else
        {
            AddTypeError(*Found, (ContainerType == EJsonType::Array) ? "an array" : "an object");
        }
    }

    Scopes.Add(Move(NewScope));
}

void FJsonArchive::PushElementScope(EJsonType ContainerType)
{
    FScope& Enclosing = CurrentScope();

    FScope NewScope;
    int32  ElementIndex = Enclosing.ElementCursor;

    if (bIsSaving)
    {
        if (FJsonValue* Array = Enclosing.SaveValue; Array && Array->IsArray())
        {
            ElementIndex = Array->Num();

            FJsonValue Container = (ContainerType == EJsonType::Array) ? FJsonValue::MakeArray() : FJsonValue::MakeObject();
            NewScope.SaveValue   = &Array->Add(Move(Container));
        }
    }
    else if (const FJsonValue* Array = Enclosing.LoadValue; Array && Array->IsArray())
    {
        if (ElementIndex < Array->Num())
        {
            const FJsonValue& Element = (*Array)[ElementIndex];
            if (Element.GetType() == ContainerType)
            {
                NewScope.LoadValue = &Element;
            }
        }
    }

    ++Enclosing.ElementCursor;

    PathSegments.Emplace(String::CreateFormatted("[%d]", ElementIndex));
    Scopes.Add(Move(NewScope));
}

void FJsonArchive::PopScope()
{
    CHECK(Scopes.Size() > 1);
    Scopes.Pop();
    PathSegments.Pop();
}

String FJsonArchive::GetPath() const
{
    String Path;
    for (int32 Index = 0; Index < PathSegments.Size(); ++Index)
    {
        const String& Segment = PathSegments[Index];
        if (!Path.IsEmpty() && !Segment.StartsWith("["))
        {
            Path.Append('.');
        }

        Path.Append(Segment);
    }

    return Path;
}

void FJsonArchive::AddError(const CHAR* Format, ...)
{
    CHAR Message[GMaxErrorMessageLength];
    Message[0] = 0;

    va_list ArgList;
    va_start(ArgList, Format);
    vsnprintf(Message, sizeof(Message), Format, ArgList);
    va_end(ArgList);

    const String Path = GetPath();
    if (Path.IsEmpty())
    {
        Errors.Emplace(Message);
    }
    else
    {
        Errors.Add(String::CreateFormatted("%s: %s", Path.Data(), Message));
    }
}

void FJsonArchive::AddTypeError(const FJsonValue& Found, const CHAR* Expected)
{
    AddError("expected %s, found %s", Expected, GetTypeName(Found.GetType()));
}

void FJsonArchive::AppendErrors(const FJsonArchive& Nested)
{
    const String Path = GetPath();
    for (int32 Index = 0; Index < Nested.Errors.Size(); ++Index)
    {
        const String& NestedError = Nested.Errors[Index];
        if (Path.IsEmpty())
        {
            Errors.Add(NestedError);
        }
        else
        {
            const CHAR* Separator = NestedError.StartsWith("[") ? "" : ".";
            Errors.Add(String::CreateFormatted("%s%s%s", Path.Data(), Separator, NestedError.Data()));
        }
    }
}

String FJsonArchive::GetErrorString() const
{
    String Result;
    for (int32 Index = 0; Index < Errors.Size(); ++Index)
    {
        if (Index > 0)
        {
            Result.Append('\n');
        }

        Result.Append(Errors[Index]);
    }

    return Result;
}


FJsonArchive::FObjectScope::FObjectScope(FJsonArchive& InArchive, const CHAR* Name)
    : Archive(InArchive)
{
    Archive.PushNamedScope(Name, EJsonType::Object);
}

FJsonArchive::FObjectScope::FObjectScope(FJsonArchive& InArchive)
    : Archive(InArchive)
{
    Archive.PushElementScope(EJsonType::Object);
}

FJsonArchive::FObjectScope::~FObjectScope()
{
    Archive.PopScope();
}

bool FJsonArchive::FObjectScope::IsValid() const
{
    const FScope& Scope = Archive.CurrentScope();
    return Archive.IsSaving() ? (Scope.SaveValue != nullptr) : (Scope.LoadValue != nullptr);
}

FJsonArchive::FArrayScope::FArrayScope(FJsonArchive& InArchive, const CHAR* Name)
    : Archive(InArchive)
{
    Archive.PushNamedScope(Name, EJsonType::Array);
}

FJsonArchive::FArrayScope::~FArrayScope()
{
    Archive.PopScope();
}

bool FJsonArchive::FArrayScope::IsValid() const
{
    const FScope& Scope = Archive.CurrentScope();
    return Archive.IsSaving() ? (Scope.SaveValue != nullptr) : (Scope.LoadValue != nullptr);
}

int32 FJsonArchive::FArrayScope::Num() const
{
    const FScope&     Scope = Archive.CurrentScope();
    const FJsonValue* Array = Archive.IsSaving() ? Scope.SaveValue : Scope.LoadValue;
    return Array ? Array->Num() : 0;
}
