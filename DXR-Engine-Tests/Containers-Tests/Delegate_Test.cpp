#include "Delegate_Test.h"

#if RUN_DELEGATE_TEST
#include <iostream>

#include <Core/Containers/Tuple.h>
#include <Core/Containers/Pair.h>
#include <Core/Delegates/Delegate.h>
#include <Core/Delegates/MulticastDelegate.h>
#include <Core/Delegates/Event.h>

/* Functions */
static int32 StaticFunc( int32 Num )
{
    std::cout << "StaticFunc=" << Num << std::endl;
    return Num + 1;
}

static void StaticFunc2( int32 Num )
{
    std::cout << "StaticFunc2=" << Num << std::endl;
}

/* Classes */
struct CBase
{
    virtual ~CBase() = default;

    int32 Func( int32 Num )
    {
        std::cout << "MemberFunc=" << Num << std::endl;
        return Num + 1;
    }

    int32 ConstFunc( int32 Num ) const
    {
        std::cout << "ConstMemberFunc=" << Num << std::endl;
        return Num + 1;
    }

    virtual int32 VirtualFunc( int32 Num ) = 0;

    virtual int32 VirtualConstFunc( int32 Num ) const = 0;
};

struct CDerived : public CBase
{
    virtual int32 VirtualFunc( int32 Num ) override final
    {
        std::cout << "Virtual MemberFunc=" << Num << std::endl;
        return Num + 1;
    }

    virtual int32 VirtualConstFunc( int32 Num ) const override final
    {
        std::cout << "Virtual ConstMemberFunc=" << Num << std::endl;
        return Num + 1;
    }
};

struct CBase2
{
    virtual ~CBase2() = default;

    void Func( int32 Num )
    {
        std::cout << "MemberFunc2=" << Num << std::endl;
    }

    void ConstFunc( int32 Num ) const
    {
        std::cout << "ConstMemberFunc2=" << Num << std::endl;
    }

    virtual void VirtualFunc( int32 Num ) = 0;

    virtual void VirtualConstFunc( int32 Num ) const = 0;
};

struct CDerived2 : public CBase2
{
    virtual void VirtualFunc( int32 Num ) override final
    {
        std::cout << "Virtual MemberFunc2=" << Num << std::endl;
    }

    virtual void VirtualConstFunc( int32 Num ) const override final
    {
        std::cout << "Virtual ConstMemberFunc2=" << Num << std::endl;
    }
};

/* Tuple func */
static void TupleFunc( int32 Num0, int32 Num1, int32 Num2, int32 Num3 )
{
    std::cout << "Tuple func Num0=" << Num0 << ", Num1=" << Num1 << ", Num2=" << Num2 << ", Num3=" << Num3 << std::endl;
}

/* Declare global event */
DECLARE_EVENT( CSomeEvent, CEventDispacher, int32 );
CSomeEvent GSomeEvent;

class CEventDispacher
{
public:
    void Func()
    {
        GSomeEvent.Broadcast( 42 );
    }
};

