#include "RemoteShaderCompilerTests.h"

#include <Core/Containers/Map.h>
#include <Core/Containers/SharedPtr.h>
#include <Core/Filesystem/File.h>
#include <Core/Memory/Memory.h>
#include <Core/Misc/Paths.h>
#include <Core/Network/NetworkSocket.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Platform/PlatformThreadMisc.h>
#include <Core/Platform/PlatformTime.h>
#include <ShaderCompiler/ShaderCompileJob.h>
#include <ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompiler/ShaderJobFile.h>
#include <ShaderCompiler/ShaderPreprocessor.h>
#include <ShaderCompiler/ShaderSourceHash.h>
#include <ShaderCompiler/Remote/RemoteShaderCompilerClient.h>
#include <ShaderCompiler/Remote/RemoteShaderCompilerProtocol.h>
#include <ShaderCompiler/Remote/RemoteShaderCompilerServer.h>
#include <ShaderCore/MSLShaderBindings.h>
#include <ShaderCore/ShaderCode.h>

#include "TestCommon/TestMacros.h"

static constexpr uint32 GTestTimeoutMs = 10000;

static FString GetClearBufferSource()
{
    return Paths::GetEngineDir() + TEXT("/Runtime/VulkanRHI/Shaders/Internal/ClearBufferUAV.hlsl");
}

static FString GetMetalClearBufferSource()
{
    return Paths::GetEngineDir() + TEXT("/Runtime/MetalRHI/Shaders/Internal/ClearBufferUAV.hlsl");
}

static FString GetClearTextureSource()
{
    return Paths::GetEngineDir() + TEXT("/Runtime/MetalRHI/Shaders/Internal/ClearTextureUAV.hlsl");
}

static bool WriteTestFile(const FString& FilePath, const CHAR_T* Text)
{
    if (!File::CreateDirectoryTree(File::GetDirectoryOf(FilePath)))
    {
        return false;
    }

    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(FilePath);
    return FileHandle && File::WriteTextFile(FileHandle.Get(), FString(Text));
}

static TArray<uint8> ToBytes(const CHAR_T* Text)
{
    TArray<uint8> Bytes;
    for (const CHAR_T* Character = Text; *Character; ++Character)
    {
        Bytes.Add(static_cast<uint8>(*Character));
    }

    return Bytes;
}

static uint64 GetTestMilliseconds()
{
    return static_cast<uint64>(static_cast<double>(FPlatformTime::QueryPerformanceCounter()) * 1000.0 / static_cast<double>(FPlatformTime::QueryPerformanceFrequency()));
}

class FMemorySourceProvider final : public IShaderSourceProvider
{
public:
    void AddFile(const FString& Path, const TArray<CHAR_T>& Text)
    {
        Files.Add(Path, Text);
    }

    NODISCARD virtual bool FileExists(const FString& Path) const override final
    {
        return Files.Contains(Path);
    }

    virtual bool ReadFile(const FString& Path, TArray<CHAR_T>& OutText) const override final
    {
        const TArray<CHAR_T>* Text = Files.Find(Path);
        if (!Text)
        {
            return false;
        }

        OutText = *Text;
        return true;
    }

    TMap<FString, TArray<CHAR_T>> Files;
};

static TSharedPtr<FRemoteRequestSources> MakeRequestSources(const TArray<FShaderSourceFile>& Files)
{
    TSharedPtr<FRemoteRequestSources> Sources = MakeSharedPtr<FRemoteRequestSources>();
    for (const FShaderSourceFile& File : Files)
    {
        Sources->Files.Add(RemoteShaderCompilerProtocol::CollapsePath(FString(FRemoteShaderCompilerServer::SourceRoot) + '/' + File.Path), MakeSharedPtr<TArray<uint8>>(File.Contents));
    }

    return Sources;
}

static bool SendAll(FNetworkSocket& Socket, const TArray<uint8>& Bytes)
{
    int32 Offset = 0;
    while (Offset < Bytes.Size())
    {
        int32 BytesSent = 0;
        const ESocketResult Result = Socket.Send(Bytes.Data() + Offset, Bytes.Size() - Offset, BytesSent);
        if (Result == ESocketResult::WouldBlock)
        {
            continue;
        }

        if (Result != ESocketResult::Success)
        {
            return false;
        }

        Offset += BytesSent;
    }

    return true;
}

static bool ReadFrame(FNetworkSocket& Socket, TArray<uint8>& Buffer, RemoteShaderCompilerProtocol::FFrame& OutFrame)
{
    const uint64 Deadline = GetTestMilliseconds() + GTestTimeoutMs;

    TArray<uint8> Chunk;
    Chunk.Resize(64 * 1024);

    while (GetTestMilliseconds() < Deadline)
    {
        bool bHasFrame = false;
        if (!RemoteShaderCompilerProtocol::TryDecodeFrame(Buffer, OutFrame, bHasFrame))
        {
            return false;
        }

        if (bHasFrame)
        {
            return true;
        }

        FNetworkSocket* Sockets[] = { &Socket };
        bool            bReadable = false;
        if (FNetworkSocket::WaitForRead(Sockets, 1, &bReadable, 50) < 0)
        {
            return false;
        }

        if (!bReadable)
        {
            continue;
        }

        int32 BytesRead = 0;
        if (Socket.Recv(Chunk.Data(), Chunk.Size(), BytesRead) != ESocketResult::Success)
        {
            return false;
        }

        Buffer.Append(Chunk.Data(), BytesRead);
    }

    return false;
}

static void AddFileRefs(const TArray<FShaderSourceFile>& Files, bool bWithContents, RemoteShaderCompilerProtocol::FFrame& InOutFrame)
{
    FJsonValue FileRefs = FJsonValue::MakeArray();
    FJsonValue NewFiles = FJsonValue::MakeArray();
    for (const FShaderSourceFile& File : Files)
    {
        FJsonValue FileRef = FJsonValue::MakeObject();
        FileRef.AddMember(TEXT("path"), FJsonValue(File.Path));
        FileRef.AddMember(TEXT("hash"), FJsonValue(File.Hash));
        FileRefs.Add(::Move(FileRef));

        if (bWithContents)
        {
            FJsonValue NewFile = FJsonValue::MakeObject();
            NewFile.AddMember(TEXT("path"), FJsonValue(File.Path));
            NewFile.AddMember(TEXT("hash"), FJsonValue(File.Hash));
            NewFile.AddMember(TEXT("offset"), FJsonValue(InOutFrame.Payload.Size()));
            NewFile.AddMember(TEXT("size"), FJsonValue(File.Contents.Size()));
            NewFiles.Add(::Move(NewFile));

            InOutFrame.Payload.Append(File.Contents.Data(), File.Contents.Size());
        }
    }

    InOutFrame.Header.AddMember(TEXT("files"), ::Move(FileRefs));
    InOutFrame.Header.AddMember(TEXT("newFiles"), ::Move(NewFiles));
}

static const FShaderResourceBinding* FindBinding(const FShaderCodeView& CodeView, EShaderResourceType Type, int32* OutIndex = nullptr)
{
    const TArrayView<const FShaderResourceBinding> Bindings = CodeView.GetBindings();
    for (int32 Index = 0; Index < Bindings.Size(); ++Index)
    {
        if (Bindings[Index].Type == Type && Bindings[Index].Space != EShaderBindingSpace::ShaderConstants)
        {
            if (OutIndex)
            {
                *OutIndex = Index;
            }

            return &Bindings[Index];
        }
    }

    return nullptr;
}

