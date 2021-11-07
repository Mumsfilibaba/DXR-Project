#include "Main/EngineMain.inl"

#if defined(PLATFORM_MACOS)

// TODO: The commandline should be saved somewhere
int main( int , const char** )
{
    return EngineMain();
}

#endif