/* Test */
void Delegate_Test()
{
    std::cout << std::endl << "----Testing Tuple----" << std::endl << std::endl;
    TTuple<int32, float, double, std::string> Tuple( 5, 0.9f, 5.0, "A string" );

    std::cout << "Size=" << Tuple.Size() << std::endl;

    std::cout << "NumElements=" << TTuple<int, float, double>::NumElements << std::endl;

    std::cout << "GetByIndex<0>=" << Tuple.GetByIndex<0>() << std::endl;
    std::cout << "GetByIndex<1>=" << Tuple.GetByIndex<1>() << std::endl;
    std::cout << "GetByIndex<2>=" << Tuple.GetByIndex<2>() << std::endl;

    std::cout << "Get<int>=" << Tuple.Get<int>() << std::endl;
    std::cout << "Get<float>=" << Tuple.Get<float>() << std::endl;
    std::cout << "Get<double>=" << Tuple.Get<double>() << std::endl;

    std::cout << "TupleGetByIndex<0>=" << TupleGetByIndex<0>( Tuple ) << std::endl;
    std::cout << "TupleGetByIndex<1>=" << TupleGetByIndex<1>( Tuple ) << std::endl;
    std::cout << "TupleGetByIndex<2>=" << TupleGetByIndex<2>( Tuple ) << std::endl;

    std::cout << "TupleGet<int>=" << TupleGet<int>( Tuple ) << std::endl;
    std::cout << "TupleGet<float>=" << TupleGet<float>( Tuple ) << std::endl;
    std::cout << "TupleGet<double>=" << TupleGet<double>( Tuple ) << std::endl;

    TTuple<int32, float, double, std::string> Tuple2;
    Tuple2 = Tuple;

    TTuple<int32, float, double, std::string> Tuple3 = Move( Tuple2 );

    std::cout << "operator==" << std::boolalpha << (Tuple == Tuple3) << std::endl;
    std::cout << "operator!=" << std::boolalpha << (Tuple != Tuple3) << std::endl;
    std::cout << "operator<=" << std::boolalpha << (Tuple <= Tuple3) << std::endl;
	std::cout << "operator<"  << std::boolalpha << (Tuple < Tuple3) << std::endl;
	std::cout << "operator>"  << std::boolalpha << (Tuple > Tuple3) << std::endl;
	std::cout << "operator>=" << std::boolalpha << (Tuple >= Tuple3) << std::endl;
	
	TTuple<int32, float, double> Tuple4( 5, 32.0f, 500.0 );
	TTuple<int32, float, double> Tuple5( 2, 22.0f, 100.0 );
	Tuple4.Swap( Tuple5 );
	
	std::cout << "operator==" << std::boolalpha << (Tuple4 == Tuple5) << std::endl;
	std::cout << "operator!=" << std::boolalpha << (Tuple4 != Tuple5) << std::endl;
	std::cout << "operator<=" << std::boolalpha << (Tuple4 <= Tuple5) << std::endl;
	std::cout << "operator<"  << std::boolalpha << (Tuple4 <  Tuple5) << std::endl;
	std::cout << "operator>"  << std::boolalpha << (Tuple4 >  Tuple5) << std::endl;
	std::cout << "operator>=" << std::boolalpha << (Tuple4 >= Tuple5) << std::endl;

	TTuple<float, float> PairTuple0( 80.0f, 900.0f );
	TTuple<float, float> PairTuple1( 50.0f, 100.0f );
	PairTuple1.First = 30.0f;
	PairTuple1.Second = 200.0f;

	PairTuple0.Swap(PairTuple1);
	
    std::cout << "First=" << PairTuple0.First << std::endl;
    std::cout << "Second=" << PairTuple0.Second << std::endl;

    std::cout << "Get<float>=" << PairTuple0.Get<float>() << std::endl;
    std::cout << "Get<float>=" << PairTuple0.Get<float>() << std::endl;

    std::cout << "GetByIndex<0>=" << PairTuple0.GetByIndex<0>() << std::endl;
    std::cout << "GetByIndex<1>=" << PairTuple0.GetByIndex<1>() << std::endl;

    TTuple<int32, int32> PairTuple2( 80, 900 );
    PairTuple2.ApplyAfter( TupleFunc, 30, 99 );
    PairTuple2.ApplyBefore( TupleFunc, 30, 99 );

    TTuple<int32, int32, int32> Args( 10, 20, 30 );
    Args.ApplyAfter( TupleFunc, 99 );
    Args.ApplyBefore( TupleFunc, 99 );

    TPair<std::string, int32> Pair0( "Pair0", 32 );
    TPair<std::string, int32> Pair1( "Pair1", 42 );

    std::cout << "operator==" << std::boolalpha << (Pair0 == Pair1) << std::endl;
    std::cout << "operator!=" << std::boolalpha << (Pair0 != Pair1) << std::endl;

    Pair0.Swap( Pair1 );

    std::cout << std::endl << "----Testing Delegate----" << std::endl << std::endl;

    {
        TDelegate<int32( int32 )> Delegate;
        std::cout << "IsBound=" << std::boolalpha << Delegate.IsBound() << std::endl;
        std::cout << "ExecuteIfBound=" << std::boolalpha << Delegate.ExecuteIfBound( 32 ) << std::endl;

        // Static
        Delegate.BindStatic( StaticFunc );

        std::cout << "Owner=" << Delegate.GetBoundObject() << std::endl;

        std::cout << "IsBound=" << std::boolalpha << Delegate.IsBound() << std::endl;
        std::cout << "Execute=" << Delegate.Execute( 32 ) << std::endl;

        // Lambda
        int64 x = 50;
        int64 y = 150;
        int64 z = 250;
        auto Lambda = [=]( int32 Num ) -> int32
        {
            std::cout << "Lambda (x=" << x << ", y=" << y << ", z=" << z << ") =" << Num << std::endl;
            return Num + 1;
        };

        Delegate.BindLambda( Lambda );
        std::cout << "Execute=" << Delegate.Execute( 500 ) << std::endl;

        // Members
        CBase* Base = new CDerived();
        Delegate.BindRaw( Base, &CBase::Func );
        std::cout << "Execute=" << Delegate.Execute( 100 ) << std::endl;

        Delegate.BindRaw( Base, &CBase::ConstFunc );
        std::cout << "Execute=" << Delegate.Execute( 200 ) << std::endl;

        Delegate.BindRaw( Base, &CDerived::Func );
        std::cout << "Execute=" << Delegate.Execute( 300 ) << std::endl;

        Delegate.BindRaw( Base, &CDerived::ConstFunc );
        std::cout << "Execute=" << Delegate.Execute( 400 ) << std::endl;

        std::cout << "IsObjectBound=" << std::boolalpha << Delegate.IsObjectBound( Base ) << std::endl;
        std::cout << "UnbindIfBound=" << std::boolalpha << Delegate.UnbindIfBound( Base ) << std::endl;

        Delegate.Unbind();

        std::cout << "IsObjectBound=" << std::boolalpha << Delegate.IsObjectBound( Base ) << std::endl;
        std::cout << "UnbindIfBound=" << std::boolalpha << Delegate.UnbindIfBound( Base ) << std::endl;

        // Copy
        Delegate.BindLambda( Lambda );
        Delegate.Execute( 0 );

        TDelegate<int32( int32 )> Delegate2 = Delegate;
        Delegate2.Execute( 100 );

        Delegate = Delegate2;
        Delegate.Execute( 200 );

        // Move
        TDelegate<int32( int32 )> Delegate3 = Move( Delegate2 );
        Delegate3.Execute( 300 );
        Delegate2.ExecuteIfBound( 400 );

        Delegate = Move( Delegate3 );
        Delegate.Execute( 500 );

        // Swap
        TDelegate<int32( int32 )> Static;
        Static.BindLambda( StaticFunc );
        Static.Execute( 1000 );

        TDelegate<int32( int32 )> NotStatic;
        NotStatic.BindLambda( Lambda );
        NotStatic.Execute( 2000 );

        NotStatic.Swap( Static );
        NotStatic.Execute( 3000 );
        Static.Execute( 4000 );

        // Static Create functions with and without payload
        std::cout << std::endl << "Static Create functions with and without payload" << std::endl << std::endl;
        TDelegate<int32( int32 )> Lambda2 = TDelegate<int32( int32 )>::CreateLambda( Lambda );
        Lambda2.Execute( 100 );
        TDelegate<int32()> Lambda3 = TDelegate<int32()>::CreateLambda( Lambda, 200 );
        Lambda3.Execute();

        TDelegate<int32( int32 )> Static2 = TDelegate<int32( int32 )>::CreateStatic( StaticFunc );
        Static2.Execute( 100 );
        TDelegate<int32()> Static3 = TDelegate<int32()>::CreateStatic( StaticFunc, 200 );
        Static3.Execute();

        TDelegate<int32( int32 )> Member0 = TDelegate<int32( int32 )>::CreateRaw( Base, &CBase::Func );
        Member0.Execute( 100 );
        TDelegate<int32()> Member01 = TDelegate<int32()>::CreateRaw( Base, &CBase::Func, 200 );
        Member01.Execute();

        TDelegate<int32( int32 )> Member1 = TDelegate<int32( int32 )>::CreateRaw( Base, &CBase::ConstFunc );
        Member1.Execute( 100 );
        TDelegate<int32()> Member11 = TDelegate<int32()>::CreateRaw( Base, &CBase::ConstFunc, 200 );
        Member11.Execute();

        TDelegate<int32( int32 )> Member2 = TDelegate<int32( int32 )>::CreateRaw( Base, &CDerived::Func );
        Member2.Execute( 100 );
        TDelegate<int32()> Member21 = TDelegate<int32()>::CreateRaw( Base, &CDerived::Func, 200 );
        Member21.Execute();

        TDelegate<int32( int32 )> Member3 = TDelegate<int32( int32 )>::CreateRaw( Base, &CDerived::ConstFunc );
        Member3.Execute( 100 );

        TDelegate<int32()> Member31 = TDelegate<int32()>::CreateRaw( Base, &CDerived::ConstFunc, 200 );
        Member31.Execute();

        delete Base;
    }

    std::cout << std::endl << "----Testing MultiCastDelegate----" << std::endl << std::endl;

    {
		TMulticastDelegate<> VoidMultiDelegates;
		VoidMultiDelegates.AddLambda([]()
		{
			std::cout << "Lambda Void" << std::endl;
		});
		
		VoidMultiDelegates.Broadcast();
		
        TMulticastDelegate<int32> MultiDelegates;
        std::cout << "IsBound=" << std::boolalpha << MultiDelegates.IsBound() << std::endl;

        // Static
        MultiDelegates.AddStatic( StaticFunc2 );

        std::cout << "IsBound=" << std::boolalpha << MultiDelegates.IsBound() << std::endl;

        // Lambda
        int64 x = 50;
        int64 y = 150;
        int64 z = 250;
        auto Lambda = [=]( int32 Num )
        {
            std::cout << "Lambda2 (x=" << x << ", y=" << y << ", z=" << z << ") =" << Num << std::endl;
        };

        MultiDelegates.AddLambda( Lambda );

        // Members
        CBase2* Base = new CDerived2();
        MultiDelegates.AddRaw( Base, &CBase2::Func );
        MultiDelegates.AddRaw( Base, &CBase2::ConstFunc );
        MultiDelegates.AddRaw( Base, &CDerived2::Func );
        MultiDelegates.AddRaw( Base, &CDerived2::ConstFunc );

        MultiDelegates.Broadcast( 5000 );

        std::cout << "GetCount="      << MultiDelegates.GetCount() << std::endl;
        std::cout << "UnbindIfBound=" << MultiDelegates.UnbindIfBound( Base ) << std::endl;
        std::cout << "GetCount="      << MultiDelegates.GetCount() << std::endl;
        std::cout << "UnbindIfBound=" << MultiDelegates.UnbindIfBound( Base ) << std::endl;

        delete Base;
    }

    std::cout << std::endl << "----Testing Delegate Macros----" << std::endl << std::endl;
    {
        DECLARE_DELEGATE( CSomeDelegate, int32 );
        CSomeDelegate SomeDelegate;

        DECLARE_RETURN_DELEGATE( CSomeReturnDelegate, bool, int32 );
        CSomeReturnDelegate SomeReturnDelegate;

        DECLARE_MULTICAST_DELEGATE( CSomeMulticastDelegate, int32 );
        CSomeMulticastDelegate SomeMulticastDelegate;

        GSomeEvent.AddStatic( StaticFunc2 );

        CEventDispacher EventDispacher;
        EventDispacher.Func();
    }
}

#endif
