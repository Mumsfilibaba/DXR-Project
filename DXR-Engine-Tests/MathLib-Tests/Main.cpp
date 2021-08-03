#include <cstdio>

bool TestIntPoint2();
bool TestIntPoint3();

bool TestVector2();
bool TestVector3();
bool TestVector4();

bool TestMatrix2();
bool TestMatrix3();
bool TestMatrix4();

int main()
{
    // IntPoint
    if ( TestIntPoint2() )
    {
        printf( "IntPoint2 SUCCEEDED\n" );
    }
    else
    {
        printf( "IntPoint2 FAILED\n" );
    }

    if ( TestIntPoint3() )
    {
        printf( "IntPoint3 SUCCEEDED\n" );
    }
    else
    {
        printf( "IntPoint3 FAILED\n" );
    }

    // Vertices
    if ( TestVector2() )
    {
        printf( "Vector2 SUCCEEDED\n" );
    }
    else
    {
        printf( "Vector2 FAILED\n" );
    }

    if ( TestVector3() )
    {
        printf( "Vector3 SUCCEEDED\n" );
    }
    else
    {
        printf( "Vector3 FAILED\n" );
    }

    if ( TestVector4() )
    {
        printf( "Vector4 SUCCEEDED\n" );
    }
    else
    {
        printf( "Vector4 FAILED\n" );
    }

    // Matrices
    if ( TestMatrix2() )
    {
        printf( "Matrix2 SUCCEEDED\n" );
    }
    else
    {
        printf( "Matrix2 FAILED\n" );
    }

    if ( TestMatrix3() )
    {
        printf( "Matrix3 SUCCEEDED\n" );
    }
    else
    {
        printf( "Matrix3 FAILED\n" );
    }

    if ( TestMatrix4() )
    {
        printf( "Matrix4 SUCCEEDED\n" );
    }
    else
    {
        printf( "Matrix4 FAILED\n" );
    }

    return 0;
}