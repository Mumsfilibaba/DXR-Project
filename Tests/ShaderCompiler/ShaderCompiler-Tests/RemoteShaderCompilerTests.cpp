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

static String GetClearBufferSource()
{
    return Paths::GetEngineDir() + "/Runtime/VulkanRHI/Shaders/Internal/ClearBufferUAV.hlsl";
}

static String GetMetalClearBufferSource()
{
    return Paths::GetEngineDir() + "/Runtime/MetalRHI/Shaders/Internal/ClearBufferUAV.hlsl";
}

static String GetClearTextureSource()
{
    return Paths::GetEngineDir() + "/Runtime/MetalRHI/Shaders/Internal/ClearTextureUAV.hlsl";
}

static bool WriteTestFile(const String& FilePath, const CHAR* Text)
{
    if (!File::CreateDirectoryTree(File::GetDirectoryOf(FilePath)))
    {
        return false;
    }

    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(FilePath);
    return FileHandle && File::WriteTextFile(FileHandle.Get(), String(Text));
}

static TArray<uint8> ToBytes(const CHAR* Text)
{
    TArray<uint8> Bytes;
    for (const CHAR* Character = Text; *Character; ++Character)
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
    void AddFile(const String& Path, const TArray<CHAR>& Text)
    {
        Files.Add(Path, Text);
    }

    NODISCARD virtual bool FileExists(const String& Path) const override final
    {
        return Files.Contains(Path);
    }

    virtual bool ReadFile(const String& Path, TArray<CHAR>& OutText) const override final
    {
        const TArray<CHAR>* Text = Files.Find(Path);
        if (!Text)
        {
            return false;
        }

        OutText = *Text;
        return true;
    }

    TMap<String, TArray<CHAR>> Files;
};

static TSharedPtr<FRemoteRequestSources> MakeRequestSources(const TArray<FShaderSourceFile>& Files)
{
    TSharedPtr<FRemoteRequestSources> Sources = MakeSharedPtr<FRemoteRequestSources>();
    for (const FShaderSourceFile& File : Files)
    {
        Sources->Files.Add(RemoteShaderCompilerProtocol::CollapsePath(String(FRemoteShaderCompilerServer::SourceRoot) + '/' + File.Path), MakeSharedPtr<TArray<uint8>>(File.Contents));
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
        FileRef.AddMember("path", FJsonValue(File.Path));
        FileRef.AddMember("hash", FJsonValue(File.Hash));
        FileRefs.Add(::Move(FileRef));

        if (bWithContents)
        {
            FJsonValue NewFile = FJsonValue::MakeObject();
            NewFile.AddMember("path", FJsonValue(File.Path));
            NewFile.AddMember("hash", FJsonValue(File.Hash));
            NewFile.AddMember("offset", FJsonValue(InOutFrame.Payload.Size()));
            NewFile.AddMember("size", FJsonValue(File.Contents.Size()));
            NewFiles.Add(::Move(NewFile));

            InOutFrame.Payload.Append(File.Contents.Data(), File.Contents.Size());
        }
    }

    InOutFrame.Header.AddMember("files", ::Move(FileRefs));
    InOutFrame.Header.AddMember("newFiles", ::Move(NewFiles));
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

    TEST_SECTION("CRLF and LF contents hash the same");
    TEST_EXPECT_EQ(ShaderSourceHash::Compute(ToBytes("float4 A;\r\nfloat4 B;\r\n")), ShaderSourceHash::Compute(ToBytes("float4 A;\nfloat4 B;\n")));

    TEST_SECTION("Different contents hash differently");
    TEST_EXPECT(ShaderSourceHash::Compute(ToBytes("float4 A;\n")) != ShaderSourceHash::Compute(ToBytes("float4 B;\n")));

    TEST_SECTION("Empty contents hash to zero");
    TEST_EXPECT_EQ(ShaderSourceHash::Compute(TArray<uint8>()), 0u);

    TEST_SECTION("NormalizeLineEndings removes every carriage return");
    TArray<uint8> Contents = ToBytes("A\r\nB\rC\n");
    ShaderSourceHash::NormalizeLineEndings(Contents);
    TEST_EXPECT(Contents == ToBytes("A\nBC\n"));

    TEST_END();
}