bool ShaderSourceHashLineEndings_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("CRLF and LF contents hash the same"));
    TEST_EXPECT_EQ(ShaderSourceHash::Compute(ToBytes(TEXT("float4 A;\r\nfloat4 B;\r\n"))), ShaderSourceHash::Compute(ToBytes(TEXT("float4 A;\nfloat4 B;\n"))));

    TEST_SECTION(TEXT("Different contents hash differently"));
    TEST_EXPECT(ShaderSourceHash::Compute(ToBytes(TEXT("float4 A;\n"))) != ShaderSourceHash::Compute(ToBytes(TEXT("float4 B;\n"))));

    TEST_SECTION(TEXT("Empty contents hash to zero"));
    TEST_EXPECT_EQ(ShaderSourceHash::Compute(TArray<uint8>()), 0u);

    TEST_SECTION(TEXT("NormalizeLineEndings removes every carriage return"));
    TArray<uint8> Contents = ToBytes(TEXT("A\r\nB\rC\n"));
    ShaderSourceHash::NormalizeLineEndings(Contents);
    TEST_EXPECT(Contents == ToBytes(TEXT("A\nBC\n")));

    TEST_END();
}

bool ShaderPreprocessorMemoryProvider_Test()
{
    TEST_BEGIN();

    FShaderCompiler& Compiler = FShaderCompiler::Get();
    const FString    AssetDir = Compiler.GetAssetPath();

    FShaderDefine Defines[] = { FShaderDefine(TEXT("CLEAR_ELEMENT_UINT"), TEXT("1")) };
    const FShaderCompileInfo CompileInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

    TArray<FShaderSourceFile> Sources;
    FString                   Errors;
    TEST_SECTION(TEXT("CollectSources finds the shader and its includes"));
    TEST_EXPECT(Compiler.CollectSources(GetClearBufferSource(), CompileInfo, Sources, Errors));
    TEST_EXPECT(Sources.Size() >= 2);

    TEST_SECTION(TEXT("A shader outside the asset directory is named after its own folder"));
    TEST_EXPECT(!Sources.IsEmpty() && Sources[0].Path == TEXT("@Source/ClearBufferUAV.hlsl"));
    TEST_EXPECT(!Sources.IsEmpty() && Sources[0].LocalPath == GetClearBufferSource());

    FMemorySourceProvider Memory;
    for (const FShaderSourceFile& Source : Sources)
    {
        TArray<CHAR_T> Text;
        TEST_EXPECT(FDiskShaderSourceProvider::Get().ReadFile(Source.LocalPath, Text));
        Memory.AddFile(Source.LocalPath, Text);
    }

    TEST_SECTION(TEXT("The asset shaders it includes keep their asset-relative names"));
    bool bFoundAssetInclude = false;
    for (const FShaderSourceFile& Source : Sources)
    {
        bFoundAssetInclude |= Source.Path == TEXT("Shaders/CoreDefines.hlsli");
    }

    TEST_EXPECT(bFoundAssetInclude);

    const FString RootPath = GetClearBufferSource();
    TArray<CHAR_T> RootText;
    TEST_EXPECT(FDiskShaderSourceProvider::Get().ReadFile(RootPath, RootText));

    FShaderPreprocessor DiskPreprocessor(AssetDir + TEXT("/Shaders"));
    FShaderPreprocessor MemoryPreprocessor(AssetDir + TEXT("/Shaders"), &Memory);
    DiskPreprocessor.AddDefine(TEXT("CLEAR_ELEMENT_UINT"), TEXT("1"));
    MemoryPreprocessor.AddDefine(TEXT("CLEAR_ELEMENT_UINT"), TEXT("1"));

    FShaderPreprocessorOutput DiskOutput;
    FShaderPreprocessorOutput MemoryOutput;
    const FStringView RootView(RootText.Data(), RootText.Size());

    TEST_SECTION(TEXT("Both providers preprocess"));
    TEST_EXPECT(DiskPreprocessor.Preprocess(RootPath, RootView, DiskOutput));
    TEST_EXPECT(MemoryPreprocessor.Preprocess(RootPath, RootView, MemoryOutput));

    TEST_SECTION(TEXT("The output and the dependencies are the same"));
    TEST_EXPECT(DiskOutput.GenerateSource() == MemoryOutput.GenerateSource());
    TEST_EXPECT(DiskOutput.Dependencies == MemoryOutput.Dependencies);

    TEST_SECTION(TEXT("A file the provider does not have is an error"));
    FMemorySourceProvider Empty;
    FShaderPreprocessor   MissingPreprocessor(AssetDir + TEXT("/Shaders"), &Empty);
    FShaderPreprocessorOutput MissingOutput;
    TEST_EXPECT(!MissingPreprocessor.Preprocess(RootPath, RootView, MissingOutput));
    TEST_EXPECT(!MissingOutput.Errors.IsEmpty());

    TEST_END();
}

bool ShaderCompilerCompileJobParity_Test()
{
    TEST_BEGIN();

    FShaderCompiler& Compiler = FShaderCompiler::Get();

    for (EShaderOutputLanguage OutputLanguage : Compiler.GetSupportedOutputLanguages())
    {
        TEST_SECTION(ToString(OutputLanguage));

        FShaderDefine Defines[] = { FShaderDefine(TEXT("CLEAR_ELEMENT_UINT"), TEXT("1")) };
        const FShaderCompileInfo CompileInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, OutputLanguage, TArrayView<FShaderDefine>(Defines));

        TArray<uint8> LocalCode;
        TEST_EXPECT(Compiler.CompileFromFile(GetClearBufferSource(), CompileInfo, LocalCode));

        TArray<FShaderSourceFile> Files;
        FString                   Errors;
        TEST_EXPECT(Compiler.CollectSources(GetClearBufferSource(), CompileInfo, Files, Errors));
        if (Files.IsEmpty())
        {
            continue;
        }

        TSharedPtr<FRemoteRequestSources> Sources = MakeRequestSources(Files);

        FShaderCompileJob Job = FShaderCompileJob::FromCompileInfo(Files[0].Path, CompileInfo);
        TArray<uint8>   JobCode;
        TArray<FString> Dependencies;
        FString         Messages;
        TEST_EXPECT(Compiler.CompileJob(Job, FRemoteShaderCompilerServer::SourceRoot, *Sources, JobCode, Dependencies, Messages));

        TEST_EXPECT(!Dependencies.IsEmpty() && Dependencies[0] == Files[0].Path);
        TEST_EXPECT(JobCode == LocalCode);
    }

    TEST_SECTION(TEXT("Errors carry paths relative to the asset directory"));
    FShaderCompileJob Broken;
    Broken.SourceFile     = TEXT("Shaders/Broken.hlsl");
    Broken.EntryPoint     = TEXT("Main");
    Broken.ShaderModel    = EShaderModel::SM_6_2;
    Broken.ShaderStage    = EShaderStage::Compute;
    Broken.OutputLanguage = EShaderOutputLanguage::SPIRV;

    FRemoteRequestSources BrokenSources;
    BrokenSources.Files.Add(FString(FRemoteShaderCompilerServer::SourceRoot) + TEXT("/Shaders/Broken.hlsl"), MakeSharedPtr<TArray<uint8>>(ToBytes(TEXT("#include \"Missing.hlsli\"\n"))));

    TArray<uint8>   BrokenCode;
    TArray<FString> BrokenDependencies;
    FString         BrokenMessages;
    TEST_EXPECT(!Compiler.CompileJob(Broken, FRemoteShaderCompilerServer::SourceRoot, BrokenSources, BrokenCode, BrokenDependencies, BrokenMessages));
    TEST_EXPECT(BrokenMessages.Contains(TEXT("Shaders/Broken.hlsl")));
    TEST_EXPECT(!BrokenMessages.Contains(FRemoteShaderCompilerServer::SourceRoot));

    TEST_END();
}

