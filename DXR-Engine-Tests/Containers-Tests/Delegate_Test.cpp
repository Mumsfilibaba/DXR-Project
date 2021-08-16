#include "Delegate_Test.h"

#if RUN_DELEGATE_TEST
#include <iostream>

#include <Core/Delegates/Delegate.h>
#include <Core/Delegates/MulticastDelegate.h>

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

/* Test */
void Delegate_Test()
{
    std::cout << std::endl << "----Testing Delegate----" << std::endl << std::endl;
    
    {
        TDelegate<int32(int32)> Delegate;
        std::cout << "IsBound=" << std::boolalpha << Delegate.IsBound() << std::endl;
        std::cout << "ExecuteIfBound=" << std::boolalpha << Delegate.ExecuteIfBound(32) << std::endl;

        // Static
        Delegate.BindStatic( StaticFunc );

        std::cout << "Owner=" << Delegate.GetOwner() << std::endl;

        std::cout << "IsBound=" << std::boolalpha << Delegate.IsBound() << std::endl;
        std::cout << "Excute=" << Delegate.Execute(32) << std::endl;

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
        std::cout << "Excute=" << Delegate.Execute( 500 ) << std::endl;

        // Members
        CBase* Base = new CDerived();
        Delegate.BindRaw( Base, &CBase::Func );
        std::cout << "Excute=" << Delegate.Execute( 100 ) << std::endl;

        Delegate.BindRaw( Base, &CBase::ConstFunc );
        std::cout << "Excute=" << Delegate.Execute( 200 ) << std::endl;

        Delegate.BindRaw( Base, &CDerived::Func );
        std::cout << "Excute=" << Delegate.Execute( 300 ) << std::endl;

        Delegate.BindRaw( Base, &CDerived::ConstFunc );
        std::cout << "Excute=" << Delegate.Execute( 400 ) << std::endl;

        Delegate.Unbind();
        delete Base;

        // Copy
        Delegate.BindLambda( Lambda );
        Delegate.Execute( 0 );

        TDelegate<int32( int32 )> Delegate2 = Delegate;
        Delegate2.Execute(100);
        
        Delegate = Delegate2;
        Delegate.Execute( 200 );

        // Move
        TDelegate<int32( int32 )> Delegate3 = Move( Delegate2 );
        Delegate3.Execute( 300 );
        Delegate2.ExecuteIfBound( 400 );

        Delegate = Move( Delegate3 );
        Delegate.Execute( 500 );

        // Swap
        TDelegate<int32(int32)> Static;
        Static.BindLambda(StaticFunc);
        Static.Execute(1000);

        TDelegate<int32( int32 )> NotStatic;
        NotStatic.BindLambda(Lambda);
        NotStatic.Execute(2000);

        NotStatic.Swap(Static);
        NotStatic.Execute(3000);
        Static.Execute(4000);
    }

    std::cout << std::endl << "----Testing MultiCastDelegate----" << std::endl << std::endl;
    
    {
        TMulticastDelegate<int32> MultiDelegates;
        std::cout << "IsBound=" << std::boolalpha << MultiDelegates.IsBound() << std::endl;

        // Static
        MultiDelegates.AddStatic(StaticFunc2);

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

        MultiDelegates.Broadcast(5000);

        delete Base;
    }
}

#endif