bool ShaderPreprocessorMemoryProvider_Test()
{
    TEST_BEGIN();

    FShaderCompiler& Compiler = FShaderCompiler::Get();
    const String     AssetDir = Compiler.GetAssetPath();

    FShaderDefine Defines[] = { FShaderDefine("CLEAR_ELEMENT_UINT", "1") };
    const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

    TArray<FShaderSourceFile> Sources;
    String                    Errors;
    TEST_SECTION("CollectSources finds the shader and its includes");
    TEST_EXPECT(Compiler.CollectSources(GetClearBufferSource(), CompileInfo, Sources, Errors));
    TEST_EXPECT(Sources.Size() >= 2);

    TEST_SECTION("A shader outside the asset directory is named after its own folder");
    TEST_EXPECT(!Sources.IsEmpty() && Sources[0].Path == "@Source/ClearBufferUAV.hlsl");
    TEST_EXPECT(!Sources.IsEmpty() && Sources[0].LocalPath == GetClearBufferSource());

    FMemorySourceProvider Memory;
    for (const FShaderSourceFile& Source : Sources)
    {
        TArray<CHAR> Text;
        TEST_EXPECT(FDiskShaderSourceProvider::Get().ReadFile(Source.LocalPath, Text));
        Memory.AddFile(Source.LocalPath, Text);
    }

    TEST_SECTION("The asset shaders it includes keep their asset-relative names");
    bool bFoundAssetInclude = false;
    for (const FShaderSourceFile& Source : Sources)
    {
        bFoundAssetInclude |= Source.Path == "Shaders/CoreDefines.hlsli";
    }

    TEST_EXPECT(bFoundAssetInclude);

    const String RootPath = GetClearBufferSource();
    TArray<CHAR> RootText;
    TEST_EXPECT(FDiskShaderSourceProvider::Get().ReadFile(RootPath, RootText));

    FShaderPreprocessor DiskPreprocessor(AssetDir + "/Shaders");
    FShaderPreprocessor MemoryPreprocessor(AssetDir + "/Shaders", &Memory);
    DiskPreprocessor.AddDefine("CLEAR_ELEMENT_UINT", "1");
    MemoryPreprocessor.AddDefine("CLEAR_ELEMENT_UINT", "1");

    FShaderPreprocessorOutput DiskOutput;
    FShaderPreprocessorOutput MemoryOutput;
    const StringView RootView(RootText.Data(), RootText.Size());

    TEST_SECTION("Both providers preprocess");
    TEST_EXPECT(DiskPreprocessor.Preprocess(RootPath, RootView, DiskOutput));
    TEST_EXPECT(MemoryPreprocessor.Preprocess(RootPath, RootView, MemoryOutput));

    TEST_SECTION("The output and the dependencies are the same");
    TEST_EXPECT(DiskOutput.Render() == MemoryOutput.Render());
    TEST_EXPECT(DiskOutput.Dependencies == MemoryOutput.Dependencies);

    TEST_SECTION("A file the provider does not have is an error");
    FMemorySourceProvider Empty;
    FShaderPreprocessor   MissingPreprocessor(AssetDir + "/Shaders", &Empty);
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

        FShaderDefine Defines[] = { FShaderDefine("CLEAR_ELEMENT_UINT", "1") };
        const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, OutputLanguage, TArrayView<FShaderDefine>(Defines));

        TArray<uint8> LocalCode;
        TEST_EXPECT(Compiler.CompileFromFile(GetClearBufferSource(), CompileInfo, LocalCode));

        TArray<FShaderSourceFile> Files;
        String                    Errors;
        TEST_EXPECT(Compiler.CollectSources(GetClearBufferSource(), CompileInfo, Files, Errors));
        if (Files.IsEmpty())
        {
            continue;
        }

        TSharedPtr<FRemoteRequestSources> Sources = MakeRequestSources(Files);

        FShaderCompileJob Job = FShaderCompileJob::FromCompileInfo(Files[0].Path, CompileInfo);
        TArray<uint8>  JobCode;
        TArray<String> Dependencies;
        String         Messages;
        TEST_EXPECT(Compiler.CompileJob(Job, FRemoteShaderCompilerServer::SourceRoot, *Sources, JobCode, Dependencies, Messages));

        TEST_EXPECT(!Dependencies.IsEmpty() && Dependencies[0] == Files[0].Path);
        TEST_EXPECT(JobCode == LocalCode);
    }

    TEST_SECTION("Errors carry paths relative to the asset directory");
    FShaderCompileJob Broken;
    Broken.SourceFile     = "Shaders/Broken.hlsl";
    Broken.EntryPoint     = "Main";
    Broken.ShaderModel    = EShaderModel::SM_6_2;
    Broken.ShaderStage    = EShaderStage::Compute;
    Broken.OutputLanguage = EShaderOutputLanguage::SPIRV;

    FRemoteRequestSources BrokenSources;
    BrokenSources.Files.Add(String(FRemoteShaderCompilerServer::SourceRoot) + "/Shaders/Broken.hlsl", MakeSharedPtr<TArray<uint8>>(ToBytes("#include \"Missing.hlsli\"\n")));

    TArray<uint8>  BrokenCode;
    TArray<String> BrokenDependencies;
    String         BrokenMessages;
    TEST_EXPECT(!Compiler.CompileJob(Broken, FRemoteShaderCompilerServer::SourceRoot, BrokenSources, BrokenCode, BrokenDependencies, BrokenMessages));
    TEST_EXPECT(BrokenMessages.Contains("Shaders/Broken.hlsl"));
    TEST_EXPECT(!BrokenMessages.Contains(FRemoteShaderCompilerServer::SourceRoot));

    TEST_END();
}