bool ShaderCompileJobJsonRoundTrip_Test()
{
    TEST_BEGIN();

    FShaderCompileJob Job;
    Job.Name              = TEXT("FTestCS#3");
    Job.SourceFile        = TEXT("@Source/ClearTextureUAV.hlsl");
    Job.EntryPoint        = TEXT("Main");
    Job.ShaderModel       = EShaderModel::SM_6_6;
    Job.ShaderStage       = EShaderStage::Compute;
    Job.OutputLanguage    = EShaderOutputLanguage::MSL;
    Job.bOptimize         = false;
    Job.bDebugInfo        = true;
    Job.bHasEngineDefines = true;
    Job.Defines.Emplace(TEXT("CLEAR_DIMENSION"), TEXT("3"));
    Job.Defines.Emplace(TEXT("EMPTY"), TEXT(""));
    Job.IncludeDirs.Emplace(TEXT("@Include0"));
    Job.IncludeDirs.Emplace(TEXT("Shaders/Internal"));

    FShaderCompileJob Parsed;
    FString           Error;
    TEST_SECTION(TEXT("ToJson reads back"));
    TEST_EXPECT(FShaderCompileJob::FromJson(Job.ToJson(), Parsed, Error));
    TEST_EXPECT_EQ(Parsed.GetKey(), Job.GetKey());
    TEST_EXPECT(Parsed.Name == Job.Name);
    TEST_EXPECT(Parsed.ShaderModel == Job.ShaderModel && Parsed.OutputLanguage == Job.OutputLanguage);
    TEST_EXPECT(Parsed.bDebugInfo && !Parsed.bOptimize && Parsed.bHasEngineDefines);
    TEST_EXPECT_EQ(Parsed.Defines.Size(), 2);
    TEST_EXPECT(Parsed.IncludeDirs == Job.IncludeDirs);

    TEST_SECTION(TEXT("A job without include directories leaves the field out"));
    FShaderCompileJob NoIncludes = Job;
    NoIncludes.IncludeDirs.Clear();
    TEST_EXPECT(NoIncludes.ToJson().Find(TEXT("includeDirs")) == nullptr);
    TEST_EXPECT(NoIncludes.GetKey() != Job.GetKey());

    TEST_SECTION(TEXT("Every enum value reads back"));
    for (uint8 Value = static_cast<uint8>(EShaderStage::Vertex); Value <= static_cast<uint8>(EShaderStage::RayCallable); ++Value)
    {
        EShaderStage Stage = EShaderStage::Unknown;
        TEST_EXPECT(TryParseShaderStage(ToString(static_cast<EShaderStage>(Value)), Stage) && Stage == static_cast<EShaderStage>(Value));
    }

    for (uint8 Value = static_cast<uint8>(EShaderModel::SM_6_0); Value <= static_cast<uint8>(EShaderModel::SM_6_10); ++Value)
    {
        EShaderModel Model = EShaderModel::Unknown;
        TEST_EXPECT(TryParseShaderModel(ToString(static_cast<EShaderModel>(Value)), Model) && Model == static_cast<EShaderModel>(Value));
    }

    TEST_SECTION(TEXT("Malformed jobs are rejected"));
    FJsonValue Missing = Job.ToJson();
    Missing.RemoveMember(TEXT("entry"));
    TEST_EXPECT(!FShaderCompileJob::FromJson(Missing, Parsed, Error));

    FJsonValue BadStage = Job.ToJson();
    BadStage.AddMember(TEXT("stage"), FJsonValue(TEXT("Tessellation")));
    TEST_EXPECT(!FShaderCompileJob::FromJson(BadStage, Parsed, Error));

    TEST_SECTION(TEXT("The key ignores the name and follows the defines"));
    FShaderCompileJob Renamed = Job;
    Renamed.Name = TEXT("Other");
    TEST_EXPECT_EQ(Renamed.GetKey(), Job.GetKey());

    FShaderCompileJob Redefined = Job;
    Redefined.Defines[0].Value = TEXT("4");
    TEST_EXPECT(Redefined.GetKey() != Job.GetKey());

    TEST_END();
}

bool ShaderJobFileMerge_Test()
{
    TEST_BEGIN();

    const FString FilePath = Paths::GetEngineDir() + TEXT("/Build/RemoteShaderCompilerTests.shaderjob");

    FShaderCompileJob Job;
    Job.SourceFile     = TEXT("Shaders/Example.hlsl");
    Job.EntryPoint     = TEXT("Main");
    Job.ShaderModel    = EShaderModel::SM_6_2;
    Job.ShaderStage    = EShaderStage::Compute;
    Job.OutputLanguage = EShaderOutputLanguage::SPIRV;

    FShaderCompileJob Other = Job;
    Other.Defines.Emplace(TEXT("CLEAR_ELEMENT_UINT"), TEXT("1"));

    TEST_SECTION(TEXT("Duplicates are dropped"));
    FShaderJobFile JobFile;
    TEST_EXPECT(JobFile.Add(TEXT("Vulkan"), FShaderCompileJob(Job)));
    TEST_EXPECT(!JobFile.Add(TEXT("Vulkan"), FShaderCompileJob(Job)));
    TEST_EXPECT(!JobFile.Add(TEXT("vulkan"), FShaderCompileJob(Job)));
    TEST_EXPECT(JobFile.Add(TEXT("Vulkan"), FShaderCompileJob(Other)));
    TEST_EXPECT(JobFile.Add(TEXT("Metal"), FShaderCompileJob(Job)));
    TEST_EXPECT_EQ(JobFile.GetEntries().Size(), 3);

    TEST_SECTION(TEXT("Save and Load keep every entry"));
    TEST_EXPECT(JobFile.Save(FilePath));

    FShaderJobFile Loaded;
    FString        Error;
    TEST_EXPECT(Loaded.Load(FilePath, Error));
    TEST_EXPECT_EQ(Loaded.GetEntries().Size(), 3);

    TEST_SECTION(TEXT("Loading into a file merges without duplicates"));
    TEST_EXPECT(Loaded.Load(FilePath, Error));
    TEST_EXPECT_EQ(Loaded.GetEntries().Size(), 3);
    TEST_EXPECT(!Loaded.Add(TEXT("Metal"), FShaderCompileJob(Job)));

    TEST_SECTION(TEXT("Filter selects by RHI"));
    const FString VulkanName = TEXT("VULKAN");
    TEST_EXPECT_EQ(Loaded.Filter(TArrayView<const FString>(&VulkanName, 1)).Size(), 2);
    TEST_EXPECT_EQ(Loaded.Filter(TArrayView<const FString>()).Size(), 3);

    TEST_SECTION(TEXT("A missing file is not an error"));
    FShaderJobFile Missing;
    TEST_EXPECT(Missing.Load(Paths::GetEngineDir() + TEXT("/Build/DoesNotExist.shaderjob"), Error));
    TEST_EXPECT(Missing.GetEntries().IsEmpty());

    TEST_END();
}

