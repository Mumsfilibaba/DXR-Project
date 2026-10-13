#include "MemoryProfilerTests.h"

#include <Core/Memory/MemoryProfiler.h>
#include <Core/Platform/PlatformThread.h>
#include <Core/Templates/CString.h>
#include <Core/Threading/Runnable.h>

#include "TestCommon/TestMacros.h"

struct FTaggingRunnable : public FRunnable
{
    virtual int32 Run() override
    {
        FMemoryProfiler& Profiler = FMemoryProfiler::Get();
        bSawUntagged = (Profiler.GetCurrentTag() == MEMORY_NAME_UNTAGGED);

        Profiler.PushTag(Profiler.InternDynamic("MemoryProfilerTest.Worker"));
        WorkerTag = Profiler.GetCurrentTag();
        Profiler.PopTag();
        return 0;
    }

    bool          bSawUntagged = false;
    FMemoryNameId WorkerTag    = MEMORY_NAME_UNTAGGED;
};

bool MemoryProfiler_Test()
{
    TEST_BEGIN();

    FMemoryProfiler&    Profiler = FMemoryProfiler::Get();
    FMemoryTagRegistry& Registry = Profiler.GetRegistry();

    // The test body runs inside FProgramLoop::Run, which tags it with the program's title
    const FMemoryNameId ProgramTag = Profiler.GetCurrentTag();

    TEST_SECTION("The program's body runs under the program's tag, outside every scope");
#if ENABLE_MEMORY_PROFILER
    TEST_EXPECT_EQ(CString::Strcmp(Registry.GetName(ProgramTag), "Core-Tests"), 0);
#else
    TEST_EXPECT_EQ(ProgramTag, MEMORY_NAME_UNTAGGED);
#endif
    TEST_EXPECT_EQ(Profiler.GetCurrentScope(), MEMORY_SCOPE_ROOT);
    TEST_EXPECT_EQ(CString::Strcmp(Registry.GetName(MEMORY_NAME_UNTAGGED), "Untagged"), 0);
    TEST_EXPECT_EQ(CString::Strcmp(Registry.GetName(MEMORY_NAME_OVERFLOW), "Overflow"), 0);

    TEST_SECTION("The same literal from two translation units is one name, and so is a copy of it built at runtime");
    {
        static CONST_INIT FMemoryName SharedName("MemoryProfilerTest.Shared");
        const FMemoryNameId Id = SharedName.GetId();

        TEST_EXPECT(Id != MEMORY_NAME_UNTAGGED);
        TEST_EXPECT(Id != MEMORY_NAME_OVERFLOW);
        TEST_EXPECT_EQ(GetSharedMemoryNameIdFromOtherUnit(), Id);
        TEST_EXPECT_EQ(SharedName.GetId(), Id);

        CHAR RuntimeCopy[] = "MemoryProfilerTest.Shared";
        TEST_EXPECT_EQ(Profiler.InternDynamic(RuntimeCopy), Id);

        TEST_EXPECT_EQ(CString::Strcmp(Registry.GetName(Id), "MemoryProfilerTest.Shared"), 0);
        TEST_EXPECT(Registry.GetName(Id) != static_cast<const CHAR*>(RuntimeCopy));
    }

    TEST_SECTION("Different names are different ids, and a name that changes under the same buffer is matched by content");
    {
        CHAR Buffer[] = "MemoryProfilerTest.First";
        const FMemoryNameId First = Profiler.InternDynamic(Buffer);

        CString::Strcpy(Buffer, "MemoryProfilerTest.Other");
        const FMemoryNameId Other = Profiler.InternDynamic(Buffer);

        TEST_EXPECT(First != Other);
        TEST_EXPECT_EQ(CString::Strcmp(Registry.GetName(First), "MemoryProfilerTest.First"), 0);
        TEST_EXPECT_EQ(CString::Strcmp(Registry.GetName(Other), "MemoryProfilerTest.Other"), 0);
    }

    TEST_SECTION("Module tags nest, and every pop restores the tag before it");
    {
        const FMemoryNameId Outer = Profiler.InternDynamic("MemoryProfilerTest.Outer");
        const FMemoryNameId Inner = Profiler.InternDynamic("MemoryProfilerTest.Inner");

        Profiler.PushTag(Outer);
        TEST_EXPECT_EQ(Profiler.GetCurrentTag(), Outer);

        Profiler.PushTag(Inner);
        TEST_EXPECT_EQ(Profiler.GetCurrentTag(), Inner);

        Profiler.PopTag();
        TEST_EXPECT_EQ(Profiler.GetCurrentTag(), Outer);

        Profiler.PopTag();
        TEST_EXPECT_EQ(Profiler.GetCurrentTag(), ProgramTag);
    }

    TEST_SECTION("Pushing past the stack's depth keeps the innermost tag it could hold, and still unwinds to the tag before");
    {
        constexpr int32 Depth = FMemoryThreadState::MaxTagDepth + 8;
        const FMemoryNameId Deep = Profiler.InternDynamic("MemoryProfilerTest.Deep");

        for (int32 Index = 0; Index < Depth; ++Index)
        {
            Profiler.PushTag(Deep);
        }

        TEST_EXPECT_EQ(Profiler.GetCurrentTag(), Deep);

        for (int32 Index = 0; Index < Depth; ++Index)
        {
            Profiler.PopTag();
        }

        TEST_EXPECT_EQ(Profiler.GetCurrentTag(), ProgramTag);
    }

    TEST_SECTION("Scopes form a tree, so a path is one node however often it is entered");
    {
        const FMemoryNameId Shadows = Profiler.InternDynamic("MemoryProfilerTest.Shadows");
        const FMemoryNameId Cascade = Profiler.InternDynamic("MemoryProfilerTest.Cascade");

        const FMemoryScopeId BeforeShadows = Profiler.PushScope(Shadows);
        const FMemoryScopeId ShadowsNode   = Profiler.GetCurrentScope();
        TEST_EXPECT_EQ(BeforeShadows, MEMORY_SCOPE_ROOT);
        TEST_EXPECT_EQ(Registry.GetScopeParent(ShadowsNode), MEMORY_SCOPE_ROOT);
        TEST_EXPECT_EQ(Registry.GetScopeName(ShadowsNode), Shadows);

        const FMemoryScopeId BeforeCascade = Profiler.PushScope(Cascade);
        const FMemoryScopeId CascadeNode   = Profiler.GetCurrentScope();
        TEST_EXPECT_EQ(BeforeCascade, ShadowsNode);
        TEST_EXPECT_EQ(Registry.GetScopeParent(CascadeNode), ShadowsNode);
        TEST_EXPECT_EQ(Registry.GetScopeName(CascadeNode), Cascade);

        Profiler.PopScope(BeforeCascade);
        TEST_EXPECT_EQ(Profiler.GetCurrentScope(), ShadowsNode);

        Profiler.PopScope(BeforeShadows);
        TEST_EXPECT_EQ(Profiler.GetCurrentScope(), MEMORY_SCOPE_ROOT);

        const int32 NumNodes = Registry.GetNumScopeNodes();
        const FMemoryScopeId AgainShadows = Profiler.PushScope(Shadows);
        Profiler.PushScope(Cascade);
        TEST_EXPECT_EQ(Profiler.GetCurrentScope(), CascadeNode);
        TEST_EXPECT_EQ(Registry.GetNumScopeNodes(), NumNodes);
        Profiler.PopScope(ShadowsNode);
        Profiler.PopScope(AgainShadows);

        TEST_SECTION("The same name under another parent is a node of its own");
        const FMemoryScopeId BeforeTopCascade = Profiler.PushScope(Cascade);
        TEST_EXPECT(Profiler.GetCurrentScope() != CascadeNode);
        TEST_EXPECT_EQ(Registry.GetScopeParent(Profiler.GetCurrentScope()), MEMORY_SCOPE_ROOT);
        Profiler.PopScope(BeforeTopCascade);
    }

    TEST_SECTION("Tags are per thread, and a thread's state is released when it exits");
    {
        const FMemoryNameId MainTag = Profiler.InternDynamic("MemoryProfilerTest.Main");
        Profiler.PushTag(MainTag);

        const int32 LiveStatesBefore = FMemoryProfilerStateAllocator::GetNumLiveStates();

        FTaggingRunnable* Runnable = new FTaggingRunnable();
        IPlatformThread*  Thread   = FPlatformThread::Create(Runnable, "MemoryProfilerTest", false);
        Thread->WaitForCompletion();

        TEST_EXPECT(Runnable->bSawUntagged);
        TEST_EXPECT_EQ(Runnable->WorkerTag, Profiler.InternDynamic("MemoryProfilerTest.Worker"));
        TEST_EXPECT_EQ(Profiler.GetCurrentTag(), MainTag);
        TEST_EXPECT_EQ(FMemoryProfilerStateAllocator::GetNumLiveStates(), LiveStatesBefore);

        delete Thread;
        delete Runnable;
        Profiler.PopTag();
    }

#if ENABLE_MEMORY_PROFILER
    TEST_SECTION("The macros push and pop for the block they are declared in");
    {
        TEST_EXPECT_EQ(Profiler.GetCurrentTag(), ProgramTag);
        {
            MEMORY_MODULE_SCOPE("MemoryProfilerTest.MacroModule");
            const FMemoryNameId ModuleTag = Profiler.GetCurrentTag();
            TEST_EXPECT_EQ(CString::Strcmp(Registry.GetName(ModuleTag), "MemoryProfilerTest.MacroModule"), 0);

            {
                MEMORY_SCOPE("MemoryProfilerTest.MacroScope");
                MEMORY_SCOPE("MemoryProfilerTest.MacroInner");

                const FMemoryScopeId InnerNode = Profiler.GetCurrentScope();
                const FMemoryScopeId OuterNode = Registry.GetScopeParent(InnerNode);
                TEST_EXPECT_EQ(CString::Strcmp(Registry.GetName(Registry.GetScopeName(InnerNode)), "MemoryProfilerTest.MacroInner"), 0);
                TEST_EXPECT_EQ(CString::Strcmp(Registry.GetName(Registry.GetScopeName(OuterNode)), "MemoryProfilerTest.MacroScope"), 0);
                TEST_EXPECT_EQ(Registry.GetScopeParent(OuterNode), MEMORY_SCOPE_ROOT);
            }

            TEST_EXPECT_EQ(Profiler.GetCurrentScope(), MEMORY_SCOPE_ROOT);
            TEST_EXPECT_EQ(Profiler.GetCurrentTag(), ModuleTag);
        }

        TEST_EXPECT_EQ(Profiler.GetCurrentTag(), ProgramTag);
    }
#endif

    TEST_END();
}