bool ShaderCompileJobJsonRoundTrip_Test()
{
    TEST_BEGIN();

    FShaderCompileJob Job;
    Job.Name              = "FTestCS#3";
    Job.SourceFile        = "@Source/ClearTextureUAV.hlsl";
    Job.EntryPoint        = "Main";
    Job.ShaderModel       = EShaderModel::SM_6_6;
    Job.ShaderStage       = EShaderStage::Compute;
    Job.OutputLanguage    = EShaderOutputLanguage::MSL;
    Job.bOptimize         = false;
    Job.bDebugInfo        = true;
    Job.bHasEngineDefines = true;
    Job.Defines.Emplace("CLEAR_DIMENSION", "3");
    Job.Defines.Emplace("EMPTY", "");
    Job.IncludeDirs.Emplace("@Include0");
    Job.IncludeDirs.Emplace("Shaders/Internal");

    FShaderCompileJob Parsed;
    String            Error;
    TEST_SECTION("ToJson reads back");
    TEST_EXPECT(FShaderCompileJob::FromJson(Job.ToJson(), Parsed, Error));
    TEST_EXPECT_EQ(Parsed.GetKey(), Job.GetKey());
    TEST_EXPECT(Parsed.Name == Job.Name);
    TEST_EXPECT(Parsed.ShaderModel == Job.ShaderModel && Parsed.OutputLanguage == Job.OutputLanguage);
    TEST_EXPECT(Parsed.bDebugInfo && !Parsed.bOptimize && Parsed.bHasEngineDefines);
    TEST_EXPECT_EQ(Parsed.Defines.Size(), 2);
    TEST_EXPECT(Parsed.IncludeDirs == Job.IncludeDirs);

    TEST_SECTION("A job without include directories leaves the field out");
    FShaderCompileJob NoIncludes = Job;
    NoIncludes.IncludeDirs.Clear();
    TEST_EXPECT(NoIncludes.ToJson().Find("includeDirs") == nullptr);
    TEST_EXPECT(NoIncludes.GetKey() != Job.GetKey());

    TEST_SECTION("Every enum value reads back");
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

    TEST_SECTION("Malformed jobs are rejected");
    FJsonValue Missing = Job.ToJson();
    Missing.RemoveMember("entry");
    TEST_EXPECT(!FShaderCompileJob::FromJson(Missing, Parsed, Error));

    FJsonValue BadStage = Job.ToJson();
    BadStage.AddMember("stage", FJsonValue("Tessellation"));
    TEST_EXPECT(!FShaderCompileJob::FromJson(BadStage, Parsed, Error));

    TEST_SECTION("The key ignores the name and follows the defines");
    FShaderCompileJob Renamed = Job;
    Renamed.Name = "Other";
    TEST_EXPECT_EQ(Renamed.GetKey(), Job.GetKey());

    FShaderCompileJob Redefined = Job;
    Redefined.Defines[0].Value = "4";
    TEST_EXPECT(Redefined.GetKey() != Job.GetKey());

    TEST_END();
}

bool ShaderJobFileMerge_Test()
{
    TEST_BEGIN();

    const String FilePath = Paths::GetEngineDir() + "/Build/RemoteShaderCompilerTests.shaderjob";

    FShaderCompileJob Job;
    Job.SourceFile     = "Shaders/Example.hlsl";
    Job.EntryPoint     = "Main";
    Job.ShaderModel    = EShaderModel::SM_6_2;
    Job.ShaderStage    = EShaderStage::Compute;
    Job.OutputLanguage = EShaderOutputLanguage::SPIRV;

    FShaderCompileJob Other = Job;
    Other.Defines.Emplace("CLEAR_ELEMENT_UINT", "1");

    TEST_SECTION("Duplicates are dropped");
    FShaderJobFile JobFile;
    TEST_EXPECT(JobFile.Add("Vulkan", FShaderCompileJob(Job)));
    TEST_EXPECT(!JobFile.Add("Vulkan", FShaderCompileJob(Job)));
    TEST_EXPECT(!JobFile.Add("vulkan", FShaderCompileJob(Job)));
    TEST_EXPECT(JobFile.Add("Vulkan", FShaderCompileJob(Other)));
    TEST_EXPECT(JobFile.Add("Metal", FShaderCompileJob(Job)));
    TEST_EXPECT_EQ(JobFile.GetEntries().Size(), 3);

    TEST_SECTION("Save and Load keep every entry");
    TEST_EXPECT(JobFile.Save(FilePath));

    FShaderJobFile Loaded;
    String         Error;
    TEST_EXPECT(Loaded.Load(FilePath, Error));
    TEST_EXPECT_EQ(Loaded.GetEntries().Size(), 3);

    TEST_SECTION("Loading into a file merges without duplicates");
    TEST_EXPECT(Loaded.Load(FilePath, Error));
    TEST_EXPECT_EQ(Loaded.GetEntries().Size(), 3);
    TEST_EXPECT(!Loaded.Add("Metal", FShaderCompileJob(Job)));

    TEST_SECTION("Filter selects by RHI");
    const String VulkanName = "VULKAN";
    TEST_EXPECT_EQ(Loaded.Filter(TArrayView<const String>(&VulkanName, 1)).Size(), 2);
    TEST_EXPECT_EQ(Loaded.Filter(TArrayView<const String>()).Size(), 3);

    TEST_SECTION("A missing file is not an error");
    FShaderJobFile Missing;
    TEST_EXPECT(Missing.Load(Paths::GetEngineDir() + "/Build/DoesNotExist.shaderjob", Error));
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
    First.Header.AddMember("hops", FJsonValue(0));
    First.Payload   = ToBytes("payload bytes");

    FProtocol::FFrame Second;
    Second.Type      = FProtocol::EMessageType::Ping;
    Second.RequestId = 8;

    TArray<uint8> Bytes;
    FProtocol::EncodeFrame(First, Bytes);
    FProtocol::EncodeFrame(Second, Bytes);

    TEST_SECTION("A split read only yields the frame once it is complete");
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
            TEST_EXPECT(Frame.Header.Find("hops") && Frame.Header.Find("hops")->GetInt64Or(-1) == 0);
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

    TEST_SECTION("Two frames in one buffer decode one at a time");
    Buffer = Bytes;
    FProtocol::FFrame Frame;
    bool bHasFrame = false;
    TEST_EXPECT(FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame) && bHasFrame && Frame.RequestId == 7);
    TEST_EXPECT(FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame) && bHasFrame && Frame.RequestId == 8);
    TEST_EXPECT(FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame) && !bHasFrame);

    TEST_SECTION("A bad magic, an unknown type or an oversized frame is rejected");
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

    TEST_SECTION("A header that is not a JSON object is rejected");
    Header = FProtocol::FFrameHeader();
    Header.HeaderSize = 3;
    Buffer.Clear();
    Buffer.Append(reinterpret_cast<const uint8*>(&Header), static_cast<int32>(sizeof(Header)));
    Buffer.Append(ToBytes("[1]"));
    TEST_EXPECT(!FProtocol::TryDecodeFrame(Buffer, Frame, bHasFrame));

    TEST_END();
}

