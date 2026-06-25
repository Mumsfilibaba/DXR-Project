#include "Function_Test.h"

#if RUN_TFUNCTION_TEST
#include "TestUtils.h"

#include <Core/Containers/Function.h>

namespace
{
    static int32 FreeAdd(int32 First, int32 Second)
    {
        return First + Second;
    }

    static int32 FreeNegate(int32 First, int32 Second)
    {
        return -(First + Second);
    }

    static int32 Free4(int32 First, int32 Second, int32 Third, int32 Fourth)
    {
        return First + Second + Third + Fourth;
    }

    struct FFunctor
    {
        int32 operator()(int32 First, int32 Second) const
        {
            return (First * Second) + Bias;
        }

        int32 Bias = 0;
    };

    struct FObject
    {
        int32 Add(int32 First, int32 Second)
        {
            return First + Second;
        }

        int32 ConstAdd(int32 First, int32 Second) const
        {
            return First + Second;
        }
    };

    struct FVirtualBase
    {
        virtual ~FVirtualBase() = default;
        virtual int32 Scale(int32 In) const
        {
            return In;
        }
    };

    struct FVirtualDerived : public FVirtualBase
    {
        virtual int32 Scale(int32 In) const override
        {
            return In * 2;
        }
    };
}

bool TFunction_Test()
{
    TEST_BEGIN();

    TEST_SECTION("TFunction default / IsValid / operator bool / Reset");
    {
        TFunction<int32(int32, int32)> Empty;
        TEST_EXPECT(!Empty.IsValid());
        TEST_EXPECT(!static_cast<bool>(Empty));

        TFunction<int32(int32, int32)> Add = &FreeAdd;
        TEST_EXPECT(Add.IsValid());
        TEST_EXPECT(static_cast<bool>(Add));
        TEST_EXPECT(Add(2, 3) == 5);

        Add.Reset();
        TEST_EXPECT(!Add.IsValid());
    }

    TEST_SECTION("TFunction Bind / functor with state");
    {
        TFunction<int32(int32, int32)> Func;
        FFunctor Functor{ 10 };
        Func.Bind(Functor);
        TEST_EXPECT(Func(2, 3) == 16);

        Func.Bind(&FreeAdd);
        TEST_EXPECT(Func(4, 5) == 9);
    }

    TEST_SECTION("TFunction copy / move / nullptr assignment");
    {
        TFunction<int32(int32, int32)> Original = &FreeAdd;
        TFunction<int32(int32, int32)> Copy = Original;
        TEST_EXPECT(Copy.IsValid());
        TEST_EXPECT(Copy(1, 1) == 2);

        TFunction<int32(int32, int32)> Moved = ::Move(Original);
        TEST_EXPECT(Moved.IsValid());
        TEST_EXPECT(!Original.IsValid());
        TEST_EXPECT(Moved(3, 4) == 7);

        Moved = nullptr;
        TEST_EXPECT(!Moved.IsValid());
    }

    TEST_SECTION("TFunction Swap");
    {
        TFunction<int32(int32, int32)> First = &FreeAdd;
        TFunction<int32(int32, int32)> Second = &FreeNegate;
        First.Swap(Second);
        TEST_EXPECT(First(2, 3) == -5);
        TEST_EXPECT(Second(2, 3) == 5);
    }

    TEST_SECTION("TFunctionRef free function (regression for non-owning pointer storage)");
    {
        TFunctionRef<int32(int32, int32)> Ref = FreeAdd;
        TEST_EXPECT(Ref.IsValid());
        TEST_EXPECT(Ref(6, 7) == 13);

        int32 (*FuncPtr)(int32, int32) = &FreeNegate;
        TFunctionRef<int32(int32, int32)> RefPtr = FuncPtr;
        TEST_EXPECT(RefPtr(6, 7) == -13);
    }

    TEST_SECTION("TFunctionRef functor / Bind / Swap");
    {
        FFunctor Functor{ 1 };
        TFunctionRef<int32(int32, int32)> Ref = Functor;
        TEST_EXPECT(Ref(2, 3) == 7);

        Ref.Bind(FreeAdd);
        TEST_EXPECT(Ref(2, 3) == 5);

        FFunctor OtherFunctor{ 100 };
        TFunctionRef<int32(int32, int32)> Other = OtherFunctor;
        Ref.Swap(Other);
        TEST_EXPECT(Ref(2, 3) == 106);
    }

    TEST_SECTION("TUniqueFunction move-only semantics");
    {
        TUniqueFunction<int32(int32, int32)> Empty;
        TEST_EXPECT(!Empty.IsValid());

        TUniqueFunction<int32(int32, int32)> Func = &FreeAdd;
        TEST_EXPECT(Func.IsValid());
        TEST_EXPECT(static_cast<bool>(Func));
        TEST_EXPECT(Func(8, 9) == 17);

        TUniqueFunction<int32(int32, int32)> Moved = ::Move(Func);
        TEST_EXPECT(Moved.IsValid());
        TEST_EXPECT(!Func.IsValid());
        TEST_EXPECT(Moved(1, 2) == 3);

        Moved = nullptr;
        TEST_EXPECT(!Moved.IsValid());
    }

    TEST_SECTION("TUniqueFunction Swap / Reset / stateful capture");
    {
        int32 Base = 40;
        TUniqueFunction<int32(int32)> Func = [Base](int32 Add) -> int32
        {
            return Base + Add;
        };
        
        TEST_EXPECT(Func.IsValid());
        TEST_EXPECT(Func(2) == 42);

        TUniqueFunction<int32(int32)> Other;
        Other.Swap(Func);
        TEST_EXPECT(Other.IsValid());
        TEST_EXPECT(!Func.IsValid());
        TEST_EXPECT(Other(3) == 43);

        Other.Reset();
        TEST_EXPECT(!Other.IsValid());
    }

    TEST_SECTION("Bind: member / const member / payload / partial application / Invoke");
    {
        FObject Obj;
        auto MemberBind = Bind(&FObject::Add, &Obj);
        TEST_EXPECT(MemberBind(2, 3) == 5);

        auto ConstBind = Bind(&FObject::ConstAdd, &Obj);
        TEST_EXPECT(ConstBind(4, 5) == 9);

        // Partial application: bind leading args, supply the rest at call time.
        auto Partial = Bind(Free4, 1, 2);
        TEST_EXPECT(Partial(3, 4) == 10);

        // Full payload: all args bound, call with no args.
        auto Full = Bind(FreeAdd, 7, 8);
        TEST_EXPECT(Full() == 15);

        // Member with object + args all bound as payload.
        auto MemberPayload = Bind(&FObject::Add, &Obj, 10, 20);
        TEST_EXPECT(MemberPayload() == 30);

        // Bind member without object; object passed at call time.
        auto Unbound = Bind(&FObject::Add);
        TEST_EXPECT(Unbound(&Obj, 5, 6) == 11);

        // Invoke free + member directly.
        TEST_EXPECT(Invoke(FreeAdd, 3, 4) == 7);
        TEST_EXPECT(Invoke(&FObject::Add, &Obj, 5, 6) == 11);
    }

    TEST_SECTION("Bind / Invoke virtual member (dynamic dispatch)");
    {
        FVirtualDerived Derived;
        FVirtualBase* Base = &Derived;

        auto VirtualBind = Bind(&FVirtualBase::Scale, Base);
        TEST_EXPECT(VirtualBind(21) == 42);
        TEST_EXPECT(Invoke(&FVirtualBase::Scale, Base, 5) == 10);
    }

    TEST_SECTION("TFunction holding member bind / value+ref capturing lambdas");
    {
        FObject Obj;
        TFunction<int32(int32, int32)> MemberFunc = Bind(&FObject::Add, &Obj);
        TEST_EXPECT(MemberFunc(10, 20) == 30);

        int32 Captured = 100;
        TFunction<int32(int32)> ByValue = [Captured](int32 In)
        {
            return Captured + In;
        };
        TEST_EXPECT(ByValue(1) == 101);

        int32 Counter = 0;
        TFunction<void(int32)> ByRef = [&Counter](int32 In)
        {
            Counter += In;
        };
        ByRef(5);
        TEST_EXPECT(Counter == 5);
    }

    TEST_SECTION("TFunctionRef holding member bind (non-owning)");
    {
        FObject Obj;
        auto MemberBind = Bind(&FObject::Add, &Obj);
        TFunctionRef<int32(int32, int32)> Ref = MemberBind;
        TEST_EXPECT(Ref(2, 3) == 5);
    }

    TEST_SECTION("TFunction heap-storage stress (heavy capture, copy/move, no leaks)");
    {
        FInstanced::Reset();
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom     Random(Seed);
            const int32 Bias = static_cast<int32>(Random.RandInt(0, 1000));

            FInstanced Payload(Bias);
            int64      Pad[16] = {};
            
            TFunction<int32(int32)> Func = [Payload, Pad, Bias](int32 In) -> int32
            {
                (void)Pad;
                return In + Payload.GetId() + Bias;
            };

            TEST_EXPECT(Func.IsValid());
            TEST_EXPECT(Func(1) == (1 + Bias + Bias));

            const int32 Repeats = (TargetSize % 64) + 1;
            for (int32 Step = 0; Step < Repeats; ++Step)
            {
                TFunction<int32(int32)> Copy = Func;
                TEST_EXPECT(Copy(2) == (2 + Bias + Bias));

                TFunction<int32(int32)> Moved = ::Move(Copy);
                TEST_EXPECT(Moved(3) == (3 + Bias + Bias));
                TEST_EXPECT(!Copy.IsValid());
            }
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_END();
}
#endif