bool RemoteProtocolFraming_Test()
{
    TEST_BEGIN();

    using FProtocol = RemoteShaderCompilerProtocol;

    FProtocol::FFrame First;
    First.Type      = FProtocol::EMessageType::CompileRequest;
    First.RequestId = 7;
    First.Header    = FJsonValue::MakeObject();
    First.Header.AddMember(TEXT("hops"), FJsonValue(0));
    First.Payload   = ToBytes(TEXT("payload bytes"));

    FProtocol::FFrame Second;
    Second.Type      = FProtocol::EMessageType::Ping;
    Second.RequestId = 8;

    TArray<uint8> Bytes;
    FProtocol::EncodeFrame(First, Bytes);
    FProtocol::EncodeFrame(Second, Bytes);

    TEST_SECTION(TEXT("A split read only yields the frame once it is complete"));
    TArray<uint8> Buffer;
    int32 NumDecoded = 0;
    for (int32 Index = 0; Index < Bytes.Size(); ++Index)
    {
        Buffer.Add(Bytes[Index]);

        FProtocol::FFrame Frame;
        bool bHasFrame = false;
        TEST_EXPECT(FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame));
        if (!bHasFrame)
        {
            continue;
        }

        if (NumDecoded == 0)
        {
            TEST_EXPECT(Frame.Type == FProtocol::EMessageType::CompileRequest);
            TEST_EXPECT_EQ(Frame.RequestId, 7u);
            TEST_EXPECT(Frame.Payload == First.Payload);
            TEST_EXPECT(Frame.Header.Find(TEXT("hops")) && Frame.Header.Find(TEXT("hops"))->GetInt64Or(-1) == 0);
        }
        else
        {
            TEST_EXPECT(Frame.Type == FProtocol::EMessageType::Ping);
            TEST_EXPECT_EQ(Frame.RequestId, 8u);
        }

        NumDecoded++;
    }

    TEST_EXPECT_EQ(NumDecoded, 2);
    TEST_EXPECT(Buffer.IsEmpty());

    TEST_SECTION(TEXT("Two frames in one buffer decode one at a time"));
    Buffer = Bytes;
    FProtocol::FFrame Frame;
    bool bHasFrame = false;
    TEST_EXPECT(FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame) && bHasFrame && Frame.RequestId == 7);
    TEST_EXPECT(FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame) && bHasFrame && Frame.RequestId == 8);
    TEST_EXPECT(FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame) && !bHasFrame);

    TEST_SECTION(TEXT("A bad magic, an unknown type or an oversized frame is rejected"));
    FProtocol::FFrameHeader Header;
    Header.Magic = 0x12345678;
    Buffer.Clear();
    Buffer.Append(reinterpret_cast<const uint8*>(&Header), static_cast<int32>(sizeof(Header)));
    TEST_EXPECT(!FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame));

    Header = FProtocol::FFrameHeader();
    Header.Type = static_cast<FProtocol::EMessageType>(99);
    Buffer.Clear();
    Buffer.Append(reinterpret_cast<const uint8*>(&Header), static_cast<int32>(sizeof(Header)));
    TEST_EXPECT(!FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame));

    Header = FProtocol::FFrameHeader();
    Header.PayloadSize = FProtocol::MaxPayloadSize + 1;
    Buffer.Clear();
    Buffer.Append(reinterpret_cast<const uint8*>(&Header), static_cast<int32>(sizeof(Header)));
    TEST_EXPECT(!FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame));

    TEST_SECTION(TEXT("A header that is not a JSON object is rejected"));
    Header = FProtocol::FFrameHeader();
    Header.HeaderSize = 3;
    Buffer.Clear();
    Buffer.Append(reinterpret_cast<const uint8*>(&Header), static_cast<int32>(sizeof(Header)));
    Buffer.Append(ToBytes(TEXT("[1]")));
    TEST_EXPECT(!FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame));

    TEST_END();
}

bool RemoteProtocolSourcePaths_Test()
{
    TEST_BEGIN();

    using FProtocol = RemoteShaderCompilerProtocol;

    TEST_SECTION(TEXT("Relative paths of plain names are valid, whichever folder they name"));
    TEST_EXPECT(FProtocol::IsValidSourcePath(TEXT("Shaders/Common.hlsli")));
    TEST_EXPECT(FProtocol::IsValidSourcePath(TEXT("@Source/ClearBufferUAV.hlsl")));
    TEST_EXPECT(FProtocol::IsValidSourcePath(TEXT("@Include0")));
    TEST_EXPECT(FProtocol::IsValidSourcePath(TEXT("@Include1/Sub/Common.hlsli")));

    TEST_SECTION(TEXT("Paths that could leave the server's source root are rejected"));
    TEST_EXPECT(!FProtocol::IsValidSourcePath(TEXT("")));
    TEST_EXPECT(!FProtocol::IsValidSourcePath(TEXT("Shaders/../Engine.ini")));
    TEST_EXPECT(!FProtocol::IsValidSourcePath(TEXT("@Source/../../Core/Core.h")));
    TEST_EXPECT(!FProtocol::IsValidSourcePath(TEXT("Shaders/./Common.hlsli")));
    TEST_EXPECT(!FProtocol::IsValidSourcePath(TEXT("Shaders//Common.hlsli")));
    TEST_EXPECT(!FProtocol::IsValidSourcePath(TEXT("/Shaders/Common.hlsli")));
    TEST_EXPECT(!FProtocol::IsValidSourcePath(TEXT("C:/Shaders/Common.hlsli")));
    TEST_EXPECT(!FProtocol::IsValidSourcePath(TEXT("Shaders\\Common.hlsli")));

    TEST_SECTION(TEXT("CollapsePath drops '.' and resolves '..'"));
    TEST_EXPECT(FProtocol::CollapsePath(TEXT("/Root/Assets/Shaders/Sub/../Common.hlsli")) == TEXT("/Root/Assets/Shaders/Common.hlsli"));
    TEST_EXPECT(FProtocol::CollapsePath(TEXT("Shaders/./Common.hlsli")) == TEXT("Shaders/Common.hlsli"));
    TEST_EXPECT(FProtocol::CollapsePath(TEXT("C:\\Assets\\Shaders\\Common.hlsli")) == TEXT("C:/Assets/Shaders/Common.hlsli"));
    TEST_EXPECT(FProtocol::CollapsePath(TEXT("/../Escape")) == TEXT("/../Escape"));

    TEST_END();
}