bool RemoteProtocolSourcePaths_Test()
{
    TEST_BEGIN();

    using FProtocol = RemoteShaderCompilerProtocol;

    TEST_SECTION("Relative paths of plain names are valid, whichever folder they name");
    TEST_EXPECT(FProtocol::IsValidSourcePath("Shaders/Common.hlsli"));
    TEST_EXPECT(FProtocol::IsValidSourcePath("@Source/ClearBufferUAV.hlsl"));
    TEST_EXPECT(FProtocol::IsValidSourcePath("@Include0"));
    TEST_EXPECT(FProtocol::IsValidSourcePath("@Include1/Sub/Common.hlsli"));

    TEST_SECTION("Paths that could leave the server's source root are rejected");
    TEST_EXPECT(!FProtocol::IsValidSourcePath(""));
    TEST_EXPECT(!FProtocol::IsValidSourcePath("Shaders/../Engine.ini"));
    TEST_EXPECT(!FProtocol::IsValidSourcePath("@Source/../../Core/Core.h"));
    TEST_EXPECT(!FProtocol::IsValidSourcePath("Shaders/./Common.hlsli"));
    TEST_EXPECT(!FProtocol::IsValidSourcePath("Shaders//Common.hlsli"));
    TEST_EXPECT(!FProtocol::IsValidSourcePath("/Shaders/Common.hlsli"));
    TEST_EXPECT(!FProtocol::IsValidSourcePath("C:/Shaders/Common.hlsli"));
    TEST_EXPECT(!FProtocol::IsValidSourcePath("Shaders\\Common.hlsli"));

    TEST_SECTION("CollapsePath drops '.' and resolves '..'");
    TEST_EXPECT(FProtocol::CollapsePath("/Root/Assets/Shaders/Sub/../Common.hlsli") == "/Root/Assets/Shaders/Common.hlsli");
    TEST_EXPECT(FProtocol::CollapsePath("Shaders/./Common.hlsli") == "Shaders/Common.hlsli");
    TEST_EXPECT(FProtocol::CollapsePath("C:\\Assets\\Shaders\\Common.hlsli") == "C:/Assets/Shaders/Common.hlsli");
    TEST_EXPECT(FProtocol::CollapsePath("/../Escape") == "/../Escape");

    TEST_END();
}

