#include "Core/Json/Json.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/Paths.h"
#include "Core/Platform/PlatformFile.h"
#include "ShaderCompiler/ShaderJobFile.h"

static TAutoConsoleVariable<String> CVarJobFileName(
    "Renderer.ShaderCache.JobFileName",
    "FileName for the file listing every shader combination the editor used, for every RHI. Compiled by the ShaderCompiler tool.",
    "Shaders.shaderjob");

static uint64 MakeEntryKey(const String& RHIName, const FShaderCompileJob& Job)
{
    uint64 Key = Job.GetKey();
    for (int32 Index = 0; Index < RHIName.Length(); ++Index)
    {
        const CHAR Character = RHIName[Index];
        HashCombine(Key, static_cast<uint32>((Character >= 'A' && Character <= 'Z') ? Character - 'A' + 'a' : Character));
    }

    return Key;
}

String FShaderJobFile::GetDefaultFilePath()
{
    return Paths::GetAssetDir() + '/' + CVarJobFileName.GetValue();
}

bool FShaderJobFile::Load(const String& FilePath, String& OutError)
{
    if (!FPlatformFile::IsFile(*FilePath))
    {
        return true;
    }

    FJsonValue Root;
    FJsonError ParseError;
    if (!Json::LoadFromFile(FilePath, Root, &ParseError))
    {
        OutError = String::Printf("'%s' is not valid JSON: %s", *FilePath, *ParseError.ToString());
        return false;
    }

    const FJsonValue* VersionValue = Root.Find("version");
    if (!Root.IsObject() || !VersionValue || VersionValue->GetInt64Or(0) != Version)
    {
        OutError = String::Printf("'%s' is not a version %d shader job file", *FilePath, Version);
        return false;
    }

    const FJsonValue* JobsValue = Root.Find("jobs");
    if (!JobsValue || !JobsValue->IsArray())
    {
        OutError = String::Printf("'%s' has no 'jobs' array", *FilePath);
        return false;
    }

    for (int32 Index = 0; Index < JobsValue->Num(); ++Index)
    {
        const FJsonValue& JobValue = (*JobsValue)[Index];

        String RHIName;
        const FJsonValue* RHIValue = JobValue.Find("rhi");
        if (!RHIValue || !RHIValue->TryGetString(RHIName) || RHIName.IsEmpty())
        {
            OutError = String::Printf("'%s': job %d has no 'rhi'", *FilePath, Index);
            return false;
        }

        FShaderCompileJob Job;
        String            JobError;
        if (!FShaderCompileJob::FromJson(JobValue, Job, JobError))
        {
            OutError = String::Printf("'%s': job %d: %s", *FilePath, Index, *JobError);
            return false;
        }

        Add(RHIName, ::Move(Job));
    }

    return true;
}

bool FShaderJobFile::Save(const String& FilePath) const
{
    FJsonValue Jobs = FJsonValue::MakeArray();
    for (const FShaderJobFileEntry& Entry : Entries)
    {
        // "rhi" goes first so a diff shows which RHI a job belongs to without scrolling
        FJsonValue Job = FJsonValue::MakeObject();
        Job.AddMember("rhi", FJsonValue(Entry.RHIName));

        const FJsonValue JobValue = Entry.Job.ToJson();
        for (int32 MemberIndex = 0; MemberIndex < JobValue.NumMembers(); ++MemberIndex)
        {
            FJsonValue MemberValue = JobValue.GetMemberValue(MemberIndex);
            Job.AddMember(*JobValue.GetMemberName(MemberIndex), ::Move(MemberValue));
        }

        Jobs.Add(::Move(Job));
    }

    FJsonValue Root = FJsonValue::MakeObject();
    Root.AddMember("version", FJsonValue(Version));
    Root.AddMember("jobs", ::Move(Jobs));
    return Json::SaveToFile(FilePath, Root, EJsonWriteFlags::Pretty);
}

bool FShaderJobFile::Add(const String& RHIName, FShaderCompileJob&& Job)
{
    bool bAlreadyAdded = false;
    Keys.Add(MakeEntryKey(RHIName, Job), &bAlreadyAdded);

    if (bAlreadyAdded)
    {
        return false;
    }

    FShaderJobFileEntry& Entry = Entries.Emplace();
    Entry.RHIName = RHIName;
    Entry.Job     = ::Move(Job);
    return true;
}

TArray<const FShaderJobFileEntry*> FShaderJobFile::Filter(TArrayView<const String> RHINames) const
{
    TArray<const FShaderJobFileEntry*> Result;
    for (const FShaderJobFileEntry& Entry : Entries)
    {
        bool bSelected = RHINames.IsEmpty();
        for (const String& RHIName : RHINames)
        {
            bSelected |= Entry.RHIName.Equals(RHIName, EStringCaseType::NoCase);
        }

        if (bSelected)
        {
            Result.Add(&Entry);
        }
    }

    return Result;
}