bool RemoteShaderCompilerLoopback_Test()
{
    TEST_BEGIN();

    FShaderCompiler& Compiler = FShaderCompiler::Get();

    FRemoteShaderCompilerServerSettings Settings;
    Settings.Port = 0;

    FRemoteShaderCompilerServer Server(Settings);
    TEST_SECTION(TEXT("The server listens on loopback"));
    if (!Server.Launch())
    {
        TEST_EXPECT(false);
        TEST_END();
    }

    TEST_EXPECT(Server.GetPort() != 0);

    TEST_SECTION(TEXT("The client connects and reads the Hello"));
    FRemoteShaderCompilerClient Client;
    TEST_EXPECT(Client.Connect(RemoteShaderCompilerProtocol::LoopbackAddress, Server.GetPort(), GTestTimeoutMs));
    TEST_EXPECT(Client.IsConnected());
    TEST_EXPECT(Client.CanCompile(EShaderOutputLanguage::SPIRV));
    TEST_EXPECT(Client.GetPeerName().Contains(TEXT("127.0.0.1")));
    TEST_EXPECT(Client.GetIdentity(EShaderOutputLanguage::SPIRV).HasValue() && Client.GetIdentity(EShaderOutputLanguage::SPIRV)->GetHash() == Compiler.GetLocalIdentity(EShaderOutputLanguage::SPIRV).GetHash());

    FShaderDefine Defines[] = { FShaderDefine(TEXT("CLEAR_ELEMENT_UINT"), TEXT("1")) };
    const FShaderCompileInfo CompileInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

    TEST_SECTION(TEXT("A remote compile returns the container a local compile produces"));
    TArray<uint8>   LocalCode;
    TArray<uint8>   RemoteCode;
    TArray<FString> Dependencies;
    TEST_EXPECT(Compiler.CompileFromFile(GetClearBufferSource(), CompileInfo, LocalCode));
    TEST_EXPECT(Compiler.CompileOnRemote(Client, GetClearBufferSource(), CompileInfo, RemoteCode, &Dependencies) == ERemoteCompileStatus::Succeeded);
    TEST_EXPECT(RemoteCode == LocalCode);

    TEST_SECTION(TEXT("The server's dependencies are turned back into paths on this machine"));
    TEST_EXPECT(!Dependencies.IsEmpty() && RemoteShaderCompilerProtocol::CollapsePath(Dependencies[0]) == RemoteShaderCompilerProtocol::CollapsePath(GetClearBufferSource()));

    bool bFoundAssetDependency = false;
    for (const FString& Dependency : Dependencies)
    {
        bFoundAssetDependency |= RemoteShaderCompilerProtocol::CollapsePath(Dependency) == RemoteShaderCompilerProtocol::CollapsePath(Compiler.GetAssetPath() + TEXT("/Shaders/CoreDefines.hlsli"));
    }

    TEST_EXPECT(bFoundAssetDependency);

    TEST_SECTION(TEXT("A second compile on the same connection still succeeds"));
    TEST_EXPECT(Compiler.CompileOnRemote(Client, GetClearBufferSource(), CompileInfo, RemoteCode, nullptr) == ERemoteCompileStatus::Succeeded);

    TEST_SECTION(TEXT("A shader error is a compile failure, not an unreachable server"));
    const FShaderCompileInfo MissingEntry(TEXT("DoesNotExist"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));
    TEST_EXPECT(Compiler.CompileOnRemote(Client, GetClearBufferSource(), MissingEntry, RemoteCode, nullptr) == ERemoteCompileStatus::CompileFailed);

    TEST_SECTION(TEXT("A language the server cannot produce is reported as unreachable"));
    constexpr EShaderOutputLanguage Languages[] = { EShaderOutputLanguage::DXIL, EShaderOutputLanguage::DXBC, EShaderOutputLanguage::MSL };
    for (EShaderOutputLanguage OutputLanguage : Languages)
    {
        if (Compiler.CanCompileLocally(OutputLanguage))
        {
            continue;
        }

        TEST_EXPECT(!Client.CanCompile(OutputLanguage));

        const FShaderCompileInfo Unserved(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, OutputLanguage, TArrayView<FShaderDefine>(Defines));
        TEST_EXPECT(Compiler.CompileOnRemote(Client, GetClearBufferSource(), Unserved, RemoteCode, nullptr) == ERemoteCompileStatus::Unreachable);
    }

    TEST_SECTION(TEXT("Shutting the server down disconnects the client"));
    Server.Shutdown();

    const uint64 Deadline = GetTestMilliseconds() + GTestTimeoutMs;
    while (Client.IsConnected() && GetTestMilliseconds() < Deadline)
    {
        FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(10));
    }

    TEST_EXPECT(!Client.IsConnected());
    TEST_EXPECT(Compiler.CompileOnRemote(Client, GetClearBufferSource(), CompileInfo, RemoteCode, nullptr) == ERemoteCompileStatus::Unreachable);

    Client.Disconnect();
    TEST_END();
}

bool RemoteShaderCompilerSendsFilesOnce_Test()
{
    TEST_BEGIN();

    using FProtocol = RemoteShaderCompilerProtocol;

    FShaderCompiler& Compiler = FShaderCompiler::Get();

    FRemoteShaderCompilerServerSettings Settings;
    Settings.Port = 0;

    FRemoteShaderCompilerServer Server(Settings);
    if (!Server.Launch())
    {
        TEST_EXPECT(false);
        TEST_END();
    }

    FNetworkSocket Socket(ESocketType::TCP);
    TEST_SECTION(TEXT("A raw socket connects and receives the Hello"));
    TEST_EXPECT(Socket.ConnectToHost(FProtocol::LoopbackAddress, Server.GetPort(), GTestTimeoutMs));

    TArray<uint8>     Buffer;
    FProtocol::FFrame Frame;
    TEST_EXPECT(ReadFrame(Socket, Buffer, Frame) && Frame.Type == FProtocol::EMessageType::Hello);

    FShaderDefine Defines[] = { FShaderDefine(TEXT("CLEAR_ELEMENT_SINT"), TEXT("1")) };
    const FShaderCompileInfo CompileInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

    TArray<FShaderSourceFile> Files;
    FString                   Errors;
    TEST_EXPECT(Compiler.CollectSources(GetClearBufferSource(), CompileInfo, Files, Errors));
    if (Files.IsEmpty())
    {
        Server.Shutdown();
        TEST_END();
    }

    const FShaderCompileJob Job = FShaderCompileJob::FromCompileInfo(Files[0].Path, CompileInfo);

    const auto SendRequest = [&](uint32 RequestId, bool bWithContents) -> FString
    {
        FProtocol::FFrame Request;
        Request.Type      = FProtocol::EMessageType::CompileRequest;
        Request.RequestId = RequestId;
        Request.Header    = FJsonValue::MakeObject();
        Request.Header.AddMember(TEXT("hops"), FJsonValue(0));
        Request.Header.AddMember(TEXT("job"), Job.ToJson());
        AddFileRefs(Files, bWithContents, Request);

        TArray<uint8> Bytes;
        FProtocol::EncodeFrame(Request, Bytes);

        FProtocol::FFrame Result;
        if (!SendAll(Socket, Bytes) || !ReadFrame(Socket, Buffer, Result) || Result.RequestId != RequestId)
        {
            return FString(TEXT("no result"));
        }

        const FJsonValue* Status = Result.Header.Find(TEXT("status"));
        return Status ? Status->GetStringOr(TEXT("")) : FString();
    };

    TEST_SECTION(TEXT("Referencing files that were never sent fails"));
    TEST_EXPECT(SendRequest(1, false) == FProtocol::StatusFailed);

    TEST_SECTION(TEXT("Sending the files once lets later requests only reference them"));
    TEST_EXPECT(SendRequest(2, true) == FProtocol::StatusSucceeded);
    TEST_EXPECT(SendRequest(3, false) == FProtocol::StatusSucceeded);

    TEST_SECTION(TEXT("Contents that do not match their hash are rejected"));
    TArray<FShaderSourceFile> Tampered = Files;
    Tampered[0].Contents.Add(static_cast<uint8>(' '));
    {
        FProtocol::FFrame Request;
        Request.Type      = FProtocol::EMessageType::CompileRequest;
        Request.RequestId = 4;
        Request.Header    = FJsonValue::MakeObject();
        Request.Header.AddMember(TEXT("job"), Job.ToJson());
        AddFileRefs(Tampered, true, Request);

        TArray<uint8> Bytes;
        FProtocol::EncodeFrame(Request, Bytes);

        FProtocol::FFrame Result;
        TEST_EXPECT(SendAll(Socket, Bytes) && ReadFrame(Socket, Buffer, Result));
        TEST_EXPECT(Result.Header.Find(TEXT("status")) && Result.Header.Find(TEXT("status"))->GetStringOr(TEXT("")) == FProtocol::StatusFailed);
    }

    Socket.Close();
    Server.Shutdown();
    TEST_END();
}