bool RemoteShaderCompilerLoopback_Test()
{
    TEST_BEGIN();

    FShaderCompiler& Compiler = FShaderCompiler::Get();

    FRemoteShaderCompilerServerSettings Settings;
    Settings.Port = 0;

    FRemoteShaderCompilerServer Server(Settings);
    TEST_SECTION("The server listens on loopback");
    if (!Server.Launch())
    {
        TEST_EXPECT(false);
        TEST_END();
    }

    TEST_EXPECT(Server.GetPort() != 0);

    TEST_SECTION("The client connects and reads the Hello");
    FRemoteShaderCompilerClient Client;
    TEST_EXPECT(Client.Connect(RemoteShaderCompilerProtocol::LoopbackAddress, Server.GetPort(), GTestTimeoutMs));
    TEST_EXPECT(Client.IsConnected());
    TEST_EXPECT(Client.CanCompile(EShaderOutputLanguage::SPIRV));
    TEST_EXPECT(Client.GetPeerName().Contains("127.0.0.1"));
    TEST_EXPECT(Client.GetIdentity(EShaderOutputLanguage::SPIRV).HasValue() && Client.GetIdentity(EShaderOutputLanguage::SPIRV)->GetHash() == Compiler.GetLocalIdentity(EShaderOutputLanguage::SPIRV).GetHash());

    FShaderDefine Defines[] = { FShaderDefine("CLEAR_ELEMENT_UINT", "1") };
    const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

    TEST_SECTION("A remote compile returns the container a local compile produces");
    TArray<uint8>  LocalCode;
    TArray<uint8>  RemoteCode;
    TArray<String> Dependencies;
    TEST_EXPECT(Compiler.CompileFromFile(GetClearBufferSource(), CompileInfo, LocalCode));
    TEST_EXPECT(Compiler.CompileOnRemote(Client, GetClearBufferSource(), CompileInfo, RemoteCode, &Dependencies) == ERemoteCompileStatus::Succeeded);
    TEST_EXPECT(RemoteCode == LocalCode);

    TEST_SECTION("The server's dependencies are turned back into paths on this machine");
    TEST_EXPECT(!Dependencies.IsEmpty() && RemoteShaderCompilerProtocol::CollapsePath(Dependencies[0]) == RemoteShaderCompilerProtocol::CollapsePath(GetClearBufferSource()));

    bool bFoundAssetDependency = false;
    for (const String& Dependency : Dependencies)
    {
        bFoundAssetDependency |= RemoteShaderCompilerProtocol::CollapsePath(Dependency) == RemoteShaderCompilerProtocol::CollapsePath(Compiler.GetAssetPath() + "/Shaders/CoreDefines.hlsli");
    }

    TEST_EXPECT(bFoundAssetDependency);

    TEST_SECTION("A second compile on the same connection still succeeds");
    TEST_EXPECT(Compiler.CompileOnRemote(Client, GetClearBufferSource(), CompileInfo, RemoteCode, nullptr) == ERemoteCompileStatus::Succeeded);

    TEST_SECTION("A shader error is a compile failure, not an unreachable server");
    const FShaderCompileInfo MissingEntry("DoesNotExist", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));
    TEST_EXPECT(Compiler.CompileOnRemote(Client, GetClearBufferSource(), MissingEntry, RemoteCode, nullptr) == ERemoteCompileStatus::CompileFailed);

    TEST_SECTION("A language the server cannot produce is reported as unreachable");
    constexpr EShaderOutputLanguage Languages[] = { EShaderOutputLanguage::DXIL, EShaderOutputLanguage::DXBC, EShaderOutputLanguage::MSL };
    for (EShaderOutputLanguage OutputLanguage : Languages)
    {
        if (Compiler.CanCompileLocally(OutputLanguage))
        {
            continue;
        }

        TEST_EXPECT(!Client.CanCompile(OutputLanguage));

        const FShaderCompileInfo Unserved("Main", EShaderModel::SM_6_2, EShaderStage::Compute, OutputLanguage, TArrayView<FShaderDefine>(Defines));
        TEST_EXPECT(Compiler.CompileOnRemote(Client, GetClearBufferSource(), Unserved, RemoteCode, nullptr) == ERemoteCompileStatus::Unreachable);
    }

    TEST_SECTION("Shutting the server down disconnects the client");
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
    TEST_SECTION("A raw socket connects and receives the Hello");
    TEST_EXPECT(Socket.ConnectToHost(FProtocol::LoopbackAddress, Server.GetPort(), GTestTimeoutMs));

    TArray<uint8>     Buffer;
    FProtocol::FFrame Frame;
    TEST_EXPECT(ReadFrame(Socket, Buffer, Frame) && Frame.Type == FProtocol::EMessageType::Hello);

    FShaderDefine Defines[] = { FShaderDefine("CLEAR_ELEMENT_SINT", "1") };
    const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

    TArray<FShaderSourceFile> Files;
    String                    Errors;
    TEST_EXPECT(Compiler.CollectSources(GetClearBufferSource(), CompileInfo, Files, Errors));
    if (Files.IsEmpty())
    {
        Server.Shutdown();
        TEST_END();
    }

    const FShaderCompileJob Job = FShaderCompileJob::FromCompileInfo(Files[0].Path, CompileInfo);

    const auto SendRequest = [&](uint32 RequestId, bool bWithContents) -> String
    {
        FProtocol::FFrame Request;
        Request.Type      = FProtocol::EMessageType::CompileRequest;
        Request.RequestId = RequestId;
        Request.Header    = FJsonValue::MakeObject();
        Request.Header.AddMember("hops", FJsonValue(0));
        Request.Header.AddMember("job", Job.ToJson());
        AddFileRefs(Files, bWithContents, Request);

        TArray<uint8> Bytes;
        FProtocol::EncodeFrame(Request, Bytes);

        FProtocol::FFrame Result;
        if (!SendAll(Socket, Bytes) || !ReadFrame(Socket, Buffer, Result) || Result.RequestId != RequestId)
        {
            return String("no result");
        }

        const FJsonValue* Status = Result.Header.Find("status");
        return Status ? Status->GetStringOr("") : String();
    };

    TEST_SECTION("Referencing files that were never sent fails");
    TEST_EXPECT(SendRequest(1, false) == FProtocol::StatusFailed);

    TEST_SECTION("Sending the files once lets later requests only reference them");
    TEST_EXPECT(SendRequest(2, true) == FProtocol::StatusSucceeded);
    TEST_EXPECT(SendRequest(3, false) == FProtocol::StatusSucceeded);

    TEST_SECTION("Contents that do not match their hash are rejected");
    TArray<FShaderSourceFile> Tampered = Files;
    Tampered[0].Contents.Add(static_cast<uint8>(' '));
    {
        FProtocol::FFrame Request;
        Request.Type      = FProtocol::EMessageType::CompileRequest;
        Request.RequestId = 4;
        Request.Header    = FJsonValue::MakeObject();
        Request.Header.AddMember("job", Job.ToJson());
        AddFileRefs(Tampered, true, Request);

        TArray<uint8> Bytes;
        FProtocol::EncodeFrame(Request, Bytes);

        FProtocol::FFrame Result;
        TEST_EXPECT(SendAll(Socket, Bytes) && ReadFrame(Socket, Buffer, Result));
        TEST_EXPECT(Result.Header.Find("status") && Result.Header.Find("status")->GetStringOr("") == FProtocol::StatusFailed);
    }

    Socket.Close();
    Server.Shutdown();
    TEST_END();
}

