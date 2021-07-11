#include "Actor.h"
#include "Scene.h"

Component::Component( Actor* InOwningActor )
    : CoreObject()
    , OwningActor( InOwningActor )
{
    Assert( InOwningActor != nullptr );
    CORE_OBJECT_INIT();
}

Actor::Actor()
    : CoreObject()
    , Components()
    , Transform()
{
    CORE_OBJECT_INIT();
}

Actor::~Actor()
{
    for ( Component* CurrentComponent : Components )
    {
        SafeDelete( CurrentComponent );
    }

    Components.Clear();
}

void Actor::AddComponent( Component* InComponent )
{
    Assert( InComponent != nullptr );
    Components.EmplaceBack( InComponent );

    if ( Scene )
    {
        Scene->OnAddedComponent( InComponent );
    }
}

void Actor::SetName( const std::string& InName )
{
    Name = InName;
}

Transform::Transform()
    : Matrix()
    , Translation( 0.0f, 0.0f, 0.0f )
    , Scale( 1.0f, 1.0f, 1.0f )
    , Rotation( 0.0f, 0.0f, 0.0f )
{
    CalculateMatrix();
}

void Transform::SetTranslation( float x, float y, float z )
{
    SetTranslation( CVector3( x, y, z ) );
}

void Transform::SetTranslation( const CVector3& InPosition )
{
    Translation = InPosition;
    CalculateMatrix();
}

void Transform::SetScale( float x, float y, float z )
{
    SetScale( CVector3( x, y, z ) );
}

void Transform::SetScale( const CVector3& InScale )
{
    Scale = InScale;
    CalculateMatrix();
}

void Transform::SetRotation( float x, float y, float z )
{
    SetRotation( CVector3( x, y, z ) );
}

void Transform::SetRotation( const CVector3& InRotation )
{
    Rotation = InRotation;
    CalculateMatrix();
}

void Transform::CalculateMatrix()
{
    CMatrix4 ScaleMatrix = CMatrix4::Scale(Scale);
    CMatrix4 RotationMatrix = CMatrix4::RotationRollPitchYaw(Rotation);
    CMatrix4 TranslationMatrix = CMatrix4::Translation(Translation);
    Matrix = (ScaleMatrix * RotationMatrix) * TranslationMatrix;
    Matrix = Matrix.Transpose();

    TinyMatrix = DirectX::XMFLOAT3X4(
        Matrix.m00, Matrix.m01, Matrix.m02, Matrix.m03,
        Matrix.m10, Matrix.m11, Matrix.m12, Matrix.m13,
        Matrix.m20, Matrix.m21, Matrix.m22, Matrix.m23);

    MatrixInv = Matrix.Invert();
    MatrixInv = MatrixInv.Transpose();
}