bool ShaderCodeReadEmbedded_Test()
{
    TEST_BEGIN();

    FShaderDefine Defines[] = { FShaderDefine(TEXT("CLEAR_ELEMENT_UINT"), TEXT("1")) };
    const FShaderCompileInfo CompileInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

    TArray<uint8> ShaderCode;
    TEST_EXPECT(FShaderCompiler::Get().CompileFromFile(GetClearBufferSource(), CompileInfo, ShaderCode));

    TArray<uint8> Misaligned;
    Misaligned.Add(0);
    Misaligned.Append(ShaderCode);

    FShaderCodeView Original;
    FShaderCodeView Embedded;
    TArray<uint8>   Storage;
    FString         Error;
    TEST_SECTION(TEXT("ReadEmbedded reads a misaligned copy"));
    TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, Original));
    TEST_EXPECT(FShaderCodeReader::ReadEmbedded(TArrayView<const uint8>(Misaligned.Data() + 1, ShaderCode.Size()), Storage, Embedded, &Error));
    TEST_EXPECT(Storage == ShaderCode);
    TEST_EXPECT_EQ(Embedded.GetBindings().Size(), Original.GetBindings().Size());
    TEST_EXPECT_EQ(Embedded.GetNativeCode().Size(), Original.GetNativeCode().Size());
    TEST_EXPECT_EQ(Embedded.GetInfo().ShaderConstantsSize, Original.GetInfo().ShaderConstantsSize);

    TEST_SECTION(TEXT("ReadEmbedded rejects garbage"));
    const TArray<uint8> Garbage = ToBytes(TEXT("not a shader container at all"));
    TEST_EXPECT(!FShaderCodeReader::ReadEmbedded(TArrayView<const uint8>(Garbage.Data(), Garbage.Size()), Storage, Embedded, &Error));

    TEST_END();
}

bool InternalClearBufferUAVReflection_Test()
{
    TEST_BEGIN();

    const CHAR_T* const Permutations[][2] =
    {
        { TEXT("CLEAR_ELEMENT_UINT"), TEXT("0") },
        { TEXT("CLEAR_ELEMENT_UINT"), TEXT("1") },
        { TEXT("CLEAR_ELEMENT_SINT"), TEXT("1") },
    };

    for (const auto& Permutation : Permutations)
    {
        TEST_SECTION(Permutation[0]);

        FShaderDefine Defines[] = { FShaderDefine(Permutation[0], Permutation[1]) };
        const FShaderCompileInfo CompileInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

        TArray<uint8>   ShaderCode;
        FShaderCodeView CodeView;
        TEST_EXPECT(FShaderCompiler::Get().CompileFromFile(GetClearBufferSource(), CompileInfo, ShaderCode));
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));

        TEST_EXPECT_EQ(CodeView.GetInfo().ShaderConstantsSize, static_cast<uint8>(sizeof(uint32) * 5));
        TEST_EXPECT_EQ(CodeView.GetBindings().Size(), 1);
        TEST_EXPECT(FString(CodeView.GetEntryPoint()) == TEXT("Main"));

        const FShaderResourceBinding* Output = FindBinding(CodeView, EShaderResourceType::RWTypedBuffer);
        TEST_EXPECT(Output != nullptr);
        TEST_EXPECT(Output && Output->Dimension == EShaderResourceDimension::Buffer && Output->Register == 0 && Output->Space == EShaderBindingSpace::Global);
        TEST_EXPECT_EQ(CodeView.GetSpirvOffsets().Size(), CodeView.GetBindings().Size());
    }

    TEST_END();
}

bool InternalClearUAVMSLReflection_Test()
{
    TEST_BEGIN();

    FShaderCompiler& Compiler = FShaderCompiler::Get();

    if (!Compiler.IsOutputLanguageSupported(EShaderOutputLanguage::MSL))
    {
        TEST_END();
    }

    struct FComponentCase
    {
        const CHAR_T*        Uint;
        const CHAR_T*        Sint;
        EMSLTextureComponent Component;
    };

    const FComponentCase Components[] =
    {
        { TEXT("0"), TEXT("0"), EMSLTextureComponent::Float },
        { TEXT("1"), TEXT("0"), EMSLTextureComponent::Uint  },
        { TEXT("0"), TEXT("1"), EMSLTextureComponent::Int   },
    };

    const auto ExpectConstants = [&](const FShaderCodeView& CodeView, uint8 ExpectedSize)
    {
        TEST_EXPECT_EQ(CodeView.GetInfo().ShaderConstantsSize, ExpectedSize);

        bool bFoundConstants = false;
        for (const FShaderResourceBinding& Binding : CodeView.GetBindings())
        {
            bFoundConstants |= Binding.Type == EShaderResourceType::ConstantBuffer && Binding.Space == EShaderBindingSpace::ShaderConstants;
        }

        TEST_EXPECT(bFoundConstants);
        TEST_EXPECT_EQ(CodeView.GetMSLSlots().Size(), CodeView.GetBindings().Size());
    };

    for (const FComponentCase& Case : Components)
    {
        TEST_SECTION(TEXT("ClearBufferUAV"));

        FShaderDefine Defines[] = { FShaderDefine(TEXT("CLEAR_ELEMENT_UINT"), Case.Uint), FShaderDefine(TEXT("CLEAR_ELEMENT_SINT"), Case.Sint) };
        const FShaderCompileInfo CompileInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::MSL, TArrayView<FShaderDefine>(Defines));

        TArray<uint8>   ShaderCode;
        FShaderCodeView CodeView;
        TEST_EXPECT(Compiler.CompileFromFile(GetMetalClearBufferSource(), CompileInfo, ShaderCode));
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));

        ExpectConstants(CodeView, static_cast<uint8>(sizeof(uint32) * 5));
        TEST_EXPECT(CodeView.GetMSLInfo().ThreadGroupSize[0] == 64 && CodeView.GetMSLInfo().ThreadGroupSize[1] == 1 && CodeView.GetMSLInfo().ThreadGroupSize[2] == 1);

        int32 OutputIndex = -1;
        const FShaderResourceBinding* Output = FindBinding(CodeView, EShaderResourceType::RWTypedBuffer, &OutputIndex);
        TEST_EXPECT(Output && Output->Dimension == EShaderResourceDimension::Buffer);
        TEST_EXPECT(OutputIndex >= 0 && CodeView.GetMSLSlots()[OutputIndex].NullTextureType == MakeMSLNullTextureType(EMSLTextureDimension::TextureBuffer, Case.Component));
    }

    {
        TEST_SECTION(TEXT("ClearBufferUAV Untyped"));

        FShaderDefine Defines[] = { FShaderDefine(TEXT("CLEAR_ELEMENT_UNTYPED"), TEXT("1")) };
        const FShaderCompileInfo CompileInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::MSL, TArrayView<FShaderDefine>(Defines));

        TArray<uint8>   ShaderCode;
        FShaderCodeView CodeView;
        TEST_EXPECT(Compiler.CompileFromFile(GetMetalClearBufferSource(), CompileInfo, ShaderCode));
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));

        ExpectConstants(CodeView, static_cast<uint8>(sizeof(uint32) * 5));

        bool bFoundOutput = false;
        for (const FShaderResourceBinding& Binding : CodeView.GetBindings())
        {
            bFoundOutput |= GetMSLBindingType(Binding) == EMSLBindingType::UnorderedAccessBuffer && Binding.Register == 0;
        }

        TEST_EXPECT(bFoundOutput);
    }

    struct FDimensionCase
    {
        const CHAR_T*            Value;
        EMSLTextureDimension     MSLDimension;
        EShaderResourceDimension Dimension;
    };

    const FDimensionCase Dimensions[] =
    {
        { TEXT("0"), EMSLTextureDimension::Texture1D,      EShaderResourceDimension::Texture1D      },
        { TEXT("1"), EMSLTextureDimension::Texture1DArray, EShaderResourceDimension::Texture1DArray },
        { TEXT("2"), EMSLTextureDimension::Texture2D,      EShaderResourceDimension::Texture2D      },
        { TEXT("3"), EMSLTextureDimension::Texture2DArray, EShaderResourceDimension::Texture2DArray },
        { TEXT("4"), EMSLTextureDimension::Texture3D,      EShaderResourceDimension::Texture3D      },
    };

    for (const FDimensionCase& Dimension : Dimensions)
    {
        for (const FComponentCase& Case : Components)
        {
            TEST_SECTION(TEXT("ClearTextureUAV"));

            FShaderDefine Defines[] =
            {
                FShaderDefine(TEXT("CLEAR_DIMENSION"), Dimension.Value),
                FShaderDefine(TEXT("CLEAR_ELEMENT_UINT"), Case.Uint),
                FShaderDefine(TEXT("CLEAR_ELEMENT_SINT"), Case.Sint),
            };

            const FShaderCompileInfo CompileInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::MSL, TArrayView<FShaderDefine>(Defines));

            TArray<uint8>   ShaderCode;
            FShaderCodeView CodeView;
            TEST_EXPECT(Compiler.CompileFromFile(GetClearTextureSource(), CompileInfo, ShaderCode));
            TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));

            ExpectConstants(CodeView, static_cast<uint8>(sizeof(uint32) * 4));
            TEST_EXPECT(CodeView.GetMSLInfo().ThreadGroupSize[0] == 8 && CodeView.GetMSLInfo().ThreadGroupSize[1] == 8 && CodeView.GetMSLInfo().ThreadGroupSize[2] == 1);

            int32 OutputIndex = -1;
            const FShaderResourceBinding* Output = FindBinding(CodeView, EShaderResourceType::RWTexture, &OutputIndex);
            TEST_EXPECT(Output && Output->Dimension == Dimension.Dimension);
            TEST_EXPECT(OutputIndex >= 0 && CodeView.GetMSLSlots()[OutputIndex].NullTextureType == MakeMSLNullTextureType(Dimension.MSLDimension, Case.Component));
        }
    }

    TEST_END();
}