bool ShaderCodeReadEmbedded_Test()
{
    TEST_BEGIN();

    FShaderDefine Defines[] = { FShaderDefine("CLEAR_ELEMENT_UINT", "1") };
    const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

    TArray<uint8> ShaderCode;
    TEST_EXPECT(FShaderCompiler::Get().CompileFromFile(GetClearBufferSource(), CompileInfo, ShaderCode));

    TArray<uint8> Misaligned;
    Misaligned.Add(0);
    Misaligned.Append(ShaderCode);

    FShaderCodeView Original;
    FShaderCodeView Embedded;
    TArray<uint8>   Storage;
    String          Error;
    TEST_SECTION("ReadEmbedded reads a misaligned copy");
    TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, Original));
    TEST_EXPECT(FShaderCodeReader::ReadEmbedded(TArrayView<const uint8>(Misaligned.Data() + 1, ShaderCode.Size()), Storage, Embedded, &Error));
    TEST_EXPECT(Storage == ShaderCode);
    TEST_EXPECT_EQ(Embedded.GetBindings().Size(), Original.GetBindings().Size());
    TEST_EXPECT_EQ(Embedded.GetNativeCode().Size(), Original.GetNativeCode().Size());
    TEST_EXPECT_EQ(Embedded.GetInfo().ShaderConstantsSize, Original.GetInfo().ShaderConstantsSize);

    TEST_SECTION("ReadEmbedded rejects garbage");
    const TArray<uint8> Garbage = ToBytes("not a shader container at all");
    TEST_EXPECT(!FShaderCodeReader::ReadEmbedded(TArrayView<const uint8>(Garbage.Data(), Garbage.Size()), Storage, Embedded, &Error));

    TEST_END();
}

bool InternalClearBufferUAVReflection_Test()
{
    TEST_BEGIN();

    const CHAR* const Permutations[][2] =
    {
        { "CLEAR_ELEMENT_UINT", "0" },
        { "CLEAR_ELEMENT_UINT", "1" },
        { "CLEAR_ELEMENT_SINT", "1" },
    };

    for (const auto& Permutation : Permutations)
    {
        TEST_SECTION(Permutation[0]);

        FShaderDefine Defines[] = { FShaderDefine(Permutation[0], Permutation[1]) };
        const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

        TArray<uint8>   ShaderCode;
        FShaderCodeView CodeView;
        TEST_EXPECT(FShaderCompiler::Get().CompileFromFile(GetClearBufferSource(), CompileInfo, ShaderCode));
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));

        // What FVulkanBufferClearPipelines used to write by hand
        TEST_EXPECT_EQ(CodeView.GetInfo().ShaderConstantsSize, static_cast<uint8>(sizeof(uint32) * 5));
        TEST_EXPECT_EQ(CodeView.GetBindings().Size(), 1);
        TEST_EXPECT(String(CodeView.GetEntryPoint()) == "Main");

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
        const CHAR*          Uint;
        const CHAR*          Sint;
        EMSLTextureComponent Component;
    };

    const FComponentCase Components[] =
    {
        { "0", "0", EMSLTextureComponent::Float },
        { "1", "0", EMSLTextureComponent::Uint  },
        { "0", "1", EMSLTextureComponent::Int   },
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
        TEST_SECTION("ClearBufferUAV");

        FShaderDefine Defines[] = { FShaderDefine("CLEAR_ELEMENT_UINT", Case.Uint), FShaderDefine("CLEAR_ELEMENT_SINT", Case.Sint) };
        const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::MSL, TArrayView<FShaderDefine>(Defines));

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
        TEST_SECTION("ClearBufferUAV Untyped");

        FShaderDefine Defines[] = { FShaderDefine("CLEAR_ELEMENT_UNTYPED", "1") };
        const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::MSL, TArrayView<FShaderDefine>(Defines));

        TArray<uint8>   ShaderCode;
        FShaderCodeView CodeView;
        TEST_EXPECT(Compiler.CompileFromFile(GetMetalClearBufferSource(), CompileInfo, ShaderCode));
        TEST_EXPECT(FShaderCodeReader::Read(ShaderCode, CodeView));

        ExpectConstants(CodeView, static_cast<uint8>(sizeof(uint32) * 5));

        // SPIR-V lowers RWByteAddressBuffer and RWStructuredBuffer alike, and MetalRHI binds both as a UAV buffer
        bool bFoundOutput = false;
        for (const FShaderResourceBinding& Binding : CodeView.GetBindings())
        {
            bFoundOutput |= GetMSLBindingType(Binding) == EMSLBindingType::UnorderedAccessBuffer && Binding.Register == 0;
        }

        TEST_EXPECT(bFoundOutput);
    }

    struct FDimensionCase
    {
        const CHAR*              Value;
        EMSLTextureDimension     MSLDimension;
        EShaderResourceDimension Dimension;
    };

    const FDimensionCase Dimensions[] =
    {
        { "0", EMSLTextureDimension::Texture1D,      EShaderResourceDimension::Texture1D      },
        { "1", EMSLTextureDimension::Texture1DArray, EShaderResourceDimension::Texture1DArray },
        { "2", EMSLTextureDimension::Texture2D,      EShaderResourceDimension::Texture2D      },
        { "3", EMSLTextureDimension::Texture2DArray, EShaderResourceDimension::Texture2DArray },
        { "4", EMSLTextureDimension::Texture3D,      EShaderResourceDimension::Texture3D      },
    };

    for (const FDimensionCase& Dimension : Dimensions)
    {
        for (const FComponentCase& Case : Components)
        {
            TEST_SECTION("ClearTextureUAV");

            FShaderDefine Defines[] =
            {
                FShaderDefine("CLEAR_DIMENSION", Dimension.Value),
                FShaderDefine("CLEAR_ELEMENT_UINT", Case.Uint),
                FShaderDefine("CLEAR_ELEMENT_SINT", Case.Sint),
            };

            const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::MSL, TArrayView<FShaderDefine>(Defines));

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

static constexpr const CHAR* GIncludeTestShader =
    "#include \"CoreDefines.hlsli\"\n"
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
    "}\n";

bool ShaderCompilerIncludeDirs_Test()
{
    TEST_BEGIN();

    FShaderCompiler& Compiler = FShaderCompiler::Get();

    const String RootDir    = Paths::GetEngineDir() + "/Build/RemoteShaderCompilerTests/IncludeDirs";
    const String ShaderPath = RootDir + "/Shader/Main.hlsl";
    const String FirstDir   = RootDir + "/First";
    const String SecondDir  = RootDir + "/Second";

    const String TestFiles[] =
    {
        ShaderPath,
        RootDir + "/Shader/Local.hlsli",
        FirstDir + "/Common.hlsli",
        SecondDir + "/Common.hlsli",
        SecondDir + "/OnlySecond.hlsli",
    };

    TEST_SECTION("The test files are written");
    TEST_EXPECT(WriteTestFile(TestFiles[0], GIncludeTestShader));
    TEST_EXPECT(WriteTestFile(TestFiles[1], "#define LOCAL_VALUE 3\n"));
    TEST_EXPECT(WriteTestFile(TestFiles[2], "#define COMMON_VALUE 1\n"));
    TEST_EXPECT(WriteTestFile(TestFiles[3], "#define COMMON_VALUE 2\n"));
    TEST_EXPECT(WriteTestFile(TestFiles[4], "#define SECOND_VALUE 4\n"));

    const FShaderCompileInfo BaseInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV);

    TEST_SECTION("Without the include directories the includes are not found");
    TArray<uint8> MissingCode;
    TEST_EXPECT(!Compiler.CompileFromFile(ShaderPath, BaseInfo, MissingCode));

    const String       IncludeDirs[] = { FirstDir, SecondDir };
    FShaderCompileInfo CompileInfo   = BaseInfo;
    CompileInfo.IncludeDirs = TArrayView<const String>(IncludeDirs);

    const auto Contains = [](const TArray<String>& Paths, const String& Path)
    {
        bool bFound = false;
        for (const String& Candidate : Paths)
        {
            bFound |= RemoteShaderCompilerProtocol::CollapsePath(Candidate) == RemoteShaderCompilerProtocol::CollapsePath(Path);
        }

        return bFound;
    };

    TEST_SECTION("The include directories are searched in order, after the shader's folder");
    TArray<uint8>  LocalCode;
    TArray<String> Dependencies;
    TEST_EXPECT(Compiler.CompileFromFile(ShaderPath, CompileInfo, LocalCode, &Dependencies));
    TEST_EXPECT(Contains(Dependencies, FirstDir + "/Common.hlsli"));
    TEST_EXPECT(!Contains(Dependencies, SecondDir + "/Common.hlsli"));
    TEST_EXPECT(Contains(Dependencies, SecondDir + "/OnlySecond.hlsli"));
    TEST_EXPECT(Contains(Dependencies, RootDir + "/Shader/Local.hlsli"));

    TEST_SECTION("The asset shaders are still searched");
    TEST_EXPECT(Contains(Dependencies, Compiler.GetAssetPath() + "/Shaders/CoreDefines.hlsli"));

    const auto CollectNames = [&](const FShaderCompileInfo& Info, TArray<FShaderSourceFile>& OutFiles)
    {
        String Errors;
        const bool bCollected = Compiler.CollectSources(ShaderPath, Info, OutFiles, Errors);

        TArray<String> Names;
        for (const FShaderSourceFile& File : OutFiles)
        {
            Names.Add(File.Path);
        }

        return bCollected ? Names : TArray<String>();
    };

    TEST_SECTION("CollectSources names every file after the folder it was found in");
    TArray<FShaderSourceFile> Files;
    const TArray<String>      Names = CollectNames(CompileInfo, Files);
    TEST_EXPECT(!Names.IsEmpty() && Names[0] == "@Source/Main.hlsl");
    TEST_EXPECT(Names.Contains("@Source/Local.hlsli"));
    TEST_EXPECT(Names.Contains("@Include0/Common.hlsli"));
    TEST_EXPECT(Names.Contains("@Include1/OnlySecond.hlsli"));
    TEST_EXPECT(Names.Contains("Shaders/CoreDefines.hlsli"));

    TEST_SECTION("A server compiles the named files into the same container");
    {
        FShaderCompileJob Job = FShaderCompileJob::FromCompileInfo("@Source/Main.hlsl", CompileInfo);
        Job.IncludeDirs.Clear();
        Job.IncludeDirs.Add("@Include0");
        Job.IncludeDirs.Add("@Include1");

        TSharedPtr<FRemoteRequestSources> Sources = MakeRequestSources(Files);
        TArray<uint8>  JobCode;
        TArray<String> JobDependencies;
        String         Messages;
        TEST_EXPECT(Compiler.CompileJob(Job, FRemoteShaderCompilerServer::SourceRoot, *Sources, JobCode, JobDependencies, Messages));
        TEST_EXPECT(JobCode == LocalCode);
        TEST_EXPECT(JobDependencies.Contains("@Include0/Common.hlsli"));
    }

    TEST_SECTION("A folder inside another one is named below the outer one");
    {
        const String       NestedDirs[] = { RootDir, FirstDir, SecondDir };
        FShaderCompileInfo NestedInfo   = BaseInfo;
        NestedInfo.IncludeDirs = TArrayView<const String>(NestedDirs);

        TArray<FShaderSourceFile> NestedFiles;
        const TArray<String>      NestedNames = CollectNames(NestedInfo, NestedFiles);
        TEST_EXPECT(!NestedNames.IsEmpty() && NestedNames[0] == "@Include0/Shader/Main.hlsl");
        TEST_EXPECT(NestedNames.Contains("@Include0/Shader/Local.hlsli"));
        TEST_EXPECT(NestedNames.Contains("@Include0/First/Common.hlsli"));
        TEST_EXPECT(NestedNames.Contains("@Include0/Second/OnlySecond.hlsli"));

        FShaderCompileJob Job = FShaderCompileJob::FromCompileInfo("@Include0/Shader/Main.hlsl", NestedInfo);
        Job.IncludeDirs.Clear();
        Job.IncludeDirs.Add("@Include0");
        Job.IncludeDirs.Add("@Include0/First");
        Job.IncludeDirs.Add("@Include0/Second");

        TSharedPtr<FRemoteRequestSources> Sources = MakeRequestSources(NestedFiles);
        TArray<uint8>  JobCode;
        TArray<String> JobDependencies;
        String         Messages;
        TEST_EXPECT(Compiler.CompileJob(Job, FRemoteShaderCompilerServer::SourceRoot, *Sources, JobCode, JobDependencies, Messages));
        TEST_EXPECT(JobCode == LocalCode);
    }

    TEST_SECTION("The asset directory itself has no name to send");
    {
        const String       AssetDirs[] = { Compiler.GetAssetPath() };
        FShaderCompileInfo AssetInfo   = BaseInfo;
        AssetInfo.IncludeDirs = TArrayView<const String>(AssetDirs);

        TArray<FShaderSourceFile> AssetFiles;
        String                    Errors;
        TEST_EXPECT(!Compiler.CollectSources(ShaderPath, AssetInfo, AssetFiles, Errors));
        TEST_EXPECT(!Errors.IsEmpty());
    }

    TEST_SECTION("A remote compile with include directories matches the local one");
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

            TArray<uint8>  RemoteCode;
            TArray<String> RemoteDependencies;
            TEST_EXPECT(Compiler.CompileOnRemote(Client, ShaderPath, CompileInfo, RemoteCode, &RemoteDependencies) == ERemoteCompileStatus::Succeeded);
            TEST_EXPECT(RemoteCode == LocalCode);
            TEST_EXPECT(Contains(RemoteDependencies, FirstDir + "/Common.hlsli"));
            TEST_EXPECT(Contains(RemoteDependencies, RootDir + "/Shader/Local.hlsli"));

            Client.Disconnect();
            Server.Shutdown();
        }
    }

    for (const String& TestFile : TestFiles)
    {
        FPlatformFile::DeleteFile(*TestFile);
    }

    FPlatformFile::RemoveDirectory(*(RootDir + "/Shader"));
    FPlatformFile::RemoveDirectory(*FirstDir);
    FPlatformFile::RemoveDirectory(*SecondDir);
    FPlatformFile::RemoveDirectory(*RootDir);
    FPlatformFile::RemoveDirectory(*File::GetDirectoryOf(RootDir));

    TEST_END();
}