static constexpr const CHAR_T* GIncludeTestShader =
    TEXT("#include \"CoreDefines.hlsli\"\n"
    "#include \"Common.hlsli\"\n"
    "#include \"OnlySecond.hlsli\"\n"
    "#include \"Local.hlsli\"\n"
    "\n"
    "TEXTURE_FORMAT_UNKNOWN RWBuffer<uint4> OutputBuffer : register(u0);\n"
    "\n"
    "[numthreads(1, 1, 1)]\n"
    "void Main(uint3 DispatchThreadID : SV_DispatchThreadID)\n"
    "{\n"
    "    OutputBuffer[DispatchThreadID.x] = uint4(COMMON_VALUE, SECOND_VALUE, LOCAL_VALUE, 0);\n"
    "}\n");

bool ShaderCompilerIncludeDirs_Test()
{
    TEST_BEGIN();

    FShaderCompiler& Compiler = FShaderCompiler::Get();

    const FString RootDir    = Paths::GetEngineDir() + TEXT("/Build/RemoteShaderCompilerTests/IncludeDirs");
    const FString ShaderPath = RootDir + TEXT("/Shader/Main.hlsl");
    const FString FirstDir   = RootDir + TEXT("/First");
    const FString SecondDir  = RootDir + TEXT("/Second");

    const FString TestFiles[] =
    {
        ShaderPath,
        RootDir + TEXT("/Shader/Local.hlsli"),
        FirstDir + TEXT("/Common.hlsli"),
        SecondDir + TEXT("/Common.hlsli"),
        SecondDir + TEXT("/OnlySecond.hlsli"),
    };

    TEST_SECTION(TEXT("The test files are written"));
    TEST_EXPECT(WriteTestFile(TestFiles[0], GIncludeTestShader));
    TEST_EXPECT(WriteTestFile(TestFiles[1], TEXT("#define LOCAL_VALUE 3\n")));
    TEST_EXPECT(WriteTestFile(TestFiles[2], TEXT("#define COMMON_VALUE 1\n")));
    TEST_EXPECT(WriteTestFile(TestFiles[3], TEXT("#define COMMON_VALUE 2\n")));
    TEST_EXPECT(WriteTestFile(TestFiles[4], TEXT("#define SECOND_VALUE 4\n")));

    const FShaderCompileInfo BaseInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV);

    TEST_SECTION(TEXT("Without the include directories the includes are not found"));
    TArray<uint8> MissingCode;
    TEST_EXPECT(!Compiler.CompileFromFile(ShaderPath, BaseInfo, MissingCode));

    const FString      IncludeDirs[] = { FirstDir, SecondDir };
    FShaderCompileInfo CompileInfo   = BaseInfo;
    CompileInfo.IncludeDirs = TArrayView<const FString>(IncludeDirs);

    const auto Contains = [](const TArray<FString>& Paths, const FString& Path)
    {
        bool bFound = false;
        for (const FString& Candidate : Paths)
        {
            bFound |= RemoteShaderCompilerProtocol::CollapsePath(Candidate) == RemoteShaderCompilerProtocol::CollapsePath(Path);
        }

        return bFound;
    };

    TEST_SECTION(TEXT("The include directories are searched in order, after the shader's folder"));
    TArray<uint8>   LocalCode;
    TArray<FString> Dependencies;
    TEST_EXPECT(Compiler.CompileFromFile(ShaderPath, CompileInfo, LocalCode, &Dependencies));
    TEST_EXPECT(Contains(Dependencies, FirstDir + TEXT("/Common.hlsli")));
    TEST_EXPECT(!Contains(Dependencies, SecondDir + TEXT("/Common.hlsli")));
    TEST_EXPECT(Contains(Dependencies, SecondDir + TEXT("/OnlySecond.hlsli")));
    TEST_EXPECT(Contains(Dependencies, RootDir + TEXT("/Shader/Local.hlsli")));

    TEST_SECTION(TEXT("The asset shaders are still searched"));
    TEST_EXPECT(Contains(Dependencies, Compiler.GetAssetPath() + TEXT("/Shaders/CoreDefines.hlsli")));

    const auto CollectNames = [&](const FShaderCompileInfo& Info, TArray<FShaderSourceFile>& OutFiles)
    {
        FString Errors;
        const bool bCollected = Compiler.CollectSources(ShaderPath, Info, OutFiles, Errors);

        TArray<FString> Names;
        for (const FShaderSourceFile& File : OutFiles)
        {
            Names.Add(File.Path);
        }

        return bCollected ? Names : TArray<FString>();
    };

    TEST_SECTION(TEXT("CollectSources names every file after the folder it was found in"));
    TArray<FShaderSourceFile> Files;
    const TArray<FString>     Names = CollectNames(CompileInfo, Files);
    TEST_EXPECT(!Names.IsEmpty() && Names[0] == TEXT("@Source/Main.hlsl"));
    TEST_EXPECT(Names.Contains(TEXT("@Source/Local.hlsli")));
    TEST_EXPECT(Names.Contains(TEXT("@Include0/Common.hlsli")));
    TEST_EXPECT(Names.Contains(TEXT("@Include1/OnlySecond.hlsli")));
    TEST_EXPECT(Names.Contains(TEXT("Shaders/CoreDefines.hlsli")));

    TEST_SECTION(TEXT("A server compiles the named files into the same container"));
    {
        FShaderCompileJob Job = FShaderCompileJob::FromCompileInfo(TEXT("@Source/Main.hlsl"), CompileInfo);
        Job.IncludeDirs.Clear();
        Job.IncludeDirs.Add(TEXT("@Include0"));
        Job.IncludeDirs.Add(TEXT("@Include1"));

        TSharedPtr<FRemoteRequestSources> Sources = MakeRequestSources(Files);
        TArray<uint8>   JobCode;
        TArray<FString> JobDependencies;
        FString         Messages;
        TEST_EXPECT(Compiler.CompileJob(Job, FRemoteShaderCompilerServer::SourceRoot, *Sources, JobCode, JobDependencies, Messages));
        TEST_EXPECT(JobCode == LocalCode);
        TEST_EXPECT(JobDependencies.Contains(TEXT("@Include0/Common.hlsli")));
    }

    TEST_SECTION(TEXT("A folder inside another one is named below the outer one"));
    {
        const FString      NestedDirs[] = { RootDir, FirstDir, SecondDir };
        FShaderCompileInfo NestedInfo   = BaseInfo;
        NestedInfo.IncludeDirs = TArrayView<const FString>(NestedDirs);

        TArray<FShaderSourceFile> NestedFiles;
        const TArray<FString>     NestedNames = CollectNames(NestedInfo, NestedFiles);
        TEST_EXPECT(!NestedNames.IsEmpty() && NestedNames[0] == TEXT("@Include0/Shader/Main.hlsl"));
        TEST_EXPECT(NestedNames.Contains(TEXT("@Include0/Shader/Local.hlsli")));
        TEST_EXPECT(NestedNames.Contains(TEXT("@Include0/First/Common.hlsli")));
        TEST_EXPECT(NestedNames.Contains(TEXT("@Include0/Second/OnlySecond.hlsli")));

        FShaderCompileJob Job = FShaderCompileJob::FromCompileInfo(TEXT("@Include0/Shader/Main.hlsl"), NestedInfo);
        Job.IncludeDirs.Clear();
        Job.IncludeDirs.Add(TEXT("@Include0"));
        Job.IncludeDirs.Add(TEXT("@Include0/First"));
        Job.IncludeDirs.Add(TEXT("@Include0/Second"));

        TSharedPtr<FRemoteRequestSources> Sources = MakeRequestSources(NestedFiles);
        TArray<uint8>   JobCode;
        TArray<FString> JobDependencies;
        FString         Messages;
        TEST_EXPECT(Compiler.CompileJob(Job, FRemoteShaderCompilerServer::SourceRoot, *Sources, JobCode, JobDependencies, Messages));
        TEST_EXPECT(JobCode == LocalCode);
    }

    TEST_SECTION(TEXT("The asset directory itself has no name to send"));
    {
        const FString      AssetDirs[] = { Compiler.GetAssetPath() };
        FShaderCompileInfo AssetInfo   = BaseInfo;
        AssetInfo.IncludeDirs = TArrayView<const FString>(AssetDirs);

        TArray<FShaderSourceFile> AssetFiles;
        FString                   Errors;
        TEST_EXPECT(!Compiler.CollectSources(ShaderPath, AssetInfo, AssetFiles, Errors));
        TEST_EXPECT(!Errors.IsEmpty());
    }

    TEST_SECTION(TEXT("A remote compile with include directories matches the local one"));
    {
        FRemoteShaderCompilerServerSettings Settings;
        Settings.Port = 0;

        FRemoteShaderCompilerServer Server(Settings);
        const bool bLaunched = Server.Launch();
        TEST_EXPECT(bLaunched);

        if (bLaunched)
        {
            FRemoteShaderCompilerClient Client;
            TEST_EXPECT(Client.Connect(RemoteShaderCompilerProtocol::LoopbackAddress, Server.GetPort(), GTestTimeoutMs));

            TArray<uint8>   RemoteCode;
            TArray<FString> RemoteDependencies;
            TEST_EXPECT(Compiler.CompileOnRemote(Client, ShaderPath, CompileInfo, RemoteCode, &RemoteDependencies) == ERemoteCompileStatus::Succeeded);
            TEST_EXPECT(RemoteCode == LocalCode);
            TEST_EXPECT(Contains(RemoteDependencies, FirstDir + TEXT("/Common.hlsli")));
            TEST_EXPECT(Contains(RemoteDependencies, RootDir + TEXT("/Shader/Local.hlsli")));

            Client.Disconnect();
            Server.Shutdown();
        }
    }

    for (const FString& TestFile : TestFiles)
    {
        FPlatformFile::DeleteFile(*TestFile);
    }

    FPlatformFile::RemoveDirectory(*(RootDir + TEXT("/Shader")));
    FPlatformFile::RemoveDirectory(*FirstDir);
    FPlatformFile::RemoveDirectory(*SecondDir);
    FPlatformFile::RemoveDirectory(*RootDir);
    FPlatformFile::RemoveDirectory(*File::GetDirectoryOf(RootDir));

    TEST_END();
}