bool ShaderCompilerHashIgnoresAssetPath_Test()
{
    TEST_BEGIN();

    FShaderDefine Defines[] = { FShaderDefine("CLEAR_ELEMENT_UINT", "1") };
    const FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, EShaderOutputLanguage::SPIRV, TArrayView<FShaderDefine>(Defines));

    const String                  SourceFile = "Shaders/Example.hlsl";
    const String                  AssetDir   = FShaderCompiler::Get().GetAssetPath();
    const FShaderCompilerIdentity Identity   = FShaderCompiler::Get().GetLocalIdentity(EShaderOutputLanguage::SPIRV);
    const uint64                  Hash       = FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo);

    TEST_SECTION("The routed hash uses the local identity");
    TEST_EXPECT(Identity.IsValid());
    TEST_EXPECT_EQ(Hash, FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo, Identity));

    TEST_SECTION("A different compiler identity changes the hash");
    FShaderCompilerIdentity Newer = Identity;
    Newer.VersionMinor++;
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo, Newer) != Hash);
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo, FShaderCompilerIdentity()) != Hash);

    TEST_SECTION("Include directories change the hash");
    const String             IncludeDirs[] = { String("Shaders/Internal") };
    FShaderCompileInfo       WithIncludes  = CompileInfo;
    WithIncludes.IncludeDirs = TArrayView<const String>(IncludeDirs);
    TEST_EXPECT(FShaderCompiler::Get().ComputeCompileHash(SourceFile, WithIncludes) != Hash);

    TEST_SECTION("Another asset directory gives the same hash");
    FShaderCompiler::Destroy();
    const bool bMoved = FShaderCompiler::Initialize("/Some/Other/Checkout/Assets");
    TEST_EXPECT(bMoved);
    if (bMoved)
    {
        TEST_EXPECT_EQ(FShaderCompiler::Get().ComputeCompileHash(SourceFile, CompileInfo), Hash);
        FShaderCompiler::Destroy();
    }

    TEST_EXPECT(FShaderCompiler::Initialize(AssetDir));
    TEST_END();
}