bool ShaderCompilerHashIgnoresAssetPath_Test()
{
    TEST_BEGIN();

    FShaderDefine Defines[] = { FShaderDefine(TEXT("CLEAR_ELEMENT_UINT"), TEXT("1")) };
    const FShaderCompileInfo CompileInfo(TEXT("Main"), EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

    const FString                 SourceFile = TEXT("Shaders/Example.hlsl");
    const FString                 AssetDir   = FShaderCompiler::Get().GetAssetPath();
    const FShaderCompilerIdentity Identity   = FShaderCompiler::Get().GetLocalIdentity(EShaderOutputLanguage::SPIRV);
    const uint64                  Hash       = FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo);

    TEST_SECTION(TEXT("The routed hash uses the local identity"));
    TEST_EXPECT(Identity.IsValid());
    TEST_EXPECT_EQ(Hash, FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo, Identity));

    TEST_SECTION(TEXT("A different compiler identity changes the hash"));
    FShaderCompilerIdentity Newer = Identity;
    Newer.VersionMinor++;
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo, Newer) != Hash);
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo, FShaderCompilerIdentity()) != Hash);

    TEST_SECTION(TEXT("Include directories change the hash"));
    const FString            IncludeDirs[] = { FString(TEXT("Shaders/Internal")) };
    FShaderCompileInfo       WithIncludes  = CompileInfo;
    WithIncludes.IncludeDirs = TArrayView<const FString>(IncludeDirs);
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, WithIncludes) != Hash);

    TEST_SECTION(TEXT("Another asset directory gives the same hash"));
    FShaderCompiler::Destroy();
    const bool bMoved = FShaderCompiler::Initialize(TEXT("/Some/Other/Checkout/Assets"));
    TEST_EXPECT(bMoved);
    if (bMoved)
    {
        TEST_EXPECT_EQ(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo), Hash);
        FShaderCompiler::Destroy();
    }

    TEST_EXPECT(FShaderCompiler::Initialize(AssetDir));
    TEST_END();
}